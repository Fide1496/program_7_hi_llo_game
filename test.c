#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define MAX_GAMES 10

int child1_pid, child2_pid;
int received_sigusr1 = 0, received_sigusr2 = 0;
int player1_score = 0, player2_score = 0;

// Signal handler for the parent
void parent_signal_handler(int signo) {
    if (signo == SIGUSR1) {
        received_sigusr1 = 1;
    } else if (signo == SIGUSR2) {
        received_sigusr2 = 1;
    } else if (signo == SIGCHLD) {
        wait(NULL); // Reap the child process
    } else if (signo == SIGINT) {
        kill(child1_pid, SIGTERM);
        kill(child2_pid, SIGTERM);
        exit(0);
    }
}

// Signal handler for children
void child_signal_handler(int signo) {
    if (signo == SIGUSR1) {
        received_sigusr1 = 1;
    } else if (signo == SIGUSR2) {
        received_sigusr2 = 1;
    } else if (signo == SIGINT) {
        received_sigusr1 = 0;
        received_sigusr2 = 0;
    } else if (signo == SIGTERM) {
        exit(0);
    }
}

// Function for Player 1 (Binary Search Guessing)
void player1() {
    signal(SIGUSR1, child_signal_handler);
    signal(SIGUSR2, child_signal_handler);
    signal(SIGINT, child_signal_handler);
    signal(SIGTERM, child_signal_handler);

    int min = 0, max = 101, guess;
    while (1) {
        pause(); // Wait for parent signal

        min = 0;
        max = 101;
        while (1) {
            guess = (min + max) / 2;
            FILE *fp = fopen("player1_guess.txt", "w");
            fprintf(fp, "%d\n", guess);
            fclose(fp);

            sleep(1);
            kill(getppid(), SIGUSR1);
            pause(); // Wait for parent's response

            if (received_sigusr1) min = guess;
            else if (received_sigusr2) max = guess;
            else break;
        }
    }
}

// Function for Player 2 (Random Guessing)
void player2() {
    signal(SIGUSR1, child_signal_handler);
    signal(SIGUSR2, child_signal_handler);
    signal(SIGINT, child_signal_handler);
    signal(SIGTERM, child_signal_handler);

    srand(getpid());

    int min = 0, max = 101, guess;
    while (1) {
        pause(); // Wait for parent signal

        min = 0;
        max = 101;
        while (1) {
            guess = min + rand() % (max - min);
            FILE *fp = fopen("player2_guess.txt", "w");
            fprintf(fp, "%d\n", guess);
            fclose(fp);

            sleep(1);
            kill(getppid(), SIGUSR2);
            pause(); // Wait for parent's response

            if (received_sigusr1) min = guess;
            else if (received_sigusr2) max = guess;
            else break;
        }
    }
}

// Function for the Parent (Referee)
void referee() {
    signal(SIGUSR1, parent_signal_handler);
    signal(SIGUSR2, parent_signal_handler);
    signal(SIGCHLD, parent_signal_handler);
    signal(SIGINT, parent_signal_handler);

    sleep(5); // Initial delay before starting the game
    kill(child1_pid, SIGUSR1);
    kill(child2_pid, SIGUSR2);

    for (int game_round = 1; game_round <= MAX_GAMES; game_round++) {
        received_sigusr1 = received_sigusr2 = 0;
        
        while (!(received_sigusr1 && received_sigusr2)) {
            pause(); // Wait for both players to be ready
        }

        int target = (rand() % 100) + 1; // Random target number
        printf("\nGame %d: Target number is %d\n", game_round, target);

        while (1) {
            received_sigusr1 = received_sigusr2 = 0;

            while (!(received_sigusr1 && received_sigusr2)) {
                pause(); // Wait for guesses
            }

            // Read guesses from files
            int guess1, guess2;
            FILE *fp1 = fopen("player1_guess.txt", "r");
            FILE *fp2 = fopen("player2_guess.txt", "r");
            fscanf(fp1, "%d", &guess1);
            fscanf(fp2, "%d", &guess2);
            fclose(fp1);
            fclose(fp2);

            printf("Player 1 guessed: %d, Player 2 guessed: %d\n", guess1, guess2);

            // Check if either player wins
            if (guess1 == target) {
                printf("Player 1 wins!\n");
                player1_score++;
                break;
            }
            if (guess2 == target) {
                printf("Player 2 wins!\n");
                player2_score++;
                break;
            }

            // Send feedback signals
            if (guess1 < target) kill(child1_pid, SIGUSR1);
            else kill(child1_pid, SIGUSR2);

            if (guess2 < target) kill(child2_pid, SIGUSR1);
            else kill(child2_pid, SIGUSR2);

            kill(child1_pid, SIGINT);
            kill(child2_pid, SIGINT);
        }
    }

    printf("\nFinal Results:\n");
    printf("Player 1 Score: %d\n", player1_score);
    printf("Player 2 Score: %d\n", player2_score);

    kill(child1_pid, SIGTERM);
    kill(child2_pid, SIGTERM);
}

int main() {
    srand(time(NULL)); // Seed random number generator

    child1_pid = fork();
    if (child1_pid == 0) {
        player1();
        exit(0);
    }

    child2_pid = fork();
    if (child2_pid == 0) {
        player2();
        exit(0);
    }

    referee();
    return 0;
}
