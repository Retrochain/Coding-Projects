/*
  AKSHAAN SINGH
  3/15/2023

  The purpose of this code is to simulate a working shell environment. The code
  utilizes execvp to execute normal bash commands and also includes a few custom
  commands such as cd (change directory), history -c [offset] and exit. The code
  is also suited to run piping however some of the lines in the piping method are
  wrong hence piping does not work as of now. I believe the issue is with how dup2
  is performed but due to lack of help and time I have not been able to complete it. 
  The code is still written in full so in idea it shouldve worked, maybe changing one 
  or two liines could present the desired result. I will try to update the code on a 
  future date but for now, the code is functioning normally.
*/
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

char *history[100];
int j;
int pip[2];
int pidF;

// Pipe function which will allow us to pipe the command line
int piping(char *arr[], int in, int fl, int lf) {
  if (pipe(pip) <= -1) {
    perror("Wrong pipe!");
    exit(0);
  }
  pidF = fork();

  if (pidF == -1) {
    perror("Bad fork!");
    exit(0);
  }

  // Child
  if (pidF == 0) {
    // If its the first command, dup2 with the output
    if (fl == 0 && in == 0) {
      // printf("1\n");
      dup2(pip[WRITE], 1);
      // Else dup2 with both input from the pipe and store the output
    } else if (fl >= 1 && in != 0) {
      // printf("2\n");
      dup2(in, 0);
      dup2(pip[WRITE], 1);
      // Else dup2 with input of the pipe as it is the last command
    } else if (lf == 1) {
      // printf("3\n");
      dup2(in, 0);
    }

    // Close both pipe ends and the child process
    close(in);
    close(pip[WRITE]);
    close(pidF);

    // Run the command
    execvp(arr[0], arr);
    // Prints only when execvp is not executed
    printf("Pipe failure!\n");
    exit(0);
  }

  // Wait process
  waitpid(pidF, NULL, 0);

  // Return the current open end of the pipe
  return pip[READ];
}

// Helper function to help with piping
// The function takes in the array with the arguements, starting and ending
// postions for the new array, and indicators to indicate if its the last, first
// or middle command
int helper(char *y[], int l, int fI, int lI, int fir, int las, int inn) {
  char *x[l];
  int k;

  // Make a new array that contains the commands until the occurance of the
  // first pipe
  for (k = 0; k < l; k++) {
    x[k] = malloc(100 * sizeof(char));
    if (fI < lI - 1)
      strcpy(x[k], y[++fI]);
  }
  x[k] = NULL;

  // Call the pipe function and set it to return the next pipe opening
  int temp = 0;
  temp = piping(x, temp, fir, las);
  // Free the allocated memory
  for (int rm = 0; rm < k; rm++) {
    free(x[rm]);
  }
  // Return the next pipe opening
  return temp;
}

// This function adds the current line to the history array
void addHistory(const char *cmd) {
  // If the number of command exceeds 100, push each element up one index
  if (j == 100)
    for (int x = 0; x < 99; x++)
      strcpy(history[x], history[x + 1]);
  // The array needs to emptied if the argument equals -c
  else if (strcmp(cmd, "-c") == 0)
    for (int r = 0; r < j; r++)
      free(history[r]);
  // Else add the current command to the array
  else {
    history[j] = malloc(100 * sizeof(char));
    strcpy(history[j], cmd);
    j++;
  }
}

// Method to print history
static void printHistory() {
  // Loop around the array
  for (int r = 0; r < j; r++)
    printf("%d %s\n", r, history[r]);
}

int main(int argc, char *argv[]) {

  char *myargs[100];
  char *command;
  char buffer[100];

  while (1) {
    int i = 0;
    printf("sish> ");

    // Get input from the user and store it in buffer
    if (fgets(buffer, 100, stdin) != NULL)
      // if the last digit is a newline character
      if ((buffer[strlen(buffer) - 1]) == '\n')
        buffer[strlen(buffer) - 1] = '\0';

    // Add the current command to the history
    addHistory(buffer);

    // If user inputs exit, leave the program
    if (strcmp(buffer, "exit") == 0)
      exit(0);

    // Tokenize the input command and store it in an array
    command = strtok(buffer, " ");
    while (command != NULL) {
      myargs[i] = command;
      i++;
      command = strtok(NULL, " ");
    }
    // Set last character of the array to NULL
    myargs[i] = NULL;

    // If the command is cd, use chdir to change directories
    if (strcmp(myargs[0], "cd") == 0) {
      // If directory doesnt exist, show error
      if (chdir(myargs[1]) != 0)
        perror("cd failed");
      // If the command is history, display the history
    } else if (strcmp(myargs[0], "history") == 0) {
      // If history is called by itself, display all the commands entered so
      // Far
      if (myargs[1] == NULL) {
        printHistory();
        addHistory(buffer);
        // If the command is called with -c arg, clear the history array
      } else if (strcmp(myargs[1], "-c") == 0) {
        addHistory(myargs[1]);
        j = 0;
        // Else use the offset provided to execute the command if possible
      } else {
        int index;
        // Store the current offset in an index
        sscanf(myargs[1], "%d", &index);
        // Use that index to execute the command (is possible)
        char *str;
        char *arr[102];
        int y = 0;
        // Str equals the command in the current offset
        str = history[index];

        // Tokenize the command
        char *cm = strtok(str, " ");
        while (cm != NULL) {
          arr[y] = cm;
          y++;
          cm = strtok(NULL, " ");
        }
        arr[y] = NULL;

        // Command only executes if possible
        int rc2 = fork();
        if (rc2 <= -1)
          perror("Pipe executed wrong!");

        if (rc2 == 0) {
          // Execute the program with provided arguments
          int s = execvp(arr[0], arr);
          // Printf only prints if execvp fails
          if (s == -1)
            printf("Invalid command to execute!\n");
        } else {
          // Let parent process wait
          wait(NULL);
        }
      }
      // Otherwise it is either a executable command or invalid command
    } else {
      // Call child process to execute command with args
      int pipepos = -1;
      int first = 0;
      int laspos = 0;
      int inn = 0;
      for (int y = 0; y < i; y++) {
        // If its the first command or the middle command, call the function
        // Before the next pipe
        if (strcmp(myargs[y], "|") == 0) {
          // Make a counter for first and last
          // If first is 0, thats the first cmd
          // If first is > 0, thats the middle cmd
          // If last is 1, thats the last cmd
          inn = helper(myargs, y - laspos, pipepos, y, first, 0, inn);
          pipepos = y;
          laspos = y + 1;
          first++;
          printf("\n");
        }
        // Else its the last command hence call the function on the command
        // After the last pipe
        if (myargs[y + 1] == NULL) {
          inn = helper(myargs, i - laspos, pipepos, y + 1, -1, 1, inn);
        }
      }
    }
  }
  exit(EXIT_SUCCESS);
}
