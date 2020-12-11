#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int count = 0;
// Please remember to give write permission to your argv[1] (the image you want map) by using chmod (if it doest not have the write permission by default), otherwise you will fail the map.


//recursive function that find total files in subdirectories
void files_in_sub(char* p, int entry){
    int base = 512*(33+entry-2);
    int nameByte1 = p[base];
    int attr;       //attribute byte
    int subdirectories[16];
    int index = 0;
    //go through entries to find files
    for(int i=0;i<16;i++){
        nameByte1 = p[base+i*32];
        attr = p[base+11+i*32];
        if(nameByte1==0x00){
            break;
        
        }else if(nameByte1 == 0xE5 || (attr&0x0E)!=0 ||attr == 0x0F ||nameByte1 == '.'){// Check if not empty && not a volume label && not a long file_name
            continue;
        }
        if (attr == 0x10){  //if there's sub_dir in this directory, add it to array
            subdirectories[index] = i;
            index++;
        }else{
            count++;
        }
    }
    //call this function for any subdirectories in this directories
    for (int i=0;i<index;i++){
        int temp = subdirectories[i];
        int first_logical_cluster;
        first_logical_cluster = (p[base + temp*32 + 26] & 0x00FF) + ((p[base + temp*32 + 27] & 0x00FF) << 8);
        files_in_sub(p,first_logical_cluster);
    }
     
}


void file_count(char *p){   /*find the file count in root directories*/
    unsigned long int bytesPerSector = (unsigned long int)( p[11] | p[12]<<8);
    int secPerFat = (int) (p[22] | p[23]<<8);
    int reservedSec = (int) (p[14] | p[15]<<8);
    int numOfFat = (int) (p[16]);
    int rootStart = bytesPerSector*(reservedSec + numOfFat*secPerFat);   //replace reservedSec with 1
    
    int nameByte1;  //point to the first byte of the entry
    int attr;       //attribute byte
    int counter = -1;
    int subdirectories[224];
    int index=0;
    nameByte1 = p[rootStart];
    /*go through the root directory and check for file or dir*/
    while(nameByte1 != 0x00 && counter<14*16)
    {
        counter++;
        attr = p[rootStart+11+counter*32];
        nameByte1 = p[rootStart+counter*32+32];
        if(nameByte1 != 0xE5 &&attr != 0x08 &&attr != 0x0F){ // Check if not empty && not a volume label && not a long file_name
            if (attr == 0x10){  //if there's sub_dir in this directory, add it to array
                subdirectories[index] = counter;
                index++;
            }else{  //if file found, incre count
                count++;
            }
        }
    }
    //find files in sub_dir
    for(int j=0;j<index;j++){
        counter = subdirectories[j];
        int first_logical_cluster = (p[rootStart + counter*32 + 26] & 0x00FF) + ((p[rootStart + counter*32 + 27] & 0x00FF) << 8);
        files_in_sub(p,first_logical_cluster);
    }
    
}





void diskinfo (char* p){
    
    //OperatingSystem Name:
    char * OSname = (char *)malloc(sizeof(char*));
    for(int i=0;i<8;i++){
        OSname[i] = p[i+3];
    }
    printf("OS Name: %s\n",OSname);
   

    
    unsigned long int bytesPerSector = (unsigned long int)( p[11] | p[12]<<8);
    unsigned long int numOfSector = (unsigned long int)( p[19] | p[20]<<8);
    int secPerFat = (int) (p[22] | p[23]<<8);
    int reservedSec = (int) (p[14] | p[15]<<8);
    int numOfFat = (int) (p[16]);
    int rootStart = bytesPerSector*(reservedSec + numOfFat*secPerFat);   //replace reservedSec with 1

    
    
    //Label of the disk
    char *label_name = (char*) malloc(sizeof(char*));
    int no_name = 1;
    for(int i=0;i<244;i++){ //go through root directory
        int attr = p[rootStart + 0x0b +i*32];   //go to the byte where root start, then find i-th entry, 11th byte(attribute)
        if(attr == 0x08){   //if this 4th bit(volumn label) in the attribute is checked, then its the name of the disk
            no_name = 0;
            char* index = p + rootStart + (i*32);   //create a pointer to the fileName bytes
            for(i=0;i<8;i++){
                label_name[i] = *index;
                index++;
            }
            break;
        }
    }
    if(no_name == 1){
        label_name = "NO NAME";
    }
    printf("Label of the disk: %s\n",label_name);
    
    
    //TOTAL SIZE OF DISK
    unsigned long int totalSize = bytesPerSector * numOfSector;
    printf("Total Size of the disk: %lu bytes\n",totalSize);
    
    
    //FREE SIZE OF DISK
    int freeSector = 0;
    int fatstart = bytesPerSector * 1; //FAT1 starting byte
    int buffer1,buffer2;
    int entryValue;
    for (int i=0;i<numOfSector-33+2;i++){    //first two sectors are reserved
        if (i%2 == 0) {
                //low 4 bits
                buffer1 = p[fatstart+1 + ((3*i)/2)] & 0x0F;
                //full 8 bits
                buffer2 = p[fatstart + ((3*i)/2)] & 0xFF;
                entryValue = (buffer1 << 8) | (buffer2);
            //if odd
            } else {
                //high 4 bits
                buffer1 = p[fatstart + ((3*i)/2)] & 0xF0;
                //full 8 bits
                buffer2 = p[fatstart+1 + ((3*i)/2)] & 0xFF;
                entryValue =  (buffer1 >> 4) | (buffer2 << 4);
            }
        if(entryValue == 0x000){
            freeSector++;
        }
    }
    unsigned long int freeSize = freeSector*bytesPerSector;
    printf("Free size of the disk: %lu bytes\n\n", freeSize);
    printf("==============\n");
    
    
    
    //The number of files in the disk (including all files in the root directory and files in all subdirectories)
   
    file_count(p);
    printf("The number of files in the disk: %d\n\n",count);
    printf("==============\n");
    
    
    //Number of FAT copies
    printf("Number of FAT copies: %d\n",numOfFat);
    
    
    //Sectors per FAT
    printf("Sectors per FAT: %d\n",secPerFat);
    return;
}

int main(int argc, char *argv[])
{
    if(argc!=2){
        printf("Invalid file argument\n");
        exit(0);
    }
    int fd;
    struct stat sb;
    fd = open(argv[1], O_RDWR);
   
    if(fstat(fd, &sb)  == -1){
        perror("Couldnt get file size\n");
    }
    char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory

    diskinfo(p);
    
    close(fd);
    return 0;
}
