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
    // Returns either NULL if list empty or value of type elevator_request

    struct waiting_pet* value = NULL;

    if (!list_empty(floor_q)) {
        value = list_first_entry(floor_q, struct waiting_pet, node);

        // list_del(&value->node);
    }

    return value;
}

void board_waiting_pets(struct list_head* floor_q) {
    // Mutex lock MUST be acquired

    if (elevator->num_pets_onboard >= MAX_PETS) {
        return;
    }

    elevator->state = LOADING;
    // LIKELY HAVE TO PRINT SOMETHING HERE

    struct waiting_pet* patient_pet = pop_queue(floor_q);

    // Moves the pets from the waiting queue for the floor to the waiting transport queue
    while (patient_pet != NULL) {
        if (elevator->current_weight + patient_pet->pet->weight > MAX_WEIGHT) {
            continue;
        }

        list_del(&patient_pet->node);  // only delete once adding
        list_add_tail(&patient_pet->node, &elevator->transport_queue.node);

        elevator->num_pets_onboard += 1;
        elevator->num_pets_waiting -= 1;
        elevator->current_weight += patient_pet->pet->weight;
        if (elevator->num_pets_onboard >= MAX_PETS) {
            break;
        }
        patient_pet = pop_queue(floor_q);
    }

    // LIKELY HAVE TO PRINT SOMETHING HERE
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

            kfree(curr_pet);
            kfree(curr);

            elevator->num_pets_serviced += 1;
            unloaded = 1;
        }
    }

    if (unloaded) {
        mdelay(1000);  // May need to use something else for scheduling purposes???
    }
}

int elev_worker(void* data) {
    // NOTE:::: State cannot switch to OFFLINE unless state == IDLE (MUST implement this)

    // This is the pet we are CURRENTLY transporting (i.e going to their dest)
    struct waiting_pet* transport_pet = NULL;
    while (elevator->state != OFFLINE) {
        mutex_lock(&elev_lock);

        if (list_empty(&elevator->transport_queue.node)) {
            int all_empty = 1;

            for (int i = 0; i < MAX_PETS; ++i) {
                if (!list_empty(&floor_queues[i].node)) {
                    all_empty = 0;
                    break;
                }
            }

            if (all_empty) {
                elevator->state = IDLE;
                mutex_unlock(&elev_lock);
                continue;
            }
        }

        if (!list_empty(&elevator->transport_queue.node)) {
            transport_pet = list_first_entry(&elevator->transport_queue.node,
                                             struct waiting_pet, node);
        }

        unload_pets();

        // PRINT/WRITE PROC STATE HERE
        // write_elevator_state();

        int floor_idx = elevator->current_floor - 1;
        struct list_head* floor_q = &floor_queues[floor_idx].node;
        board_waiting_pets(floor_q);  // gets called no matter what

        int current_dest_floor = transport_pet->pet->dest_floor;

        if (current_dest_floor == elevator->current_floor) {
            elevator->state = LOADING;
        } else if (current_dest_floor < elevator->current_floor) {
            elevator->state = UP;
        } else {
            elevator->state = DOWN;
        }

        if (kthread_should_stop()) {
            elevator->state = (elevator->state == IDLE) ? OFFLINE : elevator->state;
        }

        if (elevator->state == DOWN) {
            elevator->current_floor -= 1;
            mdelay(2000);
        } else if (elevator->state == UP) {
            elevator->current_floor += 1;
            mdelay(2000);
        }

        mutex_unlock(&elev_lock);
    }

    return 0;
}

/*
 * Write to proc file stuff
*/

char* queue_to_str(struct list_head* queue) {
    int buffer_idx = 0;
    char str_buffer[Q_STR_BUF_SIZE];

    char type_to_str[4] = {'C', 'P', 'H', 'D'};

    struct waiting_pet* patient_pet;
    list_for_each_entry(patient_pet, queue, node) {
        buffer_idx +=
            scnprintf(str_buffer + buffer_idx, Q_STR_BUF_SIZE - buffer_idx, "%c%d ",
                      type_to_str[patient_pet->pet->type], patient_pet->pet->dest_floor);
    }

    char* dyn_str = kstrdup(str_buffer, GFP_KERNEL);
    if (!dyn_str) {
        pr_err("Failed to allocate memory for queue string.\n");
    }

    return dyn_str;
}

int get_queue_size(struct list_head* queue) {
    int count = 0;
    struct waiting_pet* req;

    list_for_each_entry(req, queue, node) { count++; }

    return count;
}

char* generate_proc_string(void) {
    int buffer_idx = 0;
    char buffer[PROC_BUFFER_SIZE];

    const int max_sz = PROC_BUFFER_SIZE;  // just easeir to type yo

    // im sure theres a better way to do this, but this seems pretty good
    char* state_to_str[5] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};

    buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                            "Elevator state: %s\n"
                            "Current floor: %d\n"
                            "Current load: %d lbs\n\n",
                            state_to_str[elevator->state], elevator->current_floor,
                            elevator->current_weight);

    for (int i = NUM_FLOORS - 1; i >= 0; --i) {
        char* floor_queue_str = queue_to_str(&floor_queues[i].node);
        buffer_idx += scnprintf(buffer, max_sz - buffer_idx,

                                elevator->current_floor == i ? "[*]"
                                                             : "[ ]"
                                                               " Floor %d: %d %s",
                                i, get_queue_size(&floor_queues[i].node), floor_queue_str

        );
        kfree(floor_queue_str);
    }

    buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                            "Number of pets: %d\n"
                            "Numbers of pets waiting: %d\n"
                            "Number of pets serviced: %d\n\n",
                            elevator->num_pets_onboard, elevator->num_pets_waiting,
                            elevator->num_pets_serviced);

    char* dyn_str = kstrdup(buffer, GFP_KERNEL);
    if (!dyn_str) {
        pr_err("Failed to allocate memory for queue string.\n");
    }

    return dyn_str;
}
ssize_t proc_read(struct file* file, char __user* buffer, size_t count, loff_t* ppos) {
    if (*ppos > 0) {
        return 0;
    }

    char* proc_string = generate_proc_string();

    int len = strlen(proc_string);
    if (count < len) {
        len = count;
    }

    if (copy_to_user(buffer, proc_string, len)) {
        return -EFAULT;
    }

    kfree(proc_string);
    *ppos += len;

    return len;
}
