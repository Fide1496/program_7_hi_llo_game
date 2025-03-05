#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define MAX_NUM 100
#define MIN_NUM 1

void play_game(int read_fd, int write_fd) {
    int guess, response;

    srand(getpid()); // Unique seed for each process

    while (1) {
        // Generate a guess
        guess = (rand() % (MAX_NUM - MIN_NUM + 1)) + MIN_NUM;
        printf("Player (PID %d) guesses: %d\n", getpid(), guess);

        // Send guess to referee
        write(write_fd, &guess, sizeof(int));

        // Read referee's response
        read(read_fd, &response, sizeof(int));

        if (response == 0) {
            printf("Player (PID %d) guessed correctly!\n", getpid());
            exit(EXIT_SUCCESS);
        } else if (response == -1) {
            printf("Player (PID %d): Guess was too low.\n", getpid());
        } else {
            printf("Player (PID %d): Guess was too high.\n", getpid());
        }
    }
}

int main() {
    int referee_to_p1[2], p1_to_referee[2];
    int referee_to_p2[2], p2_to_referee[2];

    pipe(referee_to_p1); pipe(p1_to_referee);
    pipe(referee_to_p2); pipe(p2_to_referee);

    pid_t pid1 = fork();
    if (pid1 == 0) { // Player 1 process
        close(referee_to_p2[0]); close(referee_to_p2[1]);
        close(p2_to_referee[0]); close(p2_to_referee[1]);

        play_game(p1_to_referee[0], referee_to_p1[1]);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) { // Player 2 process
        close(referee_to_p1[0]); close(referee_to_p1[1]);
        close(p1_to_referee[0]); close(p1_to_referee[1]);

        play_game(p2_to_referee[0], referee_to_p2[1]);
    }

    // Referee process
    close(referee_to_p1[1]); close(p1_to_referee[0]);
    close(referee_to_p2[1]); close(p2_to_referee[0]);

    srand(time(NULL)); // Seed for random number
    int correct_number = (rand() % (MAX_NUM - MIN_NUM + 1)) + MIN_NUM;
    printf("Referee: The correct number is %d (hidden from players)\n", correct_number);

    int p1_guess, p2_guess, response;

    while (1) {
        // Read guesses from players
        read(p1_to_referee[1], &p1_guess, sizeof(int));
        read(p2_to_referee[1], &p2_guess, sizeof(int));

        // Check Player 1's guess
        if (p1_guess == correct_number) {
            response = 0;
            write(referee_to_p1[0], &response, sizeof(int));
            kill(pid2, SIGTERM); // Stop Player 2
            break;
        } else if (p1_guess < correct_number) {
            response = -1;
        } else {
            response = 1;
        }
        write(referee_to_p1[0], &response, sizeof(int));

        // Check Player 2's guess
        if (p2_guess == correct_number) {
            response = 0;
            write(referee_to_p2[0], &response, sizeof(int));
            kill(pid1, SIGTERM); // Stop Player 1
            break;
        } else if (p2_guess < correct_number) {
            response = -1;
        } else {
            response = 1;
        }
        write(referee_to_p2[0], &response, sizeof(int));
    }

    printf("Game Over! The correct number was %d.\n", correct_number);
    wait(NULL); // Wait for child processes to finish
    wait(NULL);

    return 0;
}
