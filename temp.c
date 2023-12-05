// #define DEBUG 1

/**
 * @file notJustCats.c
 * @author Charles "Blue" Hartsell (ckharts@clemson.edu)
 * @brief Source code file for not just cats data recovery tool
 * @version 0.1
 * @date 2022-11-29
 *
 * @copyright Copyright (c) 2022
 *
 */

// Includes
#include "notJustFunctions.h"

int main(int argc, char *argv[]) {
    #ifdef DEBUG
        printf("RUNNING notJustCats.c:main() IN DEBUG MODE\n");
    #endif

    // Validate command line arguments
    if(argc != 3) {
        printf("Usage: ./notJustCats <image file name> <output directory name>\n");
        exit(EXIT_FAILURE);
    }
    char *imgFile = argv[1];
    char *outDir = argv[2];

    #ifdef DEBUG
    printf("Image file: %s\n", imgFile);
    printf("Output directory: %s\n", outDir);
    #endif

    // Allocate memory for directory
    dir = (directory *) malloc(sizeof(directory));

    // Open image file
    fData = openFile(imgFile);
    getBootSector(fData);

    // Parse file system
    parseFileSystem(fData);

    // Output
    printDirectory(dir->head);

    // Write output to outDir
    writeOutput(outDir);

    return(EXIT_SUCCESS);
}