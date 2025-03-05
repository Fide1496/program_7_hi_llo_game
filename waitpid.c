/* 1. creates 3 children ... handling 4 concurrent processes .. the parent + 3
children
2. pauses the 3 children
3. resumes the 3 children
4. terminates the 3 children
parent:
signals to support: SIGCHLD, SIGALRM, SIGHUP
on SIGCHLD, use waitpid to determine which child or children have changed status
and what type of status change
on SIGALRM, we will move one step further along our phases of execution ...
what that means, is on the first SIGALRM, we pause the children ...
on the second, we resume the children,
on the third, we terminate the children
on SIGHUP, send SIGHUP to all of the children
this program should perform the necessary setup and enter a pause loop.
children:
signals to support: SIGHUP
on SIGHUP, we just perform a normal termination
enter a pause loop.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
int cno = 0;
int stage = 0;
pid_t pid[3];
void exitP(void)
{
    char *out = "Parent has normally terminated\n";
    write(STDOUT_FILENO, out, strlen(out));
}
void exitC(void)
{
    char *out = "Child has normally terminated\n";
    write(STDOUT_FILENO, out, strlen(out));
}
void signalHandlerP(int sig)
{
    char other[1024] = {0};
    pid_t p;
    int status, i;
    if (sig == SIGCHLD)
    {
        while ((p = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
        {
            if (WIFEXITED(status))
            {
                sprintf(other, "Child @ pid %d is exiting\n", p);
                write(STDOUT_FILENO, other, strlen(other));
            }
            if (WIFSIGNALED(status))
            {
                sprintf(other, "Child @ pid %d is exiting from a signal\n", p);
                write(STDOUT_FILENO, other, strlen(other));
            }
            if (WIFSTOPPED(status))
            {
                sprintf(other, "Child @ pid %d has paused\n", p);
                write(STDOUT_FILENO, other, strlen(other));
            }
            if (WIFCONTINUED(status))
            {
                sprintf(other, "Child @ pid %d has resumed\n", p);
                write(STDOUT_FILENO, other, strlen(other));
            }
        }
        if (p == -1)
        {
            if (errno == ECHILD)
            {
                exit(EXIT_SUCCESS);
            }
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }
    if (sig == SIGALRM)
    {
        stage++;
        switch (stage)
        {
        case 1: /* pause the children */
            for (i = 0; i < 3; i++)
            {
                sprintf(other, "Stopping child %d\n", i);
                kill(pid[i], SIGSTOP);
                write(STDOUT_FILENO, other, strlen(other));
            }
            break;
        case 2: /* resume the children */
            for (i = 0; i < 3; i++)
            {
                sprintf(other, "Restarting child %d\n", i);
                kill(pid[i], SIGCONT);
                write(STDOUT_FILENO, other, strlen(other));
            }
            break;
        case 3: /* kill the children */
            for (i = 0; i < 3; i++)
            {
                sprintf(other, "Terminating child %d\n", i);
                kill(pid[i], SIGHUP);
                write(STDOUT_FILENO, other, strlen(other));
            }
            break;
        }
    }
    if (sig == SIGHUP)
    {
        kill(0, SIGHUP);
        exit(EXIT_SUCCESS);
    }
}
void signalHandlerC(int sig)
{
    if (sig == SIGHUP)
    {
        exit(EXIT_SUCCESS);
    }
}
int main(int argc, char *argv[])
{
    int i = 0; /* loop counter */
    struct sigaction saP;
    struct sigaction saC;
    struct itimerval tv;

    saP.sa_handler = signalHandlerP;
    sigemptyset(&saP.sa_mask);
    saP.sa_flags = 0;

    saC.sa_handler = signalHandlerC;
    sigemptyset(&saC.sa_mask);
    saC.sa_flags = 0;

    tv.it_interval.tv_sec = 3;
    tv.it_interval.tv_usec = 0;
    tv.it_value.tv_sec = 3;
    tv.it_value.tv_usec = 0;
    
    /* first, add the signal hanlder for SIGCHLD */
    if (sigaction(SIGCHLD, &saP, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    /* ok .. let's fork some children */
    for (i = 0; i < 3; i++)
    {
        pid[i] = fork();
        switch (pid[i])
        {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            atexit(exitC);
            if (sigaction(SIGHUP, &saC, NULL) == -1)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            printf("Starting child %d\n", i + 1);
            while (1)
            {
                pause();
            }
            exit(EXIT_SUCCESS);
            break;
        }
    }
    /* back in the parent */
    atexit(exitP);
    if (sigaction(SIGHUP, &saP, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &saP, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (setitimer(ITIMER_REAL, &tv, NULL) == -1)
    {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        pause();
    }
    exit(EXIT_SUCCESS);
}
