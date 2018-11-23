#include<malloc.h>
#include<assert.h>
#include<chunkidms.h>


void chunkid_multiset_iterator_create_test()
{
	struct chunkID_multiSet_iterator * iter;
	struct chunkID_multiSet * set = NULL;

	iter = chunkID_multiSet_iterator_create(set);
	assert(iter == NULL);

	set = chunkID_multiSet_init(5,5);

	iter = chunkID_multiSet_iterator_create(set);
	assert(iter);
	free(iter);

	chunkID_multiSet_free(set);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void chunkid_multiset_iterator_next_test()
{
	struct chunkID_multiSet_iterator * iter;
	struct chunkID_multiSet * set = NULL;
	int res;
	chunkid_t cid;
	flowid_t fid;

	set = chunkID_multiSet_init(5,5);

	iter = chunkID_multiSet_iterator_create(set);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res);
	free(iter);

	chunkID_multiSet_add_chunk(set, 0, 0);
	chunkID_multiSet_add_chunk(set, 1, 0);
	chunkID_multiSet_add_chunk(set, 0, 1);
	chunkID_multiSet_add_chunk(set, 2, 1);
	chunkID_multiSet_add_chunk(set, 3, 1);
	chunkID_multiSet_add_chunk(set, 42, 3);
	chunkID_multiSet_add_chunk(set, 25, 3);

	iter = chunkID_multiSet_iterator_create(set);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==1 && fid==0);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==3 && fid==1);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==42 && fid==3);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==0 && fid==0);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==2 && fid==1);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==25 && fid==3);
	res = chunkID_multiSet_iterator_next(iter, &cid, &fid);
	assert(res==0 && cid==0 && fid==1);
	free(iter);

	chunkID_multiSet_free(set);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	chunkid_multiset_iterator_create_test();
	chunkid_multiset_iterator_next_test();
	return 0;
}
