#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "thsh.h"

void rmspace(char* s){
    int i, k;
    for(i = k = 0; s[i]; ++i){
        if(!isspace(s[i]) || (i > 0 && !isspace(s[i-1]))){
            s[k++] = s[i];
        }
    }
    s[k] = '\0';
}

int main(){
    char *inbuf = "cat>outfile.txt\n"; 
    size_t length = strlen(inbuf) + 1;

	char *commands [MAX_PIPELINE][MAX_ARGS];
	char *infile = NULL;
    char *outfile = NULL;

    // Lab 0: Your code here
    int i, k;
    int row = 0;
    int col;
    char **pipeline_table;
    char **in_table;
    char **out_table;
    char *hasin;
    char *hasout;

    // make a copy of inbuf
    char *com = strdup(inbuf ? inbuf : "");
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
            int in_len = strlen(in_table[0]);
            if(in_table[0][in_len - 1] == ' '){
                in_table[0][in_len - 1] = '\0'; 
            }
            infile = in_table[0];
            pipeline_table[i] = in_table[1];
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
            outfile = out_table[1];
            pipeline_table[i] = out_table[0];
        }

        // base case
        token = strtok(pipeline_table[i], " ");
        col = 0;
        // base case: no special character
        while( token != NULL ) {
            commands[i][col] = token;
            token = strtok(NULL, " ");
            col++;
        }
        commands[i][col] = '\0';
    }

    // test 
    for(i = 0; i < row; i++){
        for(k = 0; k < col; k++)
        printf("%s\n", commands[i][k]);   
    }

    printf("%s()end\n", infile);
    printf("%s()end\n", outfile);
    return row;
}