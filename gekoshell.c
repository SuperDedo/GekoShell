#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>



#define GSL_RL_BUFSIZE 64
#define GSL_PL_BUFSIZE 32
#define GSL_RL_DELIM " \n\r\a\t"
#define GSL_PIPE_DELIM "|" 
#define GSL_OUTPUT_REDIR_DELIM ">"
#define GSL_INPUT_REDIR_DELIM "<"


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
    //reidrect[0] = input fd, redirect[1] = output, equels -1 for defult value
    int redirect[2];
    char *args[];
} command_struct;

typedef struct { 
    int num_of_commands;
    command_struct* commands[];
} pipe_struct;



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

    printf("Use the \"man\" command for information on other commands\n");
    return 1;
}

void close_all_pipes(int (*pipes)[2], int num_of_pipes){
    int i = 0;

    for(i = 0; i < num_of_pipes; i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

void gsl_exec_io(command_struct *command, int (*pipes)[2], int num_of_pipes){
    if(command->redirect[1] != -1){
            dup2(command->redirect[1], STDOUT_FILENO);
    } if(command->redirect[0] != -1){
            dup2(command->redirect[0], STDIN_FILENO);
    }
    close_all_pipes(pipes, num_of_pipes);
    execvp(command->progname, command->args);
    fprintf(stderr, "gsl: executing error\n");
    exit(EXIT_FAILURE);    
}

/*int gsl_launch(command_struct *command){
    pid_t cpid;
    int status;

    cpid = fork();
    if(cpid < 0){
        fprintf(stderr, "gsl: forking error\n");
    } else if(cpid == 0){
        gsl_exec_io(command);
    } else if(cpid > 0){
        do{
        waitpid(cpid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}*/


/* can't have any builtin functions
*/
int gsl_execute_pipe(pipe_struct *pipeline){
    int n = pipeline->num_of_commands;
    int i = 0;
    int fd;
    int (*pipes)[2] = malloc((n-1) * sizeof(int[2]));
    pid_t cpid;


    for(i = 0; i < n - 1; i++){
        pipe(pipes[i]);
        pipeline->commands[i]->redirect[1] = pipes[i][1];
        pipeline->commands[i+1]->redirect[0] = pipes[i][0];
    }

    for(i = 0; i < n ; i++){
        cpid = fork();
        if(cpid < 0){
            fprintf(stderr, "gsl: forking error\n");
        } else if(cpid == 0){
            gsl_exec_io(pipeline->commands[i], pipes, n-1);
        }
    }
    close_all_pipes(pipes, n-1);

    for(i = 0; i < n ; i++){
        wait(NULL);
    }
    if((fd=pipeline->commands[n-1]->redirect[1]) != -1){
        close(fd);
    }
    if((fd=pipeline->commands[0]->redirect[0]) != -1){
        close(fd);
    }
    return 1;
}



int gsl_execute(pipe_struct *pipeline){
    int i = 0;
    
    if(pipeline->num_of_commands == 1){
        for(i = 0; i < gsl_num_builtins(); i++){
            if(strcmp(pipeline->commands[0]->progname, builtin_str[i]) == 0){
                return (*builtin_func[i])(pipeline->commands[0]->args);
            }
        }
    }

    return gsl_execute_pipe(pipeline);
}

command_struct *gsl_parse_command(char *line){
    int bufsize = GSL_PL_BUFSIZE;
    int position = 0;
    char *token;
    command_struct *command = malloc(sizeof(command_struct)+ sizeof(char*) * bufsize);
    if(!command){
        fprintf(stderr, "gsl: allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strsep(&line, GSL_RL_DELIM);
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
        token = strsep(&line, GSL_RL_DELIM);

    }
    command->args[position] = NULL;
    command->progname = command->args[0];
    command->redirect[0] = -1;
    command->redirect[1] = -1;
    return command;
}

pipe_struct *gsl_parse_pipeline(char *line){
    int bufsize = GSL_PL_BUFSIZE;
    int position = 0;
    int fd;
    char *token;
    char *temp_line;
    char *command_line;
    char *output_file;
    char *input_file;
    pipe_struct *pipeline = malloc(sizeof(pipe_struct)+ sizeof(command_struct*) * bufsize);
    pipeline->num_of_commands = 0;
    if(!pipeline){
        fprintf(stderr, "gsl: allocation error\n");
        exit(EXIT_FAILURE);
    }
    //for redircting output to file 
    temp_line = strsep(&line, GSL_OUTPUT_REDIR_DELIM);
    output_file = strsep(&line, GSL_OUTPUT_REDIR_DELIM);

    //for redircting file to input
    command_line = strsep(&temp_line, GSL_INPUT_REDIR_DELIM);
    input_file = strsep(&temp_line, GSL_INPUT_REDIR_DELIM);

    token = strsep(&command_line, GSL_PIPE_DELIM);
    while(token != NULL){
        pipeline->commands[position] = gsl_parse_command(token);
        pipeline->num_of_commands = pipeline->num_of_commands+1;
        position++;

        if(bufsize <= position){
            bufsize += GSL_PL_BUFSIZE;
            pipeline = realloc(pipeline, sizeof(pipe_struct)+ sizeof(command_struct*) * bufsize);
                if(!pipeline){
                    fprintf(stderr, "gsl: allocation error\n");
                    exit(EXIT_FAILURE);
                }
        }        
        token = strsep(&command_line, GSL_PIPE_DELIM);
    }
    if(output_file != NULL){
        fd = open(output_file, O_CREAT|O_WRONLY, 0644);
        if(fd == -1){
            fprintf(stderr, "gsl: couldn't open %s\n", output_file);
        } else {
            pipeline->commands[pipeline->num_of_commands-1]->redirect[1] = fd;
        }
    }
    if(input_file != NULL){
        fd = open(input_file, O_RDONLY, 0644);
        if(fd == -1){
            fprintf(stderr, "gsl: couldn't open %s\n", input_file);
        }else {
            pipeline->commands[0]->redirect[0] = fd;
        }
    } 
    return pipeline;
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
    char* line, *copy_line;
    pipe_struct *pipeline;
    int status;
    int i = 0;

    do{
        printf(">");
        line = gsl_read_line();
        //needed because strsep changes line
        copy_line = line;
        pipeline = gsl_parse_pipeline(line);
        status = gsl_execute(pipeline);

        free(copy_line);
        for(i = 0; i<pipeline->num_of_commands; i++){
            free(pipeline->commands[i]);
        }
        free(pipeline);
    } while(status);
    
}



int main(int argc, char **argv){
    //load config files

    //command loop
    gsl_loop();

    //suhtdown

    return EXIT_SUCCESS;
}