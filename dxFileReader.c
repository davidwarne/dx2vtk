#include "dxFileReader.h"

char read_buf[DX_READ_BUFFER_SIZE];

/**
 * @brief Opens an OpenDX file
 * @details reads through the file finding all objects and populating object descriptors. No data is actually loaded into memory.
 * @param filename The name of the OpenDX file
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
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
            // get the cursor position
            fgetpos(file->fp,&(file->objs[i].pos));
            ParseObjectHeader(&(file->objs[i]),read_buf);
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
            i++;
        }

    }
    return DX_SUCCESS;
}

/**
 * @brief Re-opens file handle without reloading headers.
 * @param file the dxFile to re-load
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 */
int DX_Reopen(dxFile *file)
{
    // check the dxFile is valid
    if (file == NULL)
    {
        return DX_MEMORY_ERROR;
    }
    file->fp = fopen(file->filename,"r");
    if (file->fp == NULL)
    {
        return DX_FILE_NOT_FOUND_ERROR;
    }
    return DX_SUCCESS;
}

/**
 * @brief closes file handle
 * @details All data is still contained in the dxFile object, to open the file
 * again without wiping current contents use DX_Reopen().
 * @param file the dxFile structure
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 */
int DX_Close(dxFile *file)
{
    // check the dxFile is valid
    if (file == NULL)
    {
        return DX_MEMORY_ERROR;
    }
    fclose(file->fp);
}

/**
 * @brief loads objects into memory
 * @details works through each object header and import data appropiately
 * making links were appropriate.
 * @param file the file to load data from
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 */
int DX_LoadAll(dxFile *file)
{
    int i;
    int rc;
    for (i=0;i<(file->numObjects);i++)
    {
#ifdef DEBUG
        printf("Reading class %d named %s\n",i,file->objs[i].name);
#endif
        fsetpos(file->fp,&(file->objs[i].pos));
        rc = LoadObjectData(&(file->objs[i]),file);
        if (rc != DX_SUCCESS)
        {
            return rc;
        }
        rc = LoadAttributes(&(file->objs[i]),file);
        if (rc != DX_SUCCESS)
        {
            return rc;
        }
#ifdef DEBUG
        {
            int j;
            printf("\t %d attributes:\n",file->objs[i].numAttributes);
            for (j=0;j<(file->objs[i].numAttributes);j++)
            {
                printf("\t %s -> %s\n",file->objs[i].attributes[j].attribute_name,file->objs[i].attributes[j].string);
            }
        }
#endif
    }
    return DX_SUCCESS;
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
int ParseObjectHeader(object *obj, char* header)
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
        ParseArrayObjectHeader(obj,ptr);
    }
    else if (streq(buffer,"field"))
    {
        obj->class = DX_FIELD;
        ParseFieldObjectHeader(obj,ptr);
    }
    else if (streq(buffer,"group"))
    {
        obj->class = DX_GROUP;
        ParseGroupObjectHeader(obj,ptr);
    }
    else
    {
        return DX_INVALID_FILE_ERROR;
    }

    return DX_SUCCESS;
}

/**
 * @brief Parses a field object header
 * @param obj a pointer to the object wrapper that will hold this field
 * @param header the pointer to header line starting from type
 * @returns DX_SUCCESS if successfully complete, otherwise an appropriate
 * error code
 */
int ParseFieldObjectHeader(object *obj,char *header)
{
    field * data;

    data = (field *)malloc(sizeof(field));
    
    if (data == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    obj->obj = (void*)data;
    obj->isLoaded = 0;
    return DX_SUCCESS;
}

/**
 * @brief Parses a group object header
 * @param obj a pointer to the object wrapper that will hold this group
 * @param header the pointer to header line starting from type
 * @returns DX_SUCCESS if successfully complete, otherwise an appropriate
 * error code
 */
int ParseGroupObjectHeader(object *obj,char *header)
{
    group * data;

    data = (group *)malloc(sizeof(group));
    
    if (data == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    obj->obj = (void*)data;
    obj->isLoaded = 0;
    return DX_SUCCESS;
}


/**
 * @brief Parses an array object header
 * @param obj a pointer to the object wrapper that will hold this array
 * @param header the pointer to header line starting from type
 * @returns DX_SUCCESS if successfully complete, otherwise an appropriate
 * error code
 */
int ParseArrayObjectHeader(object *obj,char *header)
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
 * @brief loads object data
 * @details loads data via appropriate sub-function, then imports attributes
 * and asserts isLoaded flag.
 * @param obj a pointer to the object to load
 * @param file the dxFile to load from
 * @returns DX_SUCCESS on compeletion, otherwise an appropriate error message
 */
int LoadObjectData(object *obj,dxFile * file)
{
    int rc;
    // load data from file
    switch(obj->class)
    {
        case DX_ARRAY:
            rc = LoadArrayData(obj,file);
            break;
        case DX_FIELD:
            rc = LoadFieldData(obj,file);
            break;
        case DX_GROUP:
            rc = LoadGroupData(obj,file);
            break;
    }

    if (rc != DX_SUCCESS)
    {
        return rc;
    }
    
    // TODO: get the attributes, but for now ignore
    obj->isLoaded = 1;
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
    StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
    while(streq(buffer,"component"))
    {
        data->numComponents++;
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
#ifdef DEBUG
        printf("%s\n",read_buf);
#endif
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
    }
    fsetpos(file->fp,&pos);

#ifdef DEBUG
    printf("num comp: %d\n",data->numComponents);
#endif
    // allocate memory for the component pointers
    data->components = (object **)malloc((data->numComponents)*sizeof(object *));
    if (data->components == NULL)
    {
        return DX_MEMORY_ERROR;
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
        strncpy(ref,buffer,DX_MAX_TOKEN_LENGTH);
        
        // find the object referenced 
        for (j=0;j<(file->numObjects);j++)
        {
            if (streq(file->objs[j].name,ref))
            {
                // store the pointer
                data->components[i] = &(file->objs[j]);
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
    group *data;
    char buffer[DX_MAX_TOKEN_LENGTH];
    char alias[DX_MAX_TOKEN_LENGTH];
    char ref[DX_MAX_TOKEN_LENGTH];
    
    data = (group *)(obj->obj);
    data->numMembers = 0;

    // count number of members and return start
    fgetpos(file->fp,&pos);
    ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
    StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
    while(streq(buffer,"member"))
    {
        data->numMembers++;
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
    }
    fsetpos(file->fp,&pos);

    // allocate memory for the member pointers
    data->members = (object **)malloc((data->numMembers)*sizeof(object *));
    if (data->members == NULL)
    {
        return DX_MEMORY_ERROR;
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
        strncpy(ref,buffer,DX_MAX_TOKEN_LENGTH);
        
        // find the object referenced 
        for (j=0;j<(file->numObjects);j++)
        {
            if (streq(file->objs[j].name,ref))
            {
                // store the pointer
                data->members[i] = &(file->objs[j]);
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
int LoadArrayData(object *obj,dxFile *file)
{
    size_t size;
    int i;
    array *data;

    data = (array *)(obj->obj);
    size = (data->items);
    for (i=0;i<(data->rank);i++)
    {
        size *= (data->shape[i]);
    }
    
    switch(data->dataMode)
    {
        default:
        case DX_FOLLOWS:
            switch(data->type)
            {
                case DX_FLOAT: // load float data
                {
                    float *dataf;
                    dataf = (float *)malloc(size*sizeof(float));
                    for (i=0;i<size;i++)
                    {
                        fscanf(file->fp,"%f",dataf+i);
                    }
                    data->data = (void*)dataf;
                    break;
                }
                case DX_INT: // load int data
                {
                    int *datai;
                    datai = (int *)malloc(size*sizeof(int));
                    for (i=0;i<size;i++)
                    {
                        fscanf(file->fp,"%d",datai+i);
                    }
                    data->data = (void*)datai;
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

/**
 * @brief loads attribute data for the current object
 * @param obj the object currently being read
 * @param file the OpenDX file
 * @returns DX_SUCCESS or an appropriate error code
 * @note this assumes that the file cursor is located at the start of the 
 * first attribute line
 */
int LoadAttributes(object *obj,dxFile *file)
{
    char buffer[DX_MAX_TOKEN_LENGTH];
    fpos_t pos;
    int i;
    int rc;
    obj->numAttributes = 0;
    
    // skip blank lines
    do 
    {
#ifdef DEBUG
        printf("blank\n");
#endif
        // we will need to come back here  
        fgetpos(file->fp,&pos);
        rc = ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
    } while (rc == 0 && StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH) == NULL);
    
    // count the attibutes
    while (streq(buffer,"attribute"))
    {
        obj->numAttributes++;
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
    }
    // jump back to start of attributes
    fsetpos(file->fp,&pos);
    
    // allocate memory
    obj->attributes = (attribute *)malloc((obj->numAttributes)*sizeof(attribute));
    if (obj->attributes == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    // parse attributes
    /** @todo currently external file references in attributes is not supported*/
    for (i=0;i<(obj->numAttributes);i++)
    {
        ReadLine(file->fp,read_buf,DX_READ_BUFFER_SIZE);
        // read attribute key word
        StringToken(read_buf,buffer,DX_MAX_TOKEN_LENGTH);
        // read the attribute name and store
        StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        strncpy(obj->attributes[i].attribute_name,buffer,DX_MAX_TOKEN_LENGTH);
        // read the type 
        StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        if (streq(buffer,"value"))
        {
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        }

        if (streq(buffer,"file"))
        {
            // not supported
            return DX_INVALID_FILE_ERROR;
        }
        else if (streq(buffer,"string") || streq(buffer,"number"))
        {
            // read the value
            StringToken(NULL,buffer,DX_MAX_TOKEN_LENGTH);
        }

        strncpy(obj->attributes[i].string,buffer,DX_MAX_TOKEN_LENGTH);
        obj->attributes[i].number = (int)atoi(buffer);
    }

    return DX_SUCCESS;
}
