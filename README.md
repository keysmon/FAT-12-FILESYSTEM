# FAT-12-FILESYSTEM
CSC 360  FAT 12 File_System

Name:Hang Ruan


to compile:
1. make



to run:

1. diskinfo.c	       
./diskinfo disk.ima

2. disklist.c
./disklist disk.ima

3. diskget.c
./diskget disk.ima foo.txt

4. diskput.c		

./diskput disk.ima foo.txt      (if put in root directory)
./diskput disk.ima /sub/foo.txt (if put in given directory, if directory doesnt exist in this disk,it will create the path and then put the file)

ps: 
  1. diskinfo displays files in all directories which includes all subdirectories
	2. diskput can take file path in lower or upper cases.
  3. Was running short on time before due, there're code in diskinfo and diskput which were directirectly from disklist or diskput that were not totally necessary for the purpose of its functionality.
