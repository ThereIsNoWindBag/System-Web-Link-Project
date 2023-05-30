#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

// 자식 프로세스로부터 SIGCHLD 시그널이 왔을 때 핸들러
static void sigchldHandler(int signum, siginfo_t *info, void *context)
{
    printf("handler: Caught SIGCHLD : %d\n", signum);
    printf("handler: dead process' PID : %d\n", info->si_pid);
    printf("handler: returning\n");
}

int main()
{
    pid_t spid, wpid, ipid, gpid;
    int status, savedErrno;

    struct sigaction sa;
    sa.sa_sigaction = sigchldHandler;
    sa.sa_flags = SA_SIGINFO;

    // Register the signal handler for SIGCHLD
    sigaction(SIGCHLD, &sa, NULL);

    printf("메인 함수입니다.\n");
    
    printf("시스템 서버를 생성합니다.\n");
    spid = create_system_server();
    printf("웹 서버를 생성합니다.\n");
    wpid = create_web_server();
    printf("입력 프로세스를 생성합니다.\n");
    ipid = create_input();
    printf("GUI를 생성합니다.\n");
    gpid = create_gui();

    for(int i = 0; i < 4; i++)
    {
        waitpid(0, &status, 0);
    }

    return 0;
}
