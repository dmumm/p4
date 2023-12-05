#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <stdint.h> // uint8_t, uint16_t, uint32_t
#include <stdio.h> // printf, FILE, fopen, fclose
// #include <assert.h> // assert

#pragma region constants

#define FAT_COUNT_OFFSET 16

#define RD_ENTRY_COUNT_OFFSET_LOW 17
#define RD_ENTRY_COUNT_OFFSET_HIGH 18

#define ROOT_OFFSET 0x2600

#define SECTOR_SIZE 512
#define ENTRY_SIZE 32

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
    Entry * head;
    Entry * tail;
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
    Directory * p_Root;
    BytePtr  p_DataArea;
};
#pragma endregion

#pragma region globals
// Directory * g_pD_Root;
BytePtr g_pB_Data;
DiskImage * g_pD_DiskImage;
#pragma endregion

#pragma region prototypes
void observeAndReport(bool a_Condition, string a_Message);
BytePtr openFile(char * a_Filename);
BytePtr getBootSector(BytePtr a_pB_Data);
void printData(BytePtr a_pB_Data, size_t a_n_Length);
void locateAndParseRootDirectory(BytePtr a_pB_Data);
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

    fprintf(stderr, "Image file: %s\n", pc_ImagePath);
    fprintf(stderr, "Output directory: %s\n", pc_OutputDirectoryName);

    // Allocate memory for disk
    g_pD_DiskImage = (DiskImage *)malloc(sizeof(DiskImage));
    observeAndReport(g_pD_DiskImage != NULL, "DiskImage is null");
    fprintf(stderr, "Allocated memory for disk\n");

    // Allocate memory for directory
    g_pD_DiskImage->p_Root = (Directory *)malloc(sizeof(Directory));
    observeAndReport(g_pD_DiskImage->p_Root != NULL, "p_Root is null");
    fprintf(stderr, "Allocated memory for directory\n");

    // Open image file
    g_pB_Data = openFile(pc_ImagePath);
    fprintf(stderr, "Opened image file\n");

    // Get boot sector
    getBootSector(g_pB_Data);

    // Parse file system
    locateAndParseRootDirectory(g_pB_Data);

    // // Output
    // printDirectory(g_pD_Root->head);

    // // Write output to pc_OutputDirectoryName
    // writeOutput(pc_OutputDirectoryName);

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
    rewind(pF_Input);

    BytePtr pData = (BytePtr)malloc(l_FileSize);
    observeAndReport(pData != NULL, "Error allocating memory for file");

    size_t readSize = fread(pData, 1, l_FileSize, pF_Input);
    observeAndReport(readSize == l_FileSize, "Error reading file");

    fclose(pF_Input);
    return pData;
}

/**
 * @brief   Prints the data in a file
 * @param a_pB_Data     The data to print
 * @param a_n_Length    The length of the data
*/
void printData(BytePtr a_pB_Data, size_t a_n_Length)
{
    for (size_t i = 0; i < a_n_Length; i++)
    {
        printf("%02X ", a_pB_Data[i]);
        if (i % 16 == 15) printf("\n");
    }
    printf("\n");
}


#pragma endregion



/**
 * @brief   Gets the boot sector from the data
 * @param a_pB_Data The data to get the boot sector from
 * @return  A pointer to the boot sector
 */
BytePtr getBootSector(BytePtr a_pB_Data)
{
    g_pD_DiskImage->p_BootSector = (BootSector *)malloc(sizeof(BootSector));
    observeAndReport(g_pD_DiskImage->p_BootSector != NULL, "Error allocating memory for boot sector");
    fprintf(stderr, "Allocated memory for boot sector\n");

    uint8_t fatCountByte = a_pB_Data[FAT_COUNT_OFFSET];
    g_pD_DiskImage->p_BootSector->n_Fats = fatCountByte;
    printf("FAT count: %zu\n", g_pD_DiskImage->p_BootSector->n_Fats);

    // Read the high and low bytes of the root directory count
    // Combine the high and low bytes to form the full count
    // Shift the high byte by 8 bits to the left and then combine it with the low byte
    uint8_t rdCountHigh = a_pB_Data[RD_ENTRY_COUNT_OFFSET_HIGH];
    uint8_t rdCountLow = a_pB_Data[RD_ENTRY_COUNT_OFFSET_LOW];
    g_pD_DiskImage->p_BootSector->n_RootEntries = ((size_t)rdCountHigh << 8) | rdCountLow;
    printf("Root directory entry count: %zu\n", g_pD_DiskImage->p_BootSector->n_RootEntries);

    // Validate boot sector
    // assert(pB_BootSector[510] == 0x55 && pB_BootSector[511] == 0xAA && "Invalid boot sector");
    return g_pD_DiskImage->p_BootSector;
}

/**
 * @brief   Locates and parses the root directory
 * @param a_pB_Data The data to parse
 */
void locateAndParseRootDirectory(BytePtr a_pB_Data)
{
    g_pD_DiskImage->p_Root = a_pB_Data[ROOT_OFFSET];
    Directory * i_Entry = g_pD_DiskImage->p_Root;

    // Loop through root directory
    while(* i_Entry != 0x00)
    {
        Entry * i_Entry = makeDirectory(g_pD_DiskImage->p_Root);
        fprintf(stderr, "Made directory\n");

        i_Entry->filePath[0] = '/';
        strcat(i_Entry->filePath, i_Entry->name);
        fprintf(stderr, "Set file path\n");

        // Get first data sector
        int sec = (DATA_SEC_OFFSET + i_Entry->firstLCluster - 2);
        uint8_t *dataSec = memory + (sec * SECTOR_SIZE);
        fprintf(stderr, "Got first data sector\n");

        // Detect if i_Entry is a directory
        if(i_Entry->directory == 1) {
            // handle directory
            handleDirectory(i_Entry, dataSec);
        } else {
            // handle file
            strcat(i_Entry->filePath, ".");
            strcat(i_Entry->filePath, i_Entry->ext);

            // Get data
            makeData(i_Entry, dataSec);
        }

        // Iterate
        g_pD_DiskImage->p_Root += ENTRY_SIZE;
    }
}