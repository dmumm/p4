#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <stdint.h> // byte, uint16_t, uint32_t
#include <stdio.h> // printf, FILE, fopen, fclose
#include <string.h> // strcat
#include <ctype.h> // toupper
#include <limits.h> //
#include <assert.h> // assert

#pragma region constants

#define ZERO 0

#define FAT_COUNT_OFFSET 16

#define RD_ENTRY_COUNT_OFFSET_LOW 17
#define RD_ENTRY_COUNT_OFFSET_HIGH 18

// #define ROOT_OFFSET 0x2600
#define ROOT_OFFSET 9728
#define DATA_AREA_OFFSET 0x4000

#define SECTOR_SIZE 512
#define ENTRY_SIZE 32

#define ENTRY_FILENAME_OFFSET 0
#define ENTRY_FILENAME_LENGTH 8
#define ENTRY_EXTENSION_OFFSET 8
#define ENTRY_EXTENSION_LENGTH 3
#define ENTRY_ATTRIBUTES_OFFSET 11

#define ATTR_READ_ONLY   0x01  // Binary: 00000001
#define ATTR_HIDDEN      0x02  // Binary: 00000010
#define ATTR_SYSTEM      0x04  // Binary: 00000100
#define ATTR_VOLUME_LABEL 0x08  // Binary: 00001000
#define ATTR_DIRECTORY   0x10  // Binary: 00010000
#define ATTR_ARCHIVE     0x20  // Binary: 00100000

#define BITS_PER_BYTE 8

#pragma endregion

#pragma region typedefs_structs

typedef uint8_t byte;

typedef enum { false, true } bool;
typedef char * string;

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
    byte  reserved[10];
    uint16_t first_cluster;
    uint32_t size;
    bool     is_directory;
    TimeStamp date;
    TimeStamp time;
} Entry;

typedef struct Directory {
    Entry * head;
    Entry * tail;
} Directory;

typedef struct BootSector {
    size_t n_Fats;
    size_t n_RootEntries;
    size_t n_Sectors;
    size_t n_SectorsPerFat;
} BootSector;

typedef struct DiskImage {
    BootSector * p_BootSector;
    byte ** p_FatTables;
    Directory * p_Root;
    byte * p_DataArea;
    size_t bytes;
} DiskImage;

#pragma endregion

#pragma region globals
// Directory * g_pD_Root;
byte * g_pB_Data;
DiskImage * g_pD_DiskImage;
#pragma endregion

#pragma region prototypes
void observeAndReport(bool a_Condition, string a_Message);
byte * openFile(char * a_Filename);
byte * getBootSector(byte * a_pB_Data);
void printData(byte * a_pB_Data, size_t a_n_Length);
void locateAndParseRootDirectory(byte * a_pB_Data);
Entry * generateEntry(byte * a_byteLocation);
string formatFileNaming(byte * a_pB_Data, size_t a_Length);
bool testPointer(byte * a_Ptr, size_t a_Length);
bool isReadOnly(Entry * e);
bool isHidden(Entry * e);
bool isSystem(Entry * e);
bool isVolumeLabel(Entry * e);
bool isDirectory(Entry * e);
bool isArchive(Entry * e);
bool printBinary(uint64_t a_Number, size_t bits, bool use_Prefix);
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
    fprintf(stderr, "Got boot sector\n");


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
 * @brief   Tests a pointer to see if it is null, and if not, prints the data
 * @param ptr       The pointer to test
 * @param length    The length of the data
 *
*/
bool testPointer(byte * ptr, size_t length)
{
    if (ptr == NULL)
    {
        printf("Pointer is NULL\n");
        return 0;
    }
    printf("Pointer is not NULL, testing access...\n");
    for (size_t i = 0; i < length; ++i) {
        printf("byte %zu: 0x%02x\n", i, ptr[i]);
    }
    return 1;
}

/**
 * @brief   Opens a file and returns a pointer to the data
 * @param a_Filename    The name of the file to open
 * @return  A pointer to the data in the file
 */
byte * openFile(char * a_Filename)
{
    FILE * pF_Input = fopen(a_Filename, "rb");
    observeAndReport(pF_Input != NULL, "Error opening file");

    fseek(pF_Input, 0, SEEK_END);
    long l_FileSize = ftell(pF_Input);
    rewind(pF_Input);

    byte * pData = (byte *)malloc(l_FileSize);
    observeAndReport(pData != NULL, "Error allocating memory for file");

    size_t readSize = fread(pData, 1, l_FileSize, pF_Input);
    observeAndReport(readSize == l_FileSize, "Error reading file");
    fprintf(stderr, "Read file of size %zu\n", readSize);

    g_pD_DiskImage->bytes = readSize;

    fclose(pF_Input);
    return pData;
}

/**
 * @brief   Prints the data in a file
 * @param a_pB_Data     The data to print
 * @param a_n_Length    The length of the data
*/
void printData(byte * a_pB_Data, size_t a_n_Length)
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
byte * getBootSector(byte * a_pB_Data)
{
    g_pD_DiskImage->p_BootSector = (BootSector *)malloc(sizeof(BootSector));
    observeAndReport(g_pD_DiskImage->p_BootSector != NULL, "Error allocating memory for boot sector");
    fprintf(stderr, "Allocated memory for boot sector\n");

    byte fatCountbyte = a_pB_Data[FAT_COUNT_OFFSET];
    g_pD_DiskImage->p_BootSector->n_Fats = fatCountbyte;
    printf("FAT count: %zu\n", g_pD_DiskImage->p_BootSector->n_Fats);

    // Read the high and low bytes of the root directory count
    // Combine the high and low bytes to form the full count
    // Shift the high byte by 8 bits to the left and then combine it with the low byte
    byte rdCountHigh = a_pB_Data[RD_ENTRY_COUNT_OFFSET_HIGH];
    byte rdCountLow = a_pB_Data[RD_ENTRY_COUNT_OFFSET_LOW];
    g_pD_DiskImage->p_BootSector->n_RootEntries = (rdCountHigh << 8) | rdCountLow;
    printf("Root directory entry count: %zu\n", g_pD_DiskImage->p_BootSector->n_RootEntries);

    // Validate boot sector
    // assert(pB_BootSector[510] == 0x55 && pB_BootSector[511] == 0xAA && "Invalid boot sector");
    return g_pD_DiskImage->p_BootSector;
}

/**
 * @brief   Locates and parses the root directory
 * @param a_pB_Data The data to parse
 */
void locateAndParseRootDirectory(byte * a_pB_Data)
{
    g_pD_DiskImage->p_Root->head = a_pB_Data + ROOT_OFFSET;
    byte * i_Offset = g_pD_DiskImage->p_Root->head;
    fprintf(stderr, "Located root directory\n");

    while (i_Offset != 0x00)
    {
        Entry * i_EntryPtr = generateEntry(i_Offset);
        observeAndReport(i_EntryPtr != NULL, "Error generating entry");
        fprintf(stderr, "Generated new entry\n");

        // Get first data sector
        uint16_t first_data_sector_offset = (DATA_AREA_OFFSET + i_EntryPtr->first_cluster - 2);
        byte * dataSec = a_pB_Data + (first_data_sector_offset * SECTOR_SIZE);
        fprintf(stderr, "Got first data sector\n");

        // Detect if i_EntryPtr is a directory
        if (i_EntryPtr->is_directory == 1)
        {
            // // handle directory
            // handleDirectory(i_EntryPtr, dataSec); //TODO
        }
        else
        {
            // handle file
            strcat(i_EntryPtr->filepath, ".");
            strcat(i_EntryPtr->filepath, i_EntryPtr->extension);

            // // Get data
            // makeData(e, dataSec); //TODO
        }
        i_Offset += ENTRY_SIZE;
    }
}

/**
 * @brief   Formats file name or extension to be uppercase and space padded
 * @param a_pB_Data     The data to format, non-null terminated
 * @param a_Length      The length of the data
 * @return  The formatted data
 */
string formatFileNaming(byte * a_byteLocation, size_t a_Length)
{
    observeAndReport(a_byteLocation != NULL, "Error: a_byteLocation is null");
    fprintf(stderr, "a_Length: %zu\n", a_Length);
    observeAndReport(a_Length == ENTRY_FILENAME_LENGTH || a_Length == ENTRY_EXTENSION_LENGTH, "Error: a_Length is not 8 or 3");
    fprintf(stderr, "Formatting file name\n");
    string formatted = (string)malloc(a_Length);
    observeAndReport(formatted != NULL, "Error allocating memory for formatted file name");
    fprintf(stderr, "Allocated memory for formatted file name\n");
    size_t i;
    for (i = 0; i < a_Length; i++)
    {
        fprintf(stderr, "i: %zu\n", i);
        if (a_byteLocation[i] != 0x00)
        {
            formatted[i] = toupper((unsigned char)a_byteLocation[i]);
        }
        else
        {
            break;
        }
    }

    // Pad with spaces if necessary
    for (; i < a_Length; i++) {
        formatted[i] = 0x20;
    }
    fprintf(stderr, "Formatted file name\n");
    return formatted;
}

/**
 * @brief   Generates a entry from a byte location
 * @param a_byteLocation    The byte location to generate the entry from
 * @return  A pointer to the generated entry
 */
Entry * generateEntry(byte * a_byteLocation)
{
    fprintf(stderr, "Generating entry\n");
    observeAndReport(a_byteLocation != NULL, "Error: a_byteLocation is null");

    // Allocate memory for entry
    Entry * e = (Entry *)malloc(sizeof(Entry));
    observeAndReport(e != NULL, "Error allocating memory for entry");
    e->is_directory = false;
    fprintf(stderr, "Allocated memory for entry\n");

    // Get filename
    string formattedName = formatFileNaming(a_byteLocation, ENTRY_FILENAME_LENGTH);
    fprintf(stderr, "Got file name, '%s'\n", formattedName);
    strncpy(e->filename, formattedName, ENTRY_FILENAME_LENGTH);

    e->filepath = malloc(sizeof(char));
    e->filepath[0] = '/';
    // Handle memory allocation error
    // observeAndReport(e->filepath != NULL, "Error: i_EntryPtr->filepath is null");
    // observeAndReport(e->filepath[0] != NULL, "Error: i_EntryPtr->filepath[0] is null");
    fprintf(stderr, "Set filepath[0]: '%c'\n", e->filepath[0]);
    assert(e->filepath[0] == '/' && "Error: i_EntryPtr->filepath is not '/'");
    observeAndReport(e->filepath[0] == '/', "Error: i_EntryPtr->filepath is not '/'");
    strcat(e->filepath, e->filename);
    fprintf(stderr, "Set file path\n");

    // Get extension
    string formattedExtension = formatFileNaming(a_byteLocation + ENTRY_EXTENSION_OFFSET, ENTRY_EXTENSION_LENGTH);
    strncpy(e->extension, formattedExtension, ENTRY_EXTENSION_LENGTH);
    fprintf(stderr, "Got extension, '%s'\n", e->extension);
    strcat(e->filepath, ".");
    strcat(e->filepath, e->extension);


    // Get attributes
    e->attributes = a_byteLocation + ENTRY_ATTRIBUTES_OFFSET;
    fprintf(stderr, "Got attributes, '%#x'\n", e->attributes);
    printBinary(e->attributes, 8, true);

    // Test Case 1: uint8_t
    printf("Test Case 1: uint8_t\n");
    uint8_t small_number = 5;
    printBinary(small_number, 8, true);
    // Expected Output: 0b00000101

    // Test Case 2: uint16_t
    printf("\nTest Case 2: uint16_t\n");
    uint16_t medium_number = 1024;
    printBinary(medium_number, 16, false);
    // Expected Output: 0000010000000000

    // Test Case 3: uint32_t
    printf("\nTest Case 3: uint32_t\n");
    uint32_t large_number = 4294967295;
    printBinary(large_number, 32, false);
    // Expected Output: 11111111111111111111111111111111

    // Test Case 4: uint64_t
    printf("\nTest Case 4: uint64_t\n");
    uint64_t very_large_number = 18446744073709551615ULL;
    printBinary(very_large_number, 64, true);
    // Expected Output: 0b1111111111111111111111111111111111111111111111111111111111111111

    if (isDirectory(e)) e->is_directory = true;

    // Get first cluster
    e->first_cluster = a_byteLocation[26] | a_byteLocation[27] << 8;
    fprintf(stderr, "Got first cluster, '%#x'\n", e->first_cluster);

    return e;

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

bool isDirectory(Entry * e) {
    observeAndReport(e != NULL, "Error: e is null, cannot check if directory");
    return (e->attributes & ATTR_DIRECTORY);
}
