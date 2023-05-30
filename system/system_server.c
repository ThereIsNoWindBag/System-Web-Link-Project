#include <stdio.h>
#include <sys/prctl.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int system_server()
{
    printf("나 system_server 프로세스!\n");

    while (1) {
        sleep(1);
    }

    return 0;
}

pid_t create_system_server()
{
    pid_t system_pid;
    const char *name = "system_server";

    printf("여기서 시스템 프로세스를 생성합니다.\n");
    system_pid = fork();

    if(system_pid == 0)
    {
        system_server();
    }

    return system_pid;
}
