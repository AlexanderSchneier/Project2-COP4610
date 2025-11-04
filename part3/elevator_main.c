// #include <linux/init.h>
// #include <linux/kernel.h>
// #include <linux/kthread.h>
// #include <linux/list.h>
// #include <linux/module.h>
// #include <linux/mutex.h>
// #include <linux/slab.h>  //  kmalloc/kfree
// #include <linux/string.h>

// #include "./lib/core.h"
// #include "linux/proc_fs.h"

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("cop4610t");
// MODULE_DESCRIPTION("Elevator kernel module");
// MODULE_VERSION("0.1");

// extern int (*STUB_start_elevator)(void);
// extern int (*STUB_issue_request)(int, int, int);
// extern int (*STUB_stop_elevator)(void);

// static int start_elevator(void) {
//     printk(KERN_INFO "Starting elevator thread");
//     if (elevator->state != OFFLINE) {
//         printk(KERN_WARNING "Elevator crazy wonkus bro its already on!");
//         return 1;
//     }

//     elevator->state = IDLE;
//     worker_thread = kthread_run(elev_worker, elevator, "elevator_worker");
//     return 0;
// }

// static int issue_request(int start, int dest, int type) {
//     printk(KERN_INFO "Elevator ISSUING request");
//     struct waiting_pet* patient_pet = kmalloc(sizeof(struct waiting_pet), GFP_KERNEL);
//     struct pet_s* pet = kmalloc(sizeof(struct pet_s), GFP_KERNEL);

//     pet->start_floor = start;
//     pet->dest_floor = dest;
//     pet->type = type;

//     switch (pet->type) {
//         case (T_CHIHUAHUA):
//             pet->weight = 3;
//             break;
//         case (T_PUG):
//             pet->weight = 14;
//             break;
//         case (T_PUGHUAHUA):
//             pet->weight = 10;
//             break;
//         case (T_DACHSHUND):
//             pet->weight = 16;
//             break;
//     }

//     patient_pet->pet = pet;

//     mutex_lock(&elev_lock);
//     elevator->num_pets_waiting += 1;
//     list_add_tail(&patient_pet->node, &floor_queues[start - 1].node);
//     mutex_unlock(&elev_lock);

//     printk(KERN_INFO "Added pet to floor %d queue", start);

//     return 0;
// }

// static int elevator_stop(void) {
//     if (worker_thread) {
//         kthread_stop(worker_thread);
//         worker_thread = NULL;
//         printk(KERN_INFO "Stopping elevator thread");
//         return 0;
//     }
//     printk(KERN_WARNING "Elevator already stopped or never started.\n");
//     return 1;
// }

// static const struct proc_ops
//     some_procs_ops_bro_it_just_keeps_going_why_does_it_keep_going = {
//         .proc_read = proc_read,
// };

// static int __init initialize_elevator(void) {
//     // This initializes the stubs so that when the kernel calls
//     // STUB_start_elevator these functions will be called
//     STUB_start_elevator = &start_elevator;
//     STUB_issue_request = &issue_request;
//     STUB_stop_elevator = &elevator_stop;
//     mutex_init(&elev_lock);

//     printk(KERN_INFO "Loaded elevator module FROM OVER HERE.");

//     // Not sure if its neccecary here cause we haven't started the elevator thread
//     // yet infact, maybe we should do this initialization in the elevator thread,
//     // idk.
//     mutex_lock(&elev_lock);

//     printk(KERN_INFO "In lock");
//     for (int i = 0; i < NUM_FLOORS; ++i) {
//         INIT_LIST_HEAD(&floor_queues[i].node);
//         floor_queues[i].pet = NULL;
//     }

//     mutex_unlock(&elev_lock);

//     mutex_lock(&elev_lock);
//     elevator = kmalloc(sizeof(struct elevator), GFP_KERNEL);
//     elevator->state = OFFLINE;
//     elevator->current_floor = 1;
//     elevator->num_pets_waiting = 0;
//     elevator->num_pets_serviced = 0;
//     elevator->num_pets_onboard = 0;
//     elevator->current_weight = 0;

//     INIT_LIST_HEAD(&elevator->transport_queue.node);
//     elevator->transport_queue.pet = NULL;
//     mutex_unlock(&elev_lock);

//     printk(KERN_INFO "Initialized elevator and stuff");

//     struct proc_dir_entry* entry;
//     entry = proc_create(PROC_FNAME, 0444, NULL,
//                         &some_procs_ops_bro_it_just_keeps_going_why_does_it_keep_going);

//     if (entry == NULL) {
//         pr_alert("Failed to create /proc/%s\n", PROC_FNAME);
//         return -ENOMEM;
//     }

//     pr_info("/proc/%s created\n", PROC_FNAME);

//     return 0;
// }

// static void __exit rm_elevator(void) {
//     STUB_start_elevator = NULL;
//     STUB_issue_request = NULL;
//     STUB_stop_elevator = NULL;

//     printk(KERN_INFO "Unloaded elevator module.");
//     elevator_stop();

//     remove_proc_entry(PROC_FNAME, NULL);
//     pr_info("/proc/%s removed\n", PROC_FNAME);
// }

// module_init(initialize_elevator);
// module_exit(rm_elevator);

#include "./core.h"

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/sprintf.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "linux/gfp_types.h"
#include "linux/list.h"

struct waiting_pet floor_queues[NUM_FLOORS];
struct mutex elev_lock;
struct elevator* elevator;
struct task_struct* worker_thread;

struct waiting_pet* pop_queue(struct list_head* floor_q) {
    struct waiting_pet* value = NULL;

    if (!list_empty(floor_q)) {
        value = list_first_entry(floor_q, struct waiting_pet, node);
    }

    return value;
}

void board_waiting_pets(struct list_head* floor_q) {
    // Mutex lock MUST be acquired before calling

    if (elevator->num_pets_onboard >= MAX_PETS) {
        return;
    }

    struct waiting_pet* patient_pet = pop_queue(floor_q);

    while (patient_pet != NULL) {
        // Check if pet pointer is valid
        if (patient_pet->pet == NULL) {
            // Skip this entry but don't delete it yet
            break;
        }

        // Check if adding this pet would exceed weight limit
        if (elevator->current_weight + patient_pet->pet->weight > MAX_WEIGHT) {
            // Can't board this pet, stop trying
            break;
        }

        // Board this pet
        list_del(&patient_pet->node);
        list_add_tail(&patient_pet->node, &elevator->transport_queue.node);

        elevator->num_pets_onboard += 1;
        elevator->num_pets_waiting -= 1;
        elevator->current_weight += patient_pet->pet->weight;
        
        if (elevator->num_pets_onboard >= MAX_PETS) {
            break;
        }
        
        // Get next pet in queue
        patient_pet = pop_queue(floor_q);
    }

    mdelay(1000);  // Loading delay
}

void unload_pets(void) {
    struct waiting_pet* curr;
    struct waiting_pet* next;
    int unloaded = 0;

    list_for_each_entry_safe(curr, next, &elevator->transport_queue.node, node) {
        if (curr->pet == NULL) {
            continue;
        }

        struct pet_s* curr_pet = curr->pet;

        if (elevator->current_floor == curr_pet->dest_floor) {
            list_del(&curr->node);

            elevator->num_pets_onboard -= 1;
            elevator->current_weight -= curr_pet->weight;

            kfree(curr_pet);
            kfree(curr);

            elevator->num_pets_serviced += 1;
            unloaded = 1;
        }
    }

    if (unloaded) {
        mdelay(1000);
    }
}

int elev_worker(void* data) {
    printk(KERN_INFO "Elevator worker thread started");

    while (!kthread_should_stop()) {
        mutex_lock(&elev_lock);

        // Check if elevator should stop
        if (elevator->state == OFFLINE) {
            mutex_unlock(&elev_lock);
            break;
        }

        // Check if we should be idle
        if (list_empty(&elevator->transport_queue.node)) {
            int all_empty = 1;

            for (int i = 0; i < NUM_FLOORS; ++i) {
                if (!list_empty(&floor_queues[i].node)) {
                    all_empty = 0;
                    break;
                }
            }

            if (all_empty) {
                elevator->state = IDLE;
                mutex_unlock(&elev_lock);
                msleep(100);  // Sleep briefly when idle
                continue;
            }
        }

        // Unload pets at current floor
        elevator->state = LOADING;
        unload_pets();

        // Board waiting pets at current floor
        int floor_idx = elevator->current_floor - 1;
        struct list_head* floor_q = &floor_queues[floor_idx].node;
        board_waiting_pets(floor_q);

        // Determine next direction
        struct waiting_pet* transport_pet = NULL;
        if (!list_empty(&elevator->transport_queue.node)) {
            transport_pet = list_first_entry(&elevator->transport_queue.node,
                                             struct waiting_pet, node);
        }

        if (transport_pet != NULL && transport_pet->pet != NULL) {
            int current_dest_floor = transport_pet->pet->dest_floor;

            if (current_dest_floor < elevator->current_floor) {
                elevator->state = DOWN;
            } else if (current_dest_floor > elevator->current_floor) {
                elevator->state = UP;
            } else {
                elevator->state = LOADING;
            }
        } else {
            // No pets on board, find nearest waiting pet
            int nearest_floor = -1;
            int min_distance = NUM_FLOORS + 1;

            for (int i = 0; i < NUM_FLOORS; ++i) {
                if (!list_empty(&floor_queues[i].node)) {
                    int distance = abs((i + 1) - elevator->current_floor);
                    
                    if (distance < min_distance) {
                        min_distance = distance;
                        nearest_floor = i + 1;
                    }
                }
            }

            if (nearest_floor != -1) {
                if (nearest_floor < elevator->current_floor) {
                    elevator->state = DOWN;
                } else if (nearest_floor > elevator->current_floor) {
                    elevator->state = UP;
                } else {
                    elevator->state = LOADING;
                }
            } else {
                elevator->state = IDLE;
            }
        }

        // Move the elevator
        if (elevator->state == DOWN && elevator->current_floor > 1) {
            elevator->current_floor -= 1;
            mutex_unlock(&elev_lock);
            mdelay(2000);
        } else if (elevator->state == UP && elevator->current_floor < NUM_FLOORS) {
            elevator->current_floor += 1;
            mutex_unlock(&elev_lock);
            mdelay(2000);
        } else {
            mutex_unlock(&elev_lock);
            msleep(100);
        }
    }

    printk(KERN_INFO "Elevator worker thread stopping");
    return 0;
}

/*
 * Write to proc file stuff
*/

char* queue_to_str(struct list_head* queue) {
    int buffer_idx = 0;
    char str_buffer[Q_STR_BUF_SIZE];
    memset(str_buffer, 0, Q_STR_BUF_SIZE);

    char type_to_str[4] = {'C', 'P', 'H', 'D'};

    struct waiting_pet* patient_pet;
    list_for_each_entry(patient_pet, queue, node) {
        if (patient_pet->pet != NULL) {
            buffer_idx +=
                scnprintf(str_buffer + buffer_idx, Q_STR_BUF_SIZE - buffer_idx, "%c%d ",
                          type_to_str[patient_pet->pet->type], patient_pet->pet->dest_floor);
        }
    }

    char* dyn_str = kstrdup(str_buffer, GFP_KERNEL);
    if (!dyn_str) {
        pr_err("Failed to allocate memory for queue string.\n");
        return kstrdup("", GFP_KERNEL);
    }

    return dyn_str;
}

int get_queue_size(struct list_head* queue) {
    int count = 0;
    struct waiting_pet* req;

    list_for_each_entry(req, queue, node) { 
        if (req->pet != NULL) {
            count++;
        }
    }

    return count;
}

char* generate_proc_string(void) {
    int buffer_idx = 0;
    char buffer[PROC_BUFFER_SIZE];
    memset(buffer, 0, PROC_BUFFER_SIZE);

    const int max_sz = PROC_BUFFER_SIZE;

    char* state_to_str[5] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};

    mutex_lock(&elev_lock);

    buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                            "Elevator state: %s\n"
                            "Current floor: %d\n"
                            "Current load: %d lbs\n\n",
                            state_to_str[elevator->state], 
                            elevator->current_floor,
                            elevator->current_weight);

    for (int i = NUM_FLOORS - 1; i >= 0; --i) {
        char* floor_queue_str = queue_to_str(&floor_queues[i].node);
        buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                                "%s Floor %d: %d %s\n",
                                elevator->current_floor == (i + 1) ? "[*]" : "[ ]",
                                i + 1, 
                                get_queue_size(&floor_queues[i].node), 
                                floor_queue_str);
        kfree(floor_queue_str);
    }

    buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                            "\nNumber of pets: %d\n"
                            "Number of pets waiting: %d\n"
                            "Number of pets serviced: %d\n",
                            elevator->num_pets_onboard, 
                            elevator->num_pets_waiting,
                            elevator->num_pets_serviced);

    mutex_unlock(&elev_lock);

    char* dyn_str = kstrdup(buffer, GFP_KERNEL);
    if (!dyn_str) {
        pr_err("Failed to allocate memory for proc string.\n");
        return kstrdup("Error\n", GFP_KERNEL);
    }

    return dyn_str;
}

ssize_t proc_read(struct file* file, char __user* buffer, size_t count, loff_t* ppos) {
    if (*ppos > 0) {
        return 0;
    }

    char* proc_string = generate_proc_string();
    if (!proc_string) {
        return -ENOMEM;
    }

    int len = strlen(proc_string);
    if (count < len) {
        len = count;
    }

    if (copy_to_user(buffer, proc_string, len)) {
        kfree(proc_string);
        return -EFAULT;
    }

    kfree(proc_string);
    *ppos += len;

    return len;
}