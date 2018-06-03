#include<chunk_trader.h>
#include<dbg.h>
#include<measures.h>
#include<grapes_config.h>
#include<string.h>
#include<chunkbuffer.h>
#include<chunklock.h>
#include<scheduler_common.h>
#include<peer_metadata.h>
#include<peerset.h>
#include<scheduler_la.h>
#include<transaction.h>
#include<chunkidms.h>
#include<chunkidms_trade.h>
#include<chunk_attributes.h>
#include<trade_msg_ha.h>

#include<net_helpers.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

enum distribution_type {DIST_UNIFORM, DIST_TURBO};

struct chunk_trader{
	struct chunk_buffer **cbs;
        int flows_number;
	struct chunk_locks * ch_locks;
	const struct psinstance * ps;
	int cb_size;
        int my_flowid;
	enum distribution_type dist_type;
	int offer_per_period;
	struct service_times_element * transactions;
	int peers_per_offer;
	int chunks_per_peer_offer;
};

int chunk_trader_buffer_size(const struct chunk_trader *ct)
{
	return ct->cb_size;
}

struct chunk_trader * chunk_trader_create(const struct psinstance *ps,const  char *config)
{
	struct chunk_trader *ct;
	struct tag * tags;
	char conf[80];

	ct = malloc(sizeof(struct chunk_trader));
	ct->dist_type = DIST_UNIFORM;
	ct->ps = ps;
	ct->transactions = NULL;

	tags = grapes_config_parse(config);
	if (strcmp(grapes_config_value_str_default(tags, "dist_type", ""), "turbo") == 0)
		ct->dist_type = DIST_TURBO;
	grapes_config_value_int_default(tags, "offer_per_period", &(ct->offer_per_period), 1);
	grapes_config_value_int_default(tags, "peers_per_offer", &(ct->peers_per_offer), 1);
	grapes_config_value_int_default(tags, "chunks_per_peer_offer", &(ct->chunks_per_peer_offer), 1);
	grapes_config_value_int_default(tags, "chunkbuffer_size", &(ct->cb_size), 50);
	
        ct->my_flowid = 1+ (rand()%10000); // Random flow_id from 1 to 9999
        grapes_config_value_int_default(tags, "flow_id", &(ct->my_flowid),
                        ct->my_flowid);

        
        free(tags);
	
        ct->flows_number = 0;
        ct->cbs = NULL;

        //Initialize our chunkbuffer so we'll find it in ct->cbs[0] everytime
	get_chunkbuffer(ct, ct->my_flowid);
        
        ct->ch_locks = chunk_locks_create(80);  // milliseconds of lock time
	return ct;
}

void chunk_trader_destroy(struct chunk_trader **ct)
{
        int i;

	if (ct && *ct)
	{
		if(((*ct)->ch_locks))
			chunk_locks_destroy(&((*ct)->ch_locks));
        	if(((*ct)->transactions))
			transaction_destroy(&((*ct)->transactions));
        	if(((*ct)->cbs))
                {
                        for(i=0; i<(*ct)->flows_number; i++)
                                cb_destroy((*ct)->cbs[i]);
			
                        free((*ct)->cbs);
                }
		free(*ct);
		*ct = NULL;
	}
}


int8_t chunk_trader_add_chunk(struct chunk_trader *ct, struct chunk *c)
{
	int res = -1;
        struct chunk_buffer * cb = NULL;
	if (ct && c)
	{
                cb = get_chunkbuffer(ct, c->flow_id);
                if(cb)
                {
                
	        	res = cb_add_chunk(cb, c);
		        if (res)
			        log_chunk_error(psinstance_nodeid(ct->ps), NULL, c, res);
		        res = res < 0 ? -1 : 0;
                }
        }
	return res;
}

int8_t peer_chunk_send(struct chunk_trader * ct, struct PeerChunk *pairs, int pairs_len, uint16_t transid)
{
	int i, res =-1;
	struct peer * target_peer;
	struct chunk * target_chunk;
#ifdef LOG_CHUNK
        
        cb_print(ct);
#endif

	for (i=0; i<pairs_len; i++)
	{
		target_peer = pairs[i].peer;
		target_chunk = (struct chunk *) get_chunk_multiple(ct, pairs[i].chunk);
		res = sendChunk(psinstance_nodeid(ct->ps),target_peer->id, target_chunk, transid);	//we use transactions in order to register acks for push
		if (res >= 0)
		{
			chunk_attributes_update_upon_sending(target_chunk);
			chunkID_multiSet_add_chunk(peer_bmap(target_peer), target_chunk->id, target_chunk->flow_id);
#ifdef LOG_CHUNK

			log_chunk(psinstance_nodeid(ct->ps), target_peer->id, target_chunk, "SENT");
#endif
		} 
	}

	return res >= 0? 0 : -1;
}

double trader_peer_neigh_size(struct peer **n)
{
	double res;
	res = (double)(((struct metadata *)(*n)->metadata)->neigh_size);
	return res > 0 ? res : 1;
}

double peer_evaluation_turbo(struct peer **n)
{
	return 1.0/trader_peer_neigh_size(n);
}

double peer_evaluation_uniform(struct peer **n)
{
	return 1.0;
}

double chunk_evaluation_latest(schedChunkID *cid)
{
	return (*cid)->chunk_id;
}

/** chunk actions **/
int8_t chunk_trader_push_chunk(struct chunk_trader *ct, struct chunk *c, int multiplicity)
{
	int8_t res = -1;
	struct peerset *pset;
	int peer_num;
	struct peer **peers;
	struct PeerChunk * pairs;
	size_t pairs_len;
        struct sched_chunkID *cid = malloc(sizeof(struct sched_chunkID));

	if (ct && c && multiplicity > 0)
	{
		pset = topology_get_neighbours(psinstance_topology(ct->ps));
		peer_num = peerset_size(pset);
		peers = peerset_get_peers(pset);
		
		pairs_len = MIN(multiplicity, peer_num);
		pairs = malloc(pairs_len * sizeof(struct PeerChunk));
	        cid->chunk_id = c->id;
                cid->flow_id  = c->flow_id; 
		if (ct->dist_type == DIST_TURBO)
			schedSelectChunkFirst(SCHED_WEIGHTED, peers, peer_num, &cid, 1, pairs, &pairs_len, NULL, peer_evaluation_turbo, chunk_evaluation_latest);
		else
			schedSelectChunkFirst(SCHED_WEIGHTED, peers, peer_num, &cid, 1, pairs, &pairs_len, NULL, peer_evaluation_uniform, chunk_evaluation_latest);

		peer_chunk_send(ct, pairs, pairs_len, INVALID_TRANSID);
		free(pairs);
        }
        free(cid);
	return res;
}

int8_t chunk_trader_send_ack(struct chunk_trader *ct, struct nodeID *to, uint16_t transid, int flowid, int chunkid)
{
	struct chunkID_multiSet * bmap;
        const struct chunk * c;
        struct chunk_buffer *cb=get_chunkbuffer(ct, flowid);
        if(cb)
        {
                //Do not send all chunks, only the one to ack
                c=cb_get_chunk(cb, chunkid);
                if(c!=NULL)
                {
                        bmap=chunkID_multiSet_init(1,1);
                        chunkID_multiSet_add_chunk(bmap, chunkid, flowid);
                        sendAckMS(psinstance_nodeid(ct->ps),to, bmap,transid);
#ifdef LOG_SIGNAL
                        log_signal(psinstance_nodeid(ct->ps),to,transid,sig_ack,"SENT", bmap);
#endif
                        chunkID_multiSet_free(bmap);
                }

        }

	return 0;
}

void chunk_destroy(struct chunk **c)
{
	if (c && *c)
	{
		chunk_attributes_deinit(*c);
		if((*c)->data)
			free((*c)->data);
		free(*c);
		*c = NULL;
	}
}

struct chunk * chunk_trader_parse_chunk(struct chunk_trader *ct, struct nodeID *from, uint8_t *buff, int len)
{
	int res;
	struct chunk *c = NULL;
	uint16_t transid;
	struct peer * p;

	if (ct && from && buff && len > 1)
	{
		c = malloc(sizeof(struct chunk));
		memset(c, 0, sizeof(struct chunk));
		res = parseChunkMsg(buff+1, len-1, c, &transid);
		if (res > 0)
		{
			chunk_attributes_update_upon_reception(c);
			chunk_unlock(ct->ch_locks, c->flow_id, c->id); // in case we locked it in a select message
#ifdef LOG_CHUNK
                        log_chunk(from, psinstance_nodeid(ct->ps), c, "RECEIVED");
                        cb_print(ct);
#endif
			p = nodeid_to_peer(psinstance_topology(ct->ps), from, 0);
			if (p)
				chunkID_multiSet_add_chunk(peer_bmap(p), c->id, c->flow_id);  // keep track it has this chunk for sure
			chunk_trader_send_ack(ct, from, transid, c->flow_id, c->id);

		} else {
#ifdef LOG_CHUNK
                        log_chunk_error(from, psinstance_nodeid(ct->ps), c, E_CANNOT_PARSE);
#endif
			chunk_destroy(&c);
		}
	}

	return c;
}

/** signalling actions **/
int peer_needs_chunk_filter(struct peer *p, schedChunkID cid)
{
	int min;

	if (peer_cb_size(p) == 0) // it does not have capacity
		return 0;
	if (chunkID_multiSet_check(peer_bmap(p),cid->chunk_id, cid->flow_id) < 0)  // not in bmap
	{
		if(peer_cb_size(p) > chunkID_multiSet_single_size(peer_bmap(p), cid->flow_id)) // it has room for chunks anyway
		{
			min = chunkID_multiSet_get_earliest(peer_bmap(p), cid->flow_id) - peer_cb_size(p) + chunkID_multiSet_single_size(peer_bmap(p), cid->flow_id);
			min = min < 0 ? 0 : min;
			if (cid->chunk_id >= min)
				return 1;
		}
                        if((int)chunkID_multiSet_get_earliest(peer_bmap(p), cid->flow_id) < cid ->chunk_id)  // our is reasonably new
			return 1;
	}
	return 0;
}







int8_t chunk_trader_send_offer(struct chunk_trader *ct)
{
	struct peerset *pset;
	struct peer ** neighs;
        int n_neighs, n_chunks=0, j;
	size_t n_pairs, i;
	struct PeerChunk * pairs;
	struct chunkID_multiSet * offer_cset;
	schedChunkID * ch_buff;
	int8_t res = 0;
	uint16_t transid;

	pset = topology_get_neighbours(psinstance_topology(ct->ps));
	n_neighs = peerset_size(pset);
	neighs = peerset_get_peers(pset);
	ch_buff = chunk_buffer_to_idarray(ct, &n_chunks);
	pairs = malloc(sizeof(struct PeerChunk) * n_chunks);

	for (j=0; j<ct->peers_per_offer; j++)
	{
		n_pairs = n_chunks;  // we potentially offer everything

		// the following scheduling function picks one peer at most
		if (ct->dist_type == DIST_TURBO)
			schedSelectPeerFirst(SCHED_WEIGHTED, neighs, n_neighs, ch_buff, n_chunks, pairs, &n_pairs, peer_needs_chunk_filter, peer_evaluation_turbo, chunk_evaluation_latest);
		else
			schedSelectPeerFirst(SCHED_WEIGHTED, neighs, n_neighs, ch_buff, n_chunks, pairs, &n_pairs, peer_needs_chunk_filter, peer_evaluation_uniform, chunk_evaluation_latest);

		if (n_pairs > 0)
		{
			offer_cset = chunkID_multiSet_init(0,0);
			for(i=0; i<n_pairs; i++)
				chunkID_multiSet_add_chunk(offer_cset, pairs[i].chunk->chunk_id, pairs[i].chunk->flow_id);
			transid = transaction_create(&(ct->transactions), pairs[0].peer->id);
			offerChunksMS(psinstance_nodeid(ct->ps), pairs[0].peer->id, offer_cset, ct->chunks_per_peer_offer, transid);
#ifdef LOG_SIGNAL
			log_signal(psinstance_nodeid(ct->ps), pairs[0].peer->id, transid, sig_offer, "SENT", offer_cset);
#endif
			chunkID_multiSet_free(offer_cset);
			res++;
		}
	}
	free(pairs);
        for(i=0; i<n_chunks; i++)
	        free(ch_buff[i]);
        free(ch_buff);
	return res;
}

/* Latest-useful chunk selection */
int8_t chunk_trader_handle_offer(struct chunk_trader *ct, struct peer *p, struct chunkID_multiSet *cset, int max_deliver, uint16_t trans_id)
{
	int  min, max, cid, i, nflows, *flows, flow, f, n, min_cb, *ids;
        struct chunkID_multiSet * acc_set;
        struct chunk_buffer * cb;
	struct chunk * chunks;
	
        //Create with some size !=0 : size couldn't be greater than cset_off size,
        //single_size should be equal between every node
        
        acc_set = chunkID_multiSet_init(chunkID_multiSet_size(cset), ct->cb_size);
        
	chunkID_multiSet_union(peer_bmap(p), cset);
	chunkID_multiSet_trimAll(peer_bmap(p), peer_cb_size(p));
	gettimeofday(peer_bmap_timestamp(p), NULL);
        flows = chunkID_multiSet_get_flows(cset, &nflows);
        for(f=0; f<nflows; f++)
        {
                flow = flows[f];
                if(flow != ct->my_flowid)
                {
                        /* we use counting sort to sort chunks in O(~|cset|) */
                        min = chunkID_multiSet_get_earliest(cset, flow);
                        max = chunkID_multiSet_get_latest(cset, flow);
                        ids = malloc(sizeof(int)* (max-min+1));
                        memset(ids, 0, sizeof(int)*(max-min+1));
                        cb = get_chunkbuffer(ct, flow);
			chunks = cb_get_chunks(cb, &n);

                        for(i=0; i<chunkID_multiSet_single_size(cset, flow); i++)
                        {
                                cid = chunkID_multiSet_get_chunk(cset, i, flow);
                                ids[cid-min] += 1;
                        }
                        /* we select the latest useful */
                        cid = max;
                        while(cid>=min && chunkID_multiSet_single_size(acc_set, flow) < max_deliver
					&& (n<=0 || cid>chunks[0].id))
                        {
                                if (ids[cid-min] && !cb_get_chunk(cb, cid) && !chunk_islocked(ct->ch_locks, flow, cid))
                                {
                                        chunkID_multiSet_add_chunk(acc_set, cid, flow);
                                        chunk_lock(ct->ch_locks, flow, cid, p);
                                }
                                cid--;
                        }
                        free(ids);
                }
        }


        acceptChunksMS(psinstance_nodeid(ct->ps), p->id, acc_set, trans_id);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(ct->ps), p->id, trans_id, sig_accept, "SENT", acc_set);
#endif

	chunkID_multiSet_free(acc_set);
        free(flows);

	return 0;
}

int8_t chunk_trader_handle_accept(struct chunk_trader *ct, struct peer *p, struct chunkID_multiSet *cset, int max_deliver, uint16_t transid)
{
	int i, max_chunks, pairs_len = 0, f, *flows, nflows=0;
	const struct chunk *c;
	struct PeerChunk * pairs;
        struct chunk_buffer *cb;
        schedChunkID cid = NULL;
        flows = chunkID_multiSet_get_flows(cset, &nflows);
        pairs_len = 0;
        max_chunks = MIN(ct->chunks_per_peer_offer * nflows, chunkID_multiSet_total_size(cset)); 
        pairs = malloc(sizeof(struct PeerChunk) * max_chunks);  
        
        transaction_reg_accept(ct->transactions, transid, p->id);
        
        for(f=0; f<nflows && pairs_len < max_chunks; f++)
        {
                for(i=0; i<chunkID_multiSet_single_size(cset, flows[f]) && pairs_len < max_chunks; i++)
                {
                        cid = malloc(sizeof(struct sched_chunkID));
                        cid->flow_id = flows[f];
                        cid->chunk_id = chunkID_multiSet_get_chunk(cset, i, flows[f]);
                        cb = get_chunkbuffer(ct, flows[f]);
                        if(cb)
                        {
                                c = cb_get_chunk(cb, cid->chunk_id);
                                if (c)  // if we still have it in the chunk buffer
                                {
                                        pairs[pairs_len].peer = p;
                                        pairs[pairs_len].chunk = cid;
                                        pairs_len++;
                                }
                        }
#ifdef LOG_CHUNK
                        else
                                log_chunk_error(psinstance_nodeid(ct->ps), p->id, c, E_CACHE_MISS);
#endif
                }
        }
        //Free unused pairs (if any)
        if(pairs_len < ct->chunks_per_peer_offer * nflows)
                pairs = realloc(pairs, sizeof(struct PeerChunk) * pairs_len);

	peer_chunk_send(ct, pairs, pairs_len, transid);
	for(i=0; i<pairs_len; i++)
                free(pairs[i].chunk);
        free(pairs);
        free(flows);

	return 0;
}

int8_t chunk_trader_handle_ack(struct chunk_trader *ct, struct peer *p, struct chunkID_multiSet *cset, uint16_t transid)
{
	chunkID_multiSet_union(peer_bmap(p), cset);
	
	chunkID_multiSet_trimAll(peer_bmap(p), peer_cb_size(p));
        gettimeofday(peer_bmap_timestamp(p), NULL);

	transaction_remove(&(ct->transactions), transid);
	return 0;
}

int8_t chunk_trader_msg_parse(struct chunk_trader *ct, struct nodeID *from, uint8_t *buff, int buff_len)
{
	int res, max_deliver;
	struct nodeID *bmap_owner;
	struct chunkID_multiSet *cset;
	struct peer * p;
	uint16_t trans_id;
	enum signaling_typeMS sig_type;

	res = parseSignalingMS(buff+1, buff_len-1, &bmap_owner, &cset, &max_deliver, &trans_id, &sig_type);
#ifdef LOG_SIGNAL
    log_signal(from, psinstance_nodeid(ct->ps), trans_id, sig_type, "RECEIVED", cset);
#endif
	if (res >= 0)
	{
		res = 0;
		p = nodeid_to_peer(psinstance_topology(ct->ps), from, 0);
		if (p)
			switch (sig_type) {
				case sig_send_buffermap:
					chunkID_multiSet_clear_all(peer_bmap(p), 0);
					chunkID_multiSet_union(peer_bmap(p), cset);
					gettimeofday(peer_bmap_timestamp(p), NULL);
					break;
				case sig_offer:
					p = nodeid_to_peer(psinstance_topology(ct->ps), from, 1);
					chunk_trader_handle_offer(ct, p, cset, max_deliver, trans_id);
					break;
				case sig_accept:
					chunk_trader_handle_accept(ct, p, cset, max_deliver, trans_id);
					break;
				case sig_ack:
					chunk_trader_handle_ack(ct, p, cset, trans_id);
					break;
				default:
				  res = -1;
			}
	} else
		res = -1;

    chunkID_multiSet_free(cset);
	nodeid_free(bmap_owner);
	return res;
}

int8_t chunk_trader_send_bmap(const struct chunk_trader *ct, const struct nodeID *to)
{
	struct chunkID_multiSet *bmap;

	bmap = generateChunkIDMultiSetFromChunkBuffers(ct->cbs, ct->flows_number);
	sendBufferMapMS(psinstance_nodeid(ct->ps), to, psinstance_nodeid(ct->ps), bmap, ct->cb_size, INVALID_TRANSID);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(ct->ps), to, INVALID_TRANSID, sig_send_buffermap,"SENT", bmap);
#endif
	chunkID_multiSet_free(bmap);
	return 0;
}

/** utils **/
suseconds_t chunk_trader_offer_interval(const struct chunk_trader *ct)
{
	struct peerset *pset;
	const struct peer * p;
	int i;
	double load = 0;
	double ms_int = 0;
	suseconds_t offer_int;

	offer_int = chunk_interval_measure(psinstance_measures(ct->ps));
	ms_int = offer_int;
	ms_int *= 0.5;  // we try to relax it a bit 
	
	if (ct->dist_type == DIST_TURBO) {
		pset = topology_get_neighbours(psinstance_topology(ct->ps));
		peerset_for_each(pset,p,i)
			load += peer_evaluation_turbo((struct peer **)&p);
		ms_int /= load;
	} 
        dprintf("Chunk interval estimate is %li, so offer interval is %li \n", offer_int, (suseconds_t)(ms_int / ct->offer_per_period));
	return (suseconds_t)(ms_int / ct->offer_per_period);
}


// Retrieves the correct chunkbuffer. If not found, creates it
struct chunk_buffer * get_chunkbuffer(struct chunk_trader * ct, int flowid)
{
        int i;

        if(ct->flows_number>0)
                for(i = 0; i < ct->flows_number; i++)
                        if(cb_get_flowid(ct->cbs[i]) == flowid)
                                return ct->cbs[i];

  // We didn't find the buffer -> it's a new flow!
  
        struct chunk_buffer **new_buffers=NULL;
        char conf[80];
        sprintf(conf, "size=%d", ct->cb_size);

        new_buffers = realloc(ct->cbs, sizeof(struct chunk_buffer *) * (ct->flows_number+1)); 
        if(!new_buffers)
        {
                dprintf("There are been some problems allocating new buffer!\n");
                return NULL;
        }

        ct->cbs=new_buffers;
        ct->cbs[ct->flows_number]=cb_init(conf);
        cb_set_flowid(ct->cbs[ct->flows_number] , flowid);

        ct->flows_number+=1;
        return ct->cbs[ct->flows_number-1];
}


//  Returns a a chunk from all buffers for the given cid
struct chunk const * get_chunk_multiple(struct chunk_trader * ct, schedChunkID cid)
{
  int i, num_chunks;
  struct chunk const * chunk;
  struct chunk_buffer * cb = get_chunkbuffer(ct, cid->flow_id);
  if(cb) 
     return cb_get_chunk(cb, cid->chunk_id);
  
  return NULL;
}


struct chunk ** get_chunks_multiple(const struct chunk_trader * ct, int ** num_chunks, int * total)
{
  int i;
  struct chunk ** chunks = malloc(sizeof(struct  chunk *) * ct->flows_number);
  *num_chunks = malloc(sizeof(int) * ct->flows_number);
  *total = 0;

  for(i=0; i<ct->flows_number; i++)
  {
    chunks[i]= cb_get_chunks(ct->cbs[i], &((*num_chunks)[i]) );
    *total += (*num_chunks)[i];
  }
  return chunks;
}
 
schedChunkID * chunk_buffer_to_idarray(struct chunk_trader * ct, int * size)
{

        int *num_chunks, i, f, j;
        schedChunkID *cids;
        struct chunk **chunks = get_chunks_multiple(ct, &num_chunks, size);
        cids = malloc(sizeof(schedChunkID) * *size);
        for(f=0, i=0; f<ct->flows_number; f++)
        {
                for(j=0; j< num_chunks[f]; j++)
                {
                        cids[*size - 1 - i] = malloc(sizeof(struct sched_chunkID));
                        cids[*size - 1 - i]->chunk_id = (chunks[f][j]).id;
                        cids[*size - 1 - i]->flow_id = cb_get_flowid(ct->cbs[f]);
                        i++;
                }
        }
        
        free(chunks);
        free(num_chunks);
        return cids;
}
      


void cb_print(const struct chunk_trader * ct)
{
        struct chunk *chunks;
        int num_chunks, i, j;
        for(j=0;j<ct->flows_number;j++)
        {
                chunks = cb_get_chunks(ct->cbs[j], &num_chunks);

                dprintf("\tchbuf #%i (%i elements, flow %i):",j,num_chunks, cb_get_flowid(ct->cbs[j]));
                i = 0;
                if(num_chunks)
                {
                        dprintf("%d -> ",chunks[0].id);
                        for(i=0;i<num_chunks;i++)
                        {
                                dprintf("%d ",chunks[i].id% 100);
                                dprintf("%c, ", 
                                        (chunk_islocked(ct->ch_locks,
                                                        chunks[i].flow_id, chunks[i].id))?
                                        'l' : 'u');
                        }
                }
                dprintf("\n");

        }
}

