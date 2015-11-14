/*
 * elevator bird
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
//#include <linux/genhd.h>


struct bird_data {
	struct list_head queue;
	int local_io;
	int instance_id;
	char devicename[DISK_NAME_LEN];
};

static int total_io = 0;
static int instances = 0;
static struct list_head common_queue;

static void bird_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int bird_dispatch(struct request_queue *q, int force)
{
	struct bird_data *nd = q->elevator->elevator_data;

	if (!list_empty(&common_queue)) {
		struct request *rq;
		rq = list_entry(common_queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		nd->local_io += 1;
		total_io += 1;
		if (nd->local_io % 50 == 0){
			if (nd->local_io <= 5000){
				printk(KERN_INFO "Local io [%d] [%s] %d Total io %d\n", nd->instance_id,  nd->devicename, nd->local_io, total_io);
			}			
		}
		return 1;
	}
	return 0;
}

static void bird_add_request(struct request_queue *q, struct request *rq)
{
	struct bird_data *nd = q->elevator->elevator_data;
	const char* diskname = (rq->rq_disk)->disk_name;

	list_add_tail(&rq->queuelist, &common_queue);
}

static struct request *
bird_former_request(struct request_queue *q, struct request *rq)
{
	struct bird_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &common_queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
bird_latter_request(struct request_queue *q, struct request *rq)
{
	struct bird_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &common_queue)
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
	
	nd->local_io = 0;
	nd->instance_id = instances;
	instances++;
//((q->boundary_rq)->rq_disk)->disk_name
	strcpy(nd->devicename, "devname");
	
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&common_queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void bird_exit_queue(struct elevator_queue *e)
{
	struct bird_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&common_queue));
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
