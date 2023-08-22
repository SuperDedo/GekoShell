#include <stdlib.h>
#include <stdio.h>

void gsl_loop(void){
    char* line;
    char** args;
    int status;

    do{
        printf(">");
        line = gsl_read_line();
        args = gsl_parse_line(line);
        status = gsl_exe(args);

        free(line);
        free(args);

    } while (status);
    
}



int main(int argc, char **argv){
    //load config files

    //command loop
    gsl_loop();

    //suhtdown

    return EXIT_SUCCESS;
}