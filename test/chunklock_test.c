#include<malloc.h>
#include<assert.h>
#include<chunklock.h>
#include<net_helper.h>
#include<peer.h>
#include<unistd.h>

struct peer * create_peer()
{
	struct peer * p;

	p = malloc(sizeof(struct peer));
	p->id = create_node("10.0.0.1", 6000);
	p->metadata = NULL;
	p->user_data = NULL;

	return p;
}

void destroy_peer(struct peer ** p)
{
	if (p && *p)
	{
		nodeid_free((*p)->id);
		free(*p);
		*p = NULL;
	}
}

void chunk_locks_create_test()
{
	struct chunk_locks * locks = NULL;
	locks = chunk_locks_create(2);
	assert(locks);
	chunk_locks_destroy(&locks);
	assert(locks == NULL);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void chunk_lock_test()
{
	struct peer * p;
	struct chunk_locks * locks = NULL;

	locks = chunk_locks_create(2);
	p = create_peer();

	chunk_lock(NULL,1,0, NULL);
	chunk_lock(locks, 1, 0, NULL);
	chunk_lock(locks, 2, 0, p);
	chunk_lock(locks, 1, 1, p);
	chunk_lock(locks, 2, 3, p);

	chunk_locks_destroy(&locks);
	destroy_peer(&p);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void chunk_unlock_test()
{
	struct chunk_locks * locks = NULL;
	struct peer * p;

	chunk_unlock(NULL, 0, 0);

	locks = chunk_locks_create(2);
	p = create_peer();

	chunk_unlock(locks, 3, 16);
	chunk_lock(locks, 3, 32, p);
	chunk_unlock(locks, 3, 32);

	destroy_peer(&p);
	chunk_locks_destroy(&locks);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void chunk_islocked_test()
{
	struct chunk_locks * locks = NULL;
	struct peer * p;

	locks = chunk_locks_create(2);
	p = create_peer();

	assert(chunk_islocked(NULL, 1, 123) == 0);
	assert(chunk_islocked(locks, 1, 123) == 0);

	chunk_lock(locks, 1, 32, p);
	assert(chunk_islocked(locks, 1, 32));
	chunk_unlock(locks, 1, 32);
	assert(chunk_islocked(locks, 1, 32) == 0);

	destroy_peer(&p);
	chunk_locks_destroy(&locks);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void chunk_timed_out_test()
{
	struct chunk_locks * locks = NULL;
	struct peer * p;

	locks = chunk_locks_create(0);
	p = create_peer();

	assert(chunk_islocked(NULL, 1, 123) == 0);
	assert(chunk_islocked(locks, 1, 123) == 0);

	chunk_lock(locks, 1, 32, p);
	sleep(1);
	assert(chunk_islocked(locks, 1, 32) == 0);
	chunk_unlock(locks, 1, 32);
	assert(chunk_islocked(locks, 1, 32) == 0);

	destroy_peer(&p);
	chunk_locks_destroy(&locks);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	chunk_locks_create_test();
	chunk_lock_test();
	chunk_unlock_test();
	chunk_islocked_test();
	chunk_timed_out_test();
	return 0;
}
