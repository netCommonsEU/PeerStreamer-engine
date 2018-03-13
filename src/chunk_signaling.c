/*
 * Copyright (c) 2009 Alessandro Russo
 * Copyright (c) 2009 Csaba Kiraly
 * Copyright (c) 2013-2017 Luca Baldesi
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
/*
 * Chunk Signaling API - Higher Abstraction
 *
 * The Chunk Signaling HA provides a set of primitives for chunks signaling negotiation with other peers, in order to collect information for the effective chunk exchange with other peers. <br>
 * This is a part of the Data Exchange Protocol which provides high level abstraction for chunks' negotiations, like requesting and proposing chunks.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "peer.h"
#include "peerset.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "chunk_signaling.h"
#include "net_helper.h"
#include <trade_sig_ha.h>

#include "streaming.h"
#include "topology.h"
#include "peer_metadata.h"
// #include "ratecontrol.h"
#include "dbg.h"
#include "net_helpers.h"

#define neigh_on_sign_recv false

void ack_received(const struct psinstance * ps, struct nodeID *fromid, struct chunkID_set *cset, int max_deliver, uint16_t trans_id) {
  struct peer *from = nodeid_to_peer(psinstance_topology(ps), fromid,0);   //verify that we have really sent, 0 at least garantees that we've known the peer before
  dprintf("The peer %s acked our chunk, max deliver %d, trans_id %d.\n", nodeid_static_str(fromid), max_deliver, trans_id);

  if (from) {
    chunkID_set_clear(peer_bmap(from), 0);	//TODO: some better solution might be needed to keep info about chunks we sent in flight.
    chunkID_set_union(peer_bmap(from), cset);
    gettimeofday(peer_bmap_timestamp(from), NULL);
  }

  // rc_reg_ack(trans_id);
}

void bmap_received(const struct psinstance * ps, struct nodeID *fromid, struct nodeID *ownerid, struct chunkID_set *c_set, int maxdeliver, uint16_t trans_id) {
  struct peer *owner;
  if (nodeid_equal(fromid, ownerid)) {
    owner = nodeid_to_peer(psinstance_topology(ps), ownerid, neigh_on_sign_recv);
  } else {
    dprintf("%s might be behind ",nodeid_static_str(ownerid));
    dprintf("NAT:%s\n",nodeid_static_str(fromid));
    owner = nodeid_to_peer(psinstance_topology(ps), fromid, neigh_on_sign_recv);
  }
  
  if (owner) {	//now we have it almost sure
    chunkID_set_clear(peer_bmap(owner), 0);	//TODO: some better solution might be needed to keep info about chunks we sent in flight.
    chunkID_set_union(peer_bmap(owner), c_set);
    gettimeofday(peer_bmap_timestamp(owner), NULL);
  }
}

void offer_received(const struct psinstance * ps, struct nodeID *fromid, struct chunkID_set *cset, int max_deliver, uint16_t trans_id) {
  struct chunkID_set *cset_acc;

  struct peer *from = nodeid_to_peer(psinstance_topology(ps), fromid, neigh_on_sign_recv);
  dprintf("The peer %s offers %d chunks, max deliver %d.\n", nodeid_static_str(fromid), chunkID_set_size(cset), max_deliver);

  if (from) {
    //register these chunks in the buffermap. Warning: this should be changed when offers become selective.
    chunkID_set_clear(peer_bmap(from), 0);	//TODO: some better solution might be needed to keep info about chunks we sent in flight.
    chunkID_set_union(peer_bmap(from), cset);
    gettimeofday(peer_bmap_timestamp(from), NULL);
  }

    //decide what to accept
    cset_acc = get_chunks_to_accept(psinstance_streaming(ps), fromid, cset, max_deliver, trans_id);

    //send accept message
    dprintf("\t accept %d chunks from peer %s, trans_id %d\n", chunkID_set_size(cset_acc), nodeid_static_str(fromid), trans_id);
    acceptChunks(psinstance_nodeid(ps), fromid, cset_acc, trans_id);

    chunkID_set_free(cset_acc);
}

void accept_received(const struct psinstance * ps, struct nodeID *fromid, struct chunkID_set *cset, int max_deliver, uint16_t trans_id) {
  struct peer *from = nodeid_to_peer(psinstance_topology(ps), fromid,0);   //verify that we have really offered, 0 at least garantees that we've known the peer before

  dprintf("The peer %s accepted our offer for %d chunks, max deliver %d.\n", nodeid_static_str(fromid), chunkID_set_size(cset), max_deliver);

  if (from) {
    gettimeofday(peer_bmap_timestamp(from), NULL);
  }

  // rc_reg_accept(trans_id, chunkID_set_size(cset));

  send_accepted_chunks(psinstance_streaming(ps), fromid, cset, max_deliver, trans_id);
}


 /**
 * Dispatcher for signaling messages.
 *
 * This method decodes the signaling messages, retrieving the set of chunk and the signaling
 * message, invoking the corresponding method.
 *
 * @param[in] buff buffer which contains the signaling message
 * @param[in] buff_len length of the buffer
 * @return 0 on success, <0 on error
 */

int sigParseData(const struct psinstance * ps, struct nodeID *fromid, uint8_t *buff, int buff_len) {
    struct chunkID_set *c_set;
    struct nodeID *ownerid;
    enum signaling_type sig_type;
    int max_deliver = 0;
    uint16_t trans_id = 0;
    int ret = 1;
    dprintf("Decoding signaling message...\n");

    ret = parseSignaling(buff + 1, buff_len-1, &ownerid, &c_set, &max_deliver, &trans_id, &sig_type);
#ifdef LOG_SIGNAL
    log_signal(fromid,psinstance_nodeid(ps),chunkID_set_size(c_set),trans_id,sig_type,"RECEIVED");
#endif

    if (ret < 0) {
      fprintf(stdout, "ERROR parsing signaling message\n");
      return -1;
    }
    switch (sig_type) {
        case sig_send_buffermap:
          bmap_received(ps, fromid, ownerid, c_set, max_deliver, trans_id); 
          break;
        case sig_offer:
          offer_received(ps, fromid, c_set, max_deliver, trans_id);
          break;
        case sig_accept:
          accept_received(ps, fromid, c_set, chunkID_set_size(c_set), trans_id);
          break;
	    case sig_ack:
	      ack_received(ps, fromid, c_set, chunkID_set_size(c_set), trans_id);
          break;
        default:
          ret = -1;
    }
    chunkID_set_free(c_set);
    return ret;
}
