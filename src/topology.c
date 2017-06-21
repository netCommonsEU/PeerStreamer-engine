/*
 * Copyright (c) 2014-2017 Luca Baldesi
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
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
//
#include <math.h>
#include <net_helper.h>
#include <peerset.h>
#include <peersampler.h>
#include <peer.h>
#include <grapes_msg_types.h>
//
#include "compatibility/timer.h"
//
#include "topology.h"
#include "net_helpers.h"
#include "dbg.h"
#include "measures.h"
#include "streaming.h"

#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#define NEIGHBOURHOOD_ADD 0
#define NEIGHBOURHOOD_REMOVE 1
#define DEFAULT_PEER_CBSIZE 50

#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif

struct metadata {
  uint16_t cb_size;
} __attribute__((packed));

enum peer_choice {PEER_CHOICE_RANDOM, PEER_CHOICE_BEST, PEER_CHOICE_WORST};

struct topology {
	double desired_bw;
	double desired_rtt;
	double alpha_target;
	double topo_mem;
	bool topo_out;
	bool topo_in;
	bool topo_keep_best;
	bool topo_add_best;
	int neighbourhood_target_size;
	struct timeval tout_bmap;
	struct metadata my_metadata;	

	const struct psinstance * ps;
	struct psample_context * tc;
	struct peerset * neighbourhood;
	struct peerset * swarm_bucket;
	struct peerset * locked_neighs;
};

struct peerset * topology_get_neighbours(struct topology * t)
{
	return t->neighbourhood;
}

void peerset_print(const struct peerset * pset, const char * name)
{
	const struct peer * p;
	int i;
	if(name) fprintf(stderr,"%s\n",name);
	if(pset)
		peerset_for_each(pset,p,i)
			fprintf(stderr, "\t%s\n", nodeid_static_str(p->id));
}

void update_metadata(struct topology * t)
{
	t->my_metadata.cb_size = psinstance_is_source(t->ps) ? 0 : psinstance_chunkbuffer_size(t->ps);
}

struct peer * topology_get_peer(struct topology * t, const struct nodeID * id)
{
	struct peer * p = NULL;
	p = peerset_get_peer(t->swarm_bucket,id);
	if(p == NULL)
		p = peerset_get_peer(t->neighbourhood,id);
	return p;
}

int topology_init(struct topology * t, const struct psinstance * ps, const char *config)
{
	bind_msg_type(MSG_TYPE_NEIGHBOURHOOD);
	bind_msg_type(MSG_TYPE_TOPOLOGY);
	t->tout_bmap.tv_sec = 20;
	t->tout_bmap.tv_usec = 0;
	t->ps = ps;

	t->neighbourhood = peerset_init(0);
	t->swarm_bucket = peerset_init(0);
	t->locked_neighs = peerset_init(0);

	t->desired_bw = 0;	//TODO: turn on capacity measurement and set meaningful default value
	t->desired_rtt = 0.2;
	t->alpha_target = 0.4;
	t->topo_mem = 0.7;
	t->topo_out = true; //peer selects out-neighbours
	t->topo_in = true; //peer selects in-neighbours (combined means bidirectional)
	t->topo_keep_best = false;
	t->topo_add_best = false;
	t->neighbourhood_target_size = 30;

	update_metadata(t);
	t->tc = psample_init(psinstance_nodeid(ps), &(t->my_metadata), sizeof(struct metadata), config);
	
  //fprintf(stderr,"[DEBUG] done with topology init\n");
	return t->tc && t->neighbourhood && t->swarm_bucket ? 1 : 0;
}

struct topology * topology_create(const struct psinstance *ps, const char *config)
{
	struct topology * t = NULL;
	t = malloc(sizeof(struct topology));
	topology_init(t, ps, config);
	return t;
}

/*useful during bootstrap*/
int topology_node_insert(struct topology * t, struct nodeID *id)
{
	struct metadata m = {0};
	if (topology_get_peer(t, id) == NULL)
		peerset_add_peer(t->swarm_bucket,id);
	return psample_add_peer(t->tc,id,&m,sizeof(m));
}

void topology_peer_set_metadata(struct  peer *p, const struct metadata *m)
{
	if (p)
	{
		if (m)
		{
			p->cb_size = m->cb_size;
		}
		else
		{
			p->cb_size = DEFAULT_PEER_CBSIZE;
		}

	}
}

struct peer * neighbourhood_add_peer(struct topology * t, const struct nodeID *id)
{
	struct peer * p = NULL;
	if (id)
	{
		p = peerset_pop_peer(t->swarm_bucket,id);
		if(p)
			peerset_push_peer(t->neighbourhood,p);
		else
		{
			peerset_add_peer(t->neighbourhood,id);
			p = peerset_get_peer(t->neighbourhood,id);
			peerset_push_peer(t->locked_neighs,p);
		}
		measures_add_node(psinstance_measures(t->ps), p->id);
		// fprintf(stderr,"[DEBUG] sending bmap to peer %s \n",nodeid_static_str(id));
		send_bmap(psinstance_streaming(t->ps), id);
	}
	return p;
}

void neighbourhood_remove_peer(struct topology * t, const struct nodeID *id)
{
	struct peer *p=NULL;
	if(id)
	{
		p = peerset_pop_peer(t->neighbourhood,id);
		if(p)
			peerset_push_peer(t->swarm_bucket,p);

		peerset_pop_peer(t->locked_neighs,id);
	}
}

void neighbourhood_message_parse(struct topology * t, struct nodeID *from, const uint8_t *buff, size_t len)
{
	struct metadata m = {0};
	struct peer *p = NULL;

	switch(buff[0]) {
		case NEIGHBOURHOOD_ADD:
			// fprintf(stderr,"[DEBUG] adding peer %s from message\n",nodeid_static_str(from));
			p = neighbourhood_add_peer(t, from);
			if (len >= (sizeof(struct metadata) + 1))
			{
				memmove(&m,buff+1,sizeof(struct metadata));
				topology_peer_set_metadata(p,&m);
			}
			break;

		case NEIGHBOURHOOD_REMOVE:
			neighbourhood_remove_peer(t, from);
			break;
		default:
			dprintf("Unknown neighbourhood message type");
	}
}

void topology_message_parse(struct topology * t, struct nodeID *from, const uint8_t *buff, size_t len)
{
	switch(buff[0]) {
		case MSG_TYPE_NEIGHBOURHOOD:
			if (t->topo_in)
			{
				neighbourhood_message_parse(t, from, buff+1,len);
				reg_neigh_size(psinstance_measures(t->ps), peerset_size(t->neighbourhood));
			}
			break;
		case MSG_TYPE_TOPOLOGY:
			psample_parse_data(t->tc,buff,len);
			//fprintf(stderr,"[DEBUG] received TOPO message\n");
			break;
		default:
			fprintf(stderr,"Unknown topology message type");
	}
}

void topology_sample_peers(struct topology * t)
{
	int sample_nodes_num,sample_metas_num,i;
	const struct nodeID * const * sample_nodes;
	struct metadata const * sample_metas;
	struct peer * p;
		
	//fprintf(stderr,"[DEBUG] starting peer sampling\n");
	sample_nodes = psample_get_cache(t->tc,&sample_nodes_num);
	sample_metas = psample_get_metadata(t->tc,&sample_metas_num);
	for (i=0;i<sample_nodes_num;i++)
	{
		//fprintf(stderr,"[DEBUG] sampled node: %s\n",nodeid_static_str(sample_nodes[i]));
		p = topology_get_peer(t, sample_nodes[i]);
		if(p==NULL)
		{
			peerset_add_peer(t->swarm_bucket,sample_nodes[i]);
			p = topology_get_peer(t, sample_nodes[i]);
		}
		topology_peer_set_metadata(p,&(sample_metas[i]));	
	}
}

void topology_blacklist_add(struct topology * t, struct nodeID * id)
{
}

void neighbourhood_drop_unactives(struct topology * t, struct timeval * bmap_timeout)
{
  struct timeval tnow, told;
	struct peer *const *peers;
	int i;
  gettimeofday(&tnow, NULL);
  timersub(&tnow, bmap_timeout, &told);
  peers = peerset_get_peers(t->neighbourhood);
  for (i = 0; i < peerset_size(t->neighbourhood); i++) {
    if ( (!timerisset(&peers[i]->bmap_timestamp) && timercmp(&peers[i]->creation_timestamp, &told, <) ) ||
         ( timerisset(&peers[i]->bmap_timestamp) && timercmp(&peers[i]->bmap_timestamp, &told, <)     )   ) {
      dprintf("Topo: dropping inactive %s (peersset_size: %d)\n", nodeid_static_str(peers[i]->id), peerset_size(t->neighbourhood));
      if (peerset_size(t->neighbourhood) > 1) {	// avoid dropping our last link to the world
	      topology_blacklist_add(t, peers[i]->id);
	      neighbourhood_remove_peer(t, peers[i]->id);
      }
    }
  }
	
}

void array_shuffle(void *base, int nmemb, int size) {
  int i,newpos;
  unsigned char t[size];
  unsigned char* b = base;

  for (i = nmemb - 1; i > 0; i--) {
    newpos = (rand()/(RAND_MAX + 1.0)) * (i + 1);
    memcpy(t, b + size * newpos, size);
    memmove(b + size * newpos, b + size * i, size);
    memcpy(b + size * i, t, size);
  }
}

double get_rtt_of(struct topology *t, const struct nodeID* n){
  return NAN;
}

double get_capacity_of(struct topology *t, const struct nodeID* n){
  struct peer *p = topology_get_peer(t, n);
  if (p) {
    return p->capacity;
  }

  return NAN;
}

int neighbourhood_send_msg(struct topology *t, const struct peer * p,uint8_t type)
{
	uint8_t * msg;
	int res;
	msg = malloc(sizeof(struct metadata)+2);
	msg[0] = MSG_TYPE_NEIGHBOURHOOD;
	msg[1] = type;
	memmove(msg+2,&(t->my_metadata),sizeof(struct metadata));
	res = send_to_peer(psinstance_nodeid(t->ps), p->id, msg, sizeof(struct metadata)+2);
	free(msg);
	return res;	
}

void peerset_destroy_reference_copy(struct peerset ** pset)
{
	while (peerset_size(*pset))
		peerset_pop_peer(*pset,(peerset_get_peers(*pset)[0])->id);

	peerset_destroy(pset);
}

struct peerset * peerset_create_reference_copy(struct peerset * pset)
{
	struct peerset * ns;
	const struct peer * p;
	int i;

	ns = peerset_init(0);
	peerset_for_each(pset,p,i)
		peerset_push_peer(ns, (struct peer *)p);
	return ns;
}

struct peer *nodeid_to_peer(struct topology * t, struct nodeID *id,int reg)
{
	struct peer * p;
	p = topology_get_peer(t, id);
	if(p==NULL && reg)
	{
		topology_node_insert(t, id);
		neighbourhood_add_peer(t, id);
		p = topology_get_peer(t, id);
		if(t->topo_out)
			neighbourhood_send_msg(t, p, NEIGHBOURHOOD_ADD);
	}
	return p;
}

/* move num peers from pset1 to pset2 after applying the filtering_mask function and following the given criterion */
void topology_move_peers(struct peerset * pset1, struct peerset * pset2,int num,enum peer_choice criterion,bool (*filter_mask)(const struct peer *),int (*cmp_peer)(const void* p0, const void* p1)) 
{
	struct peer * const * const_peers;
	struct peer ** peers;
	struct peer *p;
	int peers_num,i,j;

	peers_num = peerset_size(pset1);
	const_peers = peerset_get_peers(pset1);
	peers = (struct peer **)malloc(sizeof(struct peer *)*peers_num);
	if (filter_mask)
	{
		for(i = 0,j = 0; i<peers_num; i++)
			if (filter_mask(const_peers[i]))
				peers[j++] = const_peers[i];
		peers_num = j;
	} else
		memmove(peers,const_peers,peers_num*sizeof(struct peer*));

	if (criterion != PEER_CHOICE_RANDOM && cmp_peer != NULL) {
    //fprintf(stderr,"[DEBUG] choosen the qsort\n");
		qsort(peers, peers_num, sizeof(struct peer*), cmp_peer);
	} else {
    array_shuffle(peers, peers_num, sizeof(struct peer *));
	}
	for (i=0; i<peers_num && i<num; i++)
	{
		if (criterion == PEER_CHOICE_WORST)
			p = peerset_pop_peer(pset1,(peers[peers_num -i -1])->id);
		else
			p = peerset_pop_peer(pset1,(peers[i])->id);
		peerset_push_peer(pset2,p);
	}
	free(peers);
}

void peerset_reference_copy_add(struct peerset * dst, struct peerset * src)
{
  const struct peer *p;
  int i;

	peerset_for_each(src,p,i)
		peerset_push_peer(dst, (struct peer *)p);
}

void topology_signal_change(struct topology *t, const struct peerset const * old_neighs)
{
	const struct peer * p;
	int i;
  reg_neigh_size(psinstance_measures(t->ps), peerset_size(t->neighbourhood));

	// advertise changes
	if(t->topo_out)
	{
		peerset_for_each(t->neighbourhood,p,i)
    {
			if(peerset_check(old_neighs,p->id) < 0)
				neighbourhood_send_msg(t, p, NEIGHBOURHOOD_ADD);
    }
		peerset_for_each(old_neighs,p,i)
    {
			if(peerset_check(t->neighbourhood,p->id) < 0)
				neighbourhood_send_msg(t,p,NEIGHBOURHOOD_REMOVE);
    }
	}
}


void topology_update_random(struct topology * t)
{
	int discard_num;
	int others_num;

	discard_num = (int)((1-t->topo_mem) * peerset_size(t->neighbourhood));
	topology_move_peers(t->neighbourhood,t->swarm_bucket,discard_num,PEER_CHOICE_RANDOM,NULL,NULL);

	others_num = MAX(t->neighbourhood_target_size-peerset_size(t->neighbourhood),0);
	topology_move_peers(t->swarm_bucket,t->neighbourhood,others_num,PEER_CHOICE_RANDOM,NULL,NULL);
}


void topology_update(struct topology * t)
{
	struct peerset * old_neighs;
  const struct peer * p;
  int i;

  psample_parse_data(t->tc,NULL,0); // needed in order to trigger timed sending of TOPO messages

	update_metadata(t);
	topology_sample_peers(t);

  if timerisset(&(t->tout_bmap) )
		neighbourhood_drop_unactives(t, &(t->tout_bmap));

	old_neighs = peerset_create_reference_copy(t->neighbourhood);

    topology_update_random(t);

  topology_signal_change(t, old_neighs);
	peerset_destroy_reference_copy(&old_neighs);

    peerset_for_each(t->swarm_bucket,p,i)
      peerset_pop_peer(t->locked_neighs,p->id);
    peerset_clear(t->swarm_bucket,0);  // we don't remember past peers
}

void topology_destroy(struct topology **t)
{
	if (t && *t)
	{
		if(((*t)->locked_neighs))
			peerset_destroy_reference_copy(&((*t)->locked_neighs));
		if(((*t)->neighbourhood))
			peerset_destroy(&((*t)->neighbourhood));
		if(((*t)->swarm_bucket))
			peerset_destroy(&((*t)->swarm_bucket));
		if(((*t)->tc))
			psample_destroy(&((*t)->tc));
		free(*t);
	}
}
