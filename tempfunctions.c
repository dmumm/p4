/**
 * @file notJustFunctions.c
 * @author Charles "Blue" Hartsell (ckharts@clemson.edu)
 * @brief
 * @version 0.1
 * @date 2022-11-30
 *
 * @copyright Copyright (c) 2022
 *
 */

// Debug mode
//#define DEBUG 1

// Includes
#include "notJustFunctions.h"

// Utility Functions
uint8_t *openFile(char *fileName) {
    // Open file
    FILE *fp = fopen(fileName, "rb");
    if(fp == NULL) {
        printf("Error: File not found\n");
        exit(EXIT_FAILURE);
    }


    // Map memory
    uint8_t *memory = mmap(NULL, (SEC_SIZE * 2879), PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    return memory;
}

void getBootSector(uint8_t *memory) {
    // Allocate memory for boot sector
    bootSector = malloc(sizeof(struct bootSector));

    // Get number of FATs
    bootSector->fCount = (size_t) (* (memory + 16) & 0x00FF);

    // Get number of root directory entries
    bootSector->rdCount = (size_t) ((*(memory + 18) << 8) & 0x00FF) | (*(memory + 17) & 0xFF);

    // Get number of sectors
    bootSector->sCount = (size_t)((*(memory + 20) << 8) & 0x00FF) | (*(memory + 19) & 0xFF);

    // Get number of sectors in each FAT
    bootSector->secPerFat = (size_t)((*(memory + 23) << 8) & 0x00FF) | (*(memory + 22) & 0xFF);

    /*    boot->fatCount = (size_t)(*(file + 16) & 0x00FF); // byte 16
    boot->rootDirectoryCount = (size_t) ((*(file + 18) << 8) & 0x00FF) | (*(file + 17) & 0xFF); // bytes 18 and 17
    boot->sectorCount = (size_t)((*(file + 20) << 8) & 0x00FF) | (*(file + 19) & 0xFF); // bytes 20 and 19
    boot->sectorsInFat = (size_t)((*(file + 23) << 8) & 0x00FF) | (*(file + 22) & 0xFF); // bytes 23 and 22*/
}

void parseFileSystem(uint8_t *memory) {
    // Find the root directory
    uint8_t *rootDir = memory + R_DIR_OFFSET;

    // Loop through until we find the end of the root directory
    while(*rootDir != 0x00) {
        // Get entry in root directory
        dirEntry *entry = makeDirectory(rootDir);

        // Get file path
        entry->filePath[0] = '/';
        strcat(entry->filePath, entry->name);

        // Get first data sector
        int sec = (DATA_SEC_OFFSET + entry->firstLCluster - 2);
        uint8_t *dataSec = memory + (sec * SEC_SIZE);

        // Detect if entry is a directory
        if(entry->directory == 1) {
            // handle directory
            handleDirectory(entry, dataSec);
        } else {
            // handle file
            strcat(entry->filePath, ".");
            strcat(entry->filePath, entry->ext);

            // Get data
            makeData(entry, dataSec);
        }

        // Iterate
        rootDir += DIR_SIZE;
    }
}

dirEntry *makeDirectory(uint8_t *fData) {
    // allocate memory for directory entry
    dirEntry *newEntry = (dirEntry *) malloc(sizeof(dirEntry));

    // allocate memory for entry data
    newEntry->name = (char *) malloc(sizeof(char *));
    newEntry->firstLCluster = 0;
    newEntry->ext = (char *) malloc(sizeof(char *));
    newEntry->attr = (uint8_t *) malloc(sizeof(uint8_t));
    newEntry->size = 0;
    newEntry->fNum = 0;
    newEntry->directory = 0;
    newEntry->filePath = (char *) malloc(MAX_FILEPATH_SIZE);
    newEntry->data = (dataEntry *) malloc(sizeof(dataEntry));
    newEntry->data->data = (char *) malloc(SEC_SIZE);
    newEntry->next = NULL;

    // Memory is now allocated, set up directory entry
    uint8_t *temp = fData;
    char *buf = malloc(DIR_SIZE);
    memcpy(buf, temp, DIR_SIZE);

    // Copy from buffer -> struct
    memcpy(newEntry->name, buf, 8);
    memcpy(newEntry->ext, buf + 8, 3);
    memcpy(newEntry->attr, buf + 11, 1);
    memcpy(&newEntry->firstLCluster, buf + 26, 2);
    memcpy(&newEntry->size, buf + 28, 4);

    // Remove trailing spaces from name
    for(int i = 8; i > 0; i--){
        if(newEntry->name[i] == ' '){
            newEntry->name[i] = '\0';
        }
    }

    // Remove the padding
    for(int i = 3; i > 0; i--){
        if(newEntry->ext[i] == ' '){
            newEntry->ext[i] = '\0';
        }
    }

    // Deleted?
    if((*newEntry->name & 0xff) == 0xE5){
        newEntry->name[0] = '_';
    }

    // Check if entry is a directory
    if((*newEntry->attr & ATTR_MASK)== 0x10) {
        newEntry->directory = 1;
    }

    // If it's not a directory, add to dir list
    if (newEntry->directory == 0 && strcmp(newEntry->name, ".") != 0 && strcmp(newEntry->name, "..") != 0) {
        // Add to directory list
        if(dir->head == NULL) {
            dir->head = dir->tail = newEntry;
            newEntry->next = NULL;
        }
        else {
            dir->tail->next = newEntry;
            dir->tail = newEntry;
            newEntry->next = NULL;
        }
    }

    // Return entry
    return newEntry;
}

void handleDirectory(dirEntry *entry, uint8_t *dataSec) {
    // setup
    uint8_t *first = dataSec + DIR_HANDLE_OFFSET;

    // treat the first entry as if it is a root directory
    while(*first != 0x00) {
        // Get entry in root directory
        dirEntry *newEntry = makeDirectory(first);
        strcat(newEntry->filePath, entry->filePath);
        strcat(newEntry->filePath, "/");
        strcat(newEntry->filePath, newEntry->name);

        // Check if entry is a directory
        if(newEntry->directory == 1) {
            // Get first data sector
            uint8_t dataSec = (DATA_SEC_OFFSET + newEntry->firstLCluster - 2);
            uint8_t *newSec = fData + (dataSec * SEC_SIZE);

            // handle directory
            handleDirectory(newEntry, newSec);
        } else {
            // handle file ext
            strcat(newEntry->filePath, ".");
            strcat(newEntry->filePath, newEntry->ext);

            // Get data sector
            uint8_t dataSec = DATA_SEC_OFFSET + newEntry->firstLCluster - 2;
            uint8_t *newSec = fData + (dataSec * SEC_SIZE);

            makeData(newEntry, newSec);
        }

        // Iterate
        first += DIR_SIZE;
    }
}

void makeData(dirEntry *entry, uint8_t *fData) {
    // init
    uint8_t *dataSec;
    int sec;

    // allocate memory for data entry
    dataEntry *currentSec = (dataEntry *) malloc(sizeof(dataEntry));
    currentSec->data = (char *) malloc(SEC_SIZE);

    // Get FAT entry
    uint32_t fatEntry = cluster2FAT(entry->firstLCluster);

    // Check if this is the only data sector
    if(fatEntry >= 0xff8 || fatEntry == 0) {
        // Copy data
        memcpy(currentSec->data, fData, SEC_SIZE);
        entry->data = currentSec;
    } else {
        // Multiple sectors exist
        // Search for next cluster
        memcpy(currentSec->data, fData, SEC_SIZE);

        // Init entry
        entry->list = (dataList *) malloc(sizeof(dataList));
        entry->list->head = entry->list->tail = NULL;
        entry->list->num = 0;

        entry->data = currentSec;
        addData(entry, currentSec);

        while(fatEntry != 0 && fatEntry <= 0xff8) {
            // Get next sector
            dataEntry *nextSec = (dataEntry *) malloc(sizeof(dataEntry));
            nextSec->data = (char *) malloc(SEC_SIZE);

            // Make entry
            sec = (DATA_SEC_OFFSET + fatEntry - 2);
            dataSec = fData + (sec * SEC_SIZE);
            memcpy(nextSec->data, dataSec, SEC_SIZE);

            // Add to list
            addData(entry, nextSec);

            // Get next FAT entry
            fatEntry = cluster2FAT(fatEntry);
        }
    }
}

uint32_t cluster2FAT(uint16_t cluster) { // GOOD
    // Get FAT entry
    uint32_t offset = 0x200 + (3 * cluster / 2);

    // If cluster is even
    if(cluster % 2 == 0) {
        return ((0x0f & *(fData + offset + 1)) << 8) | *(fData + offset);
    }

    // Otherwise, cluster is odd
    return (*(fData + offset + 2) << 4) | ((0xf0 & *(fData + offset + 1)) >> 4);
}

void addData(dirEntry *entry, dataEntry *data) {
    // Get dataList
    dataList *list = entry->list;

    // Check if list is empty
    if(!list->head) {
        list->head = list->tail = data;
        data->next = NULL;
    } else {
        list->tail->next = data;
        list->tail = data;
        data->next = NULL;
    }

    list->num++;
}

void printDirectory(dirEntry *entry) {
    // While there are more data entries
    while(entry) {
        // Ensure file is not deleted
        if(entry->name[0] != '_') {
            if(strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0) {
                printf("FILE\tNORMAL\t%s\t%d\n", entry->filePath, entry->size);
            }
        } else {
            printf("FILE\tDELETED\t%s\t%d\n", entry->filePath, entry->size);
        }

        // Iterate
        entry = entry->next;
    }
}

void writeOutput(char *outputDir) {
    // Init variables
    dirEntry *entry = dir->head;
    char *outPath = (char *) malloc(MAX_FILEPATH_SIZE);
    FILE *outfile;
    int i = 0;
    size_t size;

    // While there are more entries
    while(entry) {
        size = 0;

        // Get path to output directory
        sprintf(outPath, "%s/%s%d.%s", outputDir, "file", i++, entry->ext);

        // Open file
        outfile = fopen(outPath, "wb");
        if(!outfile) {
            printf("Error: Could not open file %s\n", outPath);
            exit(EXIT_FAILURE);
        }

        // Check number of data sectors
        if(!entry->list) {
            // Only one data sector exists
            for(int j = 0; j < entry->size; j++) {
                if(size < entry->size) {
                    // Use fwrite to write to file
                    fwrite(&entry->data->data[j], sizeof(char), 1, outfile);

                    //fprintf(outfile, "%c", entry->data->data[j]);
                    size++;
                }
            }
        } else {
            // Multiple data sectors exist
            dataEntry *current = entry->list->head;

            // While there are more data sectors
            while(current) {
                for(int j = 0; j < SEC_SIZE; j++) {
                    if(size < entry->size) {
                        // Use fwrite to write to file
                        fwrite(&current->data[j], sizeof(char), 1, outfile);
                        size++;
                    }
                }

                // Iterate
                current = current->next;
            }
        }

        // Iterate
        entry = entry->next;
    }

    // Close file
    fclose(outfile);
}