#include <stdio.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

pid_t create_gui()
{
    pid_t gui_pid;

    printf("여기서 GUI 프로세스를 생성합니다.\n");
    sleep(3);
    gui_pid = fork();

    if(gui_pid == 0)
    {
        execl("/usr/bin/google-chrome-stable", "google-chrome-stable", "http://localhost:8282", NULL);
    }

    return gui_pid;
}
