#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

// 자식 프로세스로부터 SIGCHLD 시그널이 왔을 때 핸들러
// ¿ 핸들러 내부에서 반환된 자식 프로세스의 PID를 출력할 수 있으면 좋을텐데... ?
static void sigchldHandler(int sig)
{
    printf("handler: Caught SIGCHLD : %d\n", sig);
    printf("handler: returning\n");
}

int main()
{
    pid_t spid, wpid, ipid, gpid;
    int status, savedErrno;

    signal(SIGCHLD, sigchldHandler);

    printf("메인 함수입니다.\n");
    
    printf("시스템 서버를 생성합니다.\n");
    spid = create_system_server();
    printf("웹 서버를 생성합니다.\n");
    wpid = create_web_server();
    printf("입력 프로세스를 생성합니다.\n");
    ipid = create_input();
    printf("GUI를 생성합니다.\n");
    gpid = create_gui();

    waitpid(spid, &status, 0);
    waitpid(wpid, &status, 0);
    waitpid(ipid, &status, 0);
    waitpid(gpid, &status, 0);

    return 0;
}
