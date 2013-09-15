/* Project 2: Concurrent Processes (myshell.c)
 * Corey Johns
 * COP4610 Spring 2013
 * Deadline: 2/24/12
 *
 * Extends the earlier shell to include jobs control and background processes.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUF_LEN 80
#define MAX_ARGS 9
#define MAX_JOBS 100

// Struct definition for a command
typedef struct {
  int argc;
  char *argv[MAX_ARGS + 1]; // Hold a null char at end of array
  char *outFile, *inFile; // Handle redirection of input and output
  char bg; // Set to 1 if command is to be backgrounded
} Command;

// Struct definition for a backgrounded process
typedef struct {
  pid_t jid, pid; // Each process has job ID and process ID
  char str[BUF_LEN]; // Store the command string
} Process;

Process jobs[MAX_JOBS]; // Initialize an array to hold all background jobs
int num_jobs; // Keep track of how many jobs are running

// Function prototypes
void printPrompt();
int parseCmd(char*, Command*);
int runCmd(const Command*);
void doCd(const Command*);
void doEcho(const Command*);
void printJobs();
void findExternal(const Command*);
void execExternal(const char*, const Command*);
void fileRedirect(const Command*);
int isFile(const char*);
void checkJobs();
void removeJob(pid_t);

int main(){
  char input[BUF_LEN + 1];
  Command command;
  num_jobs = 0;

  for(;;){
    // Print prompt
    printPrompt();

    // Wait for input
    if(!fgets(input, BUF_LEN, stdin)){
      perror("Error retrieving input");
      continue; // No input, skip processing
    };
    
    input[strlen(input) - 1] = 0; // Ensure last char is null

    // Parse command
    if (!parseCmd(input, &command))
      continue; // Command could not be processed, skip execution

    // Run command
    if (!runCmd(&command))
      return 0; // Command could not be executed -- exit shell
    
    checkJobs();  // Check if any background jobs have finished
  }

  return 0;
}

// Print shell prompt
void printPrompt(){
  char* pwd = get_current_dir_name();
  printf("%s@myshell:%s> ", getenv("USER"), pwd);
  free(pwd);
  fflush(stdout);
}

// Print running jobs
void printJobs(){
  int i;
  for(i = 0; i < num_jobs; i++)
    printf("[%d] %d %s\n", jobs[i].jid+1, jobs[i].pid, jobs[i].str);
}

// Parse command from input. Removes whitespace and sets up command
// Returns 1 for success, 0 for failure
int parseCmd(char* buffer, Command* command){
  char *tokens[MAX_ARGS];
  char *ws = " \n\r\f\t\v"; // Whitespace chars for tokenizing
  int i, numTokens = 0;
  
  // Initialize command
  command->argc = command->bg = 0;
  command->outFile = command->inFile = NULL;
  
  // Tokenize command
  tokens[numTokens] = strtok(buffer, ws);
  while( tokens[numTokens] ){
    if ( ++numTokens == MAX_ARGS ) break;
    
    tokens[numTokens] = strtok(NULL, ws);
  }

  // Setup command
  for(i = 0; i < numTokens; i++){
    // Look for file redirection or environment variables
    if (!strcmp(tokens[i], ">"))
      command->outFile = tokens[++i];
    else if (!strcmp(tokens[i], "<"))
      command->inFile = tokens[++i];
    else if (!strcmp(tokens[i], "&"))
      command->bg = 1;
    else if (tokens[i][0] == '$'){
      char *var = getenv((const char*)&tokens[i][1]);
      if (!var){
        printf("%s: Undefined variable.\n", tokens[i]);
        return 0;
      }
      else {
        command->argv[command->argc] = var;
        ++(command->argc);
      }
    }
    else { // Normal command argument
      command->argv[command->argc] = tokens[i];
      ++(command->argc);
    }
  }
  
  command->argv[command->argc] = 0; // Add null terminator for argument list
  return 1;
}

// Run the specified command. Returns 1 for success, 0 for exit.
int runCmd(const Command* command){
  if(command->argc != 0){
    if (!strcmp(command->argv[0], "cd"))
      doCd(command);
    else if (!strcmp(command->argv[0], "echo"))
      doEcho(command);
    else if (!strcmp(command->argv[0], "jobs"))
      printJobs();
    else if (!strcmp(command->argv[0], "exit"))
      return 0;
    else
      findExternal(command);
  }
  return 1;
}

// Changes current working directory
void doCd(const Command* command){
  char* cwd;
  if (command->argc > 2)
    printf("cd: Two many arguments.\n");
  else {
    if (command->argc == 1)
      cwd = getenv("HOME");
    else
      cwd = command->argv[1];

    if (chdir(cwd))
      printf("%s: No such file or directory.\n", cwd);
  }
}

// Prints each argument
void doEcho(const Command* command){
  int i;
  for (i = 1; i < command->argc; i++)
    printf("%s ", command->argv[i]);
  
  printf("\n");
}

// Tries to find path to an external command
void findExternal(const Command* command){
  char *paths, *fullPath, *tokenPtr;
  
  // If command has '/' in it, try to run it directly
  if (strchr(command->argv[0], '/')){
    if (isFile(command->argv[0]))
      execExternal(command->argv[0], command);
    else
      printf("%s: Command not found.\n", command->argv[0]);
  }
  else {
    // Otherwise perform search for the command
    // First allocate space for $PATH string
    if(getenv("PATH"))
      paths = malloc(strlen(getenv("PATH")) + 1);
    else {
      perror("* findExternal(): Unable to get $PATH.\n");
      return;
    }

    // Perform string copy
    if(paths)
      strcpy(paths, getenv("PATH"));
    else {
      perror("* findExternal(): Unable to copy $PATH.\n");
      return;
    }

    // Tokenize path assuming ':' delimiter
    tokenPtr = strtok(paths, ":");
    while (tokenPtr){
      // Allocate space for path, comma
      fullPath = malloc(strlen(tokenPtr) + strlen(command->argv[0]) + 2);
      if (!fullPath){
        perror("* findExternal(): Unable to create full path.\n");
        return;
      }
      
      // Create full path to command
      strcpy(fullPath, tokenPtr);
      strcat(fullPath, "/");
      strcat(fullPath, command->argv[0]);
      
      // If path exists, run the command
      if (isFile(fullPath)){
        execExternal(fullPath, command);
        free(fullPath);
        return;
      }
      
      // Otherwise, clean up and move to next path
      free(fullPath);
      tokenPtr = strtok(0, ":");
    }

    free(paths);
    printf("%s: Command not found.\n", command->argv[0]);
  }
}

// Executes an external command
void execExternal(const char* fullPath, const Command* command){
  // Create new child process
  pid_t pid = fork();
  
  if (pid < 0)
    perror("* execExternal(): fork() failed.\n");
  else if (pid == 0){

    // Redirect if necessary
    fileRedirect(command);

    // Execute the external command
    execv(fullPath, command->argv);
    perror("* execExternal(): Unable to run command.\n");
    exit(EXIT_FAILURE);

  }
  else {
    if (command->bg == 1){
      Process p;
      p.jid = num_jobs;
      p.pid = pid;
      p.str[0] = 0;

      int i;
      char* space = " ";
      for(i = 0; i < command->argc; i++){
        strcat(p.str, command->argv[i]);
        strcat(p.str, space);
      }
      p.str[strlen(p.str)-1] = 0; // Overwrite last space with null

      jobs[num_jobs++] = p;
      
      // Print job
      printf("[%d] %d\n", p.jid+1, p.pid);
    }
    else
      waitpid(pid, 0, 0);
  }
}

// Maps stdout or stdin to a file
void fileRedirect(const Command* command){
  if (command->outFile){
    int out = open(command->outFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    if (out == -1){
      printf("%s: No such file or directory.\n", command->outFile);
      exit(EXIT_FAILURE);
    }

    dup2(out, 1);
    close(out);

  }

  if (command->inFile){
    int in = open(command->inFile, O_RDONLY);
    if (in == -1){
      printf("%s: No such file or directory.\n", command->inFile);
      exit(EXIT_FAILURE);
    }

    dup2(in, 0);
    close(in);
  }
}

// Determines if the argument is a file. Returns 1 for success, 0 for failure.
int isFile(const char* file){
  struct stat info;
  return (!stat(file, &info) && S_ISREG(info.st_mode));
}

void checkJobs(){
  if (num_jobs <= 0) return;
  pid_t id;

  do {
    id = waitpid(-1, NULL, WNOHANG);
    if (id > 0){
      removeJob(id);
    }
  } while (id > 0);
}

// Removes a job given a pid
void removeJob(pid_t id){
  int i;

  // Find which job needs to be removed
  for(i = 0; i < num_jobs; i++)
    if (jobs[i].pid == id)
      break;
  
  // Print that the job has finished
  printf("[%d] Done %s\n", jobs[i].jid+1, jobs[i].str);

  // If job is not at end, shift everything down
  if ( i != num_jobs-1)
    for(i=i+1; i < num_jobs; i++)
      jobs[i-1] = jobs[i];

  num_jobs--;
  return;
}
