/*
 * Copyright (c) 2010-2011 Stefano Traverso
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

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dbg.h"
#include "measures.h"
#include "transaction.h"

typedef struct {
	uint16_t trans_id;
	double offer_sent_time;
	double accept_received_time;
	struct nodeID *id;
	} service_time;

// List to trace peer's service times
struct service_times_element {
	service_time st;
	struct service_times_element *backward;
	struct service_times_element *forward;
	};


void _transaction_remove(struct service_times_element ** head, struct service_times_element *stl)
{
	if (stl->backward == NULL) {
		if (stl->forward)
		{
			*head = stl->forward;
			(*head)->backward = NULL;
		} else
			*head = NULL;
	}
	else {
		if (stl->forward == NULL)
			stl->backward->forward = NULL;
		else {
			stl->backward->forward = stl->forward;
			stl->forward->backward = stl->backward;
		}
	}
	nodeid_free(stl->st.id);
	free(stl);
}	

// Check the service times list to find elements over the timeout
void check_neighbor_status_list(struct service_times_element ** stl) {
	struct service_times_element *stl_iterator, *stl_aux;
	struct timeval current_time;

	gettimeofday(&current_time, NULL);
	
        dprintf("LIST: check trans_id list\n");
	
	// Check if list is empty
	if (*stl == NULL) {
		return;
		}
        
	// Start from the beginning of the list
	stl_iterator = *stl;
	stl_aux = stl_iterator->forward;
	// Iterate the list until you get the right element
	while (stl_iterator != NULL) {
		// If the element has been in the list for a period greater than the timeout, remove it
//		if ( (stl_iterator->st.accept_received_time > 0.0 && ( (current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.accept_received_time) > TRANS_ID_MAX_LIFETIME) ||  ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.offer_sent_time > TRANS_ID_MAX_LIFETIME ) ) {
		if ( (current_time.tv_sec + current_time.tv_usec*1e-6 - stl_iterator->st.offer_sent_time) > TRANS_ID_MAX_LIFETIME) {
			 dprintf("LIST TIMEOUT: trans_id %d, offer_sent_time %f, accept_received_time %f\n", stl_iterator->st.trans_id, (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.offer_sent_time  ), (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.accept_received_time));
			 //fprintf(stderr, "LIST TIMEOUT: trans_id %d, offer_sent_time %f, accept_received_time %f\n", stl_iterator->st.trans_id, (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.offer_sent_time  ), (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.accept_received_time));
		_transaction_remove(stl, stl_iterator);

			// Free the memory
		}
		stl_iterator = stl_aux;
		if (stl_iterator)
			stl_aux = stl_aux->forward;
	}
	return;
}

// register the moment when a transaction is started
// return a  new transaction id or 0 in case of failure
uint16_t transaction_create(struct service_times_element ** stl, struct nodeID *id)
{
	struct service_times_element *stl2;
	struct timeval current_time;

	if (stl && id)
	{
		check_neighbor_status_list(stl);

		gettimeofday(&current_time, NULL);


		// create a new element in the list with its offer_sent_time and set accept_received_time to -1.0
		stl2 = (struct service_times_element*) malloc(sizeof(struct service_times_element));
		stl2->st.offer_sent_time = current_time.tv_sec + current_time.tv_usec*1e-6;
		stl2->st.accept_received_time = -1.0;
		stl2->st.id = nodeid_dup(id);

		stl2->backward = NULL;

		if (*stl)
		{
			stl2->st.trans_id = (*stl)->st.trans_id + 1 ? (*stl)->st.trans_id + 1 : 1;
			(*stl)->backward = stl2;
			stl2->forward = (*stl);
			*stl = stl2;
		} else {
			stl2->st.trans_id = 1;
			stl2->forward = NULL;
			*stl = stl2;
		}
		dprintf("LIST: adding trans_id %d to the list, offer_sent_time %f -- LIST IS NOT EMPTY\n", stl2->st.trans_id, (current_time.tv_sec + current_time.tv_usec*1e-6));

		return stl2->st.trans_id;
	}
	return 0;
}


// Add the moment I received a positive select in a list
// return true if a valid trans_id is found
bool transaction_reg_accept(struct service_times_element * stl, uint16_t trans_id,const struct nodeID *id)
{
	struct service_times_element *stl_iterator;
	struct timeval current_time;

	if (stl && id && trans_id)
	{
		gettimeofday(&current_time, NULL);
		stl_iterator = stl;

		// if an accept was received, look for the trans_id and add current_time to accept_received_time field
		dprintf("LIST: changing trans_id %d to the list, accept received %f\n", trans_id, current_time.tv_sec + current_time.tv_usec*1e-6);
		// Iterate the list until you get the right element
		while (stl_iterator != NULL) {
				if (stl_iterator->st.trans_id == trans_id) {
					stl_iterator->st.accept_received_time = current_time.tv_sec + current_time.tv_usec*1e-6;
					return true;
					}
				stl_iterator = stl_iterator->forward;
		}
	}
	return false;
}

// Used to get the time elapsed from the moment I get a positive select to the moment i get the ACK
// related to the same chunk
// it return -1.0 in case no trans_id is found
double transaction_remove(struct service_times_element ** stl, uint16_t trans_id) {
	struct service_times_element *stl_iterator;
	double to_return;

    dprintf("LIST: deleting trans_id %d\n", trans_id);

	// Start from the beginning of the list
	stl_iterator = *stl;
	// Iterate the list until you get the right element
	while (stl_iterator != NULL) {
		if (stl_iterator->st.trans_id == trans_id)
			break;
		stl_iterator = stl_iterator->forward;
		}
	if (stl_iterator == NULL){
		// not found
        dprintf("LIST: deleting trans_id %d -- STL is already NULL \n", trans_id);
		return -2.0;
	}
	
	to_return = stl_iterator->st.accept_received_time;

	_transaction_remove(stl, stl_iterator);

	return to_return;
}

void transaction_destroy(struct service_times_element ** head)
{
	while(*head)
	{
		_transaction_remove(head, *head);
		// in case of nodeid_dup we should free that as well
	}

}
