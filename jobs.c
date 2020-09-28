/* COMP 530: Tar Heel SHell
 *
 * This file implements functions related to launching
 * jobs and job control.
 */
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "thsh.h"
static char ** path_table;

/* Initialize the table of PATH prefixes.
 *
 * Split the result on the parenteses, and
 * remove any trailing '/' characters.
 * The last entry should be a NULL character.
 *
 * For instance, if one's PATH environment variable is:
 *  /bin:/sbin///
 *
 * Then path_table should be:
 *  path_table[0] = "/bin"
 *  path_table[1] = "/sbin"
 *  path_table[2] = '\0'
 *
 * Hint: take a look at getenv().  If you use getenv, do NOT
 *       modify the resulting string directly, but use
 *       malloc() or another function to allocate space and copy.
 *
 * Returns 0 on success, -errno on failure.
 */

//remove any trailing '/' characters.
void trimSlash(char * token) {
    for (int i = strlen(token) - 1; i >= 0; i--) {
        if (token[i] != '/') {
            token[i + 1] = '\0';
            break;
        }
    }
}

int init_path(void) {
    // get environment PATH variable
    char * s = getenv("PATH");
    if (s == NULL) {
        return errno;
    }
    // copy to another variable to avoid modifying the resulting string directly
    char * path = malloc(strlen(s) + 1);
    strcpy(path, s);

    // count the number of paths delimited by ':'
    int i, count;
    for (i = 0, count = 0; path[i]; i++)
        count += (path[i] == ':');

    // initialize path table
    path_table = malloc(sizeof( * path_table) * (count + 1));

    // use strtok to seperate string delimited by ':'
    char * token = strtok(path, ":");
    i = 0;
    while (token != NULL) {
        trimSlash(token);
        //put delimited entry in the path table
        path_table[i] = token;
        i++;
        token = strtok(NULL, ":");
    }
    // we could use this to assign the last entry of path table as "\0"
    // path_table[i]="/0"
    return 0;
}

/* Debug helper function that just prints
 * the path table out.
 */
void print_path_table() {
      if (path_table == NULL) {
        printf("XXXXXXX Path Table Not Initialized XXXXX\n");
        return;
      }
}

/* Given the command listed in args,
 * try to execute it.
 *
 * If the first argument starts with a '.'
 * or a '/', it is an absolute path and can
 * execute as-is.
 *
 * Otherwise, search each prefix in the path_table
 * in order to find the path to the binary.
 *
 * Then fork a child and pass the path and the additional arguments
 * to execve() in the child.  Wait for exeuction to complete
 * before returning.
 *
 * stdin is a file handle to be used for standard in.
 * stdout is a file handle to be used for standard out.
 *
 * If stdin and stdout are not 0 and 1, respectively, they will be
 * closed in the parent process before this function returns.
 *
 * wait, if true, indicates that the parent should wait on the child to finish
 *
 * Returns 0 on success, -errno on failure
 */
int run_command(char *args[MAX_ARGS], int stdin, int stdout, bool wait){
  /* Lab 1: Your code here */
  int rv = 0;
  char *env[] = { 0 };	/* leave the environment list null */
  //char *argv[] = {*args, 0};
  char *path;
  struct stat stats;
  int exec_status, stat_loc;
  bool canFind = false;

  // if args[0] != '.' or '/', then search for the binary in the path table
  if(args[0][0] != '.' && args[0][0] != '/'){
    for(int i = 0; path_table[i]; i++){
        char * binary = malloc(strlen(path_table[i]) + strlen(args[0]) + 1);
        strcpy(binary, path_table[i]);
        strcat(strcat(binary, "/"), args[0]);
        //find if binary is exists; if so, let path be binary, else, loop again
        if (stat(binary, &stats) == 0){
            path = binary;
            canFind = true;
        }else{
            free(binary);
        }  
    }
    if(!canFind){
        rv = -errno;
    }
  }else{
      path = args[0];
  }
  
  // create child to exec
  int pid = fork();
  if(pid == 0){
      exec_status = execve(path, args, env);
      rv = -errno;
      exit(exec_status);
  }else{
      waitpid(pid, &stat_loc, WUNTRACED);
  }
  
  return rv;
}
