/*
 * elevator sstf
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct sstf_data {
	struct list_head queue;
};

static void sstf_merged_requests(struct request_queue *q, 
                struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
}

static int sstf_dispatch(struct request_queue *q, int force)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (!list_empty(&sd->queue)) {
		struct request *rq;
		rq = list_entry(sd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
                printk("[SSTF-RQ] disp request %llu\n", rq_end_sector(rq));

		return 1;
	}
	return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;
        struct sstf_data *tmp = NULL;
        bool d;  //Can either be up(True) or down(False)
        
        sector_t rq_location = rq_end_sector(rq);
/*        sector_t head_location = rq_end_sector(list_entry(sd->queue,
                                struct request, queuelist));
        sector_t next_location = rq_end_sector(list_next_entry(sd->queue,
                                queuelist));
        d = (head_location < next_location);


        if (rq_location > head_location && d) {
                //Going up sort ascending ->
                list_for_each (tmp, &sd->queue) {
                        if (rq_end_sector(list_entry(tmp, struct request,
                                        queuelist)) > rq_location) {
                                break;
                        }
                }
                goto insert_before_tmp;
        } else if (rq_location > head_location && !d) {
                //Going down sort ascending <-
                list_for_each_prev (tmp, &sd->queue) {
                        if (rq_end_sector(list_entry(tmp, struct request,
                                        queuelist)) < rq_location) {
                                break;
                        }
                }
                goto insert_after_tmp;
        } else if (rq_location < head_location && d) {
                //Going up sort descending <-
                list_for_each_prev (tmp, &sd->queue) {
                        if (rq_end_sector(list_entry(tmp, struct request,
                                        queuelist)) > rq_location) {
                                break;
                        }
                }
                goto insert_after_tmp;
        } else if (rq_location < head_location && !d) {
                //Going down sort descending ->
                list_for_each_prev (tmp, &sd->queue) {
                        if (rq_end_sector(list_entry(tmp, struct request,
                                        queuelist)) < rq_location) {
                                break;
                        }
                }
                goto insert_before_tmp;
        }

insert_before_tmp:
        list_add_tail(&rq->queuelist, tmp);
        return;

insert_after_tmp:
        list_add(&rq->queuelist, tmp);
        return;*/
        
        list_for_each_entry(tmp, &sd->queue, queue) {
                if(rq_end_sector(list_entry(tmp, struct request, queuelist))> rq_location){
                        break;
                }
        }
        list_add_tail(&rq->queuelist, tmp);
        
}

static struct request *
sstf_former_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &sd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
sstf_latter_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.next == &sd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct sstf_data *sd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
	if (!sd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = sd;

	INIT_LIST_HEAD(&sd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *sd = e->elevator_data;

	BUG_ON(!list_empty(&sd->queue));
	kfree(sd);
}

static struct elevator_type elevator_sstf = {
	.ops = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_former_req_fn		= sstf_former_request,
		.elevator_latter_req_fn		= sstf_latter_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
	return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Jens Axboe, edits by Nathan Shepherd");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF IO scheduler, using C-LOOK");
