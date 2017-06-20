#include<malloc.h>
#include<assert.h>
#include<transaction.h>

void transaction_create_test()
{
	struct service_times_element * head = NULL;
	struct nodeID * id;
	uint16_t tid;

	tid = transaction_create(NULL, NULL);
	assert(tid == 0);

	tid = transaction_create(&head, NULL);
	assert(tid == 0);

	id = create_node("127.0.0.1", 6000);
	tid = transaction_create(&head, id);
	assert(tid == 1);

	tid = transaction_create(&head, id);
	assert(tid == 2);

	transaction_destroy(&head);
	assert(head == NULL);
	nodeid_free(id);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void transaction_reg_accept_test()
{
	struct service_times_element * head = NULL;
	struct nodeID * id = NULL;
	uint16_t tid = 1;
	bool res;

	res = transaction_reg_accept(head, 0, id);
	assert(!res);

	res = transaction_reg_accept(head, tid, id);
	assert(!res);

	id = create_node("127.0.0.1", 6000);
	tid = transaction_create(&head, id);

	res = transaction_reg_accept(head, tid, id);
	assert(res);

	res = transaction_reg_accept(head, tid+1, id);
	assert(!res);

	nodeid_free(id);
	transaction_destroy(&head);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	transaction_create_test();
	transaction_reg_accept_test();
	return 0;
}
