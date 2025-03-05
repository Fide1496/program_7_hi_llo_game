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

pid_t pid[3];


// Parent signal handler
void sigHandlerP(int sig){
    if (sig == SIGUSR1){
        sig_usr_1 = 1;
    }
    if(sig == SIGUSR2){
        sig_usr_2 = 1;
    }
    else if (sig == SIGCHLD){
        wait(NULL);
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

        min = 0;
        max = 0;

        while(1){
            guess = (min + max) /2;
            int p1_guesses = checkError(open("player1_guesses.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644), "Open player 1 guess file");

            write(p1_guesses, guess, sizeof(int));
            close(p1_guesses);

            if(sig_usr_1) min = guess;
            else if (sig_usr_2) max = guess;
            else break;
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
        pause();

        min = 0;
        max = 101;

        while(1){
            //Validate this guessing structure
            guess = min +rand() % (max-min);

            int p2_guesses = checkError(open("player2_guesses.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644), "Open player 1 guess file");

            write(p2_guesses, guess, sizeof(int));
            close(p2_guesses);

            if(sig_usr_1) min = guess;
            else if (sig_usr_2) max = guess;
            else break;

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

        // Why the heck do I need these????
        sig_usr_1 = 0;
        sig_usr_2 = 0;

        // Wait until start signals are both recieved
        while(!(sig_usr_1 && sig_usr_2)){
            pause();
        }

        int correct_answer = (rand()% 100+1);
        printf("Game #%d: Correct asnwer is %d\n", i, correct_answer);

        while(1){
            sig_usr_1 = 0;
            sig_usr_2 = 0;

            while(!(sig_usr_1 && sig_usr_2)){
                pause();
            }

            int p1_guess, p2_guess;

            int p1_guess_file = checkError(open("player_one_guesses.txt", O_RDONLY), "Open player 1 guess file");
            int p2_guess_file = checkError(open("player_two_guesses.txt", O_RDONLY), "Open player 2 guess file");

            fscanf(p1_guess_file, "%d", &p1_guess);
            fscanf(p2_guess_file, "%d", &p2_guess);

            int test_guess = read(p1_guess_file)

            close(p1_guess_file);
            close(p2_guess_file);

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

    referee();

    exit(EXIT_SUCCESS);
}