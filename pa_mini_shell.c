/*
|--------------------------------------------------------------------------
|           PA Mini Shell
|--------------------------------------------------------------------------
|
| Author    Paulius Aleksiunas
| 
| Date      2016-10-23
|
*/


#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>


#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIM " \t\r\n\a"


/*
|--------------------------------------------------------------------------
| Function Declarations 
|--------------------------------------------------------------------------
*/
int pa_cd(char **args);
int pa_help(char **args);
int pa_exit(char **args);


/*
|--------------------------------------------------------------------------
| Welcome Message
|--------------------------------------------------------------------------
*/
void welcomeMessage(){
        printf("\n\t----------------------------------------------------------------\n");
        printf("\t   Mini Shell\n");
        printf("\t----------------------------------------------------------------\n");
        printf("\t   Enter program name, arguments, and press enter.\n");
        printf("\t----------------------------------------------------------------\n");
        printf("\n\n");
}


/*
|--------------------------------------------------------------------------
| Built-in Commands
|--------------------------------------------------------------------------
*/
char *builtinCommands[] = {
  "cd",
  "help",
  "exit",
};


/*
|--------------------------------------------------------------------------
| Built-in Functions
|--------------------------------------------------------------------------
*/
int (*builtinFunctions[]) (char **) = {
  &pa_cd,
  &pa_help,
  &pa_exit,
};


/*
|--------------------------------------------------------------------------
| Built-ins Count
|--------------------------------------------------------------------------
*/
int builtinsCount() {
  return sizeof(builtinCommands) / sizeof(char *);
}


/*
|--------------------------------------------------------------------------
| Change Directory
|--------------------------------------------------------------------------
|
| Built-in command.
|
*/
int pa_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "Expected an argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("Error occured while changing directory");
    }
  }

  return 1;
}


/*
|--------------------------------------------------------------------------
| Help
|--------------------------------------------------------------------------
|
| Built-in command.
|
*/
int pa_help(char **args)
{
  int i;

  printf("Paulius Aleksiunas Mini Shell\n");
  printf("Enter program name, arguments, and press enter.\n");
  printf("The following are built-in:\n");

  for (i = 0; i < builtinsCount(); i++) {
    printf("  %s\n", builtinCommands[i]);
  }

  printf("Use the 'man' command for information on other programs.\n");

  return 1;
}


/*
|--------------------------------------------------------------------------
| Exit
|--------------------------------------------------------------------------
|
| Built-in command.
|
*/
int pa_exit(char **args)
{
  return 0;
}


/*
|--------------------------------------------------------------------------
| Count Commands
|--------------------------------------------------------------------------
|
| Calculating the number of programs. They are seperated by '|'.
|
*/
int countCommands(char **args)
{
  int commandsCount = 1;
  int l = 0;

  // Calculating the number of programs.
  while (args[l] != NULL) {
    if (strcmp(args[l], "|") == 0) {
      commandsCount++;
    }

    l++;
  }

  return commandsCount;
}


/*
|--------------------------------------------------------------------------
| Launch
|--------------------------------------------------------------------------
|
| Launch a program and wait for it to terminate.
|
*/
int launch(char **args)
{
  pid_t pid, wpid;
  int status;

  // Forking: duplicating the process.  
  pid = fork();

  if (pid == 0) {
    // Child process. Starting the program.
    if (execvp(args[0], args) == -1) {
      perror("Error while starting the program");
    }

    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("Error forking");
  } else {
    // Parent process
    do {
      // Waiting while program finish running.
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


/*
|--------------------------------------------------------------------------
| File I/O
|--------------------------------------------------------------------------
|
| Manage I/O redirection.
|
*/
int fileIO(char **argsWithoutRedirection, char *inputFile, char *outputFile, int option)
{
  int fileDescriptor;
  pid_t pid, wpid;
  int status;

  // Forking: duplicating the process.  
  pid = fork();

  if (pid == 0) {
    // Option 0: output redirection
    if (option == 0) {
      // We open (create) the file truncating it at 0, for write only
      fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
      // We replace de standard output with the appropriate file
      dup2(fileDescriptor, STDOUT_FILENO); 
      close(fileDescriptor);
    } else if (option == 1) {
      // We open file for read only (it's STDIN)
      fileDescriptor = open(inputFile, O_RDONLY, 0600);  
      // We replace de standard input with the appropriate file
      dup2(fileDescriptor, STDIN_FILENO);
      close(fileDescriptor);
      // Same as before for the output file
      fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
      dup2(fileDescriptor, STDOUT_FILENO);
      close(fileDescriptor);     
    }

    // Child process. Starting the program.
    if (execvp(argsWithoutRedirection[0], argsWithoutRedirection) == -1) {
      perror("Error while starting the program");
    }

    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("Error forking");
  } else {
    // Parent process
    do {
      // Waiting while program finish running.
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


/*
|--------------------------------------------------------------------------
| Remove Redirection Symbols
|--------------------------------------------------------------------------
|
| Removes '<' and '>' symbols.
|
*/
char **removeRedirectionSymbols(char **args)
{
  char **argsWithoutRedirection = malloc(64 * sizeof(char*));
  int i = 0;

  while (args[i] != NULL){
    if ((strcmp(args[i], ">") == 0) || (strcmp(args[i], "<") == 0)) {
      break;
    }

    argsWithoutRedirection[i] = args[i];
    i++;
  }

  return argsWithoutRedirection;
}


/*
|--------------------------------------------------------------------------
| Execute
|--------------------------------------------------------------------------
|
| Execute shell built-in or launch program.
|
*/
int execute(char **args)
{
  char **argsWithoutRedirection;
  int i;
  int j = 0;

  argsWithoutRedirection = removeRedirectionSymbols(args);

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < builtinsCount(); i++) {
    // Cheking if command is equal to buil-in command.
    if (strcmp(args[0], builtinCommands[i]) == 0) {
      return (*builtinFunctions[i])(args);
    }
  }

  while (args[j] != NULL) {
    if (strcmp(args[j], ">") == 0) {
      if (args[j + 1] == NULL) {
        printf("Not enough input arguments.\n");

        return 1;
      }

      fileIO(argsWithoutRedirection, NULL, args[j + 1], 0);

      return 1;
    } else if (strcmp(args[j], "<") == 0) {
        if (args[j + 1] == NULL || args[j + 2] == NULL || args[j + 3] == NULL) {
          printf("Not enough input arguments.\n");

          return 1;
        } else {
          if (strcmp(args[j + 2], ">") != 0) {
            printf("Expected '>' but found %s\n", args[j + 2]);

            return 1;
          } 
        }

        fileIO(argsWithoutRedirection, args[j + 1], args[j + 3], 1);

        return 1;
    }

    j++;
  }

  return launch(args);
}


/*
|--------------------------------------------------------------------------
| Read Line
|--------------------------------------------------------------------------
|
| Read a line from stdin.
|
*/
char *readLine()
{
  char *line = NULL;
  size_t bufferSize = 0;

  getline(&line, &bufferSize, stdin);

  return line;
}


/*
|--------------------------------------------------------------------------
| Validate Tokens
|--------------------------------------------------------------------------
|
| Check if tokens buffer was initialized properly.
|
*/
void validateTokens(char **tokens)
{
  if (!tokens) {
    fprintf(stderr, "Allocation error\n");

    exit(EXIT_FAILURE);
  }
}


/*
|--------------------------------------------------------------------------
| Split Line
|--------------------------------------------------------------------------
|
| Split a line into tokens.
|
*/
char **splitLine(char *line)
{
  int bufferSize = TOKEN_BUFFER_SIZE;
  int position = 0;
  char **tokens = malloc(bufferSize * sizeof(char*));
  char *token;

  validateTokens(tokens);

  // Tokenizing the string with specified delim.
  token = strtok(line, TOKEN_DELIM);

  while (token != NULL) {
    // Adding token to tokens array. 
    tokens[position] = token;
    position++;

    // Reallocating array if buffer is full.
    if (position >= bufferSize) {
      bufferSize += TOKEN_BUFFER_SIZE;
      tokens = realloc(tokens, bufferSize * sizeof(char*));
      validateTokens(tokens);
    }

    token = strtok(NULL, TOKEN_DELIM);
  }

  tokens[position] = NULL;

  return tokens;
}


/*
|--------------------------------------------------------------------------
| Loop
|--------------------------------------------------------------------------
|
| Loop incoming data and execute it.
|
*/
void loop()
{
  char *line;
  char **args;
  int status = 1;

  while (status) {
    // Printing a prompt.
    printf(": ");

    // Read command from input.
    line = readLine();

    // Seperate command and arguments.
    args = splitLine(line);

    // Run parsed command.
    status = execute(args);

    free(line);
    free(args);
  }
}


/*
|--------------------------------------------------------------------------
| Main
|--------------------------------------------------------------------------
|
*/
int main(int argc, char **argv)
{
  welcomeMessage();

  // Start looping incoming data from stdin.
  loop();

  return EXIT_SUCCESS;
}
