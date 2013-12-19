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
        if (streq(read_buf,"object"))
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
        if (streq(read_buf,"object"))
        {
            ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
#ifdef DEBUG
            printf("HEADER %d %s\n",i,read_buf);
#endif
            i++;
            DX_ParseObjectHeader(&(file->objs[i]),read_buf);
#ifdef DEBUG
            printf("Class: %hhu\n",file->objs[i].class);
            printf("name: %s ,",file->objs[i].name);
            printf("number: %d\n",file->objs[i].number);
            printf("Loaded: %hhu\n",file->objs[i].isLoaded);
            if (file->objs[i].class == DX_ARRAY)
            {
                int j;
                array *data = (array*)(file->objs[i].obj);
                printf("\ttype: %hhu\n",data->type);
                printf("\tcategory: %hhu\n",data->category);
                printf("\trank: %d\n",data->rank);
                printf("\tshape:");
                for (j = 0;j<(data->rank);j++)
                {
                    printf(" %d",data->shape[j]);
                }
                printf("\n");
                printf("\titems: %d\n",data->items);
                printf("\tmode: %hhu\n",data->dataMode);


            }
#endif
        }

    }
}

/**
 * @brief Parses an object header
 * @details This just tests what type of object it is and differs to an
 * object specific parser. On successful execution, the object header 
 * information will be set.
 * @param obj a pointer to an object wrapper
 * @param header the header text line
 * @returns DX_SUCCESS or an appropiate error code
 */
int DX_ParseObjectHeader(object *obj, const char* header)
{
    //determine type and defer to a sub-function
    char name[DX_MAX_TOKEN_LENGTH];
    char buffer[DX_MAX_TOKEN_LENGTH];
    char *ptr;
    int rc;
    unsigned char state;
   
    // object name/id
    StringToken(header,buffer,DX_MAX_TOKEN_LENGTH);
    strncpy(name,buffer,DX_MAX_TOKEN_LENGTH);
    obj->number = atoi(name);
    strncpy(obj->name,name,DX_MAX_TOKEN_LENGTH);
    // next token must be class
    StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
    if (!streq(buffer,"class"))
    {
        return DX_INVALID_FILE_ERROR;    
    }
    // get the class type
    ptr = StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
    if (streq(buffer,"array"))
    {
        obj->class = DX_ARRAY;
        ParseArrayObjectHeader(name,obj,ptr);
    }
    else if (streq(buffer,"field"))
    {
        obj->class = DX_FIELD;
    }
    else if (streq(buffer,"group"))
    {
        obj->class = DX_GROUP;
    }
    else
    {
        return DX_INVALID_FILE_ERROR;
    }

    return DX_SUCCESS;
}

/**
 * @brief Parses an array object header
 * @param name The id of the object as parsed by ParseObjectHeader()
 * @param obj a pointer to the object wrapper that will hold this array
 * @param header the pointer to header line starting from type
 * @returns DX_SUCCESS if successfully complete, otherwise an appropriate
 * error code
 */
int ParseArrayObjectHeader(char * name, object *obj,const char *header)
{
    char buffer[DX_MAX_TOKEN_LENGTH];
    array *data;

    // allocate memory for new array
    data = (array *)malloc(sizeof(array));
    if (data == NULL)
    {
        return DX_MEMORY_ERROR; 
    }

    StringToken(header,buffer,DX_MAX_TOKEN_LENGTH);
    do 
    {
        if (streq(buffer,"type"))
        {
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            // TODO: extend as needed
            if (streq(buffer,"float"))
            {
                data->type = DX_FLOAT;
            }
            else 
            {
                data->type = DX_INT;
            }
        }
        else if (streq(buffer,"category"))
        {
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            if (streq(buffer,"real"))
            {
                data->category = DX_REAL;
            }
            else 
            {
                data->category = DX_COMPLEX;
            }
        }
        else if (streq(buffer,"rank"))
        {
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            data->rank = atoi(buffer);
        }
        else if (streq(buffer,"shape"))
        {
            int i;
            for (i=0;i<(data->rank);i++)
            {
                StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
                data->shape[i] = atoi(buffer);
            }
        }
        else if (streq(buffer,"items"))
        {
            // read the number of items
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            data->items = atoi(buffer);

            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            while (!streq(buffer,"data"))
            {
                if (streq(buffer,"lsb"))
                {
                    data->endian = DX_LSB;
                }
                else if (streq(buffer,"msb"))
                {
                    data->endian = DX_MSB;
                }
                else if (streq(buffer,"text"))
                {
                    data->dataType = DX_TEXT;
                }
                else if (streq(buffer,"ieee"))
                {
                    data->dataType = DX_IEEE;
                }
                else if (streq(buffer,"binary"))
                {
                    data->dataType = DX_BINARY;
                }
                else if (streq(buffer,"ascii"))
                {
                    data->dataType = DX_ASCII; 
                }
                StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            }
            // now we extract the data mode
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            if (streq(buffer,"mode"))
            {
                StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
            }

            if (streq(buffer,"file"))
            {
                data->dataMode = DX_FILE;
                StringToken(NULL,data->file,DX_MAX_TOKEN_LENGTH);
            }
            else if (streq(buffer,"follows"))
            {
                data->dataMode = DX_FOLLOWS;
            }
            else
            {
                data->dataMode = DX_OFFSET;
                data->offset = atoi(buffer);
            }
        }
            
    } while(StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH) != NULL);

    obj->obj = (void *)data;
    obj->isLoaded = 0;
    return DX_SUCCESS;
}

/**
 * @brief load field data
 * @details locates the components it the object array, assigns an alias
 * to each and stores the pointer to associate the objects with the field.
 * @param obj the object pointer which wraps the field.
 * @param file the dxFile structure, assumes the stream cursor is located just
 * after the field header.
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 * @note This loader assumes all references are in the same dx file
 */
int LoadFieldData(object *obj, dxFile *file)
{
    fpos_t pos;
    int i,j;
    field *data;
    char buffer[DX_MAX_TOKEN_LENGTH];
    char alias[DX_MAX_TOKEN_LENGTH];
    char ref[DX_MAX_TOKEN_LENGTH];
    
    data = (field *)(obj->obj);
    data->numComponents = 0;

    // count number of components and return start
    fgetpos(file->fp,&pos);
    ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
    StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH)
    while(!streq(buffer,"component"))
    {
        data->numComponents++;
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH)
    }
    fsetpos(file->fp,&pos);

    // allocate memory for the component pointers
    data->components = (objects **)malloc((data->numComponents)*sizeof(object *));
    if (data->components == NULL)
    {
        return DX_MEMOY_ERROR;
    }
    // set to null
    for (i=0;i<(data->numComponents);i++)
    {
        data->components[i] = NULL;
    }

    // now parse component data and link to objects
    for (i=0;i<(data->numComponents);i++)
    {
        // read the line
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        // read component
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
        // get component alias
        StringToken(NULL,alias,DX_MAX_TOKEN_LENGTH);
        // read reference
        StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        if (streq(buffer,"value"))
        {
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        }
        StringToken(NULL,ref,DX_MAX_TOKEN_LENGTH);
        
        // find the object referenced 
        for (j=0;j<(file->numObjects);j++)
        {
            if (streq(file->objs[j].name,ref))
            {
                // store the pointer
                data->component[i] = &(file->objs[j]);
            }
        }

        if (data->components[i] == NULL)
        {
            return DX_INVALID_FILE_ERROR;
        }

        strncpy(data->components[i]->alias,alias,DX_MAX_TOKEN_LENGTH);
    }

    return DX_SUCCESS;
}

/**
 * @brief loads group data
 * @details locates the members it the object array, assigns an alias
 * to each and stores the pointer to associate the objects with the group.
 * @param obj the object pointer which wraps the group.
 * @param file the dxFile structure, assumes the stream cursor is located just
 * after the group header.
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 */
int LoadGroupData(object *obj,dxFile *file)
{    
    fpos_t pos;
    int i,j;
    field *data;
    char buffer[DX_MAX_TOKEN_LENGTH];
    char alias[DX_MAX_TOKEN_LENGTH];
    char ref[DX_MAX_TOKEN_LENGTH];
    
    data = (field *)(obj->obj);
    data->numMembers = 0;

    // count number of members and return start
    fgetpos(file->fp,&pos);
    ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
    StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH)
    while(!streq(buffer,"member"))
    {
        data->numMembers++;
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH)
    }
    fsetpos(file->fp,&pos);

    // allocate memory for the member pointers
    data->members = (objects **)malloc((data->numMembers)*sizeof(object *));
    if (data->members == NULL)
    {
        return DX_MEMOY_ERROR;
    }
    // set to null
    for (i=0;i<(data->numMembers);i++)
    {
        data->members[i] = NULL;
    }

    // now parse member data and link to objects
    for (i=0;i<(data->numMembers);i++)
    {
        // read the line
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        // read member
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
        // get member alias
        StringToken(NULL,alias,DX_MAX_TOKEN_LENGTH);
        // read reference
        StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        if (streq(buffer,"value"))
        {
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        }
        StringToken(NULL,ref,DX_MAX_TOKEN_LENGTH);
        
        // find the object referenced 
        for (j=0;j<(file->numObjects);j++)
        {
            if (streq(file->objs[j].name,ref))
            {
                // store the pointer
                data->member[i] = &(file->objs[j]);
            }
        }

        if (data->members[i] == NULL)
        {
            return DX_INVALID_FILE_ERROR;
        }

        strncpy(data->members[i]->alias,alias,DX_MAX_TOKEN_LENGTH);
    }

    return DX_SUCCESS;

}

/**
 * @brief loads array data
 * @details allocates memory and loads data array into memory
 * @param obj the pointer which wraps the array object
 * @param file the dxFile structure, assumes the stream cursor is located just
 * after the array header.
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 * @todo Currently only supports data mode follows
 */
int LoadArraydata(object *obj,dxFile *file)
{
    size_t size;

    size = (obj->rank)*(obj->shape)*(obj->items);
    
    switch(obj->dataMode)
    {
        case default:
        case DX_FOLLOWS:
            switch(obj->type)
            {
                case DX_FLOAT: // load float data
                {
                    int i;
                    float *dataf;
                    dataf = (float *)malloc(size*sizeof(float));
                    for (i=0;i<size;i++)
                    {
                        fscanf(file->fp,"%f",dataf+i);
                    }
                    obj->data (void*)dataf;
                    break;
                }
                case DX_INT: // load int data
                {
                    int i;
                    int *datai;
                    datai = (int *)malloc(size*sizeof(int));
                    for (i=0;i<size;i++)
                    {
                        fscanf(file->fp,"%f",datai+i);
                    }
                    obj->data (void*)datai;
                    break;
                }

                    break;
            }
            break;
        case DX_OFFSET:
            break;
        case DX_FILE:
            break;
    }
    return DX_SUCCESS;
}
