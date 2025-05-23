#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void ouch(int sig)
{
    printf("\nOUCH! - I got signal %d\n", sig);
    //恢复终端中断信号SIGINT的默认行为
    (void) signal(SIGINT, SIG_DFL);
}

int main()
{
    //改变终端中断信号SIGINT的默认行为，使之执行ouch函数
    //而不是终止程序的执行
    (void) signal(SIGINT, ouch);
    while(1)
    {
        printf("Hello World!\n");
        sleep(1);
    }
        return 0;
}
