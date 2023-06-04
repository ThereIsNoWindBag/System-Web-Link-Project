#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <mqueue.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <toy_message.h>

#define NUM_MESSAGES 10

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

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

    struct mq_attr mq_attrib;

    memset(&mq_attrib, 0, sizeof(mq_attrib));
    mq_attrib.mq_msgsize = sizeof(toy_msg_t);
    mq_attrib.mq_maxmsg = 10;

    watchdog_queue = mq_open("/watchdog_queue", O_CREAT|O_RDWR, 0666, &mq_attrib);
    monitor_queue = mq_open("/monitor_queue", O_CREAT|O_RDWR, 0666, &mq_attrib);
    disk_queue = mq_open("/disk_queue", O_CREAT|O_RDWR, 0666, &mq_attrib);
    camera_queue = mq_open("/camera_queue", O_CREAT|O_RDWR, 0666, &mq_attrib);
    
    printf("시스템 서버를 생성합니다.\n");
    spid = create_system_server();
    printf("웹 서버를 생성합니다.\n");
    wpid = create_web_server();
    if(wpid == -1)
    {
        kill(spid, 9);
        return 0;
    }
        
    printf("입력 프로세스를 생성합니다.\n");
    ipid = create_input();
    printf("GUI를 생성합니다.\n");
    gpid = create_gui();

    for(int i = 0; i < 4; i++)
    {
        waitpid(0, &status, 0);
    }

    mq_close(watchdog_queue);
    mq_close(monitor_queue);
    mq_close(disk_queue);
    mq_close(camera_queue);

    mq_unlink("/watchdog_queue");
    mq_unlink("/monitor_queue");
    mq_unlink("/disk_queue");
    mq_unlink("/camera_queue");

    return 0;
}
