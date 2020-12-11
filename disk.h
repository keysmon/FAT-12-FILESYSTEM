#define BYTES_PER_SECTOR 512

#define FAT_OFFSET 0x200
#define ROOT_SECTOR 19
#define ROOT_DIR_ENTRIES 224
#define ROOT_ENTRY_SIZE 32

#define OS_OFFSET 3
#define NUM_FAT_OFFSET 16
#define SECTORS_PER_FAT_OFFSET 22
#define LABEL_OFFSET 43

/* --- Function Prototypes --- */
int getTotalSize(char *src);
int getFreeSize(char *src);
int getSectorValue(int entry, char *src);


int getSectorValue(int entry, char *src) {
    int base = FAT_OFFSET;
    int buffer[2];

    //if even
    if (entry%2 == 0) {
        //low 4 bits
        buffer[0] = src[base+1 + ((3*entry)/2)] & 0x0F;
        //full 8 bits
        buffer[1] = src[base + ((3*entry)/2)] & 0xFF;

        return (buffer[0] << 8) | (buffer[1]);

    //if odd
    } else {
        //high 4 bits
        buffer[0] = src[base + ((3*entry)/2)] & 0xF0;
        //full 8 bits
        buffer[1] = src[base+1 + ((3*entry)/2)] & 0xFF;

        return (buffer[0] >> 4) | (buffer[1] << 4);

    }
}

int getTotalSize(char *src) {
    unsigned int bytes[2];
    int size = 0;

    // total size at offset 19 in Boot Sector
    bytes[0] = src[19];
    bytes[1] = src[20];

    size += (bytes[0] & 0xFF) | ((bytes[1] & 0xFF) << 8);

    return size*BYTES_PER_SECTOR;
}

int getFreeSize(char *src) {
    int i;
    int freeCount = 0;
    int numSectors = getTotalSize(src)/BYTES_PER_SECTOR;

    // look for emtry sectors in FAT
    for (i = 2; i < numSectors; i++) {
        if (getSectorValue(i, src) == 0x000) {
            freeCount++;
        }
    }

    return freeCount*BYTES_PER_SECTOR;
}
