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
#include <time.h>

int getEntryValue(char* p, int entryByte){
    int fatstart = 512;
    int buffer1,buffer2;
    int result;
    //if even
    if (entryByte%2 == 0) {
            //low 4 bits
        buffer1 = p[fatstart + 1 + ((3*entryByte)/2)] & 0x0F;
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



int getFreeFAT(char* p){
    for(int i = 0;i<2880-33+2;i++){
        if(!getEntryValue(p,i)){
            return i;
        }
    }
    return -1;
}

void setEntryValue(char* p,int entry, int value){
        int fatstart = 512;
        int buffer1,buffer2;
        //even
        if (entry%2 == 0) {
            buffer1 = ((value) & 0xFF) | (p[fatstart+1 + ((3*entry)/2)] & 0xF0);
            buffer2 = ((value>>8) & 0x0F);
            /*store the next fat entry*/
            p[fatstart + ((3*entry)/2)] = buffer1;
            p[fatstart + ((3*entry)/2)+ 1 ] = buffer2;
        
        //odd
        }else {
            buffer1 = ((value<<4) & 0xF0) | (p[fatstart + ((3*entry)/2)] & 0x0F);
            buffer2 = ((value>>4) & 0xFF);
            /*store the next fat entry*/
            p[fatstart + ((3*entry)/2)] = buffer1;
            p[fatstart + ((3*entry)/2) + 1] = buffer2;
        }
}


void create_path(char *p, int base, char* subdirectories[],int start, int end){
    
    char* file_name = p + base;;
    int next_free = 0x001;
    for(int q = start ;q < end ;q++){
        if(q != start){
            file_name = p + (next_free + 31) * 512;
            base = 512 * (next_free+31);
        }
        char* dir_name = calloc(1,sizeof(char)*9);
        /*copy the first subdiretory name into dir_name*/
        for(int i = 0;i<strlen(subdirectories[q]);i++){
            if(subdirectories[q][i] != ' '){
                if(subdirectories[q][i] != '\0'){
                    dir_name[i] = subdirectories[q][i];
                }else{
                    dir_name[i] = ' ';
                }
            }
            
            for(int j = strlen(subdirectories[q]); j< 9;j++){
                dir_name[j] = ' ';
            }
        }
        //copy filename
        strncpy(file_name, dir_name, 8);
        //set attr
        p[base+11] = 0x10;
        
        int current = next_free;
        if(q != start){
            setEntryValue(p,current,0xFFF); //set current FAT to 0xFFF
        }
        next_free = getFreeFAT(p);
        
        // set current -> first_logical_cluster to the next_free
        p[base+26] = next_free & 0xFF;
        p[base+27] = (next_free >>8 )& 0xFF;
    }
    setEntryValue(p,next_free,0xFFF); //set current FAT to next
    
    
    
    
    
    
    
    return;
}




int getFreeSize(char *p){
    int freeSector = 0;
    int fatstart = 512; //FAT1 starting byte
    int buffer1,buffer2;
    int entryValue;
    for (int i=0;i<2880-33+2;i++){    //first two sectors are reserved
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
    unsigned long int freeSize = freeSector*512;
    return freeSize;
}

void diskput(char *p, char* file){
    /*find available space on disk*/
    int freeSize = getFreeSize(p);
    int index = 0;
    /*strtok path into subdirectories array*/
    char* subdirectories[10];
    
    
    char* s = file;
    while(*s){
        *s = toupper((unsigned char) *s);
        s++;
    }
    
    
    char* sub = strtok(file,"/");
    while(sub!=NULL){
        if(index>9){
            printf("Error: too many subdirectories\n");
            return;
        }
        subdirectories[index] = sub;
        index++;
        sub = strtok(NULL,"/");
    }
    
    /*look for file in linux folder*/
    int end = index - 1;
    char* file_ext = subdirectories[end];
    
    int input_file;
    struct stat input_stat;
    input_file = open(file_ext, O_RDWR);
    if(input_file == -1){
        printf("***Couldn't find file. Exiting...\n");
        exit(0);
    }
    
    /*find whether there's enough space to store this file*/
    if(fstat(input_file,&input_stat)  == -1){
        perror("***Couldnt get file size\n");
    }
    int input_size = input_stat.st_size;
    if(input_size>freeSize){
        printf("***Error: Not Enough Space On Disk\n");
        exit(0);
    }
    /*create pointer to input file*/
    char *input = mmap(NULL,input_size,PROT_READ,MAP_SHARED,input_file,0);
    
    /*check for file_name and extension size*/
    char* input_file_name = strtok(file_ext,".");
    char* extension = strtok(NULL," ");
    
    
    if(strlen(input_file_name)>8){
        printf("fileName length (%lu) is too big (<= 8). Exiting...\n",strlen(input_file_name));
        return;
    }
    if(strlen(extension)>3){
        printf("extension length (%lu) is too big (<= 8). Exiting...\n",strlen(extension));
        return;
    }
     
    
   
    int number_of_sub = index - 1;
    //if the file includes a path
    int rootstart = 19*512;
    
    char* file_name;
    int attr;
    char* dir_name = calloc(1,sizeof(char)*9);
    
    if(number_of_sub>0){
        for(int i = 0;i<strlen(subdirectories[0]);i++){
            if(subdirectories[0][i] != ' '){
                if(subdirectories[0][i] != '\0'){
                    dir_name[i] = subdirectories[0][i];
                }else{
                    dir_name[i] = ' ';
                }
            }
            
            for(int j = strlen(subdirectories[0]); j< 9;j++){
                dir_name[j] = ' ';
            }
        }
        dir_name[8] = '\0';
        int i;
        for(i = 0;i<16*14;i++){
            file_name = p + rootstart + i*32;
            attr = p[rootstart + 11 + i*32];
            /*if filename is empty,skip*/
            if(p[rootstart + i*32] == 0x00){
                create_path(p,rootstart+32*i,subdirectories,0,index-1);
                
                break;
            }
             
            
            if( strncmp(dir_name,file_name,8) != 0 ){    //if name doesnt match, continue
                continue;
            }else if( (attr&0x10) != 0x10 ){  //if it's not a subdirectory,continue
                continue;
            }
            break;
        }
        
            /*when first sub is found in root*/
        int first_logical_sector = (p[rootstart + i*32 + 26] & 0x00FF) + ((p[rootstart + i*32 + 27] & 0x00FF) << 8);
            
        
        /* go through the rest of the sub, if any */
        
        for(int i=1;i<number_of_sub;i++){   //loop for the number of subs
            int j;
            //get next dir name
            for(int s = 0;s<strlen(subdirectories[i]);s++){
                if(subdirectories[i][s] != ' '){
                    if(subdirectories[i][s] != '\0'){
                        dir_name[s] = subdirectories[i][s];
                    }else{
                        dir_name[s] = ' ';
                    }
                }

                for(int s = strlen(subdirectories[i]); s< 9;s++){
                    dir_name[s] = ' ';
                }
            }
            dir_name[8] = '\0';
            for(j = 0;j<16;j++){    //go through the 16 entries in this section
                file_name = p + (33 + first_logical_sector-2)*512 + j*32;
                attr = p[(33 + first_logical_sector-2)*512 + j*32 + 11];
                
                //if rest is empty
                if(p[(33+first_logical_sector-2)*512 + j*32] == 0x00){
                    create_path(p, (33+first_logical_sector-2)*512+j*32 ,subdirectories,i,index-1);
                    
                    break;
                }
                
                if( strncmp(dir_name,file_name,8) != 0 ){    //if name doesnt match, continue
                    continue;
                }else if( (attr&0x10) != 0x10 ){  //if it's not a subdirectory,continue
                    continue;
                }
                // next sub has been found
                break;
            }
            
            first_logical_sector = (p[(33+first_logical_sector-2)*512 + j*32 + 26] & 0x00FF) + ((p[(33+first_logical_sector-2)*512 + j*32 + 27] & 0x00FF) << 8);
        }
       
        
        //at this point, first_logical_sector points to the folder where we'll store the file
        int next_free = getFreeFAT(p);
        int base = 512*(first_logical_sector+33-2);
        for(int i = 0;i<16;i++){
            char *nameByte = p + base +(i*32);
            int attr = p[base + 11 + (i*32)];
            /*find empty FAT entry and insert file description*/
            if( !strcmp(nameByte,"") && !attr ){
                
                /*copy file_name*/
                strncpy(nameByte,input_file_name,8);
                
                /*copy extension*/
                strncpy(nameByte + 8, extension, 3);
                
                /*copy file creation/modified datatime*/
                time_t *time = &input_stat.st_ctime;
                struct tm* input_time = localtime(time);
                
                int year = ((input_time->tm_year - 80) << 9) & 0xFE00;
                int month = ((input_time->tm_mon + 1) << 5) & 0x01E0;   //tm_mon is (0-11)
                int day = input_time->tm_mday & 0x001F;
                int hour = ((input_time->tm_hour) << 11) & 0xFE00; //hour -> high 5 bits
                int minute = ((input_time->tm_min) << 5) & 0x07E0; //minute -> middle 6 bits
                int second = (input_time->tm_sec) & 0x001F; //sec -> low 5 bits
               
                //store file date/time
                p[base + i*32 + 14] = ( hour | minute | second) & 0xFF;
                p[base + i*32 + 15] = ((hour | minute | second)>>8) & 0xFF;
                p[base + i*32 + 16] = ( year | month | day) & 0xFF;
                p[base + i*32 + 17] = ((year | month | day)>>8) & 0xFF;
                
                /*copy first_logical_sector*/
                p[base+i*32+26] = next_free & 0xFF;
                p[base+i*32+26+1] = (next_free >>8 )& 0xFF;
                
                /*copy file_size*/
                p[base+(i*32)+28] = input_size & 0xFF;
                p[base+(i*32)+28+1] = (input_size >> 8) & 0xFF;
                p[base+(i*32)+28+2] = (input_size >> 16) & 0xFF;
                p[base+(i*32)+28+3] = (input_size >> 24) & 0xFF;
                break;
            }
        }
            /*write to the disk*/
            int bytes_left = input_size;
            int bytes_to_write;
            long int input_file_counter = 0;
            
            do{
                int current = next_free;
                /*check the bytes to write to this sector*/
                if(bytes_left>512){
                    bytes_to_write = 512;
                    next_free = getFreeFAT(p);
                }else{
                    bytes_to_write = bytes_left;
                    next_free = 0xFFF;
                }
                setEntryValue(p,current,next_free);
                //write to this sector
                for(int i = 0;i<bytes_to_write;i++){
                    p[(31+current)*512+i] = input[i+input_file_counter*512];

                }
                bytes_left -= bytes_to_write;
                input_file_counter += 1;
            }while(bytes_left>0);
        
        
        
        
        
        
        
        
        
        
        return;
    }
    
    //if the path only contains a single file, code below will run
    int next_free = getFreeFAT(p);
    for(int i = 0;i<14*16;i++){
        char *nameByte = p + rootstart +(i*32);
        int attr = p[rootstart + 11 + (i*32)];
        /*find empty FAT entry and insert file description*/
        if( !strcmp(nameByte,"") && !attr ){
        
            /*copy file_name*/
            strncpy(nameByte,input_file_name,8);
            
            /*copy extension*/
            strncpy(nameByte + 8, extension, 3);
            
            
            
            
            /*copy file creation/modified datatime*/
            time_t *time = &input_stat.st_ctime;
            struct tm* input_time = localtime(time);
            
            int year = ((input_time->tm_year - 80) << 9) & 0xFE00;
            int month = ((input_time->tm_mon + 1) << 5) & 0x01E0;   //tm_mon is (0-11)
            int day = input_time->tm_mday & 0x001F;
            int hour = ((input_time->tm_hour) << 11) & 0xFE00; //hour -> high 5 bits
            int minute = ((input_time->tm_min) << 5) & 0x07E0; //minute -> middle 6 bits
            int second = (input_time->tm_sec) & 0x001F; //sec -> low 5 bits
           
            //store file date/time
            p[rootstart + i*32 + 14] = ( hour | minute | second) & 0xFF;
            p[rootstart + i*32 + 15] = ((hour | minute | second)>>8) & 0xFF;
            p[rootstart + i*32 + 16] = ( year | month | day) & 0xFF;
            p[rootstart + i*32 + 17] = ((year | month | day)>>8) & 0xFF;
            
            
            /*copy first_logical_sector*/
            p[rootstart+i*32+26] = next_free & 0xFF;
            p[rootstart+i*32+26+1] = (next_free >>8 )& 0xFF;
            /*copy file_size*/
            p[rootstart+(i*32)+28] = input_size & 0xFF;
            p[rootstart+(i*32)+28+1] = (input_size >> 8) & 0xFF;
            p[rootstart+(i*32)+28+2] = (input_size >> 16) & 0xFF;
            p[rootstart+(i*32)+28+3] = (input_size >> 24) & 0xFF;
            break;
        }
    }
   
        /*write to the disk*/
        int bytes_left = input_size;
        int bytes_to_write;
        long int input_file_counter = 0;
        
        do{
            int current = next_free;
            /*check the bytes to write to this sector*/
            if(bytes_left>512){
                bytes_to_write = 512;
                next_free = getFreeFAT(p);
            }else{
                bytes_to_write = bytes_left;
                next_free = 0xFFF;
            }
            setEntryValue(p,current,next_free);
            //write to this sector
            for(int i = 0;i<bytes_to_write;i++){
                p[(31+current)*512+i] = input[i+input_file_counter*512];

            }
            bytes_left -= bytes_to_write;
            input_file_counter += 1;
        }while(bytes_left>0);
    
    return;
}



int main (int argc, char** argv){
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
       
        diskput(p,argv[2]);
   
    
        close(fd);
        return 0;
    

}
