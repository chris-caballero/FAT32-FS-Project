#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>


char *get_input(void);

int main () {


    char *command; 
    while(1) {
        printf("$ ");
        command = get_input();

        if(command == "exit") {
            break;
        } else if(command == "info") {

        } else if(command == "size") {

        } else if(command == "ls") {

        } else if(command == "cd") {

        } else if(command == "creat") {

        } else if(command == "mkdir") {

        } else if(command == "mv") {

        } else if(command == "open") {

        } else if(command == "close") {

        } else if(command == "lseek") {

        } else if(command == "read") {

        } else if(command == "write") {

        } else if(command == "rm") {

        } else if(command == "cp") {

        } else if(command == "rmdir") {

        }

    }
    
    return 0;
}

char *get_input(void) {
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}