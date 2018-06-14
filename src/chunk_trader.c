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
#include<trade_msg_ha.h>
#include<chunkidset.h>
#include<chunk_attributes.h>

#include<net_helpers.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

enum distribution_type {DIST_UNIFORM, DIST_TURBO};

struct chunk_trader{
	struct chunk_buffer *cb;
	struct chunk_locks * ch_locks;
	const struct psinstance * ps;
	int cb_size;
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
	free(tags);

	sprintf(conf, "size=%d", ct->cb_size);
	ct->cb = cb_init(conf);
	ct->ch_locks = chunk_locks_create(80);  // milliseconds of lock time
	return ct;
}

void chunk_trader_destroy(struct chunk_trader **ct)
{
	if (ct && *ct)
	{
		if(((*ct)->ch_locks))
			chunk_locks_destroy(&((*ct)->ch_locks));
		if(((*ct)->transactions))
			transaction_destroy(&((*ct)->transactions));
		if(((*ct)->cb))
			cb_destroy((*ct)->cb);
		free(*ct);
		*ct = NULL;
	}
}

int8_t chunk_trader_add_chunk(struct chunk_trader *ct, struct chunk *c)
{
	int res = -1;

	if (ct && c)
	{
		res = cb_add_chunk(ct->cb, c);
		if (res)
			log_chunk_error(psinstance_nodeid(ct->ps), NULL, c, res);
		res = res < 0 ? -1 : 0;
	}
	return res;
}

int8_t peer_chunk_send(struct chunk_trader * ct, struct PeerChunk *pairs, int pairs_len, uint16_t transid)
{
	int i, res =-1;
	struct peer * target_peer;
	struct chunk * target_chunk;

	for (i=0; i<pairs_len; i++)
	{
		target_peer = pairs[i].peer;
		target_chunk = (struct chunk *) cb_get_chunk(ct->cb, pairs[i].chunk);

		res = sendChunk(psinstance_nodeid(ct->ps),target_peer->id, target_chunk, transid);	//we use transactions in order to register acks for push
		if (res >= 0)
		{
			chunk_attributes_update_upon_sending(target_chunk);
			chunkID_set_add_chunk(peer_bmap(target_peer), target_chunk->id);
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

double chunk_evaluation_latest(int *cid)
{
	return *cid;
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

	if (ct && c && multiplicity > 0)
	{
		pset = topology_get_neighbours(psinstance_topology(ct->ps));
		peer_num = peerset_size(pset);
		peers = peerset_get_peers(pset);
		
		pairs_len = MIN(multiplicity, peer_num);
		pairs = malloc(pairs_len * sizeof(struct PeerChunk));
		
		if (ct->dist_type == DIST_TURBO)
			schedSelectChunkFirst(SCHED_WEIGHTED, peers, peer_num, (int *)&(c->id), 1, pairs, &pairs_len, NULL, peer_evaluation_turbo, chunk_evaluation_latest);
		else
			schedSelectChunkFirst(SCHED_WEIGHTED, peers, peer_num, (int *)&(c->id), 1, pairs, &pairs_len, NULL, peer_evaluation_uniform, chunk_evaluation_latest);

		peer_chunk_send(ct, pairs, pairs_len, INVALID_TRANSID);
		free(pairs);
	}
	return res;
}

int * chunk_buffer_to_idarray(struct chunk_buffer * chbuff, int *n)
{
	int * buff, i;
	struct chunk * chunks;

	chunks = cb_get_chunks(chbuff, n);
	buff = malloc(sizeof(int)*(*n));
	for(i=0; i<*n; i++)
		buff[i] = chunks[i].id;
	return buff;
}

struct chunkID_set * chunk_buffer_to_idset(struct chunk_buffer * chbuff)
{
	struct chunk * chunks;
	int n, i;
	struct chunkID_set * bmap;

	bmap = chunkID_set_init("type=bitmap");
	chunks = cb_get_chunks(chbuff, &n);

	if (chunks)
		for(i=0; i<n; i++)
			chunkID_set_add_chunk(bmap, chunks[i].id);
	return bmap;
}

int8_t chunk_trader_send_ack(struct chunk_trader *ct, struct nodeID *to, uint16_t transid)
{
	struct chunkID_set * bmap;

	bmap = chunk_buffer_to_idset(ct->cb);
	sendAck(psinstance_nodeid(ct->ps), to, bmap, transid);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(ct->ps), to, chunkID_set_size(bmap), transid, sig_ack, "SENT");
#endif
	chunkID_set_free(bmap);
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
			chunk_unlock(ct->ch_locks, c->id); // in case we locked it in a select message
#ifdef LOG_CHUNK
			log_chunk(from, psinstance_nodeid(ct->ps), c, "RECEIVED");
#endif
			p = nodeid_to_peer(psinstance_topology(ct->ps), from, 0);
			if (p)
				chunkID_set_add_chunk(peer_bmap(p), c->id);  // keep track it has this chunk for sure
			chunk_trader_send_ack(ct, from, transid);

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
int peer_needs_chunk_filter(struct peer *p, int cid)
{
	int min;

	if (peer_cb_size(p) == 0) // it does not have capacity
		return 0;
	if (chunkID_set_check(peer_bmap(p),cid) < 0)  // not in bmap
	{
		if(peer_cb_size(p) > chunkID_set_size(peer_bmap(p))) // it has room for chunks anyway
		{
			min = chunkID_set_get_earliest(peer_bmap(p)) - peer_cb_size(p) + chunkID_set_size(peer_bmap(p));
			min = min < 0 ? 0 : min;
			if (cid >= min)
				return 1;
		}
		if((int)chunkID_set_get_earliest(peer_bmap(p)) < cid)  // our is reasonably new
			return 1;
	}
	return 0;
}

int8_t chunk_trader_send_offer(struct chunk_trader *ct)
{
	struct peerset *pset;
	struct peer ** neighs;
	int n_neighs, n_chunks, j;
	size_t n_pairs, i;
	struct PeerChunk * pairs;
	struct chunkID_set * offer_cset;
	int * ch_buff;
	int8_t res = 0;
	uint16_t transid;

	pset = topology_get_neighbours(psinstance_topology(ct->ps));
	n_neighs = peerset_size(pset);
	neighs = peerset_get_peers(pset);
	ch_buff = chunk_buffer_to_idarray(ct->cb, &n_chunks);
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
			offer_cset = chunkID_set_init("type=bitmap");
			for(i=0; i<n_pairs; i++)
				chunkID_set_add_chunk(offer_cset, pairs[i].chunk);
			transid = transaction_create(&(ct->transactions), pairs[0].peer->id);
			offerChunks(psinstance_nodeid(ct->ps), pairs[0].peer->id, offer_cset, ct->chunks_per_peer_offer, transid);
#ifdef LOG_SIGNAL
			log_signal(psinstance_nodeid(ct->ps), pairs[0].peer->id, chunkID_set_size(offer_cset), transid, sig_offer, "SENT");
#endif
			chunkID_set_free(offer_cset);
			res++;
		}
	}
	free(pairs);
	free(ch_buff);
	return res;
}

/* Latest-useful chunk selection */
int8_t chunk_trader_handle_offer(struct chunk_trader *ct, struct peer *p, struct chunkID_set *cset, int max_deliver, uint16_t trans_id)
{
	int *ids, min, max, cid, i;
	struct chunkID_set * acc_set;

	acc_set = chunkID_set_init("type=bitmap");

	chunkID_set_union(peer_bmap(p), cset);
	chunkID_set_trim(peer_bmap(p), peer_cb_size(p));
	gettimeofday(peer_bmap_timestamp(p), NULL);

	/* we use counting sort to sort chunks in O(~|cset|) */
	min = chunkID_set_get_earliest(cset);
	max = chunkID_set_get_latest(cset);
	ids = malloc(sizeof(int)* (max-min+1));
	memset(ids, 0, sizeof(int)*(max-min+1));

	for(i=0; i<chunkID_set_size(cset); i++)
	{
		cid = chunkID_set_get_chunk(cset, i);
		ids[cid-min] += 1;
	}

	/* we select the latest useful */
	cid = max;
	while(cid>=min && chunkID_set_size(acc_set) < max_deliver)
	{
		if (ids[cid-min] && !cb_get_chunk(ct->cb, cid) && !chunk_islocked(ct->ch_locks, cid))
		{
			chunkID_set_add_chunk(acc_set, cid);
			chunk_lock(ct->ch_locks, cid, p);
		}
		cid--;
	}

    acceptChunks(psinstance_nodeid(ct->ps), p->id, acc_set, trans_id);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(ct->ps), p->id, chunkID_set_size(acc_set), trans_id, sig_accept, "SENT");
#endif

	chunkID_set_free(acc_set);
	free(ids);
	return 0;
}

int8_t chunk_trader_handle_accept(struct chunk_trader *ct, struct peer *p, struct chunkID_set *cset, int max_deliver, uint16_t transid)
{
	int cid, i, max_chunks, pairs_len = 0;
	const struct chunk *c;
	struct PeerChunk * pairs;

	max_chunks = MIN(chunkID_set_size(cset), ct->chunks_per_peer_offer);
	pairs = malloc(sizeof(struct PeerChunk) * max_chunks);  

	transaction_reg_accept(ct->transactions, transid, p->id);

	for(i=0, pairs_len=0; i<chunkID_set_size(cset) && pairs_len < max_chunks; i++)
	{
		cid = chunkID_set_get_chunk(cset, i);
		c = cb_get_chunk(ct->cb, cid);
		if (c)  // if we still have it in the chunk buffer
		{
			pairs[pairs_len].peer = p;
			pairs[pairs_len].chunk = cid;
			pairs_len++;
		} 
#ifdef LOG_CHUNK
		else
			log_chunk_error(psinstance_nodeid(ct->ps), p->id, c, E_CACHE_MISS);
#endif
	}
	peer_chunk_send(ct, pairs, pairs_len, transid);
	free(pairs);

	return 0;
}

int8_t chunk_trader_handle_ack(struct chunk_trader *ct, struct peer *p, struct chunkID_set *cset, uint16_t transid)
{
	chunkID_set_union(peer_bmap(p), cset);
	chunkID_set_trim(peer_bmap(p), peer_cb_size(p));
	gettimeofday(peer_bmap_timestamp(p), NULL);

	transaction_remove(&(ct->transactions), transid);
	return 0;
}

int8_t chunk_trader_msg_parse(struct chunk_trader *ct, struct nodeID *from, uint8_t *buff, int buff_len)
{
	int res, max_deliver;
	struct nodeID *bmap_owner;
	struct chunkID_set *cset;
	struct peer * p;
	uint16_t trans_id;
	enum signaling_type sig_type;

	res = parseSignaling(buff+1, buff_len-1, &bmap_owner, &cset, &max_deliver, &trans_id, &sig_type);
#ifdef LOG_SIGNAL
    log_signal(from, psinstance_nodeid(ct->ps), chunkID_set_size(cset), trans_id, sig_type, "RECEIVED");
#endif
	if (res >= 0)
	{
		res = 0;
		p = nodeid_to_peer(psinstance_topology(ct->ps), from, 0);
		if (p)
			switch (sig_type) {
				case sig_send_buffermap:
					chunkID_set_clear(peer_bmap(p), 0);
					chunkID_set_union(peer_bmap(p), cset);
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

    chunkID_set_free(cset);
	nodeid_free(bmap_owner);
	return res;
}

int8_t chunk_trader_send_bmap(const struct chunk_trader *ct, const struct nodeID *to)
{
	struct chunkID_set *bmap;

	bmap = chunk_buffer_to_idset(ct->cb);
	sendBufferMap(psinstance_nodeid(ct->ps), to, psinstance_nodeid(ct->ps), bmap, psinstance_is_source(ct->ps) ? 0 : ct->cb_size, INVALID_TRANSID);
#ifdef LOG_SIGNAL
	log_signal(psinstance_nodeid(ct->ps), to, chunkID_set_size(bmap), INVALID_TRANSID, sig_send_buffermap,"SENT");
#endif
	chunkID_set_free(bmap);
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
	ms_int *= 0.75;  // we try to relax it a bit 
	
	if (ct->dist_type == DIST_TURBO) {
		pset = topology_get_neighbours(psinstance_topology(ct->ps));
		peerset_for_each(pset,p,i)
			load += peer_evaluation_turbo((struct peer **)&p);
		ms_int /= load;
	} 

	return (suseconds_t)(ms_int / ct->offer_per_period);
}
