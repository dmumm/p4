#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <stdint.h> // byte, uint16_t, uint32_t
#include <stdio.h> // printf, FILE, fopen, fclose
#include <string.h> // strcat
#include <ctype.h> // toupper
#include <limits.h> //
#include <assert.h> // assert

#define NULL_CHAR '\0'
#define SPACE_CHAR ' '
#define SPACE_HEX 0x20
#define DOT_CHAR 0x2E
#define DOUBLE_DOT_CHAR 0x2E
#define FORWARD_SLASH_CHAR 0x2F
#define FORWARD_SLASH_STRING "/"




#define ZERO 0
#define DIRECTORY_FILE_SIZE ZERO
#define BITS_PER_BYTE 8
#define BYTES_PER_SECTOR 512

#define BOOT_SECTOR_START 0
#define FAT1_SECTOR_START 1
#define FAT2_SECTOR_START 10
#define ROOT_SECTOR_START 19
#define DATA_SECTOR_START 33

#define BOOT_SECTOR_LENGTH 1
#define FAT_SECTOR_LENGTH 9
#define ROOT_SECTOR_LENGTH 14
#define DATA_SECTOR_LENGTH 2847

#define CLUSTER_EMPTY 0x000
#define CLUSTER_ROOT 0x000
#define CLUSTER_NONEXISTENT 0x001
#define CLUSTER_BAD 0xFF7

#define CLUSTER_NORMAL_MIN 0x002
#define CLUSTER_NORMAL_MAX 0xFF6

#define CLUSTER_RESERVED_MIN 0xFF0
#define CLUSTER_RESERVED_MAX 0xFF6

#define CLUSTER_LAST_MIN 0xFF8
#define CLUSTER_LAST_MAX 0xFFF

#define BOOT_BYTES_PER_SECTOR_OFFSET1 11
#define BOOT_BYTES_PER_SECTOR_OFFSET2 12
#define BOOT_SECTORS_PER_CLUSTER_OFFSET 13

#define BOOT_FAT_COUNT_OFFSET 16

#define BOOT_RD_ENTRY_COUNT_OFFSET1 17
#define BOOT_RD_ENTRY_COUNT_OFFSET2 18

#define BOOT_SECTORS_IN_DISK_OFFSET1 19
#define BOOT_SECTORS_IN_DISK_OFFSET2 20

#define BOOT_SECTORS_PER_FAT_OFFSET1 22
#define BOOT_SECTORS_PER_FAT_OFFSET2 23

#define BOOT_SIGNATURE_OFFSET1 510
#define BOOT_SIGNATURE_CONSTANT1 0x55
#define BOOT_SIGNATURE_OFFSET2 511
#define BOOT_SIGNATURE_CONSTANT2 0xAA

// #define ROOT_OFFSET 0x2600
#define DATA_AREA_OFFSET 0x4000

#define SECTOR_SIZE 512
#define ENTRY_SIZE 32

#define ENTRY_FILENAME_OFFSET 0
#define ENTRY_FILENAME_BYTES 8
#define ENTRY_EXTENSION_OFFSET 8
#define ENTRY_EXTENSION_BYTES 3
#define ENTRY_ATTRIBUTES_OFFSET 11
#define ENTRY_ATTRIBUTES_BYTES 1
#define ENTRY_RESERVED_OFFSET 12
#define ENTRY_RESERVED_BYTES 2

#define ENTRY_TIME_CREATED_OFFSET 14
#define ENTRY_DATE_CREATED_OFFSET 16
#define ENTRY_LAST_ACCESSED_OFFSET 18
#define ENTRY_TIME_MODIFIED_OFFSET 22
#define ENTRY_DATE_MODIFIED_OFFSET 24
#define ENTRY_DATETIME_BYTES 2

#define ENTRY_FIRST_CLUSTER_OFFSET1 26
#define ENTRY_FIRST_CLUSTER_OFFSET2 27
#define ENTRY_FIRST_CLUSTER_BYTES 2
#define ENTRY_SIZE_OFFSET 28
#define ENTRY_SIZE_BYTES 4

#define ENTRY_FREE 0xE5
#define ENTRY_FREE_AND_LAST 0x00
#define ENTRY_NO_EXTENSION '\0'

#define FILENAME_ENDED 0x00
#define FILENAME_DELETED 0xE5
#define FILENAME_SELF_DIRECTORY DOT_CHAR
#define FILENAME_PARENT_DIRECTORY DOUBLE_DOT_CHAR


#define DELETED_ENTRY_CLUSTER_LABEL 0x000


#define DIR_HANDLE_OFFSET 64 //TODO: figure out what this is
#define FIRST_DATA_SECTOR_NUM 33 //TODO: figure out what this is

#define ATTR_NULL        0x00  // Binary: 00000000
#define ATTR_READ_ONLY   0x01  // Binary: 00000001
#define ATTR_HIDDEN      0x02  // Binary: 00000010
#define ATTR_SYSTEM      0x04  // Binary: 00000100
#define ATTR_VOLUME_LABEL 0x08  // Binary: 00001000
#define ATTR_DIRECTORY   0x10  // Binary: 00010000
#define ATTR_ARCHIVE     0x20  // Binary: 00100000
#define ATTR_ROOT     0x80  // Binary: 10000000




typedef uint8_t byte;
typedef byte * byte_ptr;
typedef size_t byte_num;
typedef size_t byte_count;
typedef size_t bit_count;
typedef size_t sector_num;
typedef size_t cluster_num;
typedef size_t entry_num;

typedef enum { false, true } bool;
typedef byte sector[BYTES_PER_SECTOR];
typedef unsigned char * string;
typedef uint16_t fat_entry;


typedef struct TimeStamp {
    uint16_t created;
    uint16_t accessed;
    uint16_t modified;
} TimeStamp;

typedef struct Entry {
    string filepath;
    byte  filename[8];
    byte  extension[3];
    byte  attributes;
    // byte  reserved[10];
    size_t depth;
    fat_entry first_cluster;
    uint32_t size;
    TimeStamp date;
    TimeStamp time;
    string data;
    bool deleted;
} Entry;

typedef struct BootSector {
    size_t n_Fats;
    size_t n_RootEntries;
    size_t n_Sectors;
    size_t n_SectorsPerFat;
    size_t n_BytesPerSector;
    size_t n_SectorsPerCluster;
} BootSector;

typedef struct DiskImage {
    BootSector * p_BootSector;
    byte_ptr * p_FatTables;
    byte_ptr p_Root;
    Entry * p_RootEntry;
    byte_ptr p_DataArea;
    size_t bytes;
} DiskImage;


// Directory * g_pD_Root;
string g_ImageData;
DiskImage * g_Disk;

void observeAndReport(bool a_Condition, string a_Message);
bool testPointer(string a_Ptr, byte_count a_Length);

bool printBinary(uint64_t a_Number, bit_count bits, bool use_Prefix);
bool printEntry(const Entry * const e);
void printData(byte_ptr const a_pB_Data, byte_num a_n_Length);

int readSector(int fd, sector_num a_sector, sector * buffer);
fat_entry get_fat_entry(sector * fat_sector, entry_num entry_number);

string formatFileNaming(byte_ptr a_pB_Data, size_t a_Length);

string openFile(string a_Filename);
byte_ptr getBootSector(string a_pB_Data);
void parseFileSystem(string a_pB_Data);
void handleDirectory(Entry * a_ParentEntry, byte_ptr a_sector, size_t depth, bool a_ParentDeleted);
void makeData(Entry * a_Entry, string a_pDiskSector);

Entry * generateEntry(Entry * a_ParentEntry, byte_ptr a_byteLocation, size_t depth, bool a_ParentDeleted);


bool isReadOnly(const Entry * const e);
bool isHidden(const Entry * const e);
bool isSystem(const Entry * const e);
bool isVolumeLabel(const Entry * const e);
bool isDirectory(const Entry * const e);
bool isArchive(const Entry * const e);

/**
 * @brief Takes a file system image and outputs the files and directories
*/
int main(int argc, char * argv[])
{
    // Validate command line arguments
    if (argc != 3) {
        printf("Usage: ./notjustcats <disk_image_filename> <output_directory_path>\n");
        exit(EXIT_FAILURE);
    }

    char * pc_ImagePath = argv[1];
    char * pc_OutputDirectoryName = argv[2];

    fprintf(stderr, "Image file: %s\n", pc_ImagePath);
    fprintf(stderr, "Output directory: %s\n", pc_OutputDirectoryName);

    g_Disk = (DiskImage *)malloc(sizeof(DiskImage));
    observeAndReport(g_Disk != NULL, "DiskImage is null");

    g_Disk->p_Root = (byte_ptr)malloc(sizeof(byte));
    observeAndReport(g_Disk->p_Root != NULL, "p_Root is null");

    g_ImageData = openFile(pc_ImagePath);

    parseFileSystem(g_ImageData);

    // // Output
    // printDirectory(g_pD_Root->head);

    // // Write output to pc_OutputDirectoryName
    // writeOutput(pc_OutputDirectoryName);

    return(EXIT_SUCCESS);
}

/**
 * @brief Custom assert function
 * @param a_Condition   The condition to check
 * @param a_Message     The message to print if the condition is false
*/
void observeAndReport(bool a_Condition, string a_Message)
{
    if (a_Condition) return;
    fprintf(stderr, "Assertion failed: %s\n", a_Message);
    exit(EXIT_FAILURE);
}

/**
 * @brief   Tests a pointer to see if it is null, and if not, prints the data
 * @param ptr       The pointer to test
 * @param length    The length of the data
 *
*/
bool testPointer(string ptr, byte_count length)
{
    if (ptr == NULL) {
        printf("Pointer is NULL\n");
        return 0;
    }
    printf("Pointer is not NULL, testing access...\n");

    for (size_t i = 0; i < length; ++i)
    {
        printf("byte %zu: 0x%02x\n", i, ptr[i]);
    }
    return true;
}

/**
 * @brief   Opens a file and returns a pointer to the data
 * @param a_Filename    The name of the file to open
 * @return  A pointer to a dynamically allocated array of bytes containing the data
 */
string openFile(string a_Filename)
{
    FILE * pF_Input = fopen(a_Filename, "rb");
    observeAndReport(pF_Input != NULL, "Error opening file");

    fseek(pF_Input, ZERO, SEEK_END);
    long l_FileSize = ftell(pF_Input);
    rewind(pF_Input);

    string pData = (string)malloc(l_FileSize);
    observeAndReport(pData != NULL, "Error allocating memory for file");

    size_t readSize = fread(pData, sizeof(byte), l_FileSize, pF_Input);
    observeAndReport(readSize == l_FileSize, "Error reading file");
    fprintf(stderr, "Read file of size %zu\n", readSize);

    g_Disk->bytes = readSize;

    fclose(pF_Input);
    return pData;
}

/**
 * @brief   Prints the data in a file
 * @param a_pB_Data     The data to print
 * @param a_n_Length    The length of the data
*/
void printData(byte_ptr a_pB_Data, byte_num a_n_Length)
{

    for (byte_num i = 0; i < a_n_Length; i++)
    {
        printf("%02X ", a_pB_Data[i]);
        if (i % 16 == 15) printf("\n"); // Print a new line every 16 bytes //TODO: magic number
    }
    printf("\n");
}

/**
 * @brief   Formats file name or extension to be uppercase and space padded
 * @param a_byteLocation     The data to format, non-null terminated
 * @param a_Length      The length of the data
 * @return  The formatted data
 */
string formatFileNaming(byte_ptr a_byteLocation, byte_num a_Length)
{
    observeAndReport(a_byteLocation != NULL, "Error: a_byteLocation is null");
    observeAndReport(a_Length == ENTRY_FILENAME_BYTES || a_Length == ENTRY_EXTENSION_BYTES, "Error: a_Length is not 8 or 3");
    observeAndReport(a_byteLocation != DOT_CHAR, "Error: a_byteLocation is a dot");

    string formatted = (string)malloc(a_Length);
    observeAndReport(formatted != NULL, "Error allocating memory for formatted file name");

    size_t i;
    for (i = 0; i < a_Length; i++)
    {
        if (a_byteLocation + i == SPACE_CHAR) { break; }
        formatted[i] = toupper((unsigned char)a_byteLocation[i]); // might be trouble, was (unsigned char)
    }

    // Pad with spaces if i < a_Length
    for (; i < a_Length; i++)
    {
        formatted[i] = SPACE_CHAR;
    }

    return formatted;
}


/**
 * Combines a big byte and a little byte to form a 16-bit integer.
 *
 * @param bigByte The big byte (bigger 8 bits).
 * @param littleByte The little byte (littleer 8 bits).
 * @return The combined 16-bit integer.
 */
uint16_t combineTwoBytes(byte bigByte, byte littleByte)
{
    return (uint16_t)(bigByte << 8) | littleByte;
}

/**
 * @brief   Gets the boot sector from the data
 * @param a_pB_Data The data to get the boot sector from
 * @return  A pointer to the boot sector
 */
byte_ptr getBootSector(string a_pB_Data)
{
    bool check1 = a_pB_Data[BOOT_SIGNATURE_OFFSET1] == BOOT_SIGNATURE_CONSTANT1;
    bool check2 = a_pB_Data[BOOT_SIGNATURE_OFFSET2] == BOOT_SIGNATURE_CONSTANT2;
    fprintf(stderr, "Boot Signature Byte 1 is %02X\n", a_pB_Data[BOOT_SIGNATURE_OFFSET1]);
    fprintf(stderr, "Boot Signature Byte 2 is %02X\n", a_pB_Data[BOOT_SIGNATURE_OFFSET2]);
    observeAndReport(check1, "Error: Boot Signature Byte 1 is not 0x55");
    observeAndReport(check2, "Error: Boot Signature Byte 2 is not 0xAA");


    byte const bytes_per_sector_big = a_pB_Data[BOOT_BYTES_PER_SECTOR_OFFSET2];
    byte const bytes_per_sector_lil = a_pB_Data[BOOT_BYTES_PER_SECTOR_OFFSET1];
    size_t const bytes_per_sector = combineTwoBytes(bytes_per_sector_big, bytes_per_sector_lil);

    size_t const sectors_per_cluster = a_pB_Data[BOOT_SECTORS_PER_CLUSTER_OFFSET];

    size_t const fat_count = a_pB_Data[BOOT_FAT_COUNT_OFFSET];

    byte const rd_entry_count_big = a_pB_Data[BOOT_RD_ENTRY_COUNT_OFFSET2];
    byte const rd_entry_count_lil = a_pB_Data[BOOT_RD_ENTRY_COUNT_OFFSET1];
    size_t const rd_entry_count = combineTwoBytes(rd_entry_count_big, rd_entry_count_lil);

    byte const sectors_in_disk_big = a_pB_Data[BOOT_SECTORS_IN_DISK_OFFSET2];
    byte const sectors_in_disk_lil = a_pB_Data[BOOT_SECTORS_IN_DISK_OFFSET1];
    size_t const sectors_in_disk = combineTwoBytes(sectors_in_disk_big, sectors_in_disk_lil);

    byte const sectors_per_fat_big = a_pB_Data[BOOT_SECTORS_PER_FAT_OFFSET2];
    byte const sectors_per_fat_lil = a_pB_Data[BOOT_SECTORS_PER_FAT_OFFSET1];
    size_t const sectors_per_fat = combineTwoBytes(sectors_per_fat_big, sectors_per_fat_lil);

    g_Disk->p_BootSector = (BootSector *)malloc(sizeof(BootSector));
    observeAndReport(g_Disk->p_BootSector != NULL, "Error allocating memory for boot sector");

    g_Disk->p_BootSector->n_Fats = fat_count;
    g_Disk->p_BootSector->n_RootEntries = rd_entry_count;
    g_Disk->p_BootSector->n_Sectors = sectors_in_disk;
    g_Disk->p_BootSector->n_SectorsPerFat = sectors_per_fat;
    g_Disk->p_BootSector->n_BytesPerSector = bytes_per_sector;
    g_Disk->p_BootSector->n_SectorsPerCluster = sectors_per_cluster;

    return (byte_ptr)(g_Disk->p_BootSector);
}

/**
 * @brief   Locates and parses the root directory's entries
 * @param a_pB_Data The data to parse`
 */
void parseFileSystem(string a_pB_Data)
{
    byte_num const root_offset = ROOT_SECTOR_START * SECTOR_SIZE;
    size_t depth = ZERO;

    getBootSector(g_ImageData);
    observeAndReport(g_Disk->p_BootSector != NULL, "Error: g_Disk->p_BootSector is null");

    g_Disk->p_RootEntry = (Entry *)malloc(sizeof(Entry));
    observeAndReport(g_Disk->p_RootEntry != NULL, "Error allocating memory for root directory entry");

    memset(g_Disk->p_RootEntry->filename, NULL, ENTRY_FILENAME_BYTES);
    memset(g_Disk->p_RootEntry->extension, NULL, ENTRY_EXTENSION_BYTES);
    g_Disk->p_RootEntry->filepath = strdup(FORWARD_SLASH_STRING);
    g_Disk->p_RootEntry->depth = depth;
    g_Disk->p_RootEntry->first_cluster = CLUSTER_ROOT;
    g_Disk->p_RootEntry->size = DIRECTORY_FILE_SIZE;
    g_Disk->p_RootEntry->deleted = false;
    g_Disk->p_RootEntry->attributes = ATTR_ROOT | ATTR_DIRECTORY | ATTR_SYSTEM;
    g_Disk->p_RootEntry->data = NULL;

    g_Disk->p_Root = a_pB_Data + root_offset;
    byte_ptr i_child = g_Disk->p_Root;

    handleDirectory(g_Disk->p_RootEntry, i_child, depth, false);

    // while (i_child != ENTRY_FREE_AND_LAST)
    // {
    //     Entry * i_childEntry = generateEntry(g_Disk->p_RootEntry, i_child, depth, false);
    //     observeAndReport(i_childEntry != NULL, "Error generating entry");
    //     printEntry(i_childEntry);
    //
    //     strcat(i_childEntry->filepath, ".");
    //     strcat(i_childEntry->filepath, i_childEntry->extension);
    //
    //     sector_num first_data_sector_num = FIRST_DATA_SECTOR_NUM + i_childEntry->first_cluster - CLUSTER_NORMAL_MIN;
    //     byte_num first_data_sector_offset = first_data_sector_num * SECTOR_SIZE;
    //     byte_ptr first_data_sector = a_pB_Data + first_data_sector_offset;
    //
    //     // Detect if i_childEntry is a directory
    //     if (isDirectory(i_childEntry))
    //     {
    //         handleDirectory(i_childEntry, first_data_sector, depth, false);
    //         i_child += ENTRY_SIZE;
    //         continue;
    //     }
    //
    //     makeData(i_childEntry, first_data_sector);
    //     i_child += ENTRY_SIZE;
    // }
}




/**
 * @brief   Handles a directory
 * @param a_ParentEntry  The directory entry to handle
 * @param a_sector           The sector to handle
 */
void handleDirectory(Entry * a_ParentEntry, byte_ptr a_dataSector, size_t depth, bool parentDeleted)
{
    depth++;
    size_t entry_count = ZERO;
    byte_ptr i_child = a_dataSector;
    while (depth <= 1 && entry_count < g_Disk->p_BootSector->n_RootEntries)
    {
        Entry * i_childEntry = generateEntry(a_ParentEntry, i_child, depth, parentDeleted);

        if (i_childEntry->filename == ENTRY_FREE_AND_LAST)
        {
            free(i_childEntry->filepath);
            free(i_childEntry->filename);
            free(i_childEntry->extension);
            free(i_childEntry->filepath);
            free(i_childEntry);
            break;
        }

        strcat(i_childEntry->filepath, a_ParentEntry->filepath);

        sector_num i_first_sector_num = FIRST_DATA_SECTOR_NUM + i_childEntry->first_cluster - CLUSTER_NORMAL_MIN;
        byte_num i_first_sector_offset = i_first_sector_num * SECTOR_SIZE;
        byte_ptr i_first_sector = a_dataSector + i_first_sector_offset;

        if (isDirectory(i_childEntry))
        {
            handleDirectory(i_childEntry, i_first_sector, depth, false);
            entry_count++;
            i_childEntry += ENTRY_SIZE;
            continue;
        }

        makeData(i_childEntry, i_first_sector);
        i_child += ENTRY_SIZE;
        entry_count++;
        printEntry(i_childEntry);
    }
}

/**
 * @brief   Generates an entry from a byte location
 * @param a_ParentEntry  The parent entry
 * @param a_byteLocation The byte location to generate the entry from
 * @param depth          The depth of the entry
 * @param a_ParentDeleted Whether or not the parent entry is deleted
 * @return  The generated entry

*/
Entry * generateEntry(Entry * a_ParentEntry, byte_ptr a_byteLocation, size_t depth, bool a_ParentDeleted)
{
    observeAndReport(a_byteLocation != NULL, "Error: a_byteLocation is null");

    byte_ptr first_cluster_bigbyte = a_byteLocation + ENTRY_FIRST_CLUSTER_OFFSET2;
    byte_ptr first_cluster_littlebyte = a_byteLocation + ENTRY_FIRST_CLUSTER_OFFSET1;
    byte attributes = ATTR_NULL | a_byteLocation[ENTRY_ATTRIBUTES_OFFSET];
    printBinary(attributes, BITS_PER_BYTE, true);

    string formattedName = formatFileNaming(a_byteLocation, ENTRY_FILENAME_BYTES);
    string formattedExtension = formatFileNaming(a_byteLocation + ENTRY_EXTENSION_OFFSET, ENTRY_EXTENSION_BYTES);

    Entry * e = (Entry *)malloc(sizeof(Entry));
    observeAndReport(e != NULL, "Error allocating memory for entry");

    e->attributes = attributes;
    e->filepath = strdup(a_ParentEntry->filepath);
    e->first_cluster = combineTwoBytes(first_cluster_bigbyte, first_cluster_littlebyte);
    e->size = combineTwoBytes(a_byteLocation + ENTRY_FIRST_CLUSTER_OFFSET2, a_byteLocation + ENTRY_FIRST_CLUSTER_OFFSET1);
    e->depth = depth;
    e->deleted = a_ParentDeleted;
    memset(e->filename, NULL_CHAR, ENTRY_FILENAME_BYTES);
    memset(e->extension, ENTRY_NO_EXTENSION, ENTRY_EXTENSION_BYTES);
    printBinary(e->attributes, BITS_PER_BYTE, true);



    if (formattedName == DOUBLE_DOT_CHAR) { return e; } //TODO: wtf do i put

    // strcat(e->filepath, FORWARD_SLASH_STRING);

    if (formattedName == DOT_CHAR)
    {
        strcat(e->filepath, formattedName);
        return e;
    }

    if (formattedName == ENTRY_FREE || formattedName == ENTRY_FREE_AND_LAST)
    {
        e->deleted = true;
    }

    strncpy(e->filename, formattedName, ENTRY_FILENAME_BYTES);
    //e->filename = formattedName;
    strcat(e->filepath, formattedName);
    strncpy(e->extension, formattedExtension, ENTRY_EXTENSION_BYTES);

    if (e->extension != ENTRY_NO_EXTENSION) {
        strcat(e->filepath, ".");
        strncat(e->filepath, e->extension, ENTRY_EXTENSION_BYTES);
    }

    return e;
}

/**
 * @brief   Makes the data for a file
 * @param a_Entry       The entry to make the data for
 * @param a_pDiskSector The disk sector to make the data from
 */
void makeData(Entry * a_Entry, byte_ptr a_pDiskSector)
{
    fprintf(stderr, "Making data\n");
    observeAndReport(a_Entry != NULL, "Error: a_Entry is null");
    observeAndReport(a_pDiskSector != NULL, "Error: a_pDiskSector is null");

    // Allocate memory for data
    // a_Entry->size = a_pDiskSector[28] | a_pDiskSector[29] << 8 | a_pDiskSector[30] << 16 | a_pDiskSector[31] << 24;
    fprintf(stderr, "Allocated memory for data\n");

    // Allocate memory for data
    a_Entry->data = (string)malloc(SECTOR_SIZE);
    observeAndReport(a_Entry->data != NULL, "Error allocating memory for data");
    fprintf(stderr, "Allocated memory for data\n");

    // Copy data
    memcpy(a_Entry->data, a_pDiskSector, SECTOR_SIZE);
    fprintf(stderr, "Copied data\n");
}

bool printBinary(uint64_t a_Number, size_t bits, bool use_Prefix)
{
    const size_t max_size = sizeof(uint64_t) * BITS_PER_BYTE;
    if (bits == ZERO || bits > max_size) return false;

    unsigned int mask = 1UL << (bits - 1); // Set the mask to the highest bit of the specified range

    if (use_Prefix) printf("0b");
    for (size_t i = 0; i < bits; i++)
    {
        printf("%d", (a_Number & mask) ? 1 : 0); // Check if the current bit is 1 or 0
        mask >>= 1; // Move the mask one bit to the right
    }
    printf("\n");
    return true;
}

bool isDirectory(const Entry * const e)
{
    observeAndReport(e != NULL, "Error: e is null, cannot check if directory");
    uint8_t result = (e->attributes & ATTR_DIRECTORY);
    return (result > 0);
}

bool printEntry(const Entry * e)
{
    fprintf(stderr, "\tFilepath: %s\n", e->filepath);
    fprintf(stderr, "\tFilename: ");
    for (int i = 0; i < 8; i++) fprintf(stderr, "%c", e->filename[i]);
    fprintf(stderr, "\n\tExtension: ");
    for (int i = 0; i < 3; i++) fprintf(stderr, "%c", e->extension[i]);
    fprintf(stderr, "\n\tAttributes:  %#x\n", e->attributes);
    fprintf(stderr, "\tFirst Cluster: %#x\n", e->first_cluster);
    fprintf(stderr, "\tSize: %u\n", e->size);
    fprintf(stderr, "\tIs Directory: %s\n", isDirectory(e) ? "Yes" : "No");
    fprintf(stderr, "\tDate - Created: %u, Accessed: %u, Modified: %u\n",
        e->date.created, e->date.accessed, e->date.modified);
    fprintf(stderr, "\tTime - Created: %u, Accessed: %u, Modified: %u\n",
        e->time.created, e->time.accessed, e->time.modified);
}