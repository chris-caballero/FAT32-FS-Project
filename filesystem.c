#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include "FAT32.h"


// typedef struct {
//     unsigned short BPB_BytesPerSec;
//     unsigned char BPB_SecPerCluster;
//     unsigned short BPB_ReservedSecCnt;            // This holds the first FAT sector
//     unsigned char BPB_NumFATs;
//     unsigned int BPB_FATsize32;
//     unsigned int BPB_RootCluster;
//     unsigned int BPB_TotalSec32;
// } __attribute__((packed)) BPB_Info;

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


char* UserInput[5]; //Longest Command is 4 long, so this will give us just enough space for 4 args and End line
 

char* DynStrPushBack(char* dest, char c) //TODO: Move to a seperate file with other commonly used functions
{
    size_t total_len = strlen(dest) + 2;
    char* new_str = (char*)calloc(total_len, sizeof(char));
    strcpy(new_str, dest);
    new_str[total_len - 2] = c;
    new_str[total_len - 1] = '\0';
    free(dest);
    return new_str;
}

void GetUserInput(void) 
{
    char temp;
    int i = 0;
    while (i < 5) //Clear Array
    {
        if (UserInput[i] != NULL) {
            free(UserInput[i]);
        }
        UserInput[i] = (char*)calloc(1, sizeof(char));
        UserInput[i][0] = '\0';
        i++;
    }
    unsigned int userInputIndex = 0;
    do {
        temp = fgetc(stdin); //fgetc grabs 1 char at a time and casts it to an int.
        
        if (userInputIndex == 4)
        {
            if (temp != '\"')
            {
                printf("Error: Expected quotes around last argument");
            }
            
            temp = fgetc(stdin);
            while (temp != '\"') //if slash, append to the UserInput index, then keep grabbing
            {
                UserInput[userInputIndex] = DynStrPushBack(UserInput[userInputIndex], temp);
                temp = fgetc(stdin);
            }
            //If not end, grab keep grabbing more chars
            while (temp != '\n' && temp != '\0')
            {
                temp = fgetc(stdin);
            }
        }

        else
        {
            while (temp != ' ' && temp != '\n' && temp != '\0') //If not space, new line or end, Append.
            {
                UserInput[userInputIndex] = DynStrPushBack(UserInput[userInputIndex], temp);
                temp = fgetc(stdin);
            }
        }
        userInputIndex++;
    } while (temp != '\n' && temp != '\0' && userInputIndex < 5);

    int j = userInputIndex;
    while (j < 5) {
        free(UserInput[j]);
        UserInput[j] = calloc(10, sizeof(char));
        strcpy(UserInput[j], ". . . . .");
        j++;
    }
}

int open_file(char* filename, char* mode) {
    return 0;
}
int print_info(int offset);

int print_info(int offset) {

    printf("Bytes per Sector: %d\n", BPB.BPB_BytsPerSec);
	printf("Sectors per Cluster: %d\n", BPB.BPB_SecPerClus);
    printf("Reserved Sector Count: %d\n", BPB.BPB_RsvdSecCnt);
	printf("Number of FATs: %d\n", BPB.BPB_NumFATs);
    printf("Total Sectors: %d\n", BPB.BPB_TotSec32);  
    printf("FAT Size: %d\n", BPB.BPB_FATSz32);
    printf("Root Cluster: %d\n", BPB.BPB_RootClus);

    return 0;

}


void RunProgram()
{
    while (1) {
        printf("$ ");
        GetUserInput();

        command = UserInput[0];

        if (command == "exit") {
            //release resources 
            break;
        }
        else if (command == "info") {

        }
        else if (command == "size") {

        }
        else if (command == "ls") {

        }
        else if (command == "cd") {

        }
        else if (command == "creat") {

        }
        else if (command == "mkdir") {

        }
        else if (command == "mv") {

        }
        else if (command == "open") {
            if (openFile() == -1) {
                printf("Error");
            }

        }
        else if (command == "close") {

        }
        else if (command == "lseek") {

        }
        else if (command == "read") {

        }
        else if (command == "write") {

        }
        else if (command == "rm") {

        }
        else if (command == "cp") {

        }
        else if (command == "rmdir") {

        }

    }
}

int main() 
{
    char* filename = "./fat32.img";

    img_file = fopen(filename, "rb+");
    fread(&BPB, sizeof(BPB), 1, img_file);

    print_info(0);

    RunProgram();

    return 0;
}