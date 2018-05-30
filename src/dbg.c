/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2018 Massimo Girondi
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
#include<sys/time.h>
#include<stdio.h>
#include<stdarg.h>
#include<time.h>
#include<inttypes.h>

#include<dbg.h>
#include<chunk_trader.h>
#include<net_helpers.h>
#include<peerset.h>
#include<chunkbuffer.h>
#include<chunkidms.h>
#include<chunkidms_trade.h>

int ftprintf(FILE *stream, const char *format, ...)
{
  va_list ap;
  int ret;
  struct timeval tnow;
  
  gettimeofday(&tnow, NULL);
  fprintf(stream, "%ld.%03ld ", tnow.tv_sec, tnow.tv_usec/1000);

  va_start (ap, format);
  ret = vfprintf(stderr, format, ap);
  va_end (ap);
  
  return ret;
}

uint64_t gettimeofday_in_us(void)
{
	struct timeval what_time; //to store the epoch time

	gettimeofday(&what_time, NULL);
	return what_time.tv_sec * 1000000ULL + what_time.tv_usec;
}

void log_signal(const struct nodeID *fromid,const struct nodeID *toid,uint16_t trans_id, enum signaling_typeMS type,const char *flag, const struct chunkID_multiSet *ms)
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
	fprintf(stderr,"[SIGNAL_LOG],%"PRIu64",%s,%s,%d,%s,%s\n",gettimeofday_in_us(),sndr,rcvr,trans_id,typestr,flag);
        if(ms)
        {
                fprintf(stderr,"\t---->%d sets, %d chunks\n",chunkID_multiSet_size(ms), chunkID_multiSet_total_size(ms));
#ifdef LOG_CHUNK
                chunkID_multiSet_print(stderr, ms);
#endif
        }
}

void log_chunk(const struct nodeID *from,const struct nodeID *to,const struct chunk *c,const char * note)
{
	// semantic: [CHUNK_LOG],log_date,sender,receiver,id,size(bytes),chunk_timestamp,hopcount,notes
	char sndr[NODE_STR_LENGTH] = "ND",rcvr[NODE_STR_LENGTH] = "ND";
	if (from)
	node_addr(from,sndr,NODE_STR_LENGTH);
	if (to)
		node_addr(to,rcvr,NODE_STR_LENGTH);

	if (c)
		fprintf(stderr,"[CHUNK_LOG],%"PRIu64",%s,%s,%d[%d],%d,%"PRIu64",%i,%s\n",gettimeofday_in_us(),sndr,rcvr,c->id,c->flow_id,c->size,c->timestamp,chunk_attributes_get_hopcount(c),note);
	else
		fprintf(stderr,"[CHUNK_LOG],%"PRIu64",%s,%s,%d,%d,%d,%i,%s\n",gettimeofday_in_us(),sndr,rcvr,-1,-1,0,0,note);
}

void log_neighbourhood(const struct psinstance * ps)
{
  struct peerset * pset;
  const struct peer * p;
  int psetsize,i;
  uint64_t now;
  char me[NODE_STR_LENGTH];

  node_addr(psinstance_nodeid(ps), me, NODE_STR_LENGTH);
  pset = topology_get_neighbours(psinstance_topology(ps));
  psetsize = peerset_size(pset);
  now = gettimeofday_in_us();
  peerset_for_each(pset,p,i)
    fprintf(stderr,"[NEIGHBOURHOOD],%"PRIu64",%s,%s,%d\n",now,me,nodeid_static_str(p->id),psetsize);

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
		case E_CANNOT_PARSE:
			log_chunk(from,to,NULL,"CANNOT_PARSE");
			break;
		case E_CACHE_MISS:
			log_chunk(from,to,NULL,"CACHE_MISS");
			break;
		default:
			log_chunk(from,to,c,"ERROR");
	} 
}


