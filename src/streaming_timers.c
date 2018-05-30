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

#include<streaming_timers.h>
#include<malloc.h>
#include<dbg.h>

void usec2timeval(struct timeval *t, suseconds_t usec)
{	
	t->tv_sec = usec / 1000000;
	t->tv_usec = usec % 1000000;
}

void timeradd_interval(struct timeval * dst, const suseconds_t interval)
	/* interval is in micro-seconds */
{
	struct timeval tmp;
	usec2timeval(&tmp, interval);
	timeradd(dst, &tmp, dst);
}


int streaming_timers_init(struct streaming_timers * psl, suseconds_t offer_interval)
{
	gettimeofday(&(psl->awake_epoch), NULL);
	psl->offer_epoch = psl->awake_epoch;
	timeradd_interval(&(psl->offer_epoch), offer_interval);
	psl->chunk_epoch = psl->awake_epoch;
	return 0;
}

void streaming_timers_set_timeout(struct streaming_timers * psl, suseconds_t interval, int8_t userfds)
{
	struct timeval tnow;
	struct timeval delta;

	psl->awake_epoch = psl->offer_epoch;
	if (userfds && timercmp(&(psl->chunk_epoch), &(psl->offer_epoch), <))
		psl->awake_epoch = psl->chunk_epoch;

	gettimeofday(&tnow, NULL);
	if (timercmp(&tnow, &(psl->awake_epoch), >))
	{
		psl->sleep_timer.tv_sec = 0;
		psl->sleep_timer.tv_usec = 0;
	}
	else {
		timersub(&(psl->awake_epoch), &tnow, &(psl->sleep_timer));
		usec2timeval(&delta, interval);
		if (timercmp(&delta, &(psl->sleep_timer), <))
			psl->sleep_timer = delta;
	}
}

enum streaming_action streaming_timers_state_handler(struct streaming_timers * psl, int data_state)
{
	enum streaming_action action = NO_ACTION;
	struct timeval current_epoch;
		
	switch (data_state) {
		case -1: // Error
			dtprintf("Invalid data state!\n");
			break;
		case 0: // timeout, no socket has data to pick
			gettimeofday(&current_epoch, NULL);
			if (timercmp(&(psl->chunk_epoch), &current_epoch, <)) // chunk seeding time! 
				action = INJECT_ACTION;
			else 
				if (timercmp(&(psl->offer_epoch), &current_epoch, <)) // offer time
					action = OFFER_ACTION;
			break;
		case 1: // incoming msg
			action = PARSE_MSG_ACTION;
			break;
		case 2: // input file descriptor ready
			action = INJECT_ACTION;
			break;
	}

	psl->loop_counter++;
	return action;
}

int8_t streaming_timers_update_flag(struct streaming_timers * psl)
{
	if (psl->loop_counter >= UPDATE_EPOCH)
	{
		psl->loop_counter = 0;
		return 1;
	}
	return 0;
}

void streaming_timers_update_chunk_time(struct streaming_timers * psl, suseconds_t interval)
{
	timeradd_interval(&(psl->chunk_epoch), interval);
}

void streaming_timers_update_offer_time(struct streaming_timers * psl, suseconds_t interval)
{
	timeradd_interval(&(psl->offer_epoch), interval);
}

