#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

static int player_one_min=0;
static int player_one_max=100;
static int player_two_min=0;
static int player_two_max=100;

int checkError(int val, const char *msg) {
    if (val == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return val;
}

void signalHandler(int sig){
    char response;

    if(sig == SIGINT){
        fflush(stdin);
        printf("\nAre you sure you want to exit (Y/n)? \n");
        scanf(" %c", &response);

        if (response == 'Y' || response == 'y') {
            // if (pPid>0){
            //     kill(pPid, SIGTERM);
            // }
            exit(EXIT_SUCCESS);
        }

        // while(1)
        // {
        //     pid = waitpid(0,NULL,0);
        //     if(pid == -1){
        //         if(errno ==ECHILD) exit(EXIT_FAILURE);
        //         if(errno == EINTR) continue;
        //         perror("wait");
        //         exit(EXIT_FAILURE);
        //     }
    
        // }
    }
    if (sig == SIGCHLD) {
        printf("Child has exited\n");
        while(waitpid(-1, NULL, WNOHANG) > 0);
        exit(EXIT_SUCCESS);
    }
    if (sig == SIGUSR1) {
        printf("Warning! Roll outside of bounds\n");
    }
    if (sig == SIGUSR2) {
        printf("Warning! Pitch outside of bounds\n");
    }
    if(sig == SIGTERM){
        printf("Parent has died, child terminating...\n");
        exit(EXIT_SUCCESS);
    }
}

int player_one_guess(){

    // Implement guessing strategy
    int guess = (player_one_max - player_one_min)/2;

    return guess;
}

int player_two_guess(){

    // Implement guessing strategy
    int guess = rand() % (player_two_max - player_two_min + 1)
    + player_two_min;

    return guess;
}

int main(){

    pid_t childPID;
    sigset_t blockSet, oldSet;
    pid_t child[2];
    pid_t pid;
    int i = 0;
    

    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signalHandler;
    sa.sa_flags = 0;

    // Register signals in parent
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);


    for(int i; i< 10; i++){
        printf("Game $%d\n", i+1);

        int answer = rand(); 

        while(1){
            pid_t player_one = fork();
            pid_t player_two = fork();

            int p1_guess = player_one_guess();
            int p2_guess = player_two_guess();



        }
    }
    
    for(i = 0; i<10; i++){
        printf("Game #%d \n", i+1);
        
        int x;
        for(x = 0; x < 2; x++){



            switch(childPID = fork())
            {
            case -1:
                perror("fork\n");
                exit(EXIT_FAILURE);
            case 0:
    
                struct sigaction sa_child;
                sigemptyset(&sa_child.sa_mask);
                sa_child.sa_handler = signalHandler;
                sa_child.sa_flags = 0;
                
                // Handle SIGTERM in child
                sigaction(SIGTERM, &sa_child, NULL);
                
                int player_one_guess = player_one();
                
                printf("Child process done. Exiting...\n");
                exit(EXIT_SUCCESS);
    
            default:
    
                // Handles signals from the child to the parent
                struct sigaction sa;
                sigemptyset(&sa.sa_mask);
                sa.sa_handler = signalHandler;
                sa.sa_flags = 0;
    
                // Register signals in parent
                sigaction(SIGINT, &sa, NULL);
                sigaction(SIGCHLD, &sa, NULL);
                sigaction(SIGUSR1, &sa, NULL);
                sigaction(SIGUSR2, &sa, NULL);
                
                sigprocmask(SIG_SETMASK, &oldSet, NULL);
    
                while(1) {
                    pause();  
                }
            }
        }
        // exit(EXIT_SUCCESS);

    }

    exit(EXIT_SUCCESS);
}