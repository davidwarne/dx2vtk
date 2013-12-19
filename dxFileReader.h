/**
 * @file dxFileReader.h
 * @brief Reads an OpenDX data file
 * 
 * @details This library implements a light weight reader for the 
 * native OpenDX data format. The complete standard is not necessarily
 * implemented.
 *
 * @author David J. Warne (david.warne@qut.edu.au)
 * @author High Performance Computing and Research Support
 * @author Queensland University of Technology
 *
 */
#ifndef __DXFILEREADER_H
#define __DXFILEREADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ioutils.h"

// buffer sizes
#define DX_MAX_FILENAME_LENGTH 256
#define DX_MAX_TOKEN_LENGTH 32
#define DX_COMMENT_LENGTH 256
#define DX_READ_BUFFER_SIZE 2048

// return codes
#define DX_SUCCESS 1
#define DX_MEMORY_ERROR 0
#define DX_FILE_NOT_FOUND_ERROR -1
#define DX_INVALID_FILE_ERROR -1

// object classes
#define DX_FIELD 0
#define DX_ATTRIBUTE 1
#define DX_CONSTANTARRAY 2
#define DX_ARRAY 3
#define DX_REGULARARRAY 4
#define DX_PRODUCTARRAY 5
#define DX_GRIDPOSITIONS 6
#define DX_PATHARRAY 7
#define DX_MESHARRAY 8
#define DX_GRIDCONNECTIONS 9
#define DX_GROUP 10

#define DX_MAX_RANK 10

// data types
#define DX_INT 0
#define DX_FLOAT 1

// data categories
#define DX_REAL 0
#define DX_COMPLEX 1

#define DX_LSB 0
#define DX_MSB 1

#define DX_TEXT 0
#define DX_IEEE 1
#define DX_BINARY 2
#define DX_ASCII 3

#define DX_OFFSET 0
#define DX_FILE 1
#define DX_FOLLOWS 2

#define streq(a,b) (strcmp((a),(b)) == 0)

// type defs
typedef struct dxFile_struct dxFile;
typedef struct object_struct object;
typedef struct attribute_struct attribute;
typedef struct field_struct field;
typedef struct array_struct array;
typedef struct group_struct group;

/*DX data object*/
struct object_struct{
    unsigned char class;
    char alias[DX_MAX_TOKEN_LENGTH];
    char name[DX_MAX_TOKEN_LENGTH];
    int number;
    unsigned char isLoaded;
    void *obj; // pointer to actual class instance
    int numAttributes;
    object *attributes;
};

struct array_struct{
    unsigned char type;
    unsigned char category;
    int rank;
    int shape[DX_MAX_RANK];
    int items;
    unsigned char endian;
    unsigned char dataType;
    unsigned char dataMode;
    char file[DX_MAX_TOKEN_LENGTH];
    int offset;
    void *data;
};

struct attribute_struct{
    char attribute_name[DX_MAX_TOKEN_LENGTH];
    int value;
    char string[DX_COMMENT_LENGTH];
    char name[DX_MAX_TOKEN_LENGTH];
    int number;
    char file[DX_MAX_FILENAME_LENGTH];

};

struct field_struct{
    int numComponents;
    object **components;
};

struct groups_struct{
    int numMembers;
    object **members;
};

struct dxFile_struct{
    char *filename;
    FILE *fp;
    int numObjects;
    object *objs;
};

// function prototypes
int DX_Open(dxFile *file,const char * filename);
int DX_LoadAll(dxFile *file);
int ParseObjectHeader(object *obj, const char* header);
int ParseArrayObjectHeader(char * name, object *obj,const char *header);
int LoadData(object *obj,dxFile *file);
int LoadArrayData(object *obj,dxFile *file);
int LoadFieldData(object *obj,dxFile *file);
int LoadGroupData(object *obj,dxFile *file);
#endif
