#include "dxFileReader.h"

char read_buf[DX_READ_BUFFER_SIZE];

/**
 * @brief Opens an OpenDX file
 * @details reads through the file finding all objects and populating object descriptors. No data is actually loaded into memory.
 * @param filename The name of the OpenDX file
 * @returns a pointer to a valid dxfile struct, NULL if an error occurred.
 */
int DX_Open(dxFile *file, const char * filename)
{
    int i;
    // check the dxFile is valid
    if (file == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    // open the file
    file->filename = (char*)malloc(DX_MAX_FILENAME_LENGTH*sizeof(char));
    if (file->filename == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    strncpy((void*)(file->filename),(void *)filename,DX_MAX_FILENAME_LENGTH);
    
    file->fp = fopen(file->filename,"r");
    if (file->fp == NULL)
    {
        return DX_FILE_NOT_FOUND_ERROR;
    }

#ifdef DEBUG
    printf("Reading file [%s]...\n",file->filename);
#endif

    // first pass, coun the number of objects
    file->numObjects = 0;
    while (NextToken(file->fp,read_buf,DX_READ_BUFFER_SIZE) == 0)
    {
        if (strcmp(read_buf,"object") == 0)
        {
            file->numObjects++;
        }
    }

#ifdef DEBUG
    printf("File contains %d objects\n",file->numObjects);
#endif

    // second pass, read object headers
    file->objs = (object *)malloc((file->numObjects)*sizeof(object));
    if (file->objs == NULL)
    {
        return DX_MEMORY_ERROR; 
    }

    rewind(file->fp);
    i=0;
    while (NextToken(file->fp,read_buf,DX_READ_BUFFER_SIZE) == 0)
    {
        if (strcmp(read_buf,"object") == 0)
        {
            ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
#ifdef DEBUG
            printf("Object %d %s\n",i,read_buf);
#endif
            i++;
            DX_ReadObjectHeader(&(file->objs[i]),read_buf);
        }

    }
}

/**
 * @brief Exracts the object header information from the header line
 */
int DX_ReadObjectHeader(object *obj, char* header)
{
    //determine type and defer to a sub-function
    char name[DX_MAX_TOKEN_LENGTH];
    char buffer[DX_MAX_TOKEN_LENGTH];
    int rc;
    unsigned char state;
   
    // object name/id
    StringToken(header,buffer,DX_MAX_TOKEN_LENGTH);
    strncpy(name,buffer,DX_MAX_TOKEN_LENGTH);
    // next token must be class
    StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
    if (strcmp(buffer,"class") != 0)
    {
        return DX_INVALID_FILE_ERROR;    
    }
    // get the class type
    StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
    if (strcmp(buffer,"array") == 0)
    {
        obj->class = DX_ARRAY;
        rc = StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        while (rc == 0)
        {
            if (strcmp(buffer,"type") == 0)
            {
                
            }
            else if(strcmp(buffer,"category") == 0)
            {
            }
            else if (strcmp(buffer,"rank") == 0)
            {
            }
            else if (strcmp(buffer,"shape") == 0)
            {
            }
            else if (strcmp(buffer,"items") == 0)
            {
            }
            rc = StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        }
    }
    else if (strcmp(buffer,"field") == 0)
    {
        obj->class = DX_FIELD;
    }
    else if (strcmp(buffer,"group") == 0)
    {
        obj->class = DX_GROUP;
    }
    else
    {
        return DX_INVALID_FILE_ERROR;
    }
}

/**
 * @brief Imports object data into memory
 * @param object the dxobject to load data for
 */
/*int DX_ImportComponent(dxobject *object, const char * options)
{
}*/

/**
 * @brief Frees memory consumed by the objects data.
 * @param object the dxobject to clean the data of.
 */
/*int DX_CleanObject(dxobject *object)
{
}*/

/**
 * @brief Closes the OpenDX file handle
 * @param file the OpenDX file to close
 */
/*int DX_Close(dxfile * file)
{
    if (file != NULL)
    {
        return (fclose(file->fp) == 0) ? DX_SUCCESS : DX_UNKNOWN_ERROR;
    }
    else
    {
        return DX_NULL_POINTER; 
    }
}*/

#define S_TRIM 1
#define S_COMMENT 2
#define S_EOF 3
#define S_TOKEN 4
#define S_DONE 5
#define S_STRING 6

/**
 * @brief Tokenises string allowing embbeded strings.
 * @details The behaviour is similar to strtok(), except a substring
 * nested within " " is considered a single token.
 * @param buffer The input string buffer, if buffer is NULL then the 
 * process continues.
 * @param token the ouput buffer for the next token.
 * @param size the size of the token buffer.
 * @retVal 0 The State machine terminated at an accepting state, 
 * and token stores a valid token.
 * @retVal 1 The state machine terminated in a non-accepting state.
 */
int StringToken(char * buffer, char* token,int size)
{
    char c;
    unsigned char state;
    int index;
    static char *buf = NULL;
    static int pos = 0;
    
    if (buffer != NULL)
    {
        pos = 0;
        buf = buffer;
    }
    
    index = 0;
    state = S_TRIM;
    while (state != S_DONE && index < size)
    {
        c = buf[pos]; pos++;
        switch (state)
        {
            case S_TRIM: /*clearing leading whitespace*/
                switch(c)
                {
                    case ' ':
                    case '\n':
                    case '\t':
                        state = S_TRIM;
                        break;
                    case '"':
                        state = S_STRING;
                        break;
                    default:
                        state = S_TOKEN;
                        buf[index] = c;
                        index++;
                        break;
                }
                break;
            case S_TOKEN: // reading a token
                switch(c)
                {
                    case '"':
                        state = S_STRING;
                        break;
                    case ' ':
                    case '\t':
                    case '\n':
                        token[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        buf[index] = c;
                        index++;
                        break;
                }
                break;
            case S_STRING: // reading a string
                switch (c)
                {
                    case '"':
                        token[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        buf[index] = c;
                        index++;
                        break;
                }
                break;
        }
    }
    
    return (state != S_DONE);

}

/**
 * @brief reads the next token from a file stream.
 * @details A token is a contiguous sequence of non-whitespace
 * characters, or a sequance bounded by double "s. 
 * Comment lines (identified by the #) are also ignored.
 * @param fp the input file stream
 * @param buffer the token output stream
 * @param size the bytes in buffer
 * @retVal 0 The State machine terminated at an accepting state, 
 * and token stores a valid token.
 * @retVal 1 The state machine terminated in a non-accepting state.
 */
int NextToken(FILE *fp,char * buffer, int size)
{
    char c;
    unsigned char state;
    int index;
    index = 0;
    state = S_TRIM;
    while (state != S_EOF && state != S_DONE && index < size)
    {
        c = fgetc(fp);
        switch (state)
        {
            case S_TRIM: /*clearing leading whitespace*/
                switch(c)
                {
                    case ' ':
                    case '\n':
                    case '\t':
                        state = S_TRIM;
                        break;
                    case '#':
                        state = S_COMMENT;
                        break;
                    case EOF:
                        state = S_EOF;
                        break;
                    case '"':
                        state = S_STRING;
                        break;
                    default:
                        state = S_TOKEN;
                        buffer[index] = c;
                        index++;
                        break;
                }
                break;
            case S_COMMENT: /*ignoring comment lines*/
                switch(c)
                {
                    case '\n':
                        state = S_TRIM;
                        break;
                    case EOF:
                        state = S_EOF;
                        break;
                    default:
                        state = S_COMMENT;
                        break;
                }
                break;
            case S_TOKEN:
                switch(c)
                {
                    case '"':
                        state = S_STRING;
                        break;
                    case ' ':
                    case '\t':
                    case '\n':
                    case EOF:
                        buffer[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        buffer[index] = c;
                        index++;
                        break;
                }
                break;
            case S_STRING:
                switch (c)
                {
                    case '"':
                    case EOF:
                        buffer[index] = '\0';
                        state = S_DONE;
                        break;
                    default:
                        buffer[index] = c;
                        index++;
                        break;
                }
                break;
        }
    }
    return (state == S_EOF);
}

/**
 * @brief Reads the next line in the input file stream.
 * @details Just keeps reading until \n or EOF or read 
 * extactly size chars.
 * @param fp the input file stream
 * @param buffer the output line buffer
 * @param size the size of buffer
 * @return 1 if EOF is reached
 */
int ReadLine(FILE *fp,char *buffer,int size)
{
    char c;
    int index;
    index = 0;
    c = fgetc(fp);
    while (c != '\n' && c != EOF && index < size)
    {
        buffer[index] = c;
        index++;
        c = fgetc(fp);
    }
    buffer[index] = '\0';
    return (c == EOF);
}
