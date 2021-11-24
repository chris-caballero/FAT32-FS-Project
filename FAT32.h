// This file shall contain the data struct for a Fat32 reservedRegion
struct ReservedRegion {
    //BPB
    unsigned int BS_jmpBoot; //Offset 0, Size 3
    char BS_OEMName[9]; //Offset 3, Size 8
    unsigned int BPB_BytsPerSec; //Offset 11 Size 2
    unsigned int BPB_SecPerClus; //Offset 13 Size 1
    unsigned int BPB_RsvdSecCnt; //Offset 14 Size 2
    unsigned int BPB_NumFATs;   //Offset 16, Size 1
    unsigned int BPB_RootEntCnt; //Offset 17, size 2
    unsigned int BPB_TotSec16;  //Offset 19 Size 2
    unsigned int BPB_Media; //Offset 21 Size 1
    unsigned int BPB_FATSz16; //Offset 22 Size 2
    unsigned int BPB_SecPerTrk; //Offset 24 Size 2
    unsigned int BPB_NumHeads; //Offset 26 Size 2
    unsigned int BPB_HiddSec; //Offset 29 Size 4
    unsigned int BPB_TotSec32; //Offset 32 Size 4
    //Skipping FAT12 & 16
    //Extended BPB
    unsigned int BPB_FATSz32; //Offset 36 Size 4
    unsigned int BPB_ExtFlags; //Offset 40 Size 2
    unsigned int BPB_FSVer; //Offset 42 Size 2
    unsigned int BPB_RootClus; //Offset 44 Size 4
    unsigned int BPB_FSInfo; //Offset 48 Size 2
    unsigned int BPB_BkBootSec; //Offset 50 Size 2
    unsigned int BPB_Reserved; //Offset 52 Size 12
    unsigned int BS_DrvNum; //Offset 64 Size 1
    unsigned int BS_Reserved1; //Offset 65 Size 1
    unsigned int BS_BootSig; //Offset 66 Size 1
    unsigned int BS_VolID; //Offset 67, size 4
    char BS_VolLab[12]; //Offset 71 Size 11
    char BS_FilSysType[9]; //Offset 82 Size 8

};