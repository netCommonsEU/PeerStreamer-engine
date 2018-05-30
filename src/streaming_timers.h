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

#ifndef __STREAMING_TIMERS_H__
#define __STREAMING_TIMERS_H__

#include<sys/time.h>
#include<stdint.h>

#define UPDATE_EPOCH 10

enum streaming_action {OFFER_ACTION, INJECT_ACTION, PARSE_MSG_ACTION, NO_ACTION};

struct streaming_timers {
	struct timeval sleep_timer;
	struct timeval awake_epoch;
	struct timeval offer_epoch;
	struct timeval chunk_epoch;
	uint32_t loop_counter;
};

int streaming_timers_init(struct streaming_timers * psl, suseconds_t offer_interval);

void streaming_timers_set_timeout(struct streaming_timers * psl, suseconds_t interval, int8_t userfds);

enum streaming_action streaming_timers_state_handler(struct streaming_timers * psl, int data_state);

int8_t streaming_timers_update_flag(struct streaming_timers * psl);

void streaming_timers_update_chunk_time(struct streaming_timers * psl, suseconds_t interval);

void streaming_timers_update_offer_time(struct streaming_timers * psl, suseconds_t interval);

#endif
