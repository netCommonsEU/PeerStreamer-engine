#include<malloc.h>
#include<assert.h>
#include<psinstance.h>
#include<chunk_trader.h>
void chunk_trader_create_test()
{
	struct chunk_trader * ct;

	//No parameters
        ct = chunk_trader_create(NULL,NULL);
	assert(ct);     
	chunk_trader_destroy(&ct);

	//flow_id
        ct = chunk_trader_create(NULL,"flow_id=10");
	assert(ct);     
	chunk_trader_destroy(&ct);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	chunk_trader_create_test();
	return 0;
}
