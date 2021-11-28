#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include "FAT32.h"

typedef struct {
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
    unsigned int DIR_FileSize;            // offset: 28
} __attribute__((packed)) DIRENTRY;

typedef struct
{
    char name[32];
    char mode[8];
    struct FileTable* next;
} __attribute__((packed)) FILETABLE;

typedef struct {
    unsigned int current_cluster;
    DIRENTRY current_dir;
} __attribute__((packed)) ENV_Info;



FILE *  img_file;
BPB_Info BPB;
ENV_Info ENV;

int FirstDataSector;
int FirstFATSector;
// FSInfo FSI;
unsigned char ATTR_READ_ONLY = 0x01;
unsigned char ATTR_HIDDEN = 0x02;
unsigned char ATTR_SYSTEM = 0x04;
unsigned char ATTR_VOLUME_ID = 0x08;
unsigned char ATTR_DIRECTORY = 0x10;
unsigned char ATTR_ARCHIVE = 0x20;

unsigned char ATTR_LONG_NAME;



char* UserInput[5]; //Longest Command is 4 long, so this will give us just enough space for 4 args and End line
struct FileTable* root = NULL;
 
//Shell Commands
void RunProgram(void);
char* DynStrPushBack(char* dest, char c);
void GetUserInput(void);

//Builtins
int loadBPB(const char *filename);
void print_info(void);
void ls(char *dirname);
int file_size(char* filename);
void cd(char *dirname);
int open_file(char* filename, char* mode);
int close_file(char* filename);
// int create_file(char *filename);
// int create_directory(char *dirname);

//Cluster Management
int get_first_sector(int cluster_num);
int get_next_cluster(int curr_cluster);
int is_last_cluster(int cluster);

//FileTable Management
void FTAdd(const char* fileName, const char* mode);
void FTRemove(const char* fileName;
bool FTIsOpen(const char* fileName);
void FTCleanup();

//Helper functions
int find_dirname_cluster(char *dirname);
DIRENTRY new_file(char *filename);
DIRENTRY new_directory(char *dirname);
int find_and_allocate_empty_cluster(int last_cluster);
int allocate_cluster(int free_cluster, char *filename);
int find_last_cluster(void);
void create_standard_directories(int curr_cluster);
void format(char * name);


int main(int argc, const char* argv[]) {
    if(argc != 2) {
        printf("Error, incorrect number of arguments\n");
        return 0;
    }

    const char* filename = argv[1];
    loadBPB(filename);

    FirstDataSector = BPB.BPB_RsvdSecCnt + BPB.BPB_NumFATs * BPB.BPB_FATSz32;
    FirstFATSector = BPB.BPB_RsvdSecCnt;

    ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID;

    ENV.current_cluster = BPB.BPB_RootClus;

    RunProgram();

    return 0;
}

void RunProgram(void) {
    while (1) {
        printf("$ ");
        GetUserInput();

        char *command = UserInput[0];

        if (!strcmp(command, "exit")) {
            //release resources 
            FTCleanup();
            break;
        }
        else if (!strcmp(command, "info")) {
            print_info();
        }
        else if (!strcmp(command, "size")) {
            char *filename = UserInput[1];
            int size = file_size(filename);
            if(size >= 0) {
                printf("The size of the file is %d bytes\n", size);
            }
            //printf("The size of the file is %d bytes\n", get_file_size(filename));
        }   
        else if (!strcmp(command, "ls")) {
            char *dirname = UserInput[1];
            ls(dirname);  
        }
        else if (!strcmp(command, "cd")) {
            char *dirname = UserInput[1];
            cd(dirname);
        }
        else if (!strcmp(command, "creat")) {
            // char *filename = UserInput[1];
            // create_file(filename);
        }
        else if (!strcmp(command, "mkdir")) {
            // char *dirname = UserInput[1];
            // create_directory(dirname);
        }
        else if (!strcmp(command, "mv")) {

        }
        else if (!strcmp(command, "open")) {
            // if (openFile() == -1) {
            //     printf("Error");
            // }

        }
        else if (!strcmp(command, "close")) {

        }
        else if (!strcmp(command, "lseek")) {

        }
        else if (!strcmp(command, "read")) {

        }
        else if (!strcmp(command, "write")) {

        }
        else if (!strcmp(command, "rm")) {

        }
        else if (!strcmp(command, "cp")) {

        }
        else if (!strcmp(command, "rmdir")) {

        }

    }
}

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
        strcpy(UserInput[j], "");
        j++;
    }
}

void print_info(void) {
    printf("Bytes per Sector: %d\n", BPB.BPB_BytsPerSec);
	printf("Sectors per Cluster: %d\n", BPB.BPB_SecPerClus);
    printf("Reserved Sector Count: %d\n", BPB.BPB_RsvdSecCnt);
	printf("Number of FATs: %d\n", BPB.BPB_NumFATs);
    printf("Total Sectors: %d\n", BPB.BPB_TotSec32);  
    printf("FAT Size: %d\n", BPB.BPB_FATSz32);
    printf("Root Cluster: %d\n", BPB.BPB_RootClus);
}

int loadBPB(const char *filename) {
    img_file = fopen(filename, "rb+");
    fread(&BPB, sizeof(BPB), 1, img_file);
    return 0;
}

int is_last_cluster(int cluster) {
    if(cluster == 0xFFFFFFFF || cluster == 0x0FFFFFF8 || cluster == 0x0FFFFFFE) {
        return 1;
    }
    return 0;
}

int get_next_cluster(int curr_cluster) {
    int next_cluster = FirstFATSector * BPB.BPB_BytsPerSec + curr_cluster * 4;

    fseek(img_file, next_cluster, SEEK_SET);
    fread(&next_cluster, 4, 1, img_file);

    return next_cluster;
}

void ls(char *dirname) {
    DIRENTRY curr_dir;
    int i, offset;
    int curr_cluster;

    if(strcmp(dirname, "") == 0) {
        curr_cluster = ENV.current_cluster;
    } else if((curr_cluster = find_dirname_cluster(dirname)) == -1) {
        return;
    }

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                //printf("...%d, %d...", curr_dir.DIR_FirstClusterHI,curr_dir.DIR_FirstClusterLO);
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] == ' ') {
                        break;
                    }
                    printf("%c", curr_dir.DIR_Name[j]);
                }
                printf(" ");
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
        if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
            printf("\n");
        }
    }
    printf("\n");
}

void cd(char *dirname) {
    int new_cluster;
    if((new_cluster = find_dirname_cluster(dirname)) == -1) {
        return;
    } else if(new_cluster == 0) {
        new_cluster = ENV.current_cluster;
    }
    ENV.current_cluster = new_cluster;
    
    int offset = get_first_sector(ENV.current_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fread(&ENV.current_dir, sizeof(ENV.current_dir), 1, img_file);
}

DIRENTRY new_file(char *filename) {
    DIRENTRY new_dir;

    for(int i = 0; i < 8; i++) {
        new_dir.DIR_Name[i] = toupper(filename[i]);
    } 
    for(int i = 8; i < 11; i++) {
        new_dir.DIR_Name[i] = ' ';
    }
    new_dir.DIR_Attributes = 0;
    new_dir.DIR_NTRes = 0;
    new_dir.DIR_FileSize = 0;
    new_dir.DIR_FirstClusterHI = 0;
    new_dir.DIR_FirstClusterLO = 0;

    return new_dir;
}

DIRENTRY new_directory(char *dirname) {
    DIRENTRY new_dir = new_file(dirname);

    new_dir.DIR_Attributes = ATTR_DIRECTORY;

    return new_dir;
}

int find_dirname_cluster(char * dirname) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    format(dirname);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if((curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                //printf("{[%s], [%s]}", (char *) curr_dir.DIR_Name, dirname);
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != dirname[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return curr_dir.DIR_FirstClusterHI * 0x100 + curr_dir.DIR_FirstClusterLO;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: directory %s not found\n", dirname);
    return -1;
}

int file_size(char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                if(!(curr_dir.DIR_Attributes & ATTR_DIRECTORY) && strstr((char *) curr_dir.DIR_Name, filename) != NULL) {
                    //printf("Size of file %s is %d bytes", filename, curr_dir.DIR_FileSize);
                    return curr_dir.DIR_FileSize;
                } else if(strstr((char *) curr_dir.DIR_Name, filename) != NULL) {
                    printf("Error: %s is a directory\n", filename);
                    return -1;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", filename);
    return -1;
}

int get_first_sector(int cluster_num) {
    int offset = (cluster_num - 2) * BPB.BPB_SecPerClus;
    return FirstDataSector + offset;
}

void format(char * name) {
    int i;
    for(i = 0; i < strlen(name); i++) {
        name[i] = toupper(name[i]);
    } 
    for(; i < 11; i++) {
        name[i] = ' ';
    }
}

int open_file(char* filename, char* mode) {

   //Mode Check
    if (strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0 &&
        strcmp(mode, "rw") != 0 && strcmp(mode, "wr") != 0)
    {
        printf("ERROR: Invalid mode, options are: r w rw wr\n");
    }
    //Check if File exists
    else if (FTIsOpen(filename))
    {
        printf("ERROR: File already Open\n");
    }

    else
    {
    //Get Directory Contents

    //Loop thru Directory Contents

    //If match, Add to table

    }

    //If no match, Error out.
    printf("ERROR: File does not exist in current directory. \n");

    return 0;
}

int close_file(char* filename)
{
    if (!FTIsOpen(filename)) 
    {
        FTRemove(filename);
    }
    else
    {
        printf("ERROR: File not open.\n");
    }
}

void FTAdd(const char* fileName, const char* mode) {
    if (FTIsOpen(fileName)) {
        printf("ERROR: File already open.\n");
    }
    else {
        struct FileTable* tmp = calloc(1, sizeof(struct FileTable));
        strcpy(tmp->name, fileName);
        strcpy(tmp->mode, mode);
        tmp->next = NULL;
        if (root == NULL) {
            root = tmp;
        }
        else {
            struct FileTable* itr = root;
            while (itr->next != NULL) {
                itr = itr->next;
            }
            itr->next = tmp;
        }
    }
}

void FTRemove(const char* fileName) {
    struct FileTable* itr1;
    struct FileTable* itr2 = NULL;
    for (itr1 = root; itr1 != NULL; itr2 = itr1, itr1 = itr1->next) {
        if (strcmp(itr1->name, fileName) == 0) {
            if (itr2 == NULL) {
                root = itr1->next;
            }
            else {
                itr2->next = itr1->next;
            }
            free(itr1);
            return;
        }
    }
}

bool FTIsOpen(const char* fileName) {
    struct FileTable* itr;
    for (itr = root; itr != NULL; itr = itr->next) {
        if (strcmp(itr->name, fileName) == 0) {
            return true;
        }
    }
    return false;
}

void FTCleanup() {
    struct FileTable* itr1 = root;
    struct FileTable* itr2;
    while (itr1 != NULL) {
        itr2 = itr1->next;
        free(itr1);
        itr1 = itr2;
    }
    root = NULL;
}

/*
int create_file(char *filename) {
    DIRENTRY curr_dir;
    int i, offset, free;

    DIRENTRY new_dir = new_file(filename);
    int curr_cluster = ENV.current_cluster;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] == 0x0 || curr_dir.DIR_Name[0] == 0xE5) {
                // free = find_and_allocate_empty_cluster(curr_cluster);
                fseek(img_file, offset, SEEK_SET);
                fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);
                return 0;
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }

    if((free = find_and_allocate_empty_cluster(curr_cluster)) == -1) {
        return -1;
    }
    offset = get_first_sector(free) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);;

    return 0;
}

int create_directory(char *dirname) {
    DIRENTRY curr_dir;
    int i, offset, free;

    DIRENTRY new_dir = new_directory(dirname);
    int curr_cluster = ENV.current_cluster;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] == 0x0 || curr_dir.DIR_Name[0] == 0xE5) {
                //free is 4B while Lo, Hi are 2B, so we need to split into two halfs [byte 4 - byte 3] [byte 2 - byte 1]
                if((free = find_and_allocate_empty_cluster(curr_cluster)) == -1) {
                    return -1;
                }
                new_dir.DIR_FirstClusterLO = free % 0x100;
                new_dir.DIR_FirstClusterHI = free / 0x100;

                // new_dir.DIR_FirstClusterLO = curr_cluster % 0x100;
                // new_dir.DIR_FirstClusterHI = curr_cluster / 0x100;

                fseek(img_file, offset, SEEK_SET);
                fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);

                create_standard_directories(free);

                return 0;
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    if((free = find_and_allocate_empty_cluster(curr_cluster)) == -1) {
        return -1;
    }

    new_dir.DIR_FirstClusterLO = free % 0x100;
    new_dir.DIR_FirstClusterHI = free / 0x100;

    offset = get_first_sector(free) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);

    create_standard_directories(free);

    return 0;
}

void create_standard_directories(int curr_cluster) {
    //printf("%d", HI * 0x100 + LO);
    int offset;
    DIRENTRY dir = new_directory("..");
    if(ENV.current_cluster != BPB.BPB_RootClus) {
        dir.DIR_FirstClusterHI = ENV.current_dir.DIR_FirstClusterHI;
        dir.DIR_FirstClusterLO = ENV.current_dir.DIR_FirstClusterLO;
    }
    offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&dir, sizeof(DIRENTRY), 1, img_file);

    dir = new_directory(".");
    offset += sizeof(DIRENTRY);
    fseek(img_file, offset, SEEK_SET);
    fwrite(&dir, sizeof(DIRENTRY), 1, img_file);

    char end = 0x0;

    fwrite(&end, 1, 1, img_file);
}

int find_last_cluster(void) {
    int i, j, offset;
    int curr_cluster = BPB.BPB_RootClus;

    while(!is_last_cluster(curr_cluster)) {
        curr_cluster = get_next_cluster(curr_cluster);
    }
    
    return curr_cluster;
}

int find_and_allocate_empty_cluster(int last_cluster) {
    int current_cluster;
    //int last_cluster = find_last_cluster();
    int i = 0;
    int base = FirstFATSector * BPB.BPB_BytsPerSec;
    long position = base + i*4;

    while(position < FirstDataSector * BPB.BPB_BytsPerSec) {
        fseek(img_file, position, SEEK_SET);
        fread(&current_cluster, 4, 1, img_file);
        
        if(current_cluster == 0x0) {
            fseek(img_file, base + last_cluster * 4, SEEK_SET);
            fwrite(&i, 1, 4, img_file);
            int new_value = 0xFFFFFFFF;
            fseek(img_file, position, SEEK_SET);
            fwrite(&new_value, 1, 4, img_file);
            //printf("\n\nHIHIHI\n\n");
            return i;
        }

        i += 1;
        position = base + i*4;
    }

    return -1;
}
    DIRENTRY curr_dir;
    int i, j, offset;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i < BPB.BPB_BytsPerSec / sizeof(curr_dir); i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0') {
                for(j = 0; j < 11; j++) {
                    printf("%c", curr_dir.DIR_Name[j]);
                }
                printf(" ");
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
        break;
    }
    printf("\n");

    */