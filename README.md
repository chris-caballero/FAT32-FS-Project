## FAT32 System

```
Group Members:
  - Chris Caballero
  - Ryan Beck
```

```
Divison of Labour:
  - Chris Caballero:
    - FAT32 Functions
    - Run Program function
    - exit
    - info
    - size
    - ls
    - cd
    - creat
    - mkdir
    - open
    - close
    - lseek
    - read
    - write
    - rm
    - rmdir
  - Ryan Beck:
    - User Input
    - FAT32.h, The data structure
    - Filetable
    - open
    - close
    - Run Program
    - MakeFile 
    - Documentation

  - Tivvon Cruickshank
    -No Show.
```


```
How to use:
  make: Compiles the program as "project3"
  make start: Runs the produced "project3" executable using the "fat32.img"
  ./project3 <imageFile>: Executes the executable using imageFile as the target for the program.
 ```
 
 ```
 Known Bugs / Issues:
  - Write may write to open space in the current directory (visibile to ls)
    - This is likely due to errors in file creation and removal
  - Removing a file may remove unnecessarily (seems to be an edge case that only happened once)
  - The open file table may incorrectly say a file is open if it occupies a cluster that was deallocated 
    but originally refered to an open file
  - The size of a file is written to be exactly whatever the overflow value is upon write
    - In other words: If your file is 10B, and you write 50B with string "string", the file will be 50B with contents "string"
    - Seems like it should be more precise than this.
  - Deleting files does not properly check if it is the last entry or not (generally sets the first byte to 0xE5)
```

  
