#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<sys/time.h>
#include<network_manager.h>
#include<network_shaper.h>


void network_shaper_create_test()
{
	struct network_shaper * ns;

	ns = network_shaper_create(NULL);
	assert(ns);
	
	network_shaper_destroy(&ns);
	assert(ns == NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_shaper_next_sending_interval_test()
{
	struct network_shaper * ns = NULL;
	struct timeval interval;
	int8_t res;

	res = network_shaper_next_sending_interval(ns, NULL);
	assert(res < 0);

	ns = network_shaper_create(NULL);
	res = network_shaper_next_sending_interval(ns, NULL);
	assert(res < 0);

	res = network_shaper_next_sending_interval(ns, &interval);
	assert(res == 0);
	assert(interval.tv_sec == 0);
	assert(interval.tv_usec == 0);
	network_shaper_destroy(&ns);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_shaper_register_sent_bytes_test()
{
	struct network_shaper * ns = NULL;
	struct timeval interval;
	int8_t res;

	res = network_shaper_register_sent_bytes(ns, 0);
	assert(res < 0);

	ns = network_shaper_create("byterate=1000000,byterate_multiplyer=2");
	res = network_shaper_register_sent_bytes(ns, 0);
	assert(res < 0);

	res = network_shaper_register_sent_bytes(ns, 500000);
	assert(res == 0);
	network_shaper_next_sending_interval(ns, &interval);
	assert(interval.tv_sec == 0);
	assert(interval.tv_usec >= 200000);
	assert(interval.tv_usec <= 300000);

	res = network_shaper_register_sent_bytes(ns, 500000);
	assert(res == 0);
	network_shaper_next_sending_interval(ns, &interval);
	assert(interval.tv_sec == 0);
	assert(interval.tv_usec >= 450000);
	assert(interval.tv_usec <= 550000);

	network_shaper_destroy(&ns);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_shaper_update_bitrate_test()
{
	struct network_shaper * ns = NULL;
	int8_t res;

	res = network_shaper_update_bitrate(ns, 0);
	assert(res < 0);

	ns = network_shaper_create("byterate=1000000,byterate_multiplyer=2");

	res = network_shaper_update_bitrate(ns, 0);
	assert(res < 0);

	res = network_shaper_update_bitrate(ns, 500000);
	assert(res == 0);

	network_shaper_destroy(&ns);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	network_shaper_create_test();
	network_shaper_next_sending_interval_test();
	network_shaper_register_sent_bytes_test();
	network_shaper_update_bitrate_test();
	return 0;
}
