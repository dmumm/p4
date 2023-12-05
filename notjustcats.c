#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <stdint.h> // uint8_t, uint16_t, uint32_t
#include <stdio.h> // printf, FILE, fopen, fclose
#include <assert.h> // assert

#pragma region constants

#define FAT_COUNT_OFFSET 16

#define RD_ENTRY_COUNT_OFFSET_LOW 17
#define RD_ENTRY_COUNT_OFFSET_HIGH 18


#pragma endregion

#pragma region typedefs
typedef uint8_t * BytePtr;
typedef int bool;
typedef char * string;

typedef struct TimeStamp TimeStamp;
typedef struct Entry Entry;
typedef struct Directory Directory;
typedef struct BootSector BootSector;
typedef struct DiskImage DiskImage;
#pragma endregion

#pragma region structs
struct TimeStamp {
    uint16_t created;
    uint16_t accessed;
    uint16_t modified;
};

struct Entry {
    uint8_t  filename[8];
    uint8_t  extension[3];
    uint8_t  attributes;
    uint8_t  reserved[10];
    uint16_t first_cluster;
    uint32_t size;
    TimeStamp date;
    TimeStamp time;
};

struct Directory {
    struct dirEntry * head;
    struct dirEntry * tail;
};

struct BootSector {
    size_t n_Fats;
    size_t n_RootEntries;
    size_t n_Sectors;
    size_t n_SectorsPerFat;
};

struct DiskImage {
    BootSector * p_BootSector;
    BytePtr * p_FatTables;
    Entry * p_Root;
    BytePtr  p_DataArea;
};
#pragma endregion

#pragma region globals
Directory * g_pD_Root;
BytePtr g_pB_Data;
#pragma endregion

#pragma region prototypes
void observeAndReport(bool a_Condition, string a_Message);
BytePtr openFile(char * a_Filename);
BytePtr getBootSector(BytePtr a_pB_Data);
#pragma endregion

#pragma region main
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

    printf("Image file: %s\n", pc_ImagePath);
    printf("Output directory: %s\n", pc_OutputDirectoryName);

    // Allocate memory for directory
    g_pD_Root = (Directory *)malloc(sizeof(Directory));

    // Open image file
    g_pB_Data = openFile(pc_ImagePath);

    // Get boot sector
    getBootSector(g_pB_Data);

    // Parse file system
    parseFileSystem(g_pB_Data);

    // Output
    printDirectory(g_pD_Root->head);

    // Write output to pc_OutputDirectoryName
    writeOutput(pc_OutputDirectoryName);

    return(EXIT_SUCCESS);
}
#pragma endregion

#pragma region utility functions
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
 * @brief   Opens a file and returns a pointer to the data
 * @param a_Filename    The name of the file to open
 * @return  A pointer to the data in the file
 */
BytePtr openFile(char * a_Filename)
{
    FILE * pF_Input = fopen(a_Filename, "rb");
    observeAndReport(pF_Input != NULL, "Error opening file");

    fseek(pF_Input, 0, SEEK_END);
    long l_FileSize = ftell(pF_Input);

    return (BytePtr)malloc(l_FileSize);
}
#pragma endregion

/**
 * @brief   Gets the boot sector from the data
 * @param a_pB_Data The data to get the boot sector from
 * @return  A pointer to the boot sector
 */
BytePtr getBootSector(BytePtr a_pB_Data)
{
    BytePtr pB_BootSector = (BytePtr)malloc(sizeof(BootSector));
    observeAndReport(pB_BootSector != NULL, "Error allocating memory for boot sector");

    uint8_t fatCountByte = a_pB_Data[FAT_COUNT_OFFSET] & 0x00FF;
    a_pB_Data->n_Fats = (size_t)(fatCountByte);

// Read the high and low bytes of the root directory count
uint8_t rdCountHigh = memory[RD_ENTRY_COUNT_OFFSET_HIGH];
uint8_t rdCountLow  = memory[RD_ENTRY_COUNT_OFFSET_LOW];

// Combine the high and low bytes to form the full count
// Shift the high byte by 8 bits to the left and then combine it with the low byte
size_t rdCount = ((size_t)rdCountHigh << 8) | rdCountLow;

// Assign the combined value to the rdCount member of bootSector
pB_BootSector->n_RootEntries = rdCount;


    // Validate boot sector
    assert(pB_BootSector[510] == 0x55 && pB_BootSector[511] == 0xAA && "Invalid boot sector");

    return pB_BootSector;
}