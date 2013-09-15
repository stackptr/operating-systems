/* Project 1: Unix Shell (myshell.c)
 * Corey Johns
 * COP4610 Spring 2013
 * Deadline: 2/3/12
 *
 * Implements a simple shell allowing for built-in and external commands,
 * command parsing, and file redirection.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_LEN 80
#define MAX_ARGS 9

int main(){
  // Variables for parsing
  const char* ws = " \n\r\f\t\v"; // Prompt parsing delimiters
  const char* colon = ":"; // Colon delimiter for $PATH searching
  char *token, input[BUF_LEN], *token_list[BUF_LEN/2]; // Tokenized string list
  char *argv[MAX_ARGS+1];  // Account for null string to execv
  char *outfile_redir; // File to output to if necessary
  char *infile_redir; // File to use as input if necessary
  char *exec_path; // Stores path to executable
  int out_redir, in_redir; // 0 = no redirect
  int bin_found; // 0 = not found
  int argc, tokenc, i;

  // Prompt variables
  const char* user = getenv("USER");
  const char* shell = "myshell";
  char cwd[BUF_LEN];

  if (user == NULL){
    printf("ERROR: getenv failed!\n");
    exit(1);
  }

  // Start prompt
  while(1){
    // Print command prompt
    getcwd(cwd, BUF_LEN); // Update current directory
    if (cwd == NULL) {
      printf("ERROR: getcwd failed!\n");
      exit(1);
    }

    // Print prompt if getcwd is successful
    printf("%s@%s:%s> ", user, shell, cwd);

    // Wait for input
    if(fgets(input, BUF_LEN, stdin) == NULL){
      printf("ERROR: fgets failed!\n");
      exit(1);
    };
    
    // Parse input
    argc = tokenc = out_redir = in_redir = 0; // no file redirect or arguments
    token = strtok(input, ws);
    while(token != NULL) {
      token_list[tokenc] = malloc(strlen(token) + 1);
      strcpy(token_list[tokenc++], token);
      token = strtok(NULL, ws);
    }

    // Variable expansion and file redirects
    for (i = 0; i < tokenc; i++){
      argc++;
      if (token_list[i][0] == '$'){
        char *temp = ++token_list[i];
        if (getenv(token_list[i]) != NULL){
          argv[i] = malloc(strlen(getenv(temp))+1);
          strcpy(argv[i], getenv(temp));
        }
        else{
          int cx;
          char s[1];
          cx = snprintf(s, 0, "$%s: Undefined variable", temp);
          argv[i] = malloc(cx+1);
          snprintf(argv[i], cx+1, "$%s: Undefined variable", temp);
        }
        --token_list[i];
      }
      else if ( token_list[i][0] == '>'){
        out_redir = 1;
        argc--;
        i++;
        outfile_redir = malloc(strlen(token_list[i]) + 1);
        strcpy(outfile_redir, token_list[i]);

      }
      else if ( token_list[i][0] == '<'){
        in_redir = 1;
        argc--;
        i++;
        infile_redir = malloc(strlen(token_list[i]) + 1);
        strcpy(infile_redir, token_list[i]);
      }
      else {
        argv[i] = malloc(strlen(token_list[i]) + 1);
        strcpy(argv[i], token_list[i]);
      }
    }

    // Free up token list
    for(i = 0; i < tokenc; i++)
      free(token_list[i]);
    
    // Handle redirection
    int handle, backup;
    if(out_redir == 1){
      fflush(stdout);
      backup = dup(1);
      handle = open(outfile_redir, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
      if ( handle != 0){
        dup2(handle, 1);
        close(handle);
      }

    }
    if (in_redir == 1){
      fflush(stdin);
      backup = dup(0);
      handle = open(infile_redir, O_RDONLY, S_IRUSR);
      if ( handle != 0){
        dup2(handle, 0);
        close(handle);
      }
    }

    // Perform command
    if(strcmp(argv[0],"exit") == 0){
      for(i = 0; i < argc; i++)
        free(argv[i]); // Clean up before exiting
      exit(0);
    }
    else if(strcmp(argv[0],"echo") == 0){
      for(i = 1; i < argc; i++) // Print input arguments
        printf("%s ", argv[i]);
      putc('\n', stdout); // New line for next prompt
    }
    else if(strcmp(argv[0],"cd") == 0 && argc > 1){
      if(chdir(argv[1]) != 0) // Only concerned with first argument
        printf("%s: No such file or directory.\n", argv[1]);
    }
    else { // External command execution
      // Check if the program exists
      bin_found = 0;
      char* argv0_copy = argv[0]; // Store this so that the memory can be freed
      struct stat buf;
      if(strchr(argv[0], '/') != NULL){
        // If / in the argument, assume a path supplied
        if(stat(argv[0], &buf) == 0){
          bin_found = 1;
          //Valid path; store in exec_path, trim argv[0] to base argument
          exec_path = malloc(strlen(argv[0])+1);
          strcpy(exec_path, argv[0]);
          argv[0] = strrchr(argv[0], '/')+1;
        }
      }
      else {
        // Search $PATH
        char* path = malloc(strlen(getenv("PATH"))+1);
        strcpy(path, getenv("PATH"));
        token = strtok(path, colon);
        while(token != NULL){
          // For each folder specified in PATH, build an exec_path to test
          // Two extra chars for null char and appended '/'
          exec_path = malloc(strlen(token)+strlen(argv[0])+2);
          strcpy(exec_path, token); // Copy current path
          strcat(exec_path, "/"); // Append /
          strcat(exec_path, argv[0]); // Append argument
          if (stat(exec_path, &buf) == 0){
            bin_found = 1;
            free(path);
            break;
          }
          free(exec_path);
          token = strtok(NULL, colon);
        }
      }

      if (bin_found != 0){
        // Proceed to execute
        pid_t pid = fork();
        if (pid < 0){
          fprintf(stderr, "\nFailed to fork!\n");
          return 1;
        }
        if (pid == 0){
          argv[++argc] = 0;
          execv(exec_path, argv);
          fprintf(stderr, "\Exec failed!\n");
          return 1;
        }
        waitpid(pid, NULL, 0);
        free(exec_path);
      }
      else {
        printf("%s: Command not found.\n", argv[0]);
      }
      argv[0] = argv0_copy; // Restore argv[0]
    }

    // If redirection occured, reset for next input
    if (out_redir == 1){
      out_redir = 0;
      fflush(stdout);
      dup2(backup, 1);
      close(backup);
    }
    if (in_redir == 1){
      in_redir = 0;
      fflush(stdin);
      dup2(backup, 0);
      close(backup);
    }

    // Clean up after execution
    for(i = 0; i < argc; i++){
        free(argv[i]);
        argv[i] = NULL;
    }
    fflush(stdout);
  }
  return 0;
}