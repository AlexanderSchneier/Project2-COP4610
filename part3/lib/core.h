#ifndef ELEVATOR_CORE_H
#define ELEVATOR_CORE_H

#include <linux/list.h>
#include <linux/mutex.h>

#define OFFLINE 0
#define IDLE 1
#define LOADING 2
#define UP 3
#define DOWN 4

#define T_CHIHUAHUA 0
#define T_PUG 1
#define T_PUGHUAHUA 2
#define T_DACHSHUND 3

#define NUM_FLOORS 5
#define MAX_PETS 5
#define MAX_WEIGHT 50
#define PROC_BUFFER_SIZE 512
#define Q_STR_BUF_SIZE 100
#define PROC_FNAME "elevator"

// pet_struct
struct pet_s {
    int start_floor;
    int dest_floor;
    int type;
    int weight;
};

// we'll use list_entry to actually access the data (it uses byte offset to get
// the ptr to struct... kinda cool)
struct waiting_pet {
    struct pet_s* pet;
    struct list_head node;
};

struct elevator {
    // these are these pets CURRENTLY on the elevator
    struct waiting_pet transport_queue;

    int current_floor;
    int num_pets_waiting;
    int num_pets_serviced;
    int num_pets_onboard;
    int current_weight;
    int state;  // OFFLINE | IDLE | etc...
};

extern struct waiting_pet floor_queues[NUM_FLOORS];
extern struct mutex elev_lock;  // elevator_lock
extern struct elevator* elevator;
extern struct task_struct* worker_thread;

int elev_worker(void* data);
void board_waiting_pets(struct list_head* floor_q);
struct waiting_pet* pop_queue(struct list_head* floor_q);
void unload_pets(void);
void write_elevator_state(void);

char* queue_to_str(struct list_head* queue);
int get_queue_size(struct list_head* queue);
char* generate_proc_string(void);
ssize_t proc_read(struct file* file, char __user* buffer, size_t count, loff_t* ppos);

#endif