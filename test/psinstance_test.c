#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<psinstance.h>

void psinstance_create_test()
{
	struct psinstance * ps;

	ps = psinstance_create(NULL, -1, NULL);
	assert(ps == NULL);
	ps = psinstance_create("null", -1, NULL);
	assert(ps == NULL);

	ps = psinstance_create("null", 0, "port=6700");
	assert(ps);  // source created
	psinstance_destroy(&ps);
	
	ps = psinstance_create("null", 5000, NULL);
	assert(ps==NULL);  

	ps = psinstance_create("127.0.0.1", 5000, "iface=lo,port=8001");
	assert(ps);  
	psinstance_destroy(&ps);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void psinstance_ip_address_test()
{
	struct psinstance * ps = NULL;
	char ip[80];
	
	assert(psinstance_ip_address(ps, NULL, 80) < 0);

	ps = psinstance_create("127.0.0.1", 5000, "iface=lo,port=8000");

	assert(psinstance_ip_address(ps, NULL, 80) < 0);

	assert(psinstance_ip_address(ps, ip, 80) == 0);
	assert(strcmp(ip, "127.0.0.1") == 0);

	psinstance_destroy(&ps);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void psinstance_port_test()
{
	struct psinstance * ps = NULL;

	assert(psinstance_port(ps) < 0);

	ps = psinstance_create("127.0.0.1", 5000, "iface=lo,port=8000");

	assert(psinstance_port(ps) == 8000);

	psinstance_destroy(&ps);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	psinstance_create_test();
	psinstance_ip_address_test();
	psinstance_port_test();
	return 0;
}
