#include <unistd.h> 


#include <stdio.h>
#include <unistd.h>   // all syscalls used here

int main() {
    // 1. getpid()
    pid_t pid = getpid();
    printf("Process ID: %d\n", pid);

    // 2. getppid()
    pid_t ppid = getppid();
    printf("Parent Process ID: %d\n", ppid);

    // 3. write() - to stdout (fd = 1)
    const char *msg = "Hello from write()\n";
    write(1, msg, 18);

    // 4. read() - read user input from stdin (fd = 0)
    char buffer[50];
    ssize_t bytes = read(0, buffer, sizeof(buffer)-1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("You typed: %s", buffer);
    }

    // 5. close() - here we’ll close stderr (fd = 2) just for demo
    close(2);  
    // After this, errors won’t print because stderr is closed

    return 0;
}
