#include <stdio.h>
#include <sys/prctl.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int input()
{
    printf("나 input 프로세스!\n");

    while (1) {
        sleep(1);
    }

    return 0;
}

pid_t create_input()
{
    pid_t input_pid;
    const char *name = "input";

    printf("여기서 input 프로세스를 생성합니다.\n");
    input_pid = fork();

    if(input_pid == 0)
    {
        input();
    }

    return input_pid;
}
