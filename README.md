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
    - This is likely due to oversights in file creation and removal methods
  - Removing a file may remove unnecessary contents (seems to be an edge case that only happened once)
  - The open file table may incorrectly say a file is open if it occupies a cluster that was deallocated 
    but originally refered to an open file.
    - Cluster deallocation does not currently modify the open file table.
  - The size of a file is written whatever the overflow value is upon write
    - In other words: If your file is 10B, and you write 50B along with a string <str>, the file will be 50B with contents <str>
    - Likely an easy fix, but left for future work.
  - Does not currently handle the edge case of deleting the last entry  (generally sets the first byte to 0xE5)
```

  
