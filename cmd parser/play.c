#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main () {
    char * infile = NULL;
    char * outfile = NULL;
    char * commands [100][100];
    char * inbuf="infile.txt  <   c1 i1   | c2 i2 |c3 i3  >     outfile.txt\n";
    int length = strlen(inbuf);
    
    free(commands);
    char * pointer;
    pointer=malloc(length);
    memcpy(pointer, inbuf, length);
    while(*pointer==' '){
        pointer++;
    }
    char * temp = pointer;
    while(*temp!='\n'){
        if(*temp=='<'){
            *temp='\0';
            infile=pointer;
            temp++;
            while(*temp==' ')
                temp++;
            break;
        }
        temp++;
    }
    if(*temp!='\n'){
        while(pointer<temp){
            if(*pointer==' '){
                *pointer='\0';
            }
            pointer++;
        }
    }
    int comIndex = 0;
    int inputIndex = 0;
    commands[0][0]=pointer;
    while(*pointer!='\n'){
        if(*pointer=='|'){
            *pointer='\0';
            pointer++;
            inputIndex = 0;
            while(*pointer==' ')
                pointer++;
            comIndex++;
            commands[comIndex][inputIndex]=pointer;
        }else if(*pointer=='>'){
            break;
        }else if(*pointer==' '){
            *pointer='\0';
             pointer++;
            while(*pointer==' ')
                pointer++;
            if(*pointer=='>'){
                break;
            }
            if(*pointer!='|'){
                inputIndex++;
                commands[comIndex][inputIndex]=pointer;
            }else{
                pointer--;
            }
        }
        pointer++;
    }

    if(*pointer=='>'){
        *pointer='\0';
        pointer++;
        while(*pointer==' ')
            pointer++;
        outfile=pointer;
        while(*pointer!='\n')
            pointer++;
    }
    *pointer='\0';
//        char * token;
//        bool hasin = false;
//        bool hasout = false;
//        for(int i=0; i<length;i++){
//            if(inbuf[i]=='\n'){
//                inbuf[i]='\0';
//            }else if(inbuf[i]=='<'){
//                hasin = true;
//            }else if(inbuf[i]=='>'){
//                hasout = true;
//            }
//        }
//        if(hasin){
//            token = strtok(inbuf, "<");
//            infile[0] = token;
//        }
//        if(hasout){
//            token = strtok(inbuf, ">");
//            token = strtok(NULL, ">");
//            outfile[0] = token;
//        }
//
//        token = strtok(inbuf, "|");
//        int i = 0;
//        while( token != NULL ) {
//            commands[i][0]=token;
//            i++;
//            token = strtok(NULL, "|");
//        }
    //    i = 0;
    //    int j = 0;
    //    while(commands[i]){
    //        char * token = strtok(commands[i][0], " ");
    //        while( token != NULL ) {
    //            commands[i][j]=token;
    //            j++;
    //            token = strtok(NULL, " ");
    //        }
    //        i++;
    //    }
    for (int i = 0; commands[i][0]; i++) {
      printf("Pipeline Stage %d: ", i);
      for (int j = 0; commands[i][j]; j++) {
    printf("[%s] ", commands[i][j]);
      }
      printf("\n");
    }
    if (infile) {
      printf("Input redirection to file [%s]\n", infile);
    }
    if (outfile) {
      printf("Output redirection to file [%s]\n", outfile);
    }
    return 1;
}
