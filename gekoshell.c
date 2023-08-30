#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define GSL_RL_BUFSIZE 64
#define GSL_PL_BUFSIZE 32
#define GSL_RL_DELIM " \n\r\a\t"

//declaring builtin functions
int gsl_cd(char **args);
int gsl_exit(char **args);
int gsl_help(char **args);

char *builtin_str[]={
    "cd",
    "exit",
    "help"
};

int (*builtin_func[])(char **) = {
    &gsl_cd,
    &gsl_exit,
    &gsl_help
};

typedef struct{
    char *progname;
    char *args[];
} command_struct;


int gsl_num_builtins(){
    return sizeof(builtin_str)/sizeof(char *);
}

int gsl_cd(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "gsl: expected 1 argument fo \"cd\"\n");
    }else{
        if(chdir(args[1])!=0){
            perror("gsl");
        }
    }
    return 1;
}

int gsl_exit(char **args){
    return 0;
}

int gsl_help(char **args){
    int i;
    printf("Geko's Shell\n");
    printf("You can enter program and its arguments for it to run\n");
    printf("or you can use one of the follwing builtin functions: \n");
    for (i = 0; i < gsl_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the \"man\" for information on other commands\n");
    return 1;
}

int gsl_launch(command_struct *command){
    pid_t cpid;
    int status;

    cpid = fork();
    if(cpid < 0){
        fprintf(stderr, "gsl: forking error\n");
    } else if(cpid == 0){
        execvp(command->progname, command->args);
        fprintf(stderr, "gsl: executing error\n");
        exit(EXIT_FAILURE);
    } else if(cpid > 0){
        do{
        waitpid(cpid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int gsl_execute(command_struct *command){
    int i = 0;
    
    if(command->progname==NULL){
        return 1;
    }

    for(i = 0; i < gsl_num_builtins(); i++){
        if(strcmp(command->progname, builtin_str[i]) == 0){
            return (*builtin_func[i])(command->args);
        }
    }

    return gsl_launch(command);
}

command_struct *gsl_parse_line(char *line){
    int bufsize = GSL_PL_BUFSIZE;
    int position = 0;
    char *token;
    command_struct *command = malloc(sizeof(command_struct)+ sizeof(char*) * bufsize);
    if(!command){
        fprintf(stderr, "gsl: allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, GSL_RL_DELIM);
    while(token != NULL){
        command->args[position] = token;
        position++;

        if(bufsize <= position){
            bufsize += GSL_PL_BUFSIZE;
            command = realloc(command, sizeof(command_struct)+ sizeof(char*) * bufsize);
                if(!command){
                    fprintf(stderr, "gsl: allocation error\n");
                    exit(EXIT_FAILURE);
                }
        }        
        token = strtok(NULL, GSL_RL_DELIM);

    }
    command->args[position] = NULL;
    command->progname = command->args[0];
    return command;
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
    command_struct *command;
    int status;


    do{
        printf(">");
        line = gsl_read_line();
        command = gsl_parse_line(line);
        status = gsl_execute(command);

        free(line);
        free(command);

    } while(status);
    
}



int main(int argc, char **argv){
    //load config files

    //command loop
    gsl_loop();

    //suhtdown

    return EXIT_SUCCESS;
}