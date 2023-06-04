#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <sys/mman.h>
#include <signal.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

#define STACK_SIZE (8 * 1024 * 1024)

const char *FILEBROWSER_LOC = "/usr/local/bin/filebrowser";

static int start_web_server(void *arg)
{
    execl(FILEBROWSER_LOC, "filebrowser", "-p", "8282", (char *) NULL);

    return 0;
}

pid_t create_web_server()
{
    pid_t web_pid;

    char *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        printf("mmap failed\n");
        return -1;
    }

    printf("여기서 Web Server 프로세스를 생성합니다.\n");
    web_pid = clone(start_web_server, stack + STACK_SIZE, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNS | SIGCHLD, NULL);
    if (web_pid == -1)
        printf("clone failed\n");

    return web_pid;
}
