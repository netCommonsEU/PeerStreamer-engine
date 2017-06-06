#include<malloc.h>
#include<assert.h>
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

	ps = psinstance_create("127.0.0.1", 5000, "port=8000");
	assert(ps);  
	psinstance_destroy(&ps);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	psinstance_create_test();
	return 0;
}
