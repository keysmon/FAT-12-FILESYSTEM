#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/*find value of FAT entry*/
int getEntryValue(char* p, int entryByte){
    int fatstart = 512;
    int buffer1,buffer2;
    int result;
    if (entryByte%2 == 0) {
            //low 4 bits
        buffer1 = p[fatstart+1 + ((3*entryByte)/2)] & 0x0F;
            //full 8 bits
        buffer2 = p[fatstart + ((3*entryByte)/2)] & 0xFF;
        result = (buffer1 << 8) | (buffer2);
    //if odd
    }else{
            //high 4 bits
        buffer1 = p[fatstart + ((3*entryByte)/2)] & 0xF0;
            //full 8 bits
        buffer2 = p[fatstart+1 + ((3*entryByte)/2)] & 0xFF;
        result =  (buffer1 >> 4) | (buffer2 << 4);
    }
    return result;
}


void diskget(char* p, char* name){
    char* fileName;
    fileName = strtok(name,"");
    
    char* s = fileName;
    while(*s){
        *s = toupper((unsigned char) *s);
        s++;
    }
    unsigned long int bytesPerSector = (unsigned long int)( p[11] | p[12]<<8);
    int secPerFat = (int) (p[22] | p[23]<<8);
    int reservedSec = (int) (p[14] | p[15]<<8);
    int numOfFat = (int) (p[16]);
    int rootStart = bytesPerSector*(reservedSec + numOfFat*secPerFat);
    int counter = -1;
    int nameByte1 = p[rootStart];  //point to the first byte of the entry
    
    char* file_name = calloc(1, sizeof(char)*9); //Times 9 because 8 bytes + null
    char* ext = calloc(1, sizeof(char)*4); // Times 4 because 3 bytes + null
    int file_size;
    int first_logical_cluster;
    
    int fileFound = 0;
    /*find file in root directory*/
    while(nameByte1 != 0x00 && counter<33*16){  //0x00->rest is free || reached the end
        char* name_ext = calloc(1, sizeof(char)*14);
        counter++;
        int attr = p[rootStart+11+counter*32];
        int nameByte1 = p[rootStart+counter*32+32];
        if(nameByte1 != 0xE5 &&attr != 0x08 &&attr != 0x0F &&attr != 0x10){ // Check if not empty && not a volume label && not a long file_name && not a sub-directory
            /*read file_name*/
            for(int i=0;i<8;i++){
                if(p[rootStart + counter*32 + i] == ' '){
                    file_name[i] = '\0';
                    break;
                }else{
                    file_name[i] = p[rootStart + counter*32 + i];
                }
            }
            /*read ext*/
            for(int i =8;i<11;i++){
                ext[i - 8] = p[rootStart+counter*32 + i];
            }
            file_name[8] = '\0';
            ext[3] = '\0';
            
            strcat(name_ext,file_name);
            strcat(name_ext,".");
            strcat(name_ext,ext);
            /*check if this is the file*/
            if(!strcmp(name_ext,fileName)){
                fileFound = 1;
                /*get first cluster number*/
                first_logical_cluster = (p[rootStart + counter*32 + 26] & 0x00FF) + ((p[rootStart + counter*32 + 27] & 0x00FF) << 8);
                /*get file size*/
                file_size = (p[rootStart + counter*32 + 28] & 0x00FF) + ((p[rootStart + counter*32 + 29] & 0x00FF) << 8) + ((p[rootStart + counter*32 + 30] & 0x00FF) << 16) + ((p[rootStart + counter*32 + 31] & 0x00FF) << 24);
                break; // Found the file, break out from the while loop
            }
        }
    }
    if(!fileFound){ //if file not found
        printf("File not found\n");
        return;
    }
    FILE* fp = fopen(fileName,"w");
    
    int bytes_left = file_size;
   
    int entry = first_logical_cluster;
    int entryValue = first_logical_cluster;

    /*write the file into our linux system*/
    do{
        entry =  entryValue;
        if(bytes_left>=512){
            fwrite (p + (33 + entry - 2)*512 , sizeof(char), 512, fp);
        }else{
            fwrite (p + (33 + entry - 2)*512 , sizeof(char), bytes_left, fp);
        }
        bytes_left -= 512;
        entryValue = getEntryValue(p, entry) & 0xFFF;
    }
    while(entryValue < 0xFF8);   // 0xFF8 - 0xFFF -> last cluster of file
    
    fclose(fp);
    return;
}


int main (int argc, char** argv)
{
    if(argc!=3){
        printf("Invalid file argument\n");
        exit(0);
    }
    int fd;
    struct stat sb;
    fd = open(argv[1], O_RDWR);
    if(fd == -1){
        printf("***Couldn't find disk. Exiting...\n");
        exit(0);
    }
    if(fstat(fd, &sb)  == -1){
        perror("Couldnt get file size\n");
    }
    char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
   
    diskget(p,argv[2]);
  
    close(fd);
    return 0;
}

