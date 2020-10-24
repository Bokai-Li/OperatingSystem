/* COMP 530: Tar Heel SHell */

#include "thsh.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv, char **envp) {
  int input_fd = 0;
  //check if first argv is a file, if so change input file descriptor to point that file
  if(*(argv + 1) != NULL){
  	if( access( *(argv+1), F_OK ) != -1 ) {
  		input_fd=open(*(argv+1),O_RDONLY);
	}
  }
  // flag that the program should end 
  bool finished = 0;
  int ret = 0;

  // Lab 1:
  // Add support for parsing the -d option from the command line
  // and handling the case where a script is passed as input to your shell

  // Lab 1: Your code here

  ret = init_cwd();
  if (ret) {
    printf("Error initializing the current working directory: %d\n", ret);
    return ret;
  }

  ret = init_path();
  if (ret) {
    printf("Error initializing the path table: %d\n", ret);
    return ret;
  }

  while (!finished) {

    int length;
    // Buffer to hold input
    char cmd[MAX_INPUT];
    // Get a pointer to cmd that type-checks with char *
    char *buf = &cmd[0];
    char *parsed_commands[MAX_PIPELINE][MAX_ARGS];
    char *infile = NULL;
    char *outfile = NULL;
    int pipeline_steps = 0;

    if (!input_fd) {
      ret = print_prompt();
      if (ret <= 0) {
      // if we printed 0 bytes, this call failed and the program
      // should end -- this will likely never occur.
        finished = true;
        break;
      }
    }

    // Read a line of input
    length = read_one_line(input_fd, buf, MAX_INPUT);           
    if (length <= 0) {
      ret = length;
      break;
    }

    // Pass it to the parser
    pipeline_steps = parse_line(buf, length, parsed_commands, &infile, &outfile);
    if (pipeline_steps <= 0) {
      printf("Parsing error.  Cannot execute command. %d\n", -pipeline_steps);
      continue;
    }
    // Just echo the command line for now
    // file descriptor 1 -> writing to stdout
    // print the whole cmd string (write number of
    // chars/bytes equal to the length of cmd, or MAX_INPUT,
    // whichever is less)
    //
    // Comment this line once you implement
    // command handling
    // dprintf(1, "%s\n", cmd);

    // In Lab 1, you will need to add code to actually run the commands,
    // add debug printing, and handle redirection and pipelines, as
    // explained in the handout.
    //
    // For now, ret will be set to zero; once you implement command handling,
    // ret should be set to the return from the command.
    ret = 0;
    int *retval = malloc(sizeof(int));

    // if empty command or just some space, do nothing; else, do the following
    if(parsed_commands[0][0] != NULL){
      //check if it is a file
      if( access(parsed_commands[0][0], F_OK ) != -1 ) {
          //if it is not a executable, try shell script
	  if(access(parsed_commands[0][0], X_OK) ==-1){
	     //change file descriptor to that shell script file	  
	     input_fd=open(parsed_commands[0][0],O_RDONLY);
             continue;
	  }
      }
      // debugging info at the beginning of the running process
      if(*(argv + 1) != NULL && strcmp(*(argv + 1), "-d") == 0){
        fprintf(stderr, "RUNNING: [%s]\n", parsed_commands[0][0]);
      }

      /****** if no pipeline ******/
      if(pipeline_steps <= 1){
        // check if builtin
        int ifBuiltin = handle_builtin(parsed_commands[0], 0, 1, retval);   
        
        // if not builtin, check if there is any redirection
        if(ifBuiltin == 0){
          /* input & output*/
          if(infile && outfile){
            int input = open(infile, O_RDONLY);
            int output = open(outfile, O_RDWR| O_CREAT, 0777);
            ret = run_command(parsed_commands[0], input, output, 0);
          /* input */
          }else if(infile){       
            int input = open(infile, O_RDONLY);
            ret = run_command(parsed_commands[0], input, 1, 0);
          /* output */
          }else if(outfile){ 
              int output = open(outfile, O_RDWR| O_CREAT, 0777);
              ret = run_command(parsed_commands[0], 0, output, 0);
          /* regular: no input and no output */
          }else{
            ret = run_command(parsed_commands[0], 0, 1, 0);
          }
        }
      }else{
        /***** if there is pipeline *****/
        int k;
        int in, pipe_fd[2];

        in = 0;

        // if the first command include <
        if(infile){
          in = open(infile, O_RDONLY);
        }

        // for commands except the last one
        for (k = 0; k < pipeline_steps - 1; ++k){
          pipe (pipe_fd);   // create the pipe
          ret = run_command(parsed_commands[k], in, pipe_fd[1], 0); 
          close(pipe_fd[1]); // no longer need
          in = pipe_fd[0]; // the next child will read from there. 
          
        }

        // if the last command include >
        if(outfile){
          int output = open(outfile, O_RDWR| O_CREAT, 0777);
          ret = run_command(parsed_commands[k], in, output, 0);
        }else{
          ret = run_command(parsed_commands[k], in, 1, 0);
        }
       
      } // end of pipeline
      // debugging info after running
      if(*(argv + 1) != NULL && strcmp(*(argv + 1), "-d") == 0){
        fprintf(stderr, "ENDED: [%s] (ret=%d)\n", parsed_commands[0][0], ret);
      }
      
    }

    // Do NOT change this if/printf - it is used by the autograder.
    if (ret) {
      printf("Failed to run command - error %d\n", ret);
    }

  }

  return ret;
}

// test: ls | grep .txt > out.txt
