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

// data types
#define DX_INT 0
#define DX_FLOAT 1

// data categories
#define DX_REAL 0
#define DX_COMPLEX 1

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
    void *obj; // pointer to actual class instance
};

struct array_struct{
    unsigned char type;
    unsigned char category;
    unsigned char rank;
    unsigned char shape;
    unsigned char items;
    unsigned char endian;
    unsigned int dataType;
    unsigned int dataMode;
    void *data;
    int numAttributes;
    object *attributes;
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
    char name[DX_MAX_TOKEN_LENGTH];
    int value;
    int numComponents;
    int numAttributes;
    object *components;
    object *attributes;
};

struct groups_struct{
    char name[DX_MAX_TOKEN_LENGTH];
    int value;
    int numMembers;
    int numAttributes;
    object *members;
    object *attributes;
};

struct dxFile_struct{
    char *filename;
    FILE *fp;
    int numObjects;
    object *objs;
};

// function prototypes
int DX_Open(dxFile *file,const char * filename);
// helpers (should add to another library)
int StringToken(char * buffer, char* token,int size);
int NextToken(FILE *fp,char * buffer, int size);
int ReadLine(FILE *fp,char *buffer,int size);
#endif
