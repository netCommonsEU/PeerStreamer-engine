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

#include<network_shaper.h>
#include<grapes_config.h>
#include<stdio.h>

struct network_shaper {
	float multiplyer;
	float alpha_memory;
	double estimated_byterate_persecond;
	struct timeval next_sending_event;
	struct timeval last_update_time;
};

struct network_shaper * network_shaper_create(const char * config)
{
	struct network_shaper * ns = NULL;
	struct tag * tags = NULL;
	double mul;

	ns = malloc(sizeof(struct network_shaper));
	gettimeofday(&(ns->next_sending_event), NULL);
	ns->multiplyer = DEFAULT_BYTERATE_MULTIPLYER;
	ns->alpha_memory = 0.9;
	ns->estimated_byterate_persecond = DEFAULT_BYTERATE;
	gettimeofday(&(ns->last_update_time), NULL);
	gettimeofday(&(ns->next_sending_event), NULL);

	if (config)
	{
		tags = grapes_config_parse(config);
		grapes_config_value_double_default(tags, "byterate", &(ns->estimated_byterate_persecond), DEFAULT_BYTERATE);
		grapes_config_value_double_default(tags, "byterate_multiplyer", &mul, DEFAULT_BYTERATE_MULTIPLYER);
		ns->multiplyer = mul;
		free(tags);
	}

	return ns;
}

void network_shaper_destroy(struct network_shaper ** ns)
{
	if (ns && *ns)
	{
		free(*ns);
		*ns = NULL;
	}
}

int8_t network_shaper_next_sending_interval(struct network_shaper * ns, struct timeval * interval)
{
	int8_t res = -1;
	struct timeval now;

	if (ns && interval)
	{
		gettimeofday(&now, NULL);
		if (timercmp(&now, &(ns->next_sending_event), <))
			timersub(&(ns->next_sending_event), &now, interval);
		else
		{
			interval->tv_sec = 0;
			interval->tv_usec = 0;
		}
		res = 0;
	}

	return res;
}

int8_t network_shaper_register_sent_bytes(struct network_shaper * ns, size_t data_size)
{
	double period;
	uint32_t secs;
	int8_t res = -1;

	if (ns && data_size > 0)
	{
		period = ((double)data_size) / (ns->multiplyer * ns->estimated_byterate_persecond);
		secs = (time_t) period;
		ns->next_sending_event.tv_sec += secs;
		ns->next_sending_event.tv_usec += (suseconds_t) ((period - secs)*1000000);
		res = 0;
	}
	return res;
}

int8_t network_shaper_update_bitrate(struct network_shaper * ns, size_t data_size)
{
	struct timeval now;
	struct timeval interval;
	double period;
	int8_t res = -1;

	if (ns && data_size > 0)
	{
		gettimeofday(&now, NULL);
		timersub(&now, &(ns->last_update_time), &interval);
		period = interval.tv_sec + ((double)interval.tv_usec)/1000000;
		ns->estimated_byterate_persecond = ns->alpha_memory * ns->estimated_byterate_persecond + 
			(1 - ns->alpha_memory) * data_size/period;
		ns->last_update_time = now;
		res = 0;
	}

	return res;
}
