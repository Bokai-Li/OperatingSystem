/* COMP 530: Tar Heel SHell
 *
 * This module implements command parsing, following the grammar
 * in the assignment handout.
 */
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "thsh.h"

/* This function returns one line from input_fd
 *
 * buf is populated with the contents, including the newline, and
 *      including a null terminator.  Must point to a buffer
 *      allocated by the caller, and may not be NULL.
 *
 * size is the size of *buf
 *
 * Return value: the length of the string (not counting the null terminator)
 *               zero indicates the end of the input file.
 *               a negative value indicates an error (e.g., -errno)
 */
int read_one_line(int input_fd, char *buf, size_t size) {
  int count, rv;
  // pointer to next place in cmd to store a character
  char *cursor;
  // the last character that was written into cmd
  char last_char;

  assert (buf);

  /*
   * We want to continue reading characters until:
   *   - read() fails (rv will become 0) OR
   *   - count == MAX_INPUT-1 (we have no buffer space left) OR
   *   - last_char was '\n'
   * so we continue the loop while:
   *   rv is nonzero AND count < MAX_INPUT - 1 AND last_char != '\n'
   *
   * On every iteration, we:
   *   - increment cursor to advance to the next char in the cmd buffer
   *   - increment count to indicate we've stored another char in cmd
   *     (this is done during the loop termination check with "++count")
   *
   * To make the termination check work as intended, the loop starts by:
   *   - setting rv = 1 (so it's initially nonzero)
   *   - setting count = 0 (we've read no characters yet)
   *   - setting cursor = cmd (cursor at start of cmd buffer)
   *   - setting last_char = 1 (so it's initially not '\n')
   *
   * In summary:
   *   - START:
   *      set rv = 1, count = 0, cursor = cmd, last_char = 1
   *   - CONTINUE WHILE:
   *      rv (is nonzero) && count < MAX_INPUT - 1 && last_char != '\n'
   *   - UPDATE PER ITERATION:
   *      increment count and cursor
   */
  for (rv = 1, count = 0, cursor = buf, last_char = 1;
       rv && (++count < (size - 1)) && (last_char != '\n'); cursor++) {

    // read one character
    // file descriptor 0 -> reading from stdin
    // writing this one character to cursor (current place in cmd buffer)
    rv = read(input_fd, cursor, 1);
    last_char = *cursor;
  }
  // null terminate cmd buffer (so that it will print correctly)
  *cursor = '\0';

  // Deal with an error from the read call
  if (!rv) {
    count = -errno;
  }

  return count;
}


/* Parse one line of input.
 *
 * This function should populate a two-dimensional array of commands
 * and tokens.  The array itself should be pre-allocated by the
 * caller.
 *
 * The first level of the array is each stage in a pipeline, at most MAX_PIPELINE long.
 * The second level of the array is each argument to a given command, at most MAX_ARGS entries.
 * In each command buffer, the entry after the last valid entry should be NULL.
 * After the last valid pipeline buffer, there should be one command entry with just a NULL.
 *
 * For instacne, a simple command like "cd" should parse as:
 *  commands[0] = ["cd", '\0']
 *  commands[1] = ['\0']
 *
 * The first "special" character to consider is the vertical bar, or "pipe" ('|').
 * This splits a single line into multiple sub-commands that form the pipeline.
 * We will implement pipelines in lab 1, but for now, just use this character to delimit
 * commands.
 *
 * For instance, the command: "ls | grep foo\n" should be broken into:
 *
 * commands[0] = ["ls", '\0']
 * commands[1] = ["grep", "foo", '\0']
 * commands[2] = ['\0']
 *
 * Hint: Make sure to remove the newline at the end
 *
 * Hint: Make sure the implementation is robust to extra whitespace, like: "grep      foo"
 *       should still be parsed as:
 *
 * commands[0] = ["grep", "foo", '\0']
 * commands[1] = ['\0']
 *
 * This function should ignore anything after the '#' character, as
 * this is a comment.
 *
 * Finally, the command should identify file redirection characters ('<' and '>').
 * The string right after these tokens should be returned using the special output
 * parameters "infile" and "outfile".  You can assume there is at most one '<' and one
 * '>' character in the entire inbuf.
 *
 * For example, in input: "ls > out.txt", the return should be:
 *   commands[0] = ["ls", '\0']
 *   commands[1] = ['\0']
 *   outfile = "out.txt"
 *
 * Hint: Be sure your implementation is robust to arbitrary (or no) space before or after
 *       the '<' or '>' characters.  For instance, "ls>out.txt" and "ls      >      out.txt"
 *       are both syntactically valid, and should parse identically to "ls > out.txt".
 *       Similarly, "ls|grep foo" is also syntactically valid.
 *
 * You do not need to handle redirection of other handles (e.g., "foo 2>&1 out.txt").
 *
 * inbuf: a NULL-terminated buffer of input.
 *        This buffer may be changed by the function
 *        (e.g., changing some characters to \0).
 *
 * length: the length of the string in inbuf.  Should be
 *         less than the size of inbuf.
 *
 * commands: a two-dimensional array of character pointers, allocated by the caller, which
 *           this function populates.
 *
 *
 * return value: Number of entries populated in commands (1+, not counting the NULL),
 *               or -errno on failure
 */
void rmspace(char* s){
    int i, k;
    for(i = k = 0; s[i]; ++i){
        if(!isspace(s[i]) || (i > 0 && !isspace(s[i-1]))){
            s[k++] = s[i];
        }
    }
    s[k] = '\0';
}

int parse_line (char *inbuf, size_t length,
		char *commands [MAX_PIPELINE][MAX_ARGS],
		char **infile, char **outfile) {

  // Lab 0: Your code here
    int i, k;
    int row = 0;
    int col;
    char **pipeline_table;
    char **in_table;
    char **out_table;
    char *hasin;
    char *hasout;

    for (i = 0; i < MAX_PIPELINE; i++) {
        for (k = 0; k < MAX_ARGS; k++) {
            commands[i][k] = '\0';
        }
    }
    // make a copy of inbuf
    char *com = strdup(inbuf ? inbuf : "");

    if(com == NULL){
      return -errno;
    }

    i = 0;
    while(com[i] == ' '){
      i++;
    }
    if(com[i] == '#'){
      return 1;
    }
    if(com[i] == '\n'){
      return 1;
    }

    // remove '\n'
    com[strlen(com)-1] = '\0';

    // remove comments
    i = 0;
    while(com[i] != '#' && com[i]){
        i++;
    }
    com[i] = '\0';

    // count how many rows should path_table have i.e. no. of colon + 1
    for (i = 0; i < strlen(com); i++){
        if (com[i] == '|'){
            row++;
        }
    }
    row += 1;

    pipeline_table = malloc((row) * sizeof(*pipeline_table));
    in_table = malloc(2 * sizeof(*in_table));
    out_table = malloc(2 * sizeof(*out_table));

    // split pipelines
    char *pipeline = strtok(com, "|");
    i = 0;
    while( pipeline != NULL ) {
        rmspace(pipeline);                
        pipeline_table[i] = pipeline;
        pipeline = strtok(NULL, "|");
        i++;
    }

    // split tokens
    char *token;
    char *inout;
    for(i = 0; i < row; i++){
        hasin = strchr (pipeline_table[i], '<');
        hasout = strchr (pipeline_table[i], '>');

        if(hasin){
            inout = strtok(pipeline_table[i], "<");
            k = 0;
            // base case: no special character
            while( inout != NULL ) {
                rmspace(inout);
                in_table[k] = inout;
                inout = strtok(NULL, "<");
                k++;
            }   
            int in_len = strlen(in_table[1]);
            if(in_table[1][in_len - 1] == ' '){
               in_table[1][in_len - 1] = '\0'; 
            }
            *infile = in_table[1];
            pipeline_table[i] = in_table[0];
        }

        if(hasout){
            inout = strtok(pipeline_table[i], ">");
            k = 0;
            // base case: no special character
            while( inout != NULL ) {
                rmspace(inout);
                out_table[k] = inout;
                inout = strtok(NULL, ">");
                k++;
            }
            int out_len = strlen(out_table[1]);
            if(out_table[1][out_len - 1] == ' '){
              out_table[1][out_len - 1] = '\0'; 
            } 
            *outfile = out_table[1];
            pipeline_table[i] = out_table[0];
        }

        // base case
        token = strtok(pipeline_table[i], " ");
        col = 0;
        while( token != NULL ) {
            commands[i][col] = token;
            token = strtok(NULL, " ");
            col++;
        }
        commands[i][col] = '\0';
    }

  return row;
}