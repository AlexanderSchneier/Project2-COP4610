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

struct list_head floor_queues[NUM_FLOORS];
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
        
        printk(KERN_DEBUG "Worker loop: state=%d, floor=%d, onboard=%d, waiting=%d\n",
               elevator->state, elevator->current_floor, 
               elevator->num_pets_onboard, elevator->num_pets_waiting);

        // Check if elevator should stop
        if (elevator->state == OFFLINE) {
            printk(KERN_INFO "Worker detected OFFLINE state, exiting\n");
            mutex_unlock(&elev_lock);
            break;
        }

        // Check if we should be idle
        if (list_empty(&elevator->transport_queue.node)) {
            int all_empty = 1;

            for (int i = 0; i < NUM_FLOORS; ++i) {
                if (!list_empty(&floor_queues[i])) {
                    all_empty = 0;
                    printk(KERN_DEBUG "Floor %d has waiting pets\n", i+1);
                    break;
                }
            }

            if (all_empty) {
                printk(KERN_DEBUG "All queues empty, staying IDLE\n");
                elevator->state = IDLE;
                mutex_unlock(&elev_lock);
                msleep(100);  // Sleep briefly when idle
                continue;
            } else {
                printk(KERN_DEBUG "Found waiting pets, proceeding with service\n");
            }
        }

        // Unload pets at current floor
        printk(KERN_DEBUG "Setting state to LOADING\n");
        elevator->state = LOADING;
        unload_pets();

        // Board waiting pets at current floor
        int floor_idx = elevator->current_floor - 1;
        struct list_head* floor_q = &floor_queues[floor_idx];
        board_waiting_pets(floor_q);

        // Determine next direction
        struct waiting_pet* transport_pet = NULL;
        if (!list_empty(&elevator->transport_queue.node)) {
            transport_pet = list_first_entry(&elevator->transport_queue.node,
                                             struct waiting_pet, node);
        }

        if (transport_pet != NULL && transport_pet->pet != NULL) {
            int current_dest_floor = transport_pet->pet->dest_floor;
            
            printk(KERN_DEBUG "Pet on board going to floor %d\n", current_dest_floor);

            if (current_dest_floor < elevator->current_floor) {
                elevator->state = DOWN;
                printk(KERN_DEBUG "Moving DOWN\n");
            } else if (current_dest_floor > elevator->current_floor) {
                elevator->state = UP;
                printk(KERN_DEBUG "Moving UP\n");
            } else {
                elevator->state = LOADING;
                printk(KERN_DEBUG "Already at destination, LOADING\n");
            }
        } else {
            // No pets on board, find nearest waiting pet
            int nearest_floor = -1;
            int min_distance = NUM_FLOORS + 1;

            for (int i = 0; i < NUM_FLOORS; ++i) {
                if (!list_empty(&floor_queues[i])) {
                    int distance = abs((i + 1) - elevator->current_floor);
                    
                    if (distance < min_distance) {
                        min_distance = distance;
                        nearest_floor = i + 1;
                    }
                }
            }

            if (nearest_floor != -1) {
                printk(KERN_DEBUG "No pets on board, nearest waiting floor is %d\n", nearest_floor);
                if (nearest_floor < elevator->current_floor) {
                    elevator->state = DOWN;
                    printk(KERN_DEBUG "Moving DOWN to pick up\n");
                } else if (nearest_floor > elevator->current_floor) {
                    elevator->state = UP;
                    printk(KERN_DEBUG "Moving UP to pick up\n");
                } else {
                    elevator->state = LOADING;
                    printk(KERN_DEBUG "At pickup floor, LOADING\n");
                }
            } else {
                printk(KERN_DEBUG "No waiting pets found, going IDLE\n");
                elevator->state = IDLE;
            }
        }

        // Move the elevator
        if (elevator->state == DOWN && elevator->current_floor > 1) {
            printk(KERN_INFO "Elevator moving from floor %d to %d\n", 
                   elevator->current_floor, elevator->current_floor - 1);
            elevator->current_floor -= 1;
            mutex_unlock(&elev_lock);
            msleep(2000);
        } else if (elevator->state == UP && elevator->current_floor < NUM_FLOORS) {
            printk(KERN_INFO "Elevator moving from floor %d to %d\n", 
                   elevator->current_floor, elevator->current_floor + 1);
            elevator->current_floor += 1;
            mutex_unlock(&elev_lock);
            msleep(2000);
        } else {
            mutex_unlock(&elev_lock);
            msleep(100);
        }
        
        if(kthread_should_stop()){
            printk(KERN_INFO "kthread_should_stop returned true\n");
            break;
        }
        msleep(50);
    }

    printk(KERN_INFO "Elevator worker thread stopping");
    return 0;
}
// int elev_worker(void* data) {
//     printk(KERN_INFO "Elevator worker thread started");

//     while (!kthread_should_stop()) {
//         mutex_lock(&elev_lock);

//         // Check if elevator should stop
//         if (elevator->state == OFFLINE) {
//             mutex_unlock(&elev_lock);
//             break;
//         }

//         // Check if we should be idle
//         if (list_empty(&elevator->transport_queue.node)) {
//             int all_empty = 1;

//             for (int i = 0; i < NUM_FLOORS; ++i) {
//                 if (!list_empty(&floor_queues[i])) {
//                     all_empty = 0;
//                     break;
//                 }
//             }

//             if (all_empty) {
//                 elevator->state = IDLE;
//                 mutex_unlock(&elev_lock);
//                 msleep(100);  // Sleep briefly when idle
//                 continue;
//             }
//         }

//         // Unload pets at current floor
//         elevator->state = LOADING;
//         unload_pets();

//         // Board waiting pets at current floor
//         int floor_idx = elevator->current_floor - 1;
//         struct list_head* floor_q = &floor_queues[floor_idx];
//         board_waiting_pets(floor_q);

//         // Determine next direction
//         struct waiting_pet* transport_pet = NULL;
//         if (!list_empty(&elevator->transport_queue.node)) {
//             transport_pet = list_first_entry(&elevator->transport_queue.node,
//                                              struct waiting_pet, node);
//         }

//         if (transport_pet != NULL && transport_pet->pet != NULL) {
//             int current_dest_floor = transport_pet->pet->dest_floor;

//             if (current_dest_floor < elevator->current_floor) {
//                 elevator->state = DOWN;
//             } else if (current_dest_floor > elevator->current_floor) {
//                 elevator->state = UP;
//             } else {
//                 elevator->state = LOADING;
//             }
//         } else {
//             // No pets on board, find nearest waiting pet
//             int nearest_floor = -1;
//             int min_distance = NUM_FLOORS + 1;

//             for (int i = 0; i < NUM_FLOORS; ++i) {
//                 if (!list_empty(&floor_queues[i])) {
//                     int distance = abs((i + 1) - elevator->current_floor);
                    
//                     if (distance < min_distance) {
//                         min_distance = distance;
//                         nearest_floor = i + 1;
//                     }
//                 }
//             }

//             if (nearest_floor != -1) {
//                 if (nearest_floor < elevator->current_floor) {
//                     elevator->state = DOWN;
//                 } else if (nearest_floor > elevator->current_floor) {
//                     elevator->state = UP;
//                 } else {
//                     elevator->state = LOADING;
//                 }
//             } else {
//                 elevator->state = IDLE;
//             }
//         }

//         // Move the elevator
//         if (elevator->state == DOWN && elevator->current_floor > 1) {
//             elevator->current_floor -= 1;
//             mutex_unlock(&elev_lock);
//             msleep(2000);
//         } else if (elevator->state == UP && elevator->current_floor < NUM_FLOORS) {
//             elevator->current_floor += 1;
//             mutex_unlock(&elev_lock);
//             msleep(2000);
//         } else {
//             mutex_unlock(&elev_lock);
//             msleep(100);
//         }
//         if(kthread_should_stop()){
//             break;
//         }
//         msleep(50);
//     }

//     printk(KERN_INFO "Elevator worker thread stopping");
//     return 0;
// }

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
                            "Current load: %d\n"
                            "Elevator status:\n\n",
                            state_to_str[elevator->state], 
                            elevator->current_floor,
                            elevator->current_weight);

    for (int i = NUM_FLOORS - 1; i >= 0; --i) {
        char* floor_queue_str = queue_to_str(&floor_queues[i]);
        buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                                "%s Floor %d: %d %s\n",
                                elevator->current_floor == (i + 1) ? "[*]" : "[ ]",
                                i + 1, 
                                get_queue_size(&floor_queues[i]), 
                                floor_queue_str);
        kfree(floor_queue_str);
    }

    buffer_idx += scnprintf(buffer + buffer_idx, max_sz - buffer_idx,
                            "\nNumber of passengers: %d\n"
                            "Number of passengers waiting: %d\n"
                            "Number of passengers serviced: %d\n",
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

