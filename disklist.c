#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry


void print_sub(char* p, int entry,char* file_path);



void print_date_time(char * directory_entry_startPos){  //given by TA
    
    int time, date;
    int hours, minutes, day, month, year;
    
    time = *(unsigned short *)(directory_entry_startPos + timeOffset);
    date = *(unsigned short *)(directory_entry_startPos + dateOffset);
    
    //the year is stored as a value since 1980
    //the year is stored in the high seven bits
    year = ((date & 0xFE00) >> 9) + 1980;
    //the month is stored in the middle four bits
    month = (date & 0x1E0) >> 5;
    //the day is stored in the low five bits
    day = (date & 0x1F);
    
    printf("%d-%02d-%02d ", year, month, day);
    //the hours are stored in the high five bits
    hours = (time & 0xF800) >> 11;
    //the minutes are stored in the middle 6 bits
    minutes = (time & 0x7E0) >> 5;
    
    printf("%02d:%02d\n", hours, minutes);
    
    return ;
}
//print files in subdirectories
void print_sub(char* p, int entry,char* file_path){
    int base = 512*(33+entry-2);
    char* file_name = calloc(1, sizeof(char)*9); //Times 9 because 8 bytes + null
    char* ext = calloc(1, sizeof(char)*9);; // Times 4 because 3 bytes + null
    int nameByte1 = p[base];
    int attr;       //attribute byte
    char type;
    int filesize;
    int subdirectories[16];
    int index = 0;
    for(int i=0;i<16;i++){
        nameByte1 = p[base+i*32];
        attr = p[base+11+i*32];
        if(nameByte1==0x00){
            break;
        
        }else if(nameByte1 == 0xE5 || (attr&0x0E)!=0 ||attr == 0x0F ||nameByte1 == '.'){// Check if not empty && not a volume label && not a long file_name
            continue;
        }
        if (attr == 0x10){
            subdirectories[index] = i;
            index++;
            type = 'D';
        }else{
            type = 'F';
        }
        for(int j=0;j<8;j++){
            if(p[base + i*32 + j] == ' '){
                file_name[j] = '\0';
                break;
            }else{
                file_name[j] = p[base + i*32 + j];
            }
        }
        for (int j = 8; j < 11; j++){
            ext[j- 8] = p[base+i*32 + j];
        }
        file_name[8] = '\0';
        ext[3] = '\0';

        int file_sizeByte = base + i*32 + 28;
        filesize = (p[file_sizeByte] & 0x00FF) + ((p[file_sizeByte+1] & 0x00FF) << 8) + ((p[file_sizeByte+2] &0x00FF) << 16)+((p[file_sizeByte+3] & 0x00FF) << 24);
        /* output the description of file/dir */
        printf ("%c %10d ", type,filesize);
        if(type=='F'){
            printf ("%16s.", file_name);
            printf ("%s ", ext);

        }else{
            printf ("%20s ", file_name);
        }
       
        print_date_time(p+ base+i*32);
        
    }
    /*call this function for any subdirectories in this directory*/
    for (int i=0;i<index;i++){
        char* new_file_path = calloc(1, sizeof(char)*9+strlen(file_path));
        strcat(new_file_path,file_path);
        strcat(new_file_path,"/");
        int temp = subdirectories[i];
        int first_logical_cluster;
        //cat filename to path
        for(int j=0;j<8;j++){
            if(p[base + temp*32 + j] == ' '){
                file_name[j] = '\0';
                break;
            }else{
                file_name[j] = p[base + temp*32 + j];
            }
        }
        file_name[8] = '\0';
        strcat(new_file_path,file_name);
        first_logical_cluster = (p[base + temp*32 + 26] & 0x00FF) + ((p[base + temp*32 + 27] & 0x00FF) << 8);

        printf("/%s\n==================\n",new_file_path);
        print_sub(p,first_logical_cluster,new_file_path);
        
    }
     
}


void disklist (char* p)
{
    
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
    //Variables that need to be printed out
    char type;
    char* file_name = calloc(1, sizeof(char)*9); //Times 9 because 8 bytes + null
    char* ext = calloc(1, sizeof(char)*9);; // Times 4 because 3 bytes + null
    int filesize;
    
    
    nameByte1 = p[rootStart];
    printf("Root\n==================\n");
    /*output files/dir in root dir*/
    while(nameByte1 != 0x00 && counter<14*16)
    {
        counter++;
        attr = p[rootStart+11+counter*32];
        nameByte1 = p[rootStart+counter*32+32];
        if(nameByte1 != 0xE5 &&attr != 0x08 &&attr != 0x0F){ // Check if not empty && not a volume label && not a long file_name
            
            
            
            /*check for file type*/
            if (attr == 0x10){
                subdirectories[index] = counter;
                index++;
                type = 'D';
            }else{

                type = 'F';
            }
            
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
            for (int i = 8; i < 11; i++){
                ext[i - 8] = p[rootStart+counter*32 + i];
            }
            file_name[8] = '\0';
            ext[3] = '\0';
            
            
            /*read file_size*/
            int file_sizeByte = rootStart + counter*32 + 28;
            filesize = (p[file_sizeByte] & 0x00FF) + ((p[file_sizeByte+1] & 0x00FF) << 8) + ((p[file_sizeByte+2] &0x00FF) << 16)+((p[file_sizeByte+3] & 0x00FF) << 24);
            printf ("%c %10d ", type,filesize);
            if(type=='F'){
                printf ("%16s.", file_name);
                printf ("%s ", ext);

            }else{
                printf ("%20s ", file_name);
            }
           
            print_date_time(p+ rootStart+counter*32);
        }
    }
    /*call this function for any subdirectories in this directory*/
    for(int j=0;j<index;j++){
        counter = subdirectories[j];
        for(int i=0;i<8;i++){
            if(p[rootStart + counter*32 + i] == ' '){
                file_name[i] = '\0';
                break;
            }else{
                file_name[i] = p[rootStart + counter*32 + i];
            }
        }
        int first_logical_cluster = (p[rootStart + counter*32 + 26] & 0x00FF) + ((p[rootStart + counter*32 + 27] & 0x00FF) << 8);
        
        printf("/%s\n==================\n",file_name);
        print_sub(p,first_logical_cluster,file_name);
    }
    free(file_name);
    free(ext);
}


int main (int argc, char** argv)
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
   
    disklist(p);
  
    close(fd);
    return 0;
}


