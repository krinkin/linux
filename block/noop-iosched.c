/*
 * elevator noop
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>

#define ADJUSTED_BLK_QUEUE_DELAY 250
#define SCHED_MAXIMUM_QUEUES 256
#define SCHED_MAXIMUM_NAME 10

static void schedule_timer(void);

struct noop_data {
	struct list_head queue;
};


struct QUEUE_INFO {
	struct request_queue *rq;
	char name[SCHED_MAXIMUM_NAME];
};

static struct {

	int timeout;
	int ratio;

	int    queues;
        struct QUEUE_INFO queue_set[SCHED_MAXIMUM_QUEUES];
	
	int    clocks;
	
} sched_attrs;

struct kobject *ksched;

static struct timer_list sched_timer;

static ssize_t status_show(struct kobject *ko, struct kobj_attribute *at, char *buff) {
	// I hope that I fit to PAGE_SIZE

	int i=0;
	char line[100];
	sprintf(buff,"OK, CLK= %d, queues=%d\n",sched_attrs.clocks,sched_attrs.queues);

	for(i=0; i<sched_attrs.queues; i++) {
		sprintf(line,"%s: %p\n",sched_attrs.queue_set[i].name,sched_attrs.queue_set[i].rq);
		strcat(buff,line);
	}
	
	return strlen(buff);
}


static ssize_t timeout_show(struct kobject *ko, struct kobj_attribute *at, char *buff) {
	sprintf(buff,"%d\n",sched_attrs.timeout);
	return strlen(buff);
}


static ssize_t ratio_show(struct kobject *ko, struct kobj_attribute *at, char *buff) {
	sprintf(buff,"%d\n",sched_attrs.ratio);
	return strlen(buff);
}

static ssize_t ratio_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buff, size_t count)
{
	int ret;

	ret = kstrtoint(buff, 10, &sched_attrs.ratio);

	return (ret<0) ? ret : count;
}

static ssize_t timeout_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buff, size_t count)
{
	int ret;

	ret = kstrtoint(buff, 10, &sched_attrs.timeout);

	return (ret<0) ? ret : count;
}

static struct kobj_attribute status_attribute =__ATTR(status,  0444, status_show,  NULL);
static struct kobj_attribute timeout_attribute =__ATTR(timeout, 0774, timeout_show, timeout_store);
static struct kobj_attribute ratio_attribute =__ATTR(ratio,   0774, ratio_show,  ratio_store);

static struct attribute *attrs[] = {
	&status_attribute.attr,
	&timeout_attribute.attr,
	&ratio_attribute.attr,
	NULL,
};


static struct attribute_group attr_group = {
	.attrs = attrs,
};


//static struct request_queue *priority_queue =NULL;

static void noop_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int noop_dispatch(struct request_queue *q, int force)
{
	struct noop_data *nd = q->elevator->elevator_data;


	if (!list_empty(&nd->queue)) {
		struct request *rq;
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
			
		return 1;
	}
	return 0;
}



static void noop_add_request(struct request_queue *q, struct request *rq)
{
	struct noop_data *nd = q->elevator->elevator_data;
	list_add_tail(&rq->queuelist, &nd->queue);

}

static struct request *
noop_former_request(struct request_queue *q, struct request *rq)
{
	struct noop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
noop_latter_request(struct request_queue *q, struct request *rq)
{
	struct noop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int noop_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct noop_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;

	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void noop_exit_queue(struct elevator_queue *e)
{
	struct noop_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_noop = {
	.ops = {
		.elevator_merge_req_fn		= noop_merged_requests,
		.elevator_dispatch_fn		= noop_dispatch,
		.elevator_add_req_fn		= noop_add_request,
		.elevator_former_req_fn		= noop_former_request,
		.elevator_latter_req_fn		= noop_latter_request,
		.elevator_init_fn		= noop_init_queue,
		.elevator_exit_fn		= noop_exit_queue,
	},
	.elevator_name = "noop",
	.elevator_owner = THIS_MODULE,
};

static int init_sys_objs(void) {

	ksched = kobject_create_and_add("group-iosched", block_depr);
	if (!ksched)
		return -ENOMEM;
	return sysfs_create_group(ksched, &attr_group);
}

int blk_register_queue_group_schedule(struct request_queue* q, const char *name)
{
	sched_attrs.queue_set[sched_attrs.queues].rq = q;
	strncpy(sched_attrs.queue_set[sched_attrs.queues].name,name,SCHED_MAXIMUM_NAME-1);
	sched_attrs.queues++;
}

static void schedule_io(unsigned long data) {
	sched_attrs.clocks ++;
	schedule_timer();
}

static void schedule_timer(void) {

	sched_timer.expires = jiffies + sched_attrs.timeout;
	sched_timer.data = 0;
	sched_timer.function = schedule_io;
	add_timer(&sched_timer);
}


static int __init noop_init(void)
{
	init_sys_objs();
	init_timer(&sched_timer);

	sched_attrs.timeout = 5000;
	sched_attrs.ratio   = 10;

	schedule_timer();

	return elv_register(&elevator_noop);
}

static void __exit noop_exit(void)
{
	elv_unregister(&elevator_noop);
}

module_init(noop_init);
module_exit(noop_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
