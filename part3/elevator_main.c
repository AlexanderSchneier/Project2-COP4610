#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>  //  kmalloc/kfree
#include <linux/string.h>

#include "./lib/core.h"
#include "linux/proc_fs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Elevator kernel module");
MODULE_VERSION("0.1");

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

static int start_elevator(void) {
    printk(KERN_INFO "Starting elevator thread");
    if (elevator->state != OFFLINE) {
        printk(KERN_WARNING "Elevator crazy wonkus bro its already on!");
        return 1;
    }

    elevator->state = IDLE;
    worker_thread = kthread_run(elev_worker, elevator, "elevator_worker");
    return 0;
}

static int issue_request(int start, int dest, int type) {
    printk(KERN_INFO "ISSUE_REQUEST CALLED: start=%d, dest=%d, type=%d\n", start, dest, type);
    if (start < 1 || start > NUM_FLOORS || dest < 1 || dest > NUM_FLOORS) {
        printk(KERN_WARNING "Invalid floor: start=%d, dest=%d\n", start, dest);
        return -EINVAL;
    }
    printk(KERN_INFO "Elevator ISSUING request");
    struct waiting_pet* patient_pet = kmalloc(sizeof(struct waiting_pet), GFP_KERNEL);
    struct pet_s* pet = kmalloc(sizeof(struct pet_s), GFP_KERNEL);

    pet->start_floor = start;
    pet->dest_floor = dest;
    pet->type = type;

    switch (pet->type) {
        case (T_CHIHUAHUA):
            pet->weight = 3;
            break;
        case (T_PUG):
            pet->weight = 14;
            break;
        case (T_PUGHUAHUA):
            pet->weight = 10;
            break;
        case (T_DACHSHUND):
            pet->weight = 16;
            break;
    }

    patient_pet->pet = pet;

    mutex_lock(&elev_lock);
    elevator->num_pets_waiting += 1;
    list_add_tail(&patient_pet->node, &floor_queues[start - 1]);
    mutex_unlock(&elev_lock);

    printk(KERN_INFO "Added pet to floor %d queue", start);

    return 0;
}

static int elevator_stop(void) {
    mutex_lock(&elev_lock);
        elevator->state = OFFLINE;           // <-- make it visible to /proc and worker
    mutex_unlock(&elev_lock);
    
    if (worker_thread) {
        kthread_stop(worker_thread);
        worker_thread = NULL;
        printk(KERN_INFO "Stopping elevator thread");
        return 0;
    }
    printk(KERN_WARNING "Elevator already stopped or never started.\n");
    return 1;
}

static const struct proc_ops
    some_procs_ops_bro_it_just_keeps_going_why_does_it_keep_going = {
        .proc_read = proc_read,
};

static int __init initialize_elevator(void) {
    // This initializes the stubs so that when the kernel calls
    // STUB_start_elevator these functions will be called
    STUB_start_elevator = &start_elevator;
    STUB_issue_request = &issue_request;
    STUB_stop_elevator = &elevator_stop;
    mutex_init(&elev_lock);

    printk(KERN_INFO "Loaded elevator module FROM OVER HERE.");

    // Not sure if its neccecary here cause we haven't started the elevator thread
    // yet infact, maybe we should do this initialization in the elevator thread,
    // idk.
    mutex_lock(&elev_lock);

    printk(KERN_INFO "In lock");
    for (int i = 0; i < NUM_FLOORS; ++i) {
        INIT_LIST_HEAD(&floor_queues[i]);
    }

    mutex_unlock(&elev_lock);

    mutex_lock(&elev_lock);
    elevator = kmalloc(sizeof(struct elevator), GFP_KERNEL);
    elevator->state = OFFLINE;
    elevator->current_floor = 1;
    elevator->num_pets_waiting = 0;
    elevator->num_pets_serviced = 0;
    elevator->num_pets_onboard = 0;
    elevator->current_weight = 0;

    INIT_LIST_HEAD(&elevator->transport_queue.node);
    elevator->transport_queue.pet = NULL;
    mutex_unlock(&elev_lock);

    printk(KERN_INFO "Initialized elevator and stuff");

    struct proc_dir_entry* entry;
    entry = proc_create(PROC_FNAME, 0444, NULL,
                        &some_procs_ops_bro_it_just_keeps_going_why_does_it_keep_going);

    if (entry == NULL) {
        pr_alert("Failed to create /proc/%s\n", PROC_FNAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROC_FNAME);

    return 0;
}

static void __exit rm_elevator(void) {
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    printk(KERN_INFO "Unloaded elevator module.");
    elevator_stop();

    remove_proc_entry(PROC_FNAME, NULL);
    pr_info("/proc/%s removed\n", PROC_FNAME);
}

module_init(initialize_elevator);
module_exit(rm_elevator);
