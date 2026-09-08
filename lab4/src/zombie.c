#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    if (pid == 0) {

        _exit(0);
    } else if (pid > 0) {

        printf("child PID: %d\n", pid);
        printf("parent sleeping for 20 seconds...\n");
        sleep(20);

    } else {
        perror("fork");
        return 1;
    }
    return 0;
}