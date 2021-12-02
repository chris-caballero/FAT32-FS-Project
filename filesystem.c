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

struct FILETABLE
{
    char name[32];
    char mode[8];
    unsigned int root_cluster;
    unsigned int offset;
    struct FILETABLE* next;
} __attribute__((packed));

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

unsigned int bytes_per_cluster;

char* UserInput[5]; //Longest Command is 4 long, so this will give us just enough space for 4 args and End line
struct FILETABLE* root = NULL;
 
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
//open -> write
int open_file(char* filename, char* mode);
int close_file(char* filename);
void seek_position(char * filename, int offset);
int read_file(char *filename, int size);
int write_file(char *filename, int size, char* string);
void cp(char* source, char* dest);
void mv(char* source, char* dest);
// int create_file(char *filename);
// int create_directory(char *dirname);


DIRENTRY find_entry(char *name);

//Cluster Management
int get_first_sector(int cluster_num);
int get_next_cluster(int curr_cluster);
int is_last_cluster(int cluster);

//struct FILETABLE Management
void FTAdd(const char* fileName, const char* mode, int root_cluster);
void FTRemove(const char* fileName);
int FTIsOpen(const char* fileName);
void FTCleanup();
struct FILETABLE* get_entry_FT(int cluster_num);

//Helper functions
int find_dirname_cluster(char *dirname);
DIRENTRY find_filename_entry(char *filename) ;
DIRENTRY new_file(char *filename);
DIRENTRY new_directory(char *dirname);
int find_last_cluster(int curr_cluster);

void format(char * name);
int isFile(int first_cluster);
int isValidMode(char * mode);
int isValidPermissions(char *filename, char * mode);

int find_filename_cluster_offset(char *filename);

void allocate_clusters(int num_clusters, int last_cluster);
int find_empty_cluster(void);

void create_initial_entries(DIRENTRY curr_dir);


int create_file(char *filename);
int make_dir(char * dirname);

void remove_file(char *filename);
void deallocate_clusters(int curr_cluster);

int main(int argc, const char* argv[]) {
    if(argc != 2) {
        printf("Error, incorrect number of arguments\n");
        return 0;
    }

    const char* filename = argv[1];
    loadBPB(filename);

    bytes_per_cluster = BPB.BPB_BytsPerSec * BPB.BPB_SecPerClus;

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
            char *filename = UserInput[1];
            create_file(filename);
        }
        else if (!strcmp(command, "mkdir")) {
            char *dirname = UserInput[1];
            make_dir(dirname);
        }
        else if (!strcmp(command, "mv")) {
            char* source = UserInput[1];
            char* dest = UserInput[2];
            mv(source, dest);

        }
        else if (!strcmp(command, "open")) {
            char *filename = UserInput[1];
            char *mode = UserInput[2];
            open_file(filename, mode);
        }
        else if (!strcmp(command, "close")) {
            char *filename = UserInput[1];
            close_file(filename);
        }
        else if (!strcmp(command, "lseek")) {
            char *filename = UserInput[1];
            int offset = atoi(UserInput[2]);
            seek_position(filename, offset);
        }
        else if (!strcmp(command, "read")) {
            char *filename = UserInput[1];
            int size = atoi(UserInput[2]);
            read_file(filename, size);
        }
        else if (!strcmp(command, "write")) {
            char *filename = UserInput[1];
            int size = atoi(UserInput[2]);
            char * string = UserInput[3];
            write_file(filename, size, string);
        }
        else if (!strcmp(command, "rm")) {
            char *filename = UserInput[1];
            remove_file(filename);
        }
        else if (!strcmp(command, "cp")) {
            char* source = UserInput[1];
            char* dest = UserInput[2];
            cp(source, dest);

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
        
        if (userInputIndex == 3)
        {
            if (temp != '\"')
            {
                printf("Error: Expected quotes around last argument\n");
                return;
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
    if(cluster == 0xFFFFFFFF || cluster == 0x0FFFFFF8 || cluster == 0x0FFFFFFE || cluster == 0x0FFFFFFF) {
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

    char *original = (char *) malloc(sizeof(char)*strlen(dirname));
    strcpy(original, dirname);

    if(strcmp(dirname, "") == 0) {
        curr_cluster = ENV.current_cluster;
    } else if((curr_cluster = find_dirname_cluster(dirname)) == -1) {
        printf("Error: directory %s not found\n", original);
        return;
    }

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != 0xE5 && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
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

    char *original = (char *) malloc(sizeof(char)*strlen(dirname));
    strcpy(original, dirname);

    if((new_cluster = find_dirname_cluster(dirname)) == -1) {
        printf("Error: directory %s not found\n", original);
        return;
    } else if(new_cluster == 0) {
        new_cluster = BPB.BPB_RootClus;
    }
    ENV.current_cluster = new_cluster;
}

DIRENTRY new_file(char *filename) {
    DIRENTRY new_dir;

    for(int i = 0; i < strlen(filename); i++) {
        new_dir.DIR_Name[i] = (filename[0] == '.') ? filename[i] : toupper(filename[i]);
    } for(int i = strlen(filename); i < 11; i++) {
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

int find_filename_cluster_offset(char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(filename));
    strcpy(original, filename);

    format(filename);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(!(curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != filename[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return offset;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", original);

    return -1;
}

DIRENTRY find_filename_entry(char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(filename));
    strcpy(original, filename);

    format(filename);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(!(curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != filename[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return curr_dir;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", original);
    curr_dir.DIR_Name[0] = 0x0;

    return curr_dir;
}

DIRENTRY find_dirname_entry(char * dirname) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(dirname));
    strcpy(original, dirname);

    format(dirname);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if((curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != dirname[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return curr_dir;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: directory %s not found\n", original);
    curr_dir.DIR_Name[0] = 0x0;

    return curr_dir;
}

int find_dirname_cluster_offset(char *dirname) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(dirname));
    strcpy(original, dirname);

    format(dirname);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(!(curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != dirname[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return offset;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", original);

    return -1;
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
    //printf("Error: directory %s not found\n", original);
    return -1;
}

DIRENTRY find_entry(char *name) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(name));
    strcpy(original, name);

    format(name);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);
            
            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != name[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return curr_dir;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    curr_dir.DIR_Name[0] = 0x0;

    return curr_dir;  
}


int file_size(char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(filename));
    strcpy(original, filename);

    format(filename);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                if((curr_dir.DIR_Attributes & ATTR_DIRECTORY) == 0) {
                    int cont = 0;
                    for(int j = 0; j < 11; j++) {
                        if(curr_dir.DIR_Name[j] != filename[j]) {
                            cont = 1;
                            break;
                        }
                    }
                    if(!cont) {
                        return curr_dir.DIR_FileSize;
                    }
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", original);
    return -1;
}

int get_first_sector(int cluster_num) {
    int offset = (cluster_num - 2) * BPB.BPB_SecPerClus;
    return FirstDataSector + offset;
}

void format(char * name) {
    int i = 0;
    for(; i < strlen(name); i++)    
        name[i] = toupper(name[i]);
    for(; i < 11; i++)              
        name[i] = ' ';
    name[11] = '\0';
}

int open_file(char* filename, char* mode) {
    int cluster;
    DIRENTRY file = find_filename_entry(filename);
    cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
   //Mode Check
    if(file.DIR_Name[0] == 0x0) {
        printf("ERROR: File not Found\n");
        return -1;
    } else if (!isValidMode(mode)) {
        printf("ERROR: Invalid mode, options are: r w rw wr\n");
        return -1;
    } else if(!isValidPermissions(filename, mode)) {
        printf("ERROR: Invalid permissions, file %s is read-only\n", filename);
        return -1;
    }
    //Check if File exists
    else if (get_entry_FT(cluster) != NULL)
    {
        printf("ERROR: File already Open\n");
        return -1;
    } else {
        FTAdd(filename, mode, cluster);
    }

    return 0;
}

int isValidPermissions(char *filename, char * mode) {
    DIRENTRY dir;
    int offset = find_filename_cluster_offset(filename);
    fseek(img_file, offset, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img_file);
    if(((dir.DIR_Attributes & ATTR_READ_ONLY) != 0) && 
        (strcmp(mode, "w") == 0 || strcmp(mode, "rw") == 0 || strcmp(mode, "wr") == 0)) {
        return 0;
    }
    return 1;
}

int isValidMode(char * mode) {
    return !(strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0 && 
            strcmp(mode, "rw") != 0 && strcmp(mode, "wr") != 0);
}

int close_file(char* filename)
{
    DIRENTRY file = find_filename_entry(filename);
    int cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
    if (get_entry_FT(cluster) != NULL) 
    {
        FTRemove(filename);
    }
    else
    {
        printf("ERROR: File not open.\n");
        return -1;
    }
    return 0;
}

void FTAdd(const char* fileName, const char* mode, int root_cluster) {
    if (FTIsOpen(fileName)) {
        return;
    }
    else {
        struct FILETABLE* tmp = calloc(1, sizeof(struct FILETABLE));
        strcpy(tmp->name, fileName);
        strcpy(tmp->mode, mode);
        tmp->offset = 0;
        tmp->next = NULL;
        tmp->root_cluster = root_cluster;
        if (root == NULL) {
            root = tmp;
        }
        else {
            struct FILETABLE* itr = root;
            while (itr->next != NULL) {
                itr = itr->next;
            }
            itr->next = tmp;
        }
    }
}

void FTRemove(const char* fileName) {
    struct FILETABLE* itr1;
    struct FILETABLE* itr2 = NULL;
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

int FTIsOpen(const char* fileName) {
    struct FILETABLE* itr;
    for (itr = root; itr != NULL; itr = itr->next) {
        if (strcmp(itr->name, fileName) == 0) {
            return 1;
        }
    }
    return 0;
}

struct FILETABLE* get_entry_FT(int cluster_num) {
    struct FILETABLE* itr;
    for (itr = root; itr != NULL; itr = itr->next) {
        if (itr->root_cluster == cluster_num) {
            return itr;
        }
    }
    return NULL;
}

void FTCleanup() {
    struct FILETABLE* itr1 = root;
    struct FILETABLE* itr2;
    while (itr1 != NULL) {
        itr2 = itr1->next;
        free(itr1);
        itr1 = itr2;
    }
    root = NULL;
}

void seek_position(char * filename, int offset) {
    struct FILETABLE* itr;
    char *original = (char *) malloc(sizeof(char)*strlen(filename));
    strcpy(original, filename);

    DIRENTRY file = find_filename_entry(filename);
    if(file.DIR_Name[0] == 0x0) {
        return;
    }

    int clus = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;

    if((itr = get_entry_FT(clus)) != NULL) {
        if(offset > file.DIR_FileSize) {
            printf("Error: Offset out of bounds\n");
        } else {
            itr->offset = offset;
        }
    } else {
        printf("Error: %s is not open\n", original);
    }
}

int read_file(char *filename, int size) {
    struct FILETABLE* itr;
    int temp_offset, sz, curr_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(filename));
    strcpy(original, filename);

    DIRENTRY file = find_filename_entry(filename);
    if(file.DIR_Name[0] == 0x0) {
        return -1;
    }

    int first_cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
    curr_cluster = first_cluster;

    if((itr = get_entry_FT(first_cluster)) != NULL) {
        if(strcmp(itr->mode, "w") == 0) {
            printf("Error: Attempting to read a write-only file\n");
            return -1;
        }
        if(itr->offset + size > file.DIR_FileSize) {
            size = file.DIR_FileSize - itr->offset;
        }

        temp_offset = itr->offset;
        while (temp_offset > bytes_per_cluster) {
            temp_offset -= bytes_per_cluster;
        }
        //if overflow we only read to end, otherwise read the entire size
        int position = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + itr->offset;
        do {
            sz = (temp_offset + size > bytes_per_cluster) ? bytes_per_cluster - temp_offset : size;
            char buff[sz];

            fseek(img_file, position, SEEK_SET);
            fread(&buff, sizeof(buff), 1, img_file);

            buff[sz] = '\0';
            printf("%s", buff);

            size -= (bytes_per_cluster - temp_offset);
            temp_offset = 0;
            curr_cluster = get_next_cluster(curr_cluster);

            position = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
        } while(size > 0 && !is_last_cluster(curr_cluster));
    } else {
        printf("Error: %s is not open\n", original);
        return -1;
    }
    printf("\n");
    return 0;
}

int write_file(char *filename, int size, char *string) { 
    struct FILETABLE* itr;
    int temp_offset, sz, curr_cluster;

    char *original = (char *) malloc(sizeof(char)*strlen(filename));
    strcpy(original, filename);

    DIRENTRY file = find_filename_entry(filename);
    if(file.DIR_Name[0] == 0x0) {
        return -1;
    }

    int first_cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
    curr_cluster = first_cluster;

    if((itr = get_entry_FT(first_cluster)) != NULL) {
        if(strcmp(itr->mode, "r") == 0) {
            printf("Error: Attempting to write a read-only file\n");
            return -1;
        }
        //file size to be increased, otherwise remains the same
        if(itr->offset + size > file.DIR_FileSize) {
            //allocate extra clusters
            int num_curr_clusters = file.DIR_FileSize / bytes_per_cluster;
            int num_final_clusters = (itr->offset + size) / bytes_per_cluster;
            if(file.DIR_FileSize % bytes_per_cluster > 0) {
                num_curr_clusters += 1;
            }
            if((itr->offset + size) % bytes_per_cluster > 0) {
                num_final_clusters += 1;
            }
            int extra_clusters = num_final_clusters - num_curr_clusters;
            allocate_clusters(extra_clusters, find_last_cluster(curr_cluster));
            int last_cluster = find_last_cluster(last_cluster);
            
            file.DIR_FileSize = itr->offset + size;
            int temp = find_filename_cluster_offset(filename);
            fseek(img_file, temp, SEEK_SET);
            fwrite(&file, sizeof(file), 1, img_file);
        }
        //write data in clusters
        temp_offset = itr->offset;
        while (temp_offset > bytes_per_cluster) {
            temp_offset -= bytes_per_cluster;
        }
        //if overflow we only read to end, otherwise read the entire size
        int position = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + itr->offset;
        int base = 0;
        while(size > 0 && !is_last_cluster(curr_cluster)) {
            sz = (temp_offset + size > bytes_per_cluster) ? bytes_per_cluster - temp_offset : size;
            char buff[sz];
            for(int i = base; i < base + sz; i++) {
                buff[i] = string[i];
            }
            base += sz;

            fseek(img_file, position, SEEK_SET);
            fwrite(&buff, 1, sizeof(buff), img_file);

            size -= (bytes_per_cluster - temp_offset);
            temp_offset = 0;
            curr_cluster = get_next_cluster(curr_cluster);

            position = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
        }
    } else {
        printf("Error: %s is not open\n", original);
        return -1;
    }
    return 0;
}

void cp(char* source, char* dest)
{
    //Get current working Directory

    //Check if file exists

    //if dest exists, error out
   
    //if file exists, copy it to the dest

}

void mv(char* source, char* dest)
{
    if (strcmp(source, "..") == 0 || (strcmp(source, ".") == 0))
    {
        printf("Error: .. and . are not valid sources!");
        return;
    }


    int first_cluster_dest, first_cluster_source;
    DIRENTRY dest_entry, source_entry;
    if((first_cluster_dest = find_dirname_cluster(dest)) != -1) {
        //move the file
        source_entry = find_entry(source);

        dest_entry = find_dirname_entry(dest);

        if(source_entry.DIR_Attributes & ATTR_DIRECTORY) {
            //is directory, make directory
            int temp = ENV.current_cluster;
            cd(dest);

            make_dir(source);

            int new_pos = find_dirname_cluster_offset(source);
            fseek(img_file, new_pos, SEEK_SET);
            fwrite(&source_entry, sizeof(source_entry), 1, img_file);

            ENV.current_cluster = temp;
            
            int source_offset = find_dirname_cluster_offset(source);
            if(is_last_cluster(first_cluster_source)) {
                int empty = 0x0;
                fseek(img_file, source_offset, SEEK_SET);
                fwrite(&empty, 1, 1, img_file);
            } else {
                int empty = 0xE5;
                fseek(img_file, source_offset, SEEK_SET);
                fwrite(&empty, 1, 1, img_file);
            }

        } else {
            //create file
            int temp = ENV.current_cluster;
            cd(dest);

            create_file(source);

            int new_pos = find_filename_cluster_offset(source);
            fseek(img_file, new_pos, SEEK_SET);
            fwrite(&source_entry, sizeof(source_entry), 1, img_file);

            ENV.current_cluster = temp;

            int source_offset = find_filename_cluster_offset(source);
            if(is_last_cluster(first_cluster_source)) {
                int empty = 0x0;
                fseek(img_file, source_offset, SEEK_SET);
                fwrite(&empty, 1, 1, img_file);
            } else {
                int empty = 0xE5;
                fseek(img_file, source_offset, SEEK_SET);
                fwrite(&empty, 1, 1, img_file);
            }
        }

        first_cluster_source = source_entry.DIR_FirstClusterHI*0x100 + source_entry.DIR_FirstClusterLO;



        if(source_entry.DIR_Attributes & ATTR_DIRECTORY) {
            int temp = ENV.current_cluster;
            ENV.current_cluster = first_cluster_source;
            DIRENTRY parent = find_dirname_entry("..");
            ENV.current_cluster = temp;


            parent.DIR_FirstClusterHI = first_cluster_dest / 0x100;
            parent.DIR_FirstClusterLO = first_cluster_dest % 0x100;
            
            int offset = find_dirname_cluster_offset(source);
            fseek(img_file, offset, SEEK_SET);
            fwrite(&parent, sizeof(parent), 1, img_file);
        }

    } else {
        //change file/directory name
        format(dest);
        source_entry = find_entry(source);
        for(int i = 0; i < strlen(dest); i++) {
            source_entry.DIR_Name[i] = toupper(dest[i]);
        } 
        for(int i = strlen(dest); i < 11; i++) {
            source_entry.DIR_Name[i] = ' ';
        }

        int offset;
        if(source_entry.DIR_Attributes & ATTR_DIRECTORY) {
            offset = find_dirname_cluster_offset(source);
            dest_entry = find_entry(dest);
            if(dest_entry.DIR_Name[0] != 0x0 && !(dest_entry.DIR_Attributes & ATTR_DIRECTORY)) {
                printf("Cannot move directory: invalid destination argument\n");
                return;
            }
        } else {
            dest_entry = find_entry(dest);
            if(dest_entry.DIR_Name[0] != 0x0 && !(dest_entry.DIR_Attributes & ATTR_DIRECTORY)) {
                printf("The name is already being used by another file\n");
                return;
            }
            offset = find_filename_cluster_offset(source);
        }
        fseek(img_file, offset, SEEK_SET);
        fwrite(&source_entry, sizeof(source), 1, img_file);
        
        
    }

    //if source is directory and dest is a file, error out
     
    //if dest exists and source exists, error out on name basis
     
    //if dest IS a directory, source moved to be inside dest...
   
    //if dest does not exists, source is renamed to dest.  
}

int isFile(int first_cluster) {
    DIRENTRY dir;
    int offset = get_first_sector(first_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img_file);
    if(!(dir.DIR_Attributes & ATTR_DIRECTORY)) {
        return 1;
    }
    return 0;
}

//use last_cluster = -1 to just allocate a cluster, no linking
void allocate_clusters(int num_clusters, int last_cluster) {
    int empty_cluster = BPB.BPB_RootClus, next_cluster = 0xFFFFFFFF, offset = 0;
    for(int i = 0; i < num_clusters; i++) {
        if((empty_cluster = find_empty_cluster()) == -1) {
            return;
        }

        if(last_cluster != -1) {
            offset = FirstFATSector * BPB.BPB_BytsPerSec + last_cluster * 4;
            fseek(img_file, offset, SEEK_SET);
            fwrite(&empty_cluster, 4, 1, img_file);
        }
     
        offset = FirstFATSector*BPB.BPB_BytsPerSec + empty_cluster * 4;

        next_cluster = 0xFFFFFFFF;
        fseek(img_file, offset, SEEK_SET);
        fwrite(&next_cluster, 4, 1, img_file);
        last_cluster = empty_cluster;
    }
}

int find_empty_cluster(void) {
    int cluster_value;
    int curr_cluster_in_fat = BPB.BPB_RootClus, 
        offset = FirstFATSector * BPB.BPB_BytsPerSec + curr_cluster_in_fat * 4,
        bound = FirstDataSector * BPB.BPB_BytsPerSec;

    while(offset < bound) {
        fseek(img_file, offset, SEEK_SET);
        fread(&cluster_value, 4, 1, img_file);
        //printf("%d\n", offset);
        if(cluster_value == 0x0) {
            return curr_cluster_in_fat;
        }
        curr_cluster_in_fat += 1;
        offset = FirstFATSector*BPB.BPB_BytsPerSec + curr_cluster_in_fat * 4;
    }
    printf("No free clusters were found");
    return -1;
}

int find_last_cluster(int curr_cluster) {
    int i = 0;

    while(!is_last_cluster(curr_cluster)) {
        //printf("%d", i);
        curr_cluster = get_next_cluster(curr_cluster);
        i += 1;
    }
    
    return curr_cluster;
}

int create_file(char *filename) {
    DIRENTRY curr_dir;
    int i, offset;

    DIRENTRY new_dir = new_file(filename);
    int curr_cluster = ENV.current_cluster;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] == 0x0 || curr_dir.DIR_Name[0] == 0xE5) {
                new_dir.DIR_FirstClusterHI = curr_cluster / 0x100;
                new_dir.DIR_FirstClusterLO = curr_cluster % 0x100;

                fseek(img_file, offset, SEEK_SET);
                fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);
                return 0;
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }

    allocate_clusters(1, curr_cluster);
    curr_cluster = get_next_cluster(curr_cluster);

    new_dir.DIR_FirstClusterHI = curr_cluster / 0x100;
    new_dir.DIR_FirstClusterLO = curr_cluster % 0x100;

    offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);;

    return 0;
}

int make_dir(char *dirname) {
    DIRENTRY curr_dir;
    int i, offset, cluster_value;

    DIRENTRY new_dir = new_directory(dirname);
    int curr_cluster = ENV.current_cluster, 
        first_dir_cluster = find_empty_cluster();

    offset = FirstFATSector*BPB.BPB_BytsPerSec + first_dir_cluster * 4;
    cluster_value = 0x0FFFFFFF;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&cluster_value, 4, 1, img_file);

    new_dir.DIR_FirstClusterHI = first_dir_cluster / 0x100;
    new_dir.DIR_FirstClusterLO = first_dir_cluster % 0x100;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] == 0x0 || curr_dir.DIR_Name[0] == 0xE5) {
                fseek(img_file, offset, SEEK_SET);
                fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);

                create_initial_entries(new_dir);

                return 0;
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    allocate_clusters(1, curr_cluster);

    curr_cluster = get_next_cluster(curr_cluster);

    new_dir.DIR_FirstClusterHI = curr_cluster / 0x100;
    new_dir.DIR_FirstClusterLO = curr_cluster % 0x100;

    offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);

    create_initial_entries(new_dir);

    return 0;
}

void create_initial_entries(DIRENTRY curr_dir) {
    DIRENTRY parent = new_directory("..");
    DIRENTRY current = new_directory(".");

    int empty = 0x0, 
        curr_cluster = curr_dir.DIR_FirstClusterHI * 0x100 + curr_dir.DIR_FirstClusterLO, 
        offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;

    if(ENV.current_cluster == BPB.BPB_RootClus) {
        parent.DIR_FirstClusterHI = 0;
        parent.DIR_FirstClusterLO = 0;
    } else {
        parent.DIR_FirstClusterHI = ENV.current_cluster / 0x100;
        parent.DIR_FirstClusterLO = ENV.current_cluster % 0x100;
    }
    current.DIR_FirstClusterHI = curr_dir.DIR_FirstClusterHI;
    current.DIR_FirstClusterLO = curr_dir.DIR_FirstClusterLO;

    //write DIRENTRY for ..
    offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&parent, sizeof(parent), 1, img_file);
    //write DIRENTRY for .
    offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + sizeof(curr_dir);
    fseek(img_file, offset, SEEK_SET);
    fwrite(&current, sizeof(current), 1, img_file);
    fwrite(&empty, 1, 1, img_file);
}

void remove_file(char *filename) {
    DIRENTRY file = find_filename_entry(filename);
    int empty1 = 0x0, empty2 = 0xE5;

    if(file.DIR_Name[0] == empty1) {
        return;
    }
    int first_cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;

    deallocate_clusters(first_cluster);

    int offset = find_filename_cluster_offset(filename);
    fseek(img_file, offset, SEEK_SET);

    if(is_last_cluster(first_cluster)) {
        fwrite(&empty1, 1, 1, img_file);
    } else {
        fwrite(&empty2, 1, 1, img_file);
    }
}

void deallocate_clusters(int curr_cluster) {
    int empty = 0x0;
    while(!is_last_cluster(curr_cluster)) {
        printf("%d\n", curr_cluster);
        int offset = FirstFATSector * BPB.BPB_BytsPerSec + curr_cluster * 4;
        fseek(img_file, offset, SEEK_SET);
        fread(&curr_cluster, 4, 1, img_file);
        fwrite(&empty, 1, 1, img_file);
    }
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