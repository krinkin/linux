/*
 * elevator bird
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/string.h>

struct bird_data {
	struct list_head queue;
	int instance_id;
};

static int local_io[23];
static int pending_io[23];
static int priority[23];
static int lock_io[23];
static int group_id[23];


static int total_io = 0;
static int instances = 0;

static char *
bird_strncpy(char *dest, const char *src, int n)
{
    int i;

   for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++)
        dest[i] = '\0';

   return dest;
}

static void bird_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
	if (q != NULL){
		struct bird_data *nd = q->elevator->elevator_data;
		pending_io[nd->instance_id] -= 1;
	}
}

static int first_cmp;
static int second_cmp;
static int timerPrior = 1;

static int bird_dispatch(struct request_queue *q, int force)
{
	struct bird_data *nd = q->elevator->elevator_data;
	int prior_sum = 0;
	int local_sum = 0;
	int pending_io_sum = 0;
	int prior_iterator = 0;
	int gr_prior_sum = 0;
	first_cmp = 0;
	second_cmp = 0;
	timerPrior -= 1;

	if (timerPrior <= 0)
		timerPrior = instances;
	

	if (!list_empty(&nd->queue)) {
	
		struct request *rq;
		char diskname[DISK_NAME_LEN+1];

		for (prior_iterator = 0; prior_iterator < instances; ++prior_iterator){
			prior_sum += priority[prior_iterator];
		}
		
		for (prior_iterator = 0; prior_iterator < instances; ++prior_iterator){
			pending_io_sum += pending_io[prior_iterator];
		}

		for (prior_iterator = 0; prior_iterator < instances; ++prior_iterator){
			if (group_id[prior_iterator] == group_id[nd->instance_id]){
				local_sum += local_io[prior_iterator];
			}
		}
		
		for (prior_iterator = 0; prior_iterator < instances; ++prior_iterator){
			if (group_id[prior_iterator] == group_id[nd->instance_id]){
				gr_prior_sum  += priority[prior_iterator];
			}
		}

		first_cmp = local_sum;
		first_cmp *= prior_sum;
		
		second_cmp = gr_prior_sum;
		second_cmp *= pending_io_sum;
		second_cmp *= gr_prior_sum;



		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		local_io[nd->instance_id] += 1;
		total_io += 1;
		pending_io[nd->instance_id] -= 1;

		bird_strncpy(diskname, rq->rq_disk ? rq->rq_disk->disk_name : "unknown", sizeof(diskname)-1);
		diskname[sizeof(diskname)-1] = '\0';

		if (local_io[nd->instance_id] % 100 == 0){
			if (local_io[nd->instance_id] <= 50000){
				printk(KERN_INFO "Local io [%d] %d From %s Total io %d pending_io = %d Prior=%d PriorSum = %d LocalSum = %d Grpr = %d\n", nd->instance_id, local_io[nd->instance_id], diskname, total_io, pending_io[nd->instance_id], priority[nd->instance_id], prior_sum, local_sum, gr_prior_sum);

			}			
		}
		return 1;
	}
	
	return 0;
}

static void bird_add_request(struct request_queue *q, struct request *rq)
{
	struct bird_data *nd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &nd->queue);

	pending_io[nd->instance_id] += 1;
}

static struct request *
bird_former_request(struct request_queue *q, struct request *rq)
{
	struct bird_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
bird_latter_request(struct request_queue *q, struct request *rq)
{
	struct bird_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int bird_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct bird_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	
	nd->instance_id = instances;
	local_io[nd->instance_id] = 0;
	priority[nd->instance_id] = (instances+1)*2;
	group_id[nd->instance_id] = instances;
	pending_io[nd->instance_id] = 0;
	instances++;
	
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void bird_exit_queue(struct elevator_queue *e)
{
	struct bird_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_bird = {
	.ops = {
		.elevator_merge_req_fn		= bird_merged_requests,
		.elevator_dispatch_fn		= bird_dispatch,
		.elevator_add_req_fn		= bird_add_request,
		.elevator_former_req_fn		= bird_former_request,
		.elevator_latter_req_fn		= bird_latter_request,
		.elevator_init_fn		= bird_init_queue,
		.elevator_exit_fn		= bird_exit_queue,
	},
	.elevator_name = "bird",
	.elevator_owner = THIS_MODULE,
};

static int __init bird_init(void)
{
	return elv_register(&elevator_bird);
}

static void __exit bird_exit(void)
{
	elv_unregister(&elevator_bird);
}

module_init(bird_init);
module_exit(bird_exit);


MODULE_AUTHOR("Alexey Stepanov");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Bird IO scheduler");
