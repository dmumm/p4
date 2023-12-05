/**
 * @file notJustCats.h
 * @author Charles "Blue" Hartsell (ckharts@clemson.edu)
 * @brief Header file for not just cats data recovery tool
 * @version 0.1
 * @date 2022-11-29
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>

// Definitions
#define R_DIR_OFFSET 0x2600
#define FAT_OFFSET 0x203
#define SEC_SIZE 512
#define DIR_SIZE 32
#define ATTR_MASK 0xf0
#define MAX_FILEPATH_SIZE 30
#define DATA_SEC_OFFSET 33
#define DIR_HANDLE_OFFSET 64

// Structs
struct bootSector { /* holds boot sector info */
    size_t fCount;              /* number of FATs */
    size_t rdCount;             /* number of root directory entries */
    size_t sCount;              /* number of sectors */
    size_t secPerFat;           /* number of sectors in each FAT */
};

typedef struct dirEntry { /* holds directory entry info */
    char *name;                 /* name of file */
    char *ext;                  /* extension of file */
    uint8_t *attr;              /* attribute of file */
    uint16_t size;              /* size of file */
    uint32_t firstLCluster;    /* first logical cluster of file */
    int fNum;                   /* file number */
    int directory;              /* directory number */
    char *filePath;             /* file path */

    struct dataEntry *data;     /* data entry */
    struct dataList *list;  /* data list */
    struct dirEntry *next;      /* next directory entry */
} dirEntry;

typedef struct dataEntry { /* holds data entry info */
    char *data;                 /* data of file */
    struct dataEntry *next;      /* pointer to next data entry */
} dataEntry;

typedef struct directory {      /* file directory */
    struct dirEntry *head;      /* pointer to head of directory */
    struct dirEntry *tail;      /* pointer to tail of directory */
} directory;

typedef struct dataList {       /* file data list */
    int num;                    /* number of data entries */
    struct dataEntry *head;     /* pointer to head of data list */
    struct dataEntry *tail;     /* pointer to tail of data list */
} dataList;

// Global Variables
struct bootSector *bootSector;
struct directory *dir;
uint8_t *fData;

// Functions
/**
 * @brief Opens file and maps memory
 *
 * @param fileName Name of file to open
 * @return uint8_t* Pointer to mapped memory
 */
uint8_t *openFile(char *fileName);

/**
 * @brief Gets boot sector info
 *
 * @param fData Pointer to mapped memory
 */
void getBootSector(uint8_t *fData);

/**
 * @brief Gets directory info
 *
 * @param fData Pointer to mapped memory
 */
void parseFileSystem(uint8_t *fData);

/**
 * @brief Builds the directory entry and adds it to the directory
 *
 * @param fData Pointer to mapped memory
 * @return struct dataDirectory* Pointer to data directory struct
 */
dirEntry *makeDirectory(uint8_t *fData);

/**
 * @brief Handles an entry being a directory
 *
 * @param fData Pointer to mapped memory
 * @param dirEntry Pointer to directory entry
 */
void handleDirectory(dirEntry *dirEntry, uint8_t *fData);

/**
 * @brief Adds data sectors from disk by getting FAT entries
 *
 * @param fData Pointer to mapped memory
 * @param dirEntry Pointer to directory entry
 */
void makeData(dirEntry *dirEntry, uint8_t *fData);

/**
 * @brief Gets FAT entry
 *
 * @param cluster
 * @return uint32_t
 */
uint32_t cluster2FAT(uint16_t cluster);

/**
 * @brief Adds data to the specified list
 *
 * @param dirEntry
 * @param data
 */
void addData(dirEntry *dirEntry, dataEntry *data);

/**
 * @brief Prints the directory
 *
 * @param dir Pointer to directory
 */
void printDirectory(dirEntry *dir);

/**
 * @brief Write file data to output directory specified
 *
 */
void writeOutput(char *outputDir);