
#ifndef __IOUTILS_H
#define __IOUTILS_H

#include <stdio.h>
#include <string.h>

#define S_TRIM 1
#define S_COMMENT 2
#define S_EOF 3
#define S_TOKEN 4
#define S_DONE 5
#define S_STRING 6
char * StringToken(const char * buffer, char* token,int size);
int NextToken(FILE *fp,char * buffer, int size);
int ReadLine(FILE *fp,char *buffer,int size);
#endif
