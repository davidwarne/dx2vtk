/**
 * @file dx2vtk
 * @brief converts dx native format to vtk legacy format
 *
 * @author David J. Warne (david.warne@qut.edu.au)
 * @author High Performance Computing and Research Support
 * @author Queensland University of Technology
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "dxFileReader.h"
#include "vtkFileWriter.h"

#include "ioutils.h"


/**
 * @brief creates a VTK dataset from a OpenDX Field object OpenDX file.
 * @details uses the position and connection OpenDX field components to 
 * construct a VTK dataset mesh.
 * @param field pointer to the field object wrapper
 * @param pointer to the vtkFile structure to load to
 * @returns DX_SUCCESS on completion, or an appropriate error code
 */
int dxField2VTKDataSet(object *fieldObject, vtkDataFile *vtkFile)
{
    int i;
    object * pos;
    object * con;
    field *fld;
    
    fld = (field *)(fieldObject->obj);
#ifdef DEBUG
    printf("Writing Field %d [%s] %hhu\n",fieldObject->number,fieldObject->name,fieldObject->isLoaded);
    printf("Num Components: %d\n",fld->numComponents);
#endif
    
    // identify the position (geometry) and connections (topology) components
    // any other components are considered as data
    for (i=0;i<fld->numComponents;i++)
    {
        object * comp;
        comp = fld->components[i];
        if (streq(comp->alias,"positions"))
        {
            pos = comp;
        }
        else if (streq(comp->alias,"connections"))
        {
            con = comp;
        }
        else 
        {
            array * data;
            attribute *attr;
            data = (array *)(comp->obj);
            // test if data depends on positions or connections
            attr = GetAttribute(comp,"dep");
            if (attr != NULL)
            {
                if (streq(attr->string,"positions"))
                {
                
                    if (data->rank == 0)
                    {
                        vtkFile->pointdata->numScalars++;
                    }
                    else if (data->rank == 1 && data->shape[0] == 3)
                    {
                        vtkFile->pointdata->numVectors++;
                    }

                }
                else if (streq(attr->string,"connections"))
                {
                    if (data->rank == 0)
                    {
                        vtkFile->celldata->numScalars++;
                    }
                    else if ( data->rank == 1 && data->shape[0] == 3)
                    {
                        vtkFile->celldata->numVectors++;
                    }

                }
            }
        }
    }
   
    // dx positions and connections map to a vtk data set type
    if (pos->class == DX_GRIDPOSITIONS && con->class == DX_GRIDCONNECTIONS)
    {
        gridpositions *gp;
        gridconnections *gc;
        structuredPoints *spdata;
        vtkFile->geometry = VTK_STRUCTURED_POINTS;

        gp = (gridpositions *)pos->obj;
        gc = (gridconnections *)con->obj;
        
        spdata = (structuredPoints *)malloc(sizeof(structuredPoints));
        if (spdata == NULL)
        {
            return DX_MEMORY_ERROR;
        }

        spdata->dimensions[0] = 1;
        spdata->dimensions[1] = 1;
        spdata->dimensions[2] = 1;

        spdata->origin[0] = 0.0;
        spdata->origin[1] = 0.0;
        spdata->origin[2] = 0.0;

        spdata->spacing[0] = 1.0;
        spdata->spacing[1] = 1.0;
        spdata->spacing[2] = 1.0;

        // gridconnections are really not used
        if (gp->numCounts > 3)
        {
            return DX_NOT_SUPPORTED_ERROR;
        }

        for (i=0;i<gp->numCounts;i++)
        {
            spdata->dimensions[i] = gp->counts[i];
        }
        
        for (i=0;i<gp->numCounts;i++)
        {
            spdata->origin[i] = gp->origin[i];
        }

        // @todo we assume that deltas are orthogonal, if not then structured points is
        // not the best dataset type... but not quite sure which on is the best yet.
        for (i=0;i<gp->numCounts;i++)
        {
            spdata->spacing[i] = gp->deltas[i*(gp->numCounts)+ i];
        }
        
        vtkFile->dataset = spdata;
        return DX_SUCCESS;
    }
    else if (pos->class == DX_ARRAY && con->class == DX_ARRAY)
    {
        //  the following is incorrect... shape 3 connections equals polydata
        //  whereas shape 4 equals Unstructured Grid... to be simple I've just made 
        //  both map to unstructured grid, but this is not ideal

        array *pos_array;
        array *con_array;
        attribute *attr;
        unstructuredGrid *ugdata;
        vtkFile->geometry = VTK_UNSTRUCTURED_GRID;
        // extract the geometry and topology
        pos_array = (array *)pos->obj;
        con_array = (array *)con->obj;

        if ((ugdata = (unstructuredGrid *)malloc(sizeof(unstructuredGrid))) == NULL)
        {
            return DX_MEMORY_ERROR;
        }

        // get the number of positions, this maps to points
        ugdata->numPoints = pos_array->items;
        // now the number of cells
        ugdata->numCells = con_array->items;

        if (pos_array->type != DX_FLOAT || pos_array->rank != 1 || pos_array->shape[0] != 3)
        {
            return DX_INVALID_FILE_ERROR;
        }
        // allocate memory for points
        ugdata->points = (float *)malloc(3*(ugdata->numPoints)*sizeof(float));

        if (con_array->rank != 1)
        {
            return DX_INVALID_FILE_ERROR;
        }

        // allocate memory for cells
        ugdata->cells = (int *)malloc((con_array->shape[0])*(ugdata->numCells)*sizeof(int));
        ugdata->numVerts = (int *)malloc((ugdata->numCells)*sizeof(int));
        ugdata->cellTypes = (int *)malloc((ugdata->numCells)*sizeof(int));

        if ((ugdata->points == NULL) || (ugdata->cells == NULL) || (ugdata->numVerts == NULL) || (ugdata->cellTypes == NULL))
        {
            return DX_MEMORY_ERROR;
        }

        // copy the point data arrays
        memcpy((void*)ugdata->points,pos_array->data,3*(pos_array->items)*sizeof(float));
        // now the cell data (may need to re-map verts) this depends 
        // the element type attribute
        attr = GetAttribute(con,"element type");
        if (attr == NULL)
        {
            return DX_INVALID_FILE_ERROR;
        }

        // determine the element type required and map accordingly
        if (streq(attr->string,"lines"))
        {
            for (i=0;i<ugdata->numCells;i++)
            {
                ugdata->numVerts[i] = 2;
                ugdata->cellTypes[i] = VTK_LINE; 
            }
            // simple enough
            memcpy((void*)ugdata->cells,con_array->data,(con_array->shape[0])*(con_array->items)*sizeof(int));
        }
        else if (streq(attr->string,"triangles"))
        {
            for (i=0;i<ugdata->numCells;i++)
            {
                ugdata->numVerts[i] = 3;
                ugdata->cellTypes[i] = VTK_TRIANGLE; 
            }
            // again simple enough
            memcpy((void*)ugdata->cells,con_array->data,(con_array->shape[0])*(con_array->items)*sizeof(int));
        }
        else if (streq(attr->string,"quads"))
        {
            for (i=0;i<ugdata->numCells;i++)
            {
                ugdata->numVerts[i] = 4;
                ugdata->cellTypes[i] = VTK_QUAD; 
            }
            // again simple enough
            memcpy((void*)ugdata->cells,con_array->data,(con_array->shape[0])*(con_array->items)*sizeof(int));
        }
        else if (streq(attr->string,"cubes"))
        {
            return DX_NOT_SUPPORTED_ERROR;
        }
        else if (streq(attr->string,"cubes2D"))
        {
            return DX_NOT_SUPPORTED_ERROR;
        }
        else if (streq(attr->string,"cubesnD"))
        {
            return DX_NOT_SUPPORTED_ERROR;
        }
        else if (streq(attr->string,"tetrahedra"))
        {
            for (i=0;i<ugdata->numCells;i++)
            {
                ugdata->numVerts[i] = 4;
                ugdata->cellTypes[i] = VTK_TETRA; 
            }
            // again simple enough
            memcpy((void*)ugdata->cells,con_array->data,(con_array->shape[0])*(con_array->items)*sizeof(int));
        }
        
        vtkFile->dataset = ugdata;
        return DX_SUCCESS;
    }
    else
    {
        return DX_NOT_SUPPORTED_ERROR;
    }

}

/**
 * @brief converts an OpenDX data array into a vtk data attribute
 * @details Arrays in OpenDx can be scalar, vector, or tensor etc... however
 * vtk does provide a disinction here. so the the converison is done and then appended
 * a vtkData object, whic could be cell or point data.
 * @param arrayObject a pointer to the object wrapper for the data array to convert
 * @param data pointer to the vtkdata object (will either be cell or point data)
 * @returns DX_SUCCESS or completion, otherwise an appropriate error is returned
 * @note this function modifies the vtk data object, it will append scalar, vectoror tensor
 * data as required. 
 */
int dxArray2vtkData(object *arrayObject, vtkData* data)
{
    int i,j,k;
    // reset to use as an index
    j = data->numScalars;
    k = data->numVectors;
    if (!(streq(arrayObject->alias,"positions") || streq(arrayObject->alias,"connections")))
    {
        array * data_array;
        size_t nbytes;
        data_array = (array *)arrayObject->obj;
      
        switch(data_array->type)
        {
            case DX_INT:
                nbytes = DX_INT_SIZE;
                break;
            case DX_FLOAT:
                nbytes = DX_FLOAT_SIZE;
                break;
        }

        if (data_array->rank == 0)
        {
            data->scalar_data[j].data = malloc(nbytes*data_array->items);
            if (data->scalar_data[j].data == NULL)
            {
                return DX_MEMORY_ERROR;
            }
            // copydata
            memcpy(data->scalar_data[j].data,data_array->data,nbytes*data_array->items);
            data->scalar_data[j].type = data_array->type;
            strncpy(data->scalar_data[j].name,arrayObject->alias,DX_MAX_TOKEN_LENGTH);
            data->numScalars++;
        }
        else if (data_array->rank == 1 && data_array->shape[0] == 3)
        {
            data->vector_data[k].data = malloc(nbytes*(data_array->items)*data_array->shape[0]);
            if (data->vector_data[k].data == NULL)
            {
                return DX_MEMORY_ERROR;
            }
                // copydata
            memcpy(data->vector_data[k].data,data_array->data,nbytes*(data_array->items)*data_array->shape[0]);
            data->vector_data[k].type = data_array->type;
            strncpy(data->vector_data[k].name,arrayObject->alias,DX_MAX_TOKEN_LENGTH);
            data->numVectors;
        }
    }
    return DX_SUCCESS;
}



/**
 * @brief converts a dx file to a vtk file
 * @detials deteremines from the field infomation (e.g., positions, connections etc)
 * the type of vtk data to use.
 * @param dxf dx file pointer
 * @param vtkf vtk file pointer
 *
 */
int dxFile2vtkDataFiles(dxFile *dxf, vtkDataFile ***vtkf, int * numFiles)
{
    int i,j,k;
    int numFields;
    object * obj;
    object * positions;
    object * connections;
    
    object ** fieldObjects;
    series * seriesHeader;
    group * groupHeader;

    vtkDataFile **vtkFiles;

    seriesHeader = NULL;
    groupHeader = NULL;


    numFields = 1; 
    // check if any groups or series data exists
    // Assumption: only one group or one series may exist in a single file
    for (i=0;i<dxf->numObjects;i++)
    {
        if (dxf->objs[i].class == DX_SERIES)
        {
            seriesHeader = (series *)dxf->objs[i].obj;
            numFields = seriesHeader->numMembers;
            break;
        }
        else if (dxf->objs[i].class == DX_GROUP)
        {
            numFields = groupHeader->numMembers;
            groupHeader = (group *)dxf->objs[i].obj;
            break;
        }
    }
    // allocate memeory for field pointers
    fieldObjects = (object **)malloc(numFields*sizeof(object*));
    if (fieldObjects == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    // allocate memory for vtkdatafile structures
    vtkFiles = (vtkDataFile **)malloc(numFields*sizeof(vtkFiles));
    if (vtkFiles == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    for (i=0;i<numFields;i++)
    {
        vtkFiles[i] = (vtkDataFile *)malloc(sizeof(vtkDataFile));
        if (vtkFiles[i] == NULL)
        {
            return DX_MEMORY_ERROR;
        }
        vtkFiles[i]->pointdata = (vtkData *)malloc(sizeof(vtkData));
        if (vtkFiles[i]->pointdata == NULL)
        {
            return DX_MEMORY_ERROR;
        }
        vtkFiles[i]->celldata = (vtkData *)malloc(sizeof(vtkData));
        if (vtkFiles[i]->celldata == NULL)
        {
            return DX_MEMORY_ERROR;
        }
    }

    // get the list of fields to convert
    if (seriesHeader != NULL)
    {
        for (i=0;i<seriesHeader->numMembers;i++)
        {
            fieldObjects[i] = seriesHeader->members[i];
        }
    }
    else if (groupHeader != NULL)
    {
        for (i=0;i<groupHeader->numMembers;i++)
        {
            fieldObjects[i] = groupHeader->members[i];
        }
    }
    else
    {
        // no groups or series, so there must just be a lone field
        for (i=0;i<dxf->numObjects;i++)
        {
            if (dxf->objs[i].class == DX_FIELD)
            {
                fieldObjects[0] = &(dxf->objs[i]);        
            }
        }
    }

    // now do the conversions
    for (i=0;i<numFields;i++)
    {
        int rc;
        field * fieldHeader;
        // store the header info
        sprintf(vtkFiles[i]->vtkVersion,"%s",VTK_VERSION);
        sprintf(vtkFiles[i]->title,"Converted from OpenDX file %s field %d\n",dxf->filename,fieldObjects[i]->name);
    
        /** @todo for now only support ASCII vtk file (need to change this later)*/
        vtkFiles[i]->dataType = VTK_ASCII;
        vtkFiles[i]->pointdata->numScalars = 0;
        vtkFiles[i]->pointdata->numVectors = 0;
#ifdef DEBUG
        printf("VTK file header [file %d of %d]\n",i,numFiles);
        printf("\t# vtk DataFile Version %s\n",vtkFiles[i]->vtkVersion);
        printf("\t%s\n",vtkFiles[i]->title);
#endif

        rc = dxField2VTKDataSet(fieldObjects[i], vtkFiles[i]);
        if (rc != DX_SUCCESS)
        {
            return rc;
        }
        
        // allocate memory for pointt data
        vtkFiles[i]->pointdata->scalar_data = (scalar *)malloc((vtkFiles[i]->pointdata->numScalars)*sizeof(scalar));
        vtkFiles[i]->pointdata->vector_data = (vector *)malloc((vtkFiles[i]->pointdata->numVectors)*sizeof(vector));
        if (vtkFiles[i]->pointdata->scalar_data == NULL || vtkFiles[i]->pointdata->vector_data == NULL)
        {
            return DX_MEMORY_ERROR;
        }

        // convert each component 
        fieldHeader = (field *)(fieldObjects[i]->obj);
        for (j=0;j<fieldHeader->numComponents;i++)
        {
            if (fieldHeader->components[i]->class == DX_ARRAY)
            {
                attribute * attr;
                attr = GetAttribute(fieldHeader->components[i],"dep");
                if (streq(attr->string,"positions"))
                {
                    rc = dxArray2vtkData(fieldHeader->components[i],vtkFiles[i]->pointdata);
                }
                else if (streq(attr->string,"connections"))
                {
                    rc = dxArray2vtkData(fieldHeader->components[i],vtkFiles[i]->celldata);
                }

                if (rc != DX_SUCCESS)
                {
                    return rc;
                }
            }
        }
    }

    
    *vtkf = vtkFiles;
    *numFiles = numFields;

    return DX_SUCCESS;
}

/**
 * @brief the progam entry point
 */
int main(int argc, char ** argv)
{
    dxFile input;
    vtkDataFile output;
    char * dxfilename;
    char * vtkfilename;
    int i;
    int rc;
    if (argc != 3)
    {
        fprintf(stderr,"Usage: dx2vtk filename.dx filename.vtk\n");
        exit(1);
    }

    dxfilename = argv[1];
    if ((rc = DX_Open(&input,dxfilename)) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Could not Open DX file [code: %d]\n",rc);
        exit(1);
    }


    if ((rc = DX_LoadAll(&input)) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Could not Load DX file contents [code: %d]\n",rc);
        exit(1);
    }

printf(" num objects %d\n",input.numObjects);
#ifdef DEBUG
    for(i=0;i<input.numObjects;i++)
    {
        printf("name %d [%s]\n",i,input.objs[i].name);
        PrintObjectHeader(&(input.objs[i]));
    }
    exit(0);
#endif

    DX_Close(&input);

    // do conversion
    // @todo modify to convert multiple files
    /*if ((rc = dxFile2vtkDataFile(&input, &output)) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Conversion failed [code %d]\n",rc);
        exit(1);
    }

    vtkfilename = argv[2];
    VTK_Open(&output,vtkfilename);
    VTK_Write(&output);
    VTK_Close(&output);*/
}


