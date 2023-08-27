#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define GSL_RL_BUFSIZE 64
#define GSL_PL_BUFSIZE 32
#define GSL_RL_DELIM " \n\r\a\t"


int gsl_launch(char **args){
    pid_t cpid;
    int status;

    cpid = fork();
    if(cpid < 0){
        fprintf(stderr, "gsl: forking error\n");
    } else if(cpid == 0){
        execvp(args[0], args);
        fprintf(stderr, "gsl: executing error\n");
        exit(EXIT_FAILURE);
    } else if(cpid > 0){
        do{
        waitpid(cpid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

char **gsl_parse_line(char *line){
    int bufsize = GSL_PL_BUFSIZE;
    int position = 0;
    char **args = malloc(sizeof(char*) * bufsize);
    char *token;
    if(!args){
        fprintf(stderr, "gsl: allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, GSL_RL_DELIM);
    while(token != NULL){
        args[position] = token;
        position++;

        if(bufsize <= position){
            bufsize += GSL_PL_BUFSIZE;
            args  = realloc(args, sizeof(char*) * bufsize);
                if(!args){
                    fprintf(stderr, "gsl: allocation error\n");
                    exit(EXIT_FAILURE);
                }
        }        
        token = strtok(NULL, GSL_RL_DELIM);

    }
    args[position] = NULL;
    return args;
}

char *gsl_read_line(){
    int bufsize = GSL_RL_BUFSIZE;
    int positon = 0;
    char *buffer = malloc(bufsize * sizeof(char));
    int c;

    if(!buffer){
        fprintf(stderr, "gsl: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        c = getchar();
        if(c == '\n' || c == EOF){
            buffer[positon] = '\0';
            return buffer;
        }else{
            buffer[positon] = c;
        }
        positon++;
        if(positon>=bufsize){
            bufsize += GSL_RL_BUFSIZE;
            buffer = realloc(buffer,sizeof(char)*bufsize);
            if(!buffer){
                fprintf(stderr, "gsl: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

}


void gsl_loop(void){
    char* line;
    char** args;
    int status;


    do{
        printf(">");
        line = gsl_read_line();
        args = gsl_parse_line(line);
        //status = gsl_exe(args);
        status = 0;

        free(line);
        free(args);

    } while(status);
    
}



int main(int argc, char **argv){
    //load config files

    //command loop
    gsl_loop();

    //suhtdown

    return EXIT_SUCCESS;
}