/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2017 Luca Baldesi
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include <net_helper.h>
#include <chunk.h> 
#include <chunkbuffer.h> 
#include <trade_msg_la.h>
#include <trade_msg_ha.h>
#include <peerset.h>
#include <peer.h>
#include <chunkidset.h>
#include <limits.h>
#include <trade_sig_ha.h>
#ifdef CHUNK_ATTRIB_CHUNKER
#include <chunkiser_attrib.h>
#endif

#include "streaming.h"
#include "output.h"
#include "input.h"
#include "dbg.h"
#include "chunk_signaling.h"
#include "chunklock.h"
#include "topology.h"
#include "measures.h"
#include "net_helpers.h"
#include "scheduling.h"
#include "transaction.h"
#include "peer_metadata.h"

#include "scheduler_la.h"

# define CB_SIZE_TIME_UNLIMITED 1e12

void selectPeersForChunks(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, schedPeerID *selected, size_t *selected_len, filterFunction filter, peerEvaluateFunction evaluate);

struct chunk_attributes {
  uint64_t deadline;
  uint16_t deadline_increment;
  uint16_t hopcount;
} __attribute__((packed));

struct streaming_context {
	struct chunk_buffer *cb;
	struct input_desc *input;
	struct chunk_locks * ch_locks;
	struct service_times_element * transactions;
	const struct psinstance * ps;
	int cb_size;
	int bcast_after_receive_every;
	bool neigh_on_chunk_recv;
	bool send_bmap_before_push;
	uint64_t CB_SIZE_TIME;
	uint32_t chunk_loss_interval;
};

struct streaming_context * streaming_create(const struct psinstance * ps, struct input_context * inc, const char * config)
{
	struct streaming_context * stc;
	static char conf[80];

	stc = malloc(sizeof(struct streaming_context));
	stc->bcast_after_receive_every = 0;
	stc->neigh_on_chunk_recv = false;
	stc->send_bmap_before_push = false;
	stc->transactions = NULL;
	stc->CB_SIZE_TIME = CB_SIZE_TIME_UNLIMITED;	//in millisec, defaults to unlimited
	stc->chunk_loss_interval = 0;  // disable self-lossy feature (for experiments)

	stc->input = inc ? input_open(inc->filename, inc->fds, inc->fds_size, config) : NULL;

	stc->ps = ps;
	stc->cb_size = psinstance_chunkbuffer_size(ps);
	sprintf(conf, "size=%d", stc->cb_size);
	stc->cb = cb_init(conf);
	chunkDeliveryInit(psinstance_nodeid(ps));
	chunkSignalingInit(psinstance_nodeid(ps));
	stc->ch_locks = chunk_locks_create();
	return stc;
}

void streaming_destroy(struct streaming_context ** stc)
{
	if (stc && *stc)
	{
		if((*stc)->input)
			input_close((*stc)->input);
		if(((*stc)->ch_locks))
			chunk_locks_destroy(&((*stc)->ch_locks));
		if(((*stc)->transactions))
			transactions_destroy((*stc)->transactions);
		if(((*stc)->cb))
			cb_destroy((*stc)->cb);
		free((*stc));
	}
}

int _needs(const struct streaming_context * stc, struct chunkID_set *cset, int cb_size, int cid);

uint64_t gettimeofday_in_us(void)
{
  struct timeval what_time; //to store the epoch time

  gettimeofday(&what_time, NULL);
  return what_time.tv_sec * 1000000ULL + what_time.tv_usec;
}

void cb_print(const struct streaming_context * stc)
{
#ifdef DEBUG
  struct chunk *chunks;
  int num_chunks, i, id;
  chunks = cb_get_chunks(stc->cb, &num_chunks);

  dprintf("\tchbuf :");
  i = 0;
  if(num_chunks) {
    id = chunks[0].id;
    dprintf(" %d-> ",id);
    while (i < num_chunks) {
      if (id == chunks[i].id) {
        dprintf("%d",id % 10);
        i++;
      } else if (chunk_islocked(stc->ch_locks, id)) {
        dprintf("*");
      } else {
        dprintf(".");
      }
      id++;
    }
  }
  dprintf("\n");
#endif
}

void chunk_attributes_fill(struct chunk* c)
{
  struct chunk_attributes * ca;
  int priority = 1;

  assert((!c->attributes && c->attributes_size == 0)
#ifdef CHUNK_ATTRIB_CHUNKER
      || chunk_attributes_chunker_verify(c->attributes, c->attributes_size)
#endif
  );

#ifdef CHUNK_ATTRIB_CHUNKER
  if (chunk_attributes_chunker_verify(c->attributes, c->attributes_size)) {
    priority = ((struct chunk_attributes_chunker*) c->attributes)->priority;
    free(c->attributes);
    c->attributes = NULL;
    c->attributes_size = 0;
  }
#endif

  c->attributes_size = sizeof(struct chunk_attributes);
  c->attributes = ca = malloc(c->attributes_size);

  ca->deadline = c->id;
  ca->deadline_increment = priority * 2;
  ca->hopcount = 0;
}

int chunk_get_hopcount(const struct chunk* c) {
  struct chunk_attributes * ca;

  if (!c->attributes || c->attributes_size != sizeof(struct chunk_attributes)) {
    fprintf(stderr,"Warning, chunk %d with strange attributes block. Size:%d expected:%lu\n", c->id, c->attributes ? c->attributes_size : 0, sizeof(struct chunk_attributes));
    return -1;
  }

  ca = (struct chunk_attributes *) c->attributes;
  return ca->hopcount;
}

void chunk_attributes_update_received(struct chunk* c)
{
  struct chunk_attributes * ca;

  if (!c->attributes || c->attributes_size != sizeof(struct chunk_attributes)) {
    fprintf(stderr,"Warning, received chunk %d with strange attributes block. Size:%d expected:%lu\n", c->id, c->attributes ? c->attributes_size : 0, sizeof(struct chunk_attributes));
    return;
  }

  ca = (struct chunk_attributes *) c->attributes;
  ca->hopcount++;
  dprintf("Received chunk %d with hopcount %hu\n", c->id, ca->hopcount);
}

void chunk_attributes_update_sending(const struct chunk* c)
{
  struct chunk_attributes * ca;

  if (!c->attributes || c->attributes_size != sizeof(struct chunk_attributes)) {
    fprintf(stderr,"Warning, chunk %d with strange attributes block\n", c->id);
    return;
  }

  ca = (struct chunk_attributes *) c->attributes;
  ca->deadline += ca->deadline_increment;
  dprintf("Sending chunk %d with deadline %lu (increment: %d)\n", c->id, ca->deadline, ca->deadline_increment);
}

struct chunkID_set *cb_to_bmap(struct chunk_buffer *chbuf)
{
  struct chunk *chunks;
  int num_chunks, i;
  struct chunkID_set *my_bmap = chunkID_set_init("type=bitmap");
  chunks = cb_get_chunks(chbuf, &num_chunks);

  for(i=num_chunks-1; i>=0; i--) {
    chunkID_set_add_chunk(my_bmap, chunks[i].id);
  }
  return my_bmap;
}

// a simple implementation that request everything that we miss ... up to max deliver
struct chunkID_set *get_chunks_to_accept(const struct streaming_context * stc, struct nodeID *fromid, const struct chunkID_set *cset_off, int max_deliver, uint16_t trans_id){
  struct chunkID_set *cset_acc, *my_bmap;
  int i, d, cset_off_size;
  //double lossrate;
  struct peer *from = nodeid_to_peer(psinstance_topology(stc->ps), fromid, 0);

  cset_acc = chunkID_set_init("size=0");

  //reduce load a little bit if there are losses on the path from this guy
  //lossrate = get_lossrate_receive(from->id);
  //lossrate = finite(lossrate) ? lossrate : 0;	//start agressively, assuming 0 loss
  //if (rand()/((double)RAND_MAX + 1) >= 10 * lossrate ) {
    my_bmap = cb_to_bmap(stc->cb);
    cset_off_size = chunkID_set_size(cset_off);
    for (i = 0, d = 0; i < cset_off_size && d < max_deliver; i++) {
      int chunkid = chunkID_set_get_chunk(cset_off, i);
      //dprintf("\tdo I need c%d ? :",chunkid);
      if (!chunk_islocked(stc->ch_locks, chunkid) && _needs(stc, my_bmap, stc->cb_size, chunkid)) {
        chunkID_set_add_chunk(cset_acc, chunkid);
        chunk_lock(stc->ch_locks, chunkid,from);
        dtprintf("accepting %d from %s", chunkid, nodeid_static_str(fromid));
#ifdef MONL
        dprintf(", loss:%f rtt:%f", get_lossrate(fromid), get_rtt(fromid));
#endif
        dprintf("\n");
        d++;
      }
    }
    chunkID_set_free(my_bmap);
  //} else {
  //    dtprintf("accepting -- from %s loss:%f rtt:%f\n", node_addr_tr(fromid), lossrate, get_rtt(fromid));
  //}

  reg_offer_accept_in(psinstance_measures(stc->ps), chunkID_set_size(cset_acc) > 0 ? 1 : 0);

  return cset_acc;
}

void send_bmap(const struct streaming_context *stc, const struct nodeID *toid)
{
  struct chunkID_set *my_bmap = cb_to_bmap(stc->cb);
   sendBufferMap(toid,NULL, my_bmap, psinstance_is_source(stc->ps) ? 0 : stc->cb_size, 0);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(stc->ps),toid,chunkID_set_size(my_bmap),0,sig_send_buffermap,"SENT");
#endif
  chunkID_set_free(my_bmap);
}

void bcast_bmap(const struct streaming_context * stc)
{
  int i, n;
  struct peer **neighbours;
  struct peerset *pset;
  struct chunkID_set *my_bmap;

  pset = topology_get_neighbours(psinstance_topology(stc->ps));
  n = peerset_size(pset);
  neighbours = peerset_get_peers(pset);

  my_bmap = cb_to_bmap(stc->cb);	//cache our bmap for faster processing
  for (i = 0; i<n; i++) {
    sendBufferMap(neighbours[i]->id,NULL, my_bmap, psinstance_is_source(stc->ps) ? 0 : stc->cb_size, 0);
#ifdef LOG_SIGNAL
	 	log_signal(psinstance_nodeid(stc->ps),neighbours[i]->id,chunkID_set_size(my_bmap),0,sig_send_buffermap,"SENT");
#endif
  }
  chunkID_set_free(my_bmap);
}

void send_ack(const struct streaming_context * stc, struct nodeID *toid, uint16_t trans_id)
{
  struct chunkID_set *my_bmap = cb_to_bmap(stc->cb);
  sendAck(toid, my_bmap,trans_id);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(stc->ps),toid,chunkID_set_size(my_bmap),trans_id,sig_ack,"SENT");
#endif
  chunkID_set_free(my_bmap);
}

double get_average_lossrate_pset(struct peerset *pset)
{
  return 0;
}

void ack_chunk(const struct streaming_context * stc, struct chunk *c, struct nodeID *from, uint16_t trans_id)
{
  //reduce load a little bit if there are losses on the path from this guy
  double average_lossrate = get_average_lossrate_pset(topology_get_neighbours(psinstance_topology(stc->ps)));
  average_lossrate = finite(average_lossrate) ? average_lossrate : 0;	//start agressively, assuming 0 loss
  if (rand()/((double)RAND_MAX + 1) < 1 * average_lossrate ) {
    return;
  }
  send_ack(stc, from, trans_id);	//send explicit ack
}

void received_chunk(struct streaming_context * stc, struct nodeID *from, const uint8_t *buff, int len)
{
  int res;
  static struct chunk c;
  struct peer *p;
  static int bcast_cnt;
  uint16_t transid;

  res = parseChunkMsg(buff + 1, len - 1, &c, &transid);
  if (res > 0) {
		if (stc->chunk_loss_interval && c.id % stc->chunk_loss_interval == 0) {
			fprintf(stderr,"[NOISE] Chunk %d discarded >:)\n",c.id);
			free(c.data);
			free(c.attributes);
			return;
		}
    chunk_attributes_update_received(&c);
    chunk_unlock(stc->ch_locks, c.id);
    dprintf("Received chunk %d from peer: %s\n", c.id, nodeid_static_str(from));
#ifdef LOG_CHUNK
    log_chunk(from,psinstance_nodeid(stc->ps),&c,"RECEIVED");
#endif
//{fprintf(stderr, "TEO: Peer %s received chunk %d from peer: %s at: %"PRIu64" hopcount: %i Size: %d bytes\n", node_addr_tr(get_my_addr()),c.id, node_addr_tr(from), gettimeofday_in_us(), chunk_get_hopcount(&c), c.size);}
    output_deliver(psinstance_output(stc->ps), &c);
    res = cb_add_chunk(stc->cb, &c);
    reg_chunk_receive(psinstance_measures(stc->ps),c.id, c.timestamp, chunk_get_hopcount(&c), res==E_CB_OLD, res==E_CB_DUPLICATE);
    cb_print(stc);
    if (res < 0) {
      dprintf("\tchunk too old, buffer full with newer chunks\n");
#ifdef LOG_CHUNK
      log_chunk_error(from,psinstance_nodeid(stc->ps),&c,res); //{fprintf(stderr, "TEO: Received chunk: %d too old (buffer full with newer chunks) from peer: %s at: %"PRIu64"\n", c.id, node_addr_tr(from), gettimeofday_in_us());}
#endif
      free(c.data);
      free(c.attributes);
    }
    p = nodeid_to_peer(psinstance_topology(stc->ps), from, stc->neigh_on_chunk_recv);
    if (p) {	//now we have it almost sure
      chunkID_set_add_chunk(p->bmap,c.id);	//don't send it back
      gettimeofday(&p->bmap_timestamp, NULL);
    }
    ack_chunk(stc, &c, from, transid);	//send explicit ack
    if (stc->bcast_after_receive_every && bcast_cnt % stc->bcast_after_receive_every == 0) {
       bcast_bmap(stc);
    }
  } else {
    fprintf(stderr,"\tError: can't decode chunk!\n");
  }
}

struct chunk *generated_chunk(struct streaming_context * stc, suseconds_t *delta)
{
  struct chunk *c;

  c = malloc(sizeof(struct chunk));
  if (!c) {
    fprintf(stderr, "Memory allocation error!\n");
    return NULL;
  }
  memset(c, 0, sizeof(struct chunk));

  *delta = (suseconds_t)input_get(stc->input, c);
  if (*delta < 0) {
    fprintf(stderr, "Error in input!\n");
    exit(-1);
  }
  if (c->data == NULL) {
    free(c);
    return NULL;
  }
  dprintf("Generated chunk %d of %d bytes\n",c->id, c->size);
  chunk_attributes_fill(c);
  return c;
}

int add_chunk(struct streaming_context * stc, struct chunk *c)
{
  int res;

  if (stc && c && stc->cb)
  {
	  res = cb_add_chunk(stc->cb, c);
	  if (res < 0) {
	    free(c->data);
	    free(c->attributes);
	    free(c);
	    return 0;
	  }
	 // free(c);
	  return 1;
  }
  return 0;
}

uint64_t get_chunk_timestamp(const struct streaming_context * stc, int cid){
  const struct chunk *c = cb_get_chunk(stc->cb, cid);
  if (!c) return 0;

  return c->timestamp;
}

void print_chunkID_set(struct chunkID_set *cset)
{
	uint32_t * ptr = (uint32_t *) cset;
	uint32_t n_elements,i;
	int * data = (int *) &(ptr[3]);
	fprintf(stderr,"[DEBUG] Chunk ID set type: %d\n",ptr[0]);
	fprintf(stderr,"[DEBUG] Chunk ID set size: %d\n",ptr[1]);
	n_elements = ptr[2];
	fprintf(stderr,"[DEBUG] Chunk ID n_elements: %d\n",n_elements);
	fprintf(stderr,"[DEBUG] Chunk ID elements: [");
	for (i=0;i<n_elements;i++)
		fprintf(stderr,".%d.",data[i]);

	fprintf(stderr,"]\n");
}

/**
 *example function to filter chunks based on whether a given peer needs them.
 *
 * Looks at buffermap information received about the given peer.
 */
int needs_old(const struct streaming_context * stc, struct peer *n, int cid){
  struct peer * p = n;

  if (stc->CB_SIZE_TIME < CB_SIZE_TIME_UNLIMITED) {
    uint64_t ts;
    ts = get_chunk_timestamp(stc, cid);
    if (ts && (ts < gettimeofday_in_us() - stc->CB_SIZE_TIME)) {	//if we don't know the timestamp, we accept
      return 0;
    }
  }

  //dprintf("\t%s needs c%d ? :",node_addr_tr(p->id),c->id);
  if (! p->bmap) { // this will never happen since the pset module initializes bmap
    //dprintf("no bmap\n");
    return 1;	// if we have no bmap information, we assume it needs the chunk (aggressive behaviour!)
  }
	
//	fprintf(stderr,"[DEBUG] Evaluating Peer %s, CB_SIZE: %d\n",node_addr_tr(n->id),p->cb_size); // DEBUG
//	print_chunkID_set(p->bmap);																					// DEBUG

  return _needs(stc, p->bmap, peer_cb_size(p), cid);
}

/**
 * Function checking if chunkID_set cset may need chunk with id cid
 * @cset: target cset
 * @cb_size: maximum allowed numer of chunks. In the case of offer it indicates
 * 	the maximum capacity in of the receiving peer (so it's 0 for the source)
 * @cid: target chunk identifier
 */
int _needs(const struct streaming_context * stc, struct chunkID_set *cset, int cb_size, int cid){

  if (cb_size == 0) { //if it declared it does not needs chunks
    return 0;
  }

  if (stc->CB_SIZE_TIME < CB_SIZE_TIME_UNLIMITED) {
    uint64_t ts;
    ts = get_chunk_timestamp(stc, cid);
    if (ts && (ts < gettimeofday_in_us() - stc->CB_SIZE_TIME)) {	//if we don't know the timestamp, we accept
      return 0;
    }
  }

  if (chunkID_set_check(cset,cid) < 0) { //it might need the chunk
    int missing, min;
    //@TODO: add some bmap_timestamp based logic

    if (chunkID_set_size(cset) == 0) {
      //dprintf("bmap empty\n");
      return 1;	// if the bmap seems empty, it needs the chunk
    }
    missing = stc->cb_size - chunkID_set_size(cset);
    missing = missing < 0 ? 0 : missing;
    min = chunkID_set_get_earliest(cset);
      //dprintf("%s ... cid(%d) >= min(%d) - missing(%d) ?\n",(cid >= min - missing)?"YES":"NO",cid, min, missing);
    return (cid >= min - missing);
  }

  //dprintf("has it\n");
  return 0;
}

int needs(struct peer *p, int cid)
{
	int min;

	if (peer_cb_size(p) == 0) // it does not have capacity
		return 0;
	if (chunkID_set_check(p->bmap,cid) < 0)  // not in bmap
	{
		if(peer_cb_size(p) > chunkID_set_size(p->bmap)) // it has room for chunks anyway
		{
			min = chunkID_set_get_earliest(p->bmap) - peer_cb_size(p) + chunkID_set_size(p->bmap);
			min = min < 0 ? 0 : min;
			if (cid >= min)
				return 1;
		}
		if((int)chunkID_set_get_earliest(p->bmap) < cid)  // our is reasonably new
			return 1;
	}
	return 0;
}

double peerWeightReceivedfrom(struct peer **n){
  struct peer * p = *n;
  return timerisset(&p->bmap_timestamp) ? 1 : 0.1;
}

double peerWeightUniform(struct peer **n){
  return 1;
}

double peerWeightLoss(struct peer **n){
  return 1;
}

//double peerWeightRtt(struct peer **n){
//#ifdef MONL
//  double rtt = get_rtt((*n)->id);
//  //dprintf("RTT to %s: %f\n", node_addr_tr(p->id), rtt);
//  return finite(rtt) ? 1 / (rtt + 0.005) : 1 / 1;
//#else
//  return 1;
//#endif
//}

//ordering function for ELp peer selection, chunk ID based
//can't be used as weight
//double peerScoreELpID(struct nodeID **n){
//  struct chunkID_set *bmap;
//  int latest;
//  struct peer * p = nodeid_to_peer(*n, 0);
//  if (!p) return 0;
//
//  bmap = p->bmap;
//  if (!bmap) return 0;
//  latest = chunkID_set_get_latest(bmap);
//  if (latest == INT_MIN) return 0;
//
//  return -latest;
//}

double chunkScoreChunkID(int *cid){
  return (double) *cid;
}

uint64_t get_chunk_deadline(const struct streaming_context * stc, int cid){
  const struct chunk_attributes * ca;
  const struct chunk *c;

  c = cb_get_chunk(stc->cb, cid);
  if (!c) return 0;

  if (!c->attributes || c->attributes_size != sizeof(struct chunk_attributes)) {
    fprintf(stderr,"Warning, chunk %d with strange attributes block\n", c->id);
    return 0;
  }

  ca = (struct chunk_attributes *) c->attributes;
  return ca->deadline;
}

//double chunkScoreDL(int *cid){
//  return - (double)get_chunk_deadline(*cid);
//}

//double chunkScoreTimestamp(int *cid){
//  return (double) get_chunk_timestamp(*cid);
//}

void send_accepted_chunks(const struct streaming_context * stc, struct nodeID *toid, struct chunkID_set *cset_acc, int max_deliver, uint16_t trans_id){
  int i, d, cset_acc_size, res;
  struct peer *to = nodeid_to_peer(psinstance_topology(stc->ps), toid, 0);

  transaction_reg_accept(stc->transactions, trans_id, toid);

  cset_acc_size = chunkID_set_size(cset_acc);
  reg_offer_accept_out(psinstance_measures(stc->ps),cset_acc_size > 0 ? 1 : 0);	//this only works if accepts are sent back even if 0 is accepted
  for (i = 0, d=0; i < cset_acc_size && d < max_deliver; i++) {
    const struct chunk *c;
    int chunkid = chunkID_set_get_chunk(cset_acc, i);
    c = cb_get_chunk(stc->cb, chunkid);
    if (!c) {	// we should have the chunk
      dprintf("%s asked for chunk %d we do not own anymore\n", nodeid_static_str(toid), chunkid);
      continue;
    }
    if (!to || needs(to, chunkid)) {	//he should not have it. Although the "accept" should have been an answer to our "offer", we do some verification
      chunk_attributes_update_sending(c);
      res = sendChunk(toid, c, trans_id);
      if (res >= 0) {
        if(to) chunkID_set_add_chunk(to->bmap, c->id); //don't send twice ... assuming that it will actually arrive
        d++;
        reg_chunk_send(psinstance_measures(stc->ps),c->id);
#ifdef LOG_CHUNK
      	log_chunk(psinstance_nodeid(stc->ps), toid,c, "SENT_ACCEPTED");
#endif
        //{fprintf(stderr, "TEO: Sending chunk %d to peer: %s at: %"PRIu64" Result: %d Size: %d bytes\n", c->id, node_addr_tr(toid), gettimeofday_in_us(), res, c->size);}
      } else {
        fprintf(stderr,"ERROR sending chunk %d\n",c->id);
      }
    }
  }
}

int offer_peer_count(const struct streaming_context * stc)
{
  return psinstance_num_offers(stc->ps);
}

int offer_max_deliver(const struct streaming_context * stc, const struct nodeID *n)
{
  return psinstance_chunks_per_offer(stc->ps);
}

////get the rtt. Currenly only MONL version is supported
//static double get_rtt_of(struct nodeID* n){
//#ifdef MONL
//  return get_rtt(n);
//#else
//  return NAN;
//#endif
//}

#define DEFAULT_RTT_ESTIMATE 0.5

struct chunkID_set *compose_offer_cset(const struct streaming_context * stc, const struct peer *p)
{
  int num_chunks, j;
  uint64_t smallest_ts; //, largest_ts;
  double dt;
  struct chunkID_set *my_bmap = chunkID_set_init("type=bitmap");
  struct chunk *chunks = cb_get_chunks(stc->cb, &num_chunks);

  dt = DEFAULT_RTT_ESTIMATE;
  dt *= 1e6;	//convert to usec

  smallest_ts = chunks[0].timestamp;
//  largest_ts = chunks[num_chunks-1].timestamp;

  //add chunks in latest...earliest order
  if (psinstance_is_source(stc->ps)) {
    j = (num_chunks-1) * 3/4;	//do not send offers for the latest chunks from the source
  } else {
    j = num_chunks-1;
  }
  for(; j>=0; j--) {
    if (chunks[j].timestamp > smallest_ts + dt)
    chunkID_set_add_chunk(my_bmap, chunks[j].id);
  }

  return my_bmap;
}


void send_offer(const struct streaming_context * stc)
{
  struct chunk *buff;
  size_t size=0,  i, n;
  struct peer **neighbours;
  struct peerset *pset;

  pset = topology_get_neighbours(psinstance_topology(stc->ps));
  n = peerset_size(pset);
  neighbours = peerset_get_peers(pset);
  // dprintf("Send Offer: %lu neighbours\n", n);
  if (n == 0) return;
  buff = cb_get_chunks(stc->cb, (int *)&size);
  if (size == 0) return;

    size_t selectedpeers_len = offer_peer_count(stc);
    int chunkids[size];
    struct peer *nodeids[n];
    struct peer *selectedpeers[selectedpeers_len];

    //reduce load a little bit if there are losses on the path from this guy
    double average_lossrate = get_average_lossrate_pset(pset);
    average_lossrate = finite(average_lossrate) ? average_lossrate : 0;	//start agressively, assuming 0 loss
    if (rand()/((double)RAND_MAX + 1) < 10 * average_lossrate ) {
      return;
    }

    for (i = 0;i < size; i++) chunkids[size - 1 - i] = (buff+i)->id;
    for (i = 0; i<n; i++) nodeids[i] = neighbours[i];
    selectPeersForChunks(SCHED_WEIGHTING, nodeids, n, chunkids, size, selectedpeers, &selectedpeers_len, SCHED_NEEDS, SCHED_PEER);

    for (i=0; i<selectedpeers_len ; i++){
      int transid = transaction_create(stc->transactions, selectedpeers[i]->id);
      int max_deliver = offer_max_deliver(stc, selectedpeers[i]->id);
      struct chunkID_set *offer_cset = compose_offer_cset(stc, selectedpeers[i]);
      dprintf("\t sending offer(%d) to %s, cb_size: %d\n", transid, nodeid_static_str(selectedpeers[i]->id), peer_cb_size(selectedpeers[i]));
      offerChunks(selectedpeers[i]->id, offer_cset, max_deliver, transid++);
#ifdef LOG_SIGNAL
			log_signal(psinstance_nodeid(stc->ps),selectedpeers[i]->id,chunkID_set_size(offer_cset),transid,sig_offer,"SENT");
#endif
      chunkID_set_free(offer_cset);
    }
}

void log_chunk_error(const struct nodeID *from,const struct nodeID *to,const struct chunk *c,int error)
{
	switch (error) {
		case E_CB_OLD:
		log_chunk(from,to,c,"TOO_OLD");
		break;
		case E_CB_DUPLICATE:
		log_chunk(from,to,c,"DUPLICATED");
		break;
		default:
		log_chunk(from,to,c,"ERROR");
	} 
}

void log_neighbourhood(const struct streaming_context * stc)
{
  struct peerset * pset;
  const struct peer * p;
  int psetsize,i;
  uint64_t now;
  char me[NODE_STR_LENGTH];

  node_addr(psinstance_nodeid(stc->ps), me, NODE_STR_LENGTH);
  pset = topology_get_neighbours(psinstance_topology(stc->ps));
  psetsize = peerset_size(pset);
  now = gettimeofday_in_us();
  peerset_for_each(pset,p,i)
    fprintf(stderr,"[NEIGHBOURHOOD],%"PRIu64",%s,%s,%d\n",now,me,nodeid_static_str(p->id),psetsize);

}

void log_chunk(const struct nodeID *from,const struct nodeID *to,const struct chunk *c,const char * note)
{
	// semantic: [CHUNK_LOG],log_date,sender,receiver,id,size(bytes),chunk_timestamp,hopcount,notes
	char sndr[NODE_STR_LENGTH],rcvr[NODE_STR_LENGTH];
	node_addr(from,sndr,NODE_STR_LENGTH);
	node_addr(to,rcvr,NODE_STR_LENGTH);

	fprintf(stderr,"[CHUNK_LOG],%"PRIu64",%s,%s,%d,%d,%"PRIu64",%i,%s\n",gettimeofday_in_us(),sndr,rcvr,c->id,c->size,c->timestamp,chunk_get_hopcount(c),note);
}

void log_signal(const struct nodeID *fromid,const struct nodeID *toid,const int cidset_size,uint16_t trans_id,enum signaling_type type,const char *flag)
{
	char typestr[24];
	char sndr[NODE_STR_LENGTH],rcvr[NODE_STR_LENGTH];
	node_addr(fromid,sndr,NODE_STR_LENGTH);
	node_addr(toid,rcvr,NODE_STR_LENGTH);

	switch (type)
	{
		case sig_offer:
			sprintf(typestr,"%s","OFFER_SIG");
			break;
		case sig_accept:
			sprintf(typestr,"%s","ACCEPT_SIG");
			break;
		case sig_request:
			sprintf(typestr,"%s","REQUEST_SIG");
			break;
		case sig_deliver:
			sprintf(typestr,"%s","DELIVER_SIG");
			break;
		case sig_send_buffermap:
			sprintf(typestr,"%s","SEND_BMAP_SIG");
			break;
		case sig_request_buffermap:
			sprintf(typestr,"%s","REQUEST_BMAP_SIG");
			break;
		case sig_ack:
			sprintf(typestr,"%s","CHUNK_ACK_SIG");
			break;
		default:
			sprintf(typestr,"%s","UNKNOWN_SIG");

	}
	fprintf(stderr,"[OFFER_LOG],%s,%s,%d,%s,%s\n",sndr,rcvr,trans_id,typestr,flag);
}

int peer_chunk_dispatch(const struct streaming_context * stc, const struct PeerChunk  *pairs,const size_t num_pairs)
{
	int transid, res,success = 0;
	size_t i;
	const struct peer * target_peer;
	const struct chunk * target_chunk;

	for (i=0; i<num_pairs ; i++){
		target_peer = pairs[i].peer;
		target_chunk = cb_get_chunk(stc->cb, pairs[i].chunk);

		if (stc->send_bmap_before_push) {
			send_bmap(stc, target_peer->id);
		}
		chunk_attributes_update_sending(target_chunk);
		transid = transaction_create(stc->transactions, target_peer->id);
		res = sendChunk(target_peer->id, target_chunk, transid);	//we use transactions in order to register acks for push
		if (res>=0) {
#ifdef LOG_CHUNK
			log_chunk(psinstance_nodeid(stc->ps), target_peer->id, target_chunk,"SENT");
#endif
//			chunkID_set_add_chunk((target_peer)->bmap,target_chunk->id); //don't send twice ... assuming that it will actually arrive
			reg_chunk_send(psinstance_measures(stc->ps),target_chunk->id);
			success++;
		} else {
			fprintf(stderr,"ERROR sending chunk %d\n",target_chunk->id);
		}
	}
	return success;

}

int inject_chunk(const struct streaming_context * stc, const struct chunk * target_chunk,const int multiplicity)
/*injects a specific chunk in the overlay and return the number of injected copies*/
{
	struct peerset *pset;
	struct peer ** peers, ** dst_peers;
	int peers_num;
	double (* peer_evaluation) (struct peer **n);
	size_t i, selectedpairs_len = multiplicity;
	struct PeerChunk  * selectedpairs;

	pset = topology_get_neighbours(psinstance_topology(stc->ps));
	peers_num = peerset_size(pset);
	peers = peerset_get_peers(pset);
	peer_evaluation = SCHED_PEER;

  //SCHED_TYPE(SCHED_WEIGHTING, peers, peers_num, &(target_chunk->id), 1, selectedpairs, &selectedpairs_len, SCHED_NEEDS, peer_evaluation, SCHED_CHUNK);
 	dst_peers = (struct peer **) malloc(sizeof(struct peer* ) *  multiplicity);
 	selectPeersForChunks(SCHED_WEIGHTING, peers, peers_num, (int *)&(target_chunk->id) , 1, dst_peers, &selectedpairs_len, NULL, peer_evaluation);

	selectedpairs = (struct PeerChunk *)  malloc(sizeof(struct PeerChunk) * selectedpairs_len);
	for ( i=0; i<selectedpairs_len; i++)
	{
		selectedpairs[i].peer = dst_peers[i];
		selectedpairs[i].chunk = target_chunk->id;
	}

	peer_chunk_dispatch(stc, selectedpairs,selectedpairs_len);

	free(selectedpairs);
	free(dst_peers);
	return selectedpairs_len;
}

void send_chunk(const struct streaming_context * stc)
{
  struct chunk *buff;
  int size, res, i, n;
  struct peer **neighbours;
  struct peerset *pset;

  pset = topology_get_neighbours(psinstance_topology(stc->ps));
  n = peerset_size(pset);
  neighbours = peerset_get_peers(pset);
  dprintf("Send Chunk: %d neighbours\n", n);
  if (n == 0) return;
  buff = cb_get_chunks(stc->cb, &size);
  dprintf("\t %d chunks in buffer...\n", size);
  if (size == 0) return;

  /************ STUPID DUMB SCHEDULING ****************/
  //target = n * (rand() / (RAND_MAX + 1.0)); /*0..n-1*/
  //c = size * (rand() / (RAND_MAX + 1.0)); /*0..size-1*/
  /************ /STUPID DUMB SCHEDULING ****************/

  /************ USE SCHEDULER ****************/
  {
    size_t selectedpairs_len = 1;
    int chunkids[size];
		int transid;
    struct peer *nodeids[n];
    struct PeerChunk selectedpairs[1];
  
    for (i = 0;i < size; i++) chunkids[size - 1 - i] = (buff+i)->id;
    for (i = 0; i<n; i++) nodeids[i] = neighbours[i];
	    SCHED_TYPE(SCHED_WEIGHTING, nodeids, n, chunkids, 1, selectedpairs, &selectedpairs_len, SCHED_NEEDS, SCHED_PEER, SCHED_CHUNK);
  /************ /USE SCHEDULER ****************/

    for (i=0; i<(int)selectedpairs_len ; i++){
      struct peer *p = selectedpairs[i].peer;
      const struct chunk *c = cb_get_chunk(stc->cb, selectedpairs[i].chunk);
      dprintf("\t sending chunk[%d] to ", c->id);
      dprintf("%s\n", nodeid_static_str(p->id));

      if (stc->send_bmap_before_push) {
        send_bmap(stc, p->id);
      }

      chunk_attributes_update_sending(c);
      transid = transaction_create(stc->transactions, p->id);
      res = sendChunk(p->id, c, transid);	//we use transactions in order to register acks for push
//      res = sendChunk(p->id, c, 0);	//we do not use transactions in pure push
      dprintf("\tResult: %d\n", res);
      if (res>=0) {
#ifdef LOG_CHUNK
      	log_chunk(psinstance_nodeid(stc->ps),p->id,c,"SENT");
#endif
//{fprintf(stderr, "TEO: Sending chunk %d to peer: %s at: %"PRIu64" Result: %d Size: %d bytes\n", c->id, node_addr_tr(p->id), gettimeofday_in_us(), res, c->size);}
        chunkID_set_add_chunk(p->bmap,c->id); //don't send twice ... assuming that it will actually arrive
        reg_chunk_send(psinstance_measures(stc->ps),c->id);
      } else {
        fprintf(stderr,"ERROR sending chunk %d\n",c->id);
      }
    }
  }
}
