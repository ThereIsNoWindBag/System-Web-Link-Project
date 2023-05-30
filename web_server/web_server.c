#include <stdio.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

const char *FILEBROWSER_LOC = "/usr/local/bin/filebrowser";

pid_t create_web_server()
{
    pid_t web_pid;

    printf("여기서 Web Server 프로세스를 생성합니다.\n");
    web_pid = fork();

    if(web_pid == 0)
    {
        execl(FILEBROWSER_LOC, "filebrowser", "-p", "8282", (char *) NULL);
    }

    return web_pid;
}
