#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>


typedef struct {
    unsigned short BPB_BytesPerSec;
    unsigned char BPB_SecPerCluster;
    unsigned short BPB_ReservedSecCnt;            // This holds the first FAT sector
    unsigned char BPB_NumFATs;
    unsigned int BPB_FATsize32;
    unsigned int BPB_RootCluster;
    unsigned int BPB_TotalSec32;
} __attribute__((packed)) BPB_Info;

struct DIRENTRY {
    unsigned char DIR_Name[11];             // offset: 0
    unsigned char DIR_Attributes;           // offset: 11
    unsigned char DIR_NTRes;                // offset: 12
    unsigned char DIR_CreatTimeTenth;       // offset: 13
    unsigned short DIR_CreatTime;           // offset: 14 
    unsigned short DIR_CreatDate;           // offset: 16
    unsigned short DIR_LastAccessDate;      // offset: 18
    unsigned short DIR_FirstClusterHI;      // offset: 20
    unsigned short DIR_WriteTime;           // offset: 22
    unsigned short DIR_WriteDate;           // offset: 24
    unsigned short DIR_FirstClusterLO;      // offset: 26
    unsigned short DIR_FileSize;            // offset: 28
} __attribute__((packed));


FILE *  img_file;
BPB_Info BPB;
// FSInfo FSI;
// Environment ENV;


char *get_input(void);


int open_file(char* filename, char* mode) {
    return 0;
}
int print_info(int offset);

int main () {


    char *command; 
    char *filename = "./fat32.img";

    img_file = fopen(filename, "rb+");

    print_info(0);


    /*
    while(1) {
        printf("$ ");
        command = get_input();

        if(command == "exit") {
            //release resources 
            break;
        } else if(command == "info") {

        } else if(command == "size") {

        } else if(command == "ls") {

        } else if(command == "cd") {

        } else if(command == "creat") {

        } else if(command == "mkdir") {

        } else if(command == "mv") {

        } else if(command == "open") {
            if(openFile() == -1) {
                printf("Error");
            }

        } else if(command == "close") {

        } else if(command == "lseek") {

        } else if(command == "read") {

        } else if(command == "write") {

        } else if(command == "rm") {

        } else if(command == "cp") {

        } else if(command == "rmdir") {

        }

    }
    */
    
    return 0;
}

int print_info(int offset) {

    fseek(img_file, 11, SEEK_SET);
    fread(&BPB.BPB_BytesPerSec, 2, 1, img_file);
    fread(&BPB.BPB_SecPerCluster, 1, 1, img_file);
    fread(&BPB.BPB_ReservedSecCnt, 2, 1, img_file);
    fread(&BPB.BPB_NumFATs, 1, 1, img_file);

    fseek(img_file, 32, SEEK_SET);
    fread(&BPB.BPB_TotalSec32, 4, 1, img_file);
    fread(&BPB.BPB_FATsize32, 4, 1, img_file);
    
    fseek(img_file, 4, SEEK_CUR);
    fread(&BPB.BPB_RootCluster, 4, 1, img_file);



    printf("Bytes per Sector: %d\n", BPB.BPB_BytesPerSec);
	printf("Sectors per Cluster: %d\n", BPB.BPB_SecPerCluster);
    printf("Reserved Sector Count: %d\n", BPB.BPB_ReservedSecCnt);
	printf("Number of FATs: %d\n", BPB.BPB_NumFATs);
    printf("Total Sectors: %d\n", BPB.BPB_TotalSec32);  
    printf("FAT Size: %d\n", BPB.BPB_FATsize32);
    printf("Root Cluster: %d\n", BPB.BPB_RootCluster);

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