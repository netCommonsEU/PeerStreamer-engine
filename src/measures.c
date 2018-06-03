/*
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

#include<measures.h>
#include<string.h>
#include<stdio.h>
#define DEFAULT_CHUNK_INTERVAL (1000000/25)


enum data_state {deinit, loading, ready};

struct chunk_interval_estimate {
        int flowid;
        uint32_t first_index;
	uint32_t last_index;
	uint64_t first_timestamp;
	suseconds_t chunk_interval;
	enum data_state state;
};

struct measures {
	char * filename;
	struct chunk_interval_estimate **cie;
        int cie_size;
        suseconds_t chunk_interval;
};


struct chunk_interval_estimate * get_cie (struct measures *m, int flowid)
{
  int i;
  for (i = 0; i < m->cie_size; i++)
    {
      if (m->cie[i]->flowid == flowid)
	return m->cie[i];
    }

  m->cie = realloc (m->cie, sizeof(struct chunk_interval_estimate *) * (m->cie_size + 1));
  if (m->cie)
    {
      m->cie[m->cie_size] = malloc(sizeof(struct chunk_interval_estimate));
      m->cie[m->cie_size]->flowid = flowid;
      m->cie[m->cie_size]->state = deinit;
      m->cie_size += 1;
    }
  else
    {
      //Something went wrong
      fprintf (stderr, "Error on measures update!\n");
      m->cie_size = 0;
      return NULL;
    }
  return m->cie[m->cie_size-1];
}




struct measures * measures_create(const char * filename)
{
	struct measures * m;
	m = malloc(sizeof(struct measures));
        m->cie = NULL;
        m->cie_size = 0;
        m->chunk_interval = DEFAULT_CHUNK_INTERVAL;
	return m;
}

void measures_destroy(struct measures ** m)
{
	int i;
        if (m && *m)
	{
	        if ((*m)->cie_size > 0 && (*m)->cie)
                {
                  for (i = 0; i < (*m)->cie_size; i++)
                    free ((*m)->cie[i]);
                  free ((*m)->cie);
                }
                free((*m));
	}
}

int8_t reg_chunk_receive(struct measures * m, struct chunk *c) 
{ 
	if (m && c)  // chunk interval estimation
	{
                struct chunk_interval_estimate * cie = get_cie(m, c->flow_id);
                int i,count;
                double s;
		if(cie)
                { 
                        switch (cie->state) {
                                case deinit:
                                        cie->first_index = c->id;
                                        cie->last_index = c->id;
                                        cie->first_timestamp = c->timestamp;
                                        cie->state = loading;
                                        break;
                                case loading:
                                case ready:
                                        if (c->id > (int64_t)cie->last_index)
                                        {
                                                cie->chunk_interval = ((double)(c->timestamp - cie->first_timestamp))/(c->id - cie->first_index) ;
                                                cie->last_index = c->id;
                                                cie->state = ready;
                                                for (i = 0, s = 0, count=0; i < m->cie_size; i++)
                                                {
							if( cie->state == ready && s < cie->chunk_interval)
							{
								s=cie->chunk_interval;
								count++;
							}
                                                }
                                                if(count>0)
                                                        m->chunk_interval = s;
						else
							m->chunk_interval = DEFAULT_CHUNK_INTERVAL;
                                        }
                                        break;
                        }
                        
                }
	}
	return 0;
}

suseconds_t chunk_interval_measure(const struct measures *m)
{
	return m->chunk_interval;
}
