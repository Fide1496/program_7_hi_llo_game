#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define GAMES 10

pid_t child_one_pid, child_two_pid;

static int player_one_min=0;
static int player_one_max=100;
static int player_two_min=0;
static int player_two_max=100;

int sig_usr_1 = 0;
int sig_usr_2 = 0;

int player_one_score = 0;
int player_two_score = 0;

// Check error function
int checkError(int val, const char *msg) {
    if (val == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return val;
}

// Parent signal handler
void sigHandlerP(int sig){
    if (sig == SIGUSR1){
        sig_usr_1 = 1;
    }
    if(sig == SIGUSR2){
        sig_usr_2 = 1;
    }
    else if (sig == SIGCHLD){
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }
    else if (sig ==SIGINT){
        kill(child_one_pid, SIGTERM);
        kill(child_two_pid, SIGTERM);
        exit(EXIT_SUCCESS);
    }
}

// Child signal handler
void sigHandlerC(int sig){
    if (sig == SIGUSR1){
        sig_usr_1 = 1;
    }
    if(sig == SIGUSR2){
        sig_usr_2 = 1;
    }
    else if (sig == SIGINT){
        sig_usr_1 = 0;
        sig_usr_2 = 0;
    }
    else if (sig == SIGTERM)
    {
        exit(EXIT_SUCCESS);
    }
}

// Player one: uses average of min and max as guess
void player_one(){

    signal(SIGUSR1, sigHandlerC);
    signal(SIGUSR2, sigHandlerC);
    signal(SIGINT, sigHandlerC);
    signal(SIGTERM, sigHandlerC);

    int min = 0;
    int max = 101;
    int guess;

    while(1){
        pause();

        kill(getppid(), SIGUSR1);
        min = 0;
        max = 101;

        while(1){
            guess = (min + max) /2;
            int p1_guesses = checkError(open("player1_guess.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644), "Open player 1 guess file");

            if (p1_guesses != -1) {
                char buffer[16];
                int len = snprintf(buffer, sizeof(buffer), "%d\n", guess);
                write(p1_guesses, buffer, len);
                close(p1_guesses);
            }
            sleep(1);
            kill(getppid(), SIGUSR1);

            pause();

            if(sig_usr_1) min = guess;
            if (sig_usr_2) max = guess;
        }
    }
}

// Player two: uses random number between min and max as guess
void player_two(){

    signal(SIGUSR1, sigHandlerC);
    signal(SIGUSR2, sigHandlerC);
    signal(SIGINT, sigHandlerC);
    signal(SIGTERM, sigHandlerC);

    srand(1);

    int min = 0;
    int max = 101;
    int guess;

    while(1){
        printf("Waiting for children to send first signals\n");
        pause();
        kill(getppid(), SIGUSR2);

        min = 0;
        max = 101;

        while(1){
            //Validate this guessing structure
            guess = min + rand() % (max-min);


            int p2_guesses = checkError(open("player2_guess.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644), "Open player 2 guess file");

            if (p2_guesses != -1) {
                char buffer[16];
                int len = snprintf(buffer, sizeof(buffer), "%d\n", guess);
                write(p2_guesses, buffer, len);
                close(p2_guesses);
            }

            sleep(1);
            kill(getppid(), SIGUSR2);
            pause();

            if (sig_usr_1) min = guess;
            if (sig_usr_2) max = guess;
            
        
        }
    }
}

void referee(){
    signal(SIGUSR1, sigHandlerP);
    signal(SIGUSR2, sigHandlerP);
    signal(SIGCHLD, sigHandlerP);
    signal(SIGINT, sigHandlerP);

    sleep(5);
    kill(child_one_pid, SIGUSR1);
    kill(child_two_pid, SIGUSR2);

    for (int i = 1; i <= GAMES; i++){

        // // Why the heck do I need these????
        // sig_usr_1 = 0;
        // sig_usr_2 = 0;

        // Wait until start signals are both recieved
        while(!(sig_usr_1 && sig_usr_2)){
            pause();
        }

        int correct_answer = (rand()% 100+1);
        printf("Game #%d: Correct answer is %d\n", i, correct_answer);

        while(1){
            // sig_usr_1 = 0;
            // sig_usr_2 = 0;

            while(!(sig_usr_1 && sig_usr_2)){
                pause();
            }

            char buffer1[16], buffer2[16];
            int p1_guess = 0, p2_guess = 0;

            int p1_guess_file = checkError(open("player1_guess.txt", O_RDONLY), "Open player 1 guess file");
            int p2_guess_file = checkError(open("player2_guess.txt", O_RDONLY), "Open player 2 guess file");

            if (p1_guess_file != -1) {
                int bytes_read = read(p1_guess_file, buffer1, sizeof(buffer1) - 1);
                if (bytes_read > 0) {
                    buffer1[bytes_read] = '\0';  // Null-terminate
                    p1_guess = atoi(buffer1);
                }
                close(p1_guess_file);
            }
            
            if (p2_guess_file != -1) {
                int bytes_read = read(p2_guess_file, buffer2, sizeof(buffer2) - 1);
                if (bytes_read > 0) {
                    buffer2[bytes_read] = '\0';  // Null-terminate
                    p2_guess = atoi(buffer2);
                }
                close(p2_guess_file);
            }

            printf("Player 1 guessed: %d, Player 2 guessed: %d\n",p1_guess, p2_guess);

            if(p1_guess == correct_answer){
                printf("Player 1 wins!\n");
                player_one_score++;
                break;
            }
            if(p2_guess==correct_answer){
                printf("Player 2 wins!\n");
                player_two_score++;
                break;
            }

            if(p1_guess < correct_answer)kill(child_one_pid, SIGUSR1);
            else kill(child_two_pid, SIGUSR2);

            if(p2_guess < correct_answer)kill(child_two_pid, SIGUSR1);
            else kill(child_two_pid, SIGUSR2);

            kill(child_one_pid, SIGINT);
            kill(child_two_pid, SIGINT);
        }


    }

    printf("Final Results: \n");
    printf("Player 1 score: %d", player_one_score);
    printf("Player 2 score: %d", player_two_score);

    kill(child_one_pid, SIGTERM);
    kill(child_two_pid, SIGTERM);
}

int main(){

    child_one_pid = fork();
    if (child_one_pid == 0){
        player_one();
        exit(EXIT_SUCCESS);
    }

    child_two_pid = fork();
    if(child_two_pid == 0){
        player_two();
        exit(EXIT_SUCCESS);
    }

    sleep(1);
    referee();

    exit(EXIT_SUCCESS);
}