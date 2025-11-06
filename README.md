# Elevator

[Description]

## Group Members
- **Brendan Boedy**: bsb23a@fsu.edu
- **Carson Cary**: js19@fsu.edu
- **Alexander Schneier**: ab19@fsu.edu
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: [Description]
- **Assigned to**: Alexander Schneier

### Part 2: Timer Kernel Module
- **Responsibilities**: [Description]
- **Assigned to**: Brendan Boedy

### Part 3a: Adding System Calls
- **Responsibilities**: [Description]
- **Assigned to**: Carson Cary

### Part 3b: Kernel Compilation
- **Responsibilities**: [Description]
- **Assigned to**: Alexander Schneier, Brendan Boedy

### Part 3c: Threads
- **Responsibilities**: [Description]
- **Assigned to**: Brendan Boedy, Carson Cary 

### Part 3d: Linked List
- **Responsibilities**: [Description]
- **Assigned to**: Alexander Schneier, Brendan Boedy

### Part 3e: Mutexes
- **Responsibilities**: [Description]
- **Assigned to**: Carson Cary

### Part 3f: Scheduling Algorithm
- **Responsibilities**: [Description]
- **Assigned to**: Alexadner Schneier, Carson Cary

## File Listing
```
.
├── Makefile
├── part2
│   ├── Makefile
│   └── my_timer.c
├── part3
│   ├── Makefile
│   ├── run-part3.sh
│   ├── src
│   │   ├── core.c
│   │   ├── core.h
│   │   └── elevator_main.c
│   ├── syscalls.c
│   └── tests
│       ├── elevator-test
│       │   ├── consumer.c
│       │   ├── Makefile
│       │   ├── producer.c
│       │   ├── README.md
│       │   └── wrappers.h
│       └── system-calls-test
│           ├── Makefile
│           ├── README.md
│           ├── syscheck.c
│           ├── test-syscalls.c
│           └── test-syscalls.h
├── README.md

7 directories, 21 files


```
# How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

## Compilation: Part 1, 2, and 3

```
cd Project2-COP4610
make
```

## Execution: Part 2

```
sudo insmod my_timer.ko
cat /proc/timer
sleep 2
cat /proc/timer
```
## Execution: Part 3

You can simply run `part3/run-part3.sh` which will make the kernel mod, remove (if neccecary) and insert the kernel mod, and then run watch to view the proc file

## Development Log
Each member records their contributions here.

### [Lex]

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-11-01 | Completed part 1 during meeting | 
| 2025-11-04 | part 3 step 5  |
| 2025-11-05 | debuggining on part 3 step 4 |

### [Brendan]

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-10-22 | [Set up VM and construction of Kernel Module for part 2]  |
| 2025-10-29 | [Completed part 2]  |
| 2025-10-30 | [completed part 3 steps 1-3]  |


### [Carson]

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-11-03 | Begin implementing Part 3, Step 4  |
| 2025-11-04 | Continue implementing Part 3 Step 4  |
| 2025-11-05 | Finish implementing Part 3 step 4  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| 2025-10-17 | [Lex, Brendan, Carson]              | [Part 1 and 2]   | [Completed Part 1, requested VM for Part 2 and 3]  |



## Bugs
- **Bug 1**: MakeFile accidentally made the exe from consumer instead of producer.
- **Bug 2**: Used MDELAY instead of MSleep which was causing Hold and wait which was making the program hang.




