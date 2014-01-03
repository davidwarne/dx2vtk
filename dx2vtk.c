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
 * @brief converts a dx file to a vtk file
 * @detials deteremines from the field infomation (e.g., positions, connections etc)
 * the type of vtk data to use.
 * @param dxf dx file pointer
 * @param vtkf vtk file pointer
 *
 */
int dxFile2vtkDataFile(dxFile *dxf, vtkDataFile *vtkf)
{
    // for now we are assuming that a dx file only has a single 
    // field... really a vtk file maps better to a single field,
    // multiple fields (or groups) should map to multiple vtk files.
    int i,j,k;
    object * obj;
    object * pos;
    object * con;
    field * fld;
    int numPoints;

    sprintf(vtkf->vtkVersion,"%s",VTK_VERSION);
    sprintf(vtkf->title,"Converted from OpenDX file %s\n",dxf->filename);
    /** @todo for now only support ASCII vtk file (need to change this later)*/
    
    /** @todo implement cell data also, but to do this I need to parse the dx attributes (which right now I am not)*/
    vtkf->pointdata = (vtkData *)malloc(sizeof(vtkData));
    if (vtkf->pointdata == NULL)
    {
        return DX_MEMORY_ERROR;
    }
    vtkf->celldata = NULL;

    vtkf->dataType = VTK_ASCII;

    vtkf->pointdata->numScalars = 0;
    vtkf->pointdata->numVectors = 0;
#ifdef DEBUG
    printf("VTK file header\n");
    printf("\t# vtk DataFile Version %s\n",vtkf->vtkVersion);
    printf("\t%s\n",vtkf->title);
#endif

    // find a field
    for (i=0;i<(dxf->numObjects);i++)
    {
        if (dxf->objs[i].class == DX_FIELD)
        {
            obj = &(dxf->objs[i]);
        }
    }
    fld = (field *)(obj->obj);

#ifdef DEBUG
    printf("Writing Field %d [%s] %hhu\n",obj->number,obj->name,obj->isLoaded);
    printf("Num Components: %d\n",fld->numComponents);
#endif
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
            if (((array *)(comp->obj))->rank == 0)
            {
                vtkf->pointdata->numScalars++;
            }
            else if ( ((array *)(comp->obj))->rank == 1 && ((array *)(comp->obj))->shape[0] == 3)
            {
                vtkf->pointdata->numVectors++;
            }
        }
    }


    // dx positions and connections map to a vtk data set type
    //  the following is incorrect... shape 3 connections equals polydata
    //  whereas shape 4 equals Unstructured Grid... maybe just make both cases
    //  unstructured grid...
    /** @todo put this is a function and support all types*/
    if (pos->class == DX_ARRAY && con->class == DX_ARRAY)
    {
        array *pos_array;
        array *con_array;
        attribute *attr;
        unstructuredGrid *ugdata;
        vtkf->geometry = VTK_UNSTRUCTURED_GRID;
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
        
        vtkf->dataset = ugdata;
        numPoints = ugdata->numPoints;
    }

    // allocate memory for pointt data
    vtkf->pointdata->scalar_data = (scalar *)malloc((vtkf->pointdata->numScalars)*sizeof(scalar));
    vtkf->pointdata->vector_data = (vector *)malloc((vtkf->pointdata->numVectors)*sizeof(vector));

    if (vtkf->pointdata->scalar_data == NULL || vtkf->pointdata->vector_data == NULL)
    {
        return DX_MEMORY_ERROR;
    }

    // reset to use as an index
    j = 0;
    k = 0;
    // convert all data objects
    /** @todo now that attributes are parsed I could distingish between position
     * and connection data
     */
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
        if (!(streq(comp->alias,"positions") || streq(comp->alias,"connections")))
        {
            array * data_array;
            size_t nbytes;
            data_array = (array *)comp->obj;
      
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
                vtkf->pointdata->scalar_data[j].data = malloc(nbytes*numPoints);
                if (vtkf->pointdata->scalar_data[j].data == NULL)
                {
                    return DX_MEMORY_ERROR;
                }
                // copydata
                memcpy(vtkf->pointdata->scalar_data[j].data,data_array->data,nbytes*numPoints);
                vtkf->pointdata->scalar_data[j].type = data_array->type;
                strncpy(vtkf->pointdata->scalar_data[j].name,comp->alias,DX_MAX_TOKEN_LENGTH);
                j++;
            }
            else if (data_array->rank == 1 && data_array->shape[0] == 3)
            {
                vtkf->pointdata->vector_data[k].data = malloc(nbytes*numPoints*data_array->shape[0]);
                if (vtkf->pointdata->vector_data[k].data == NULL)
                {
                    return DX_MEMORY_ERROR;
                }
                // copydata
                memcpy(vtkf->pointdata->vector_data[k].data,data_array->data,nbytes*numPoints*data_array->shape[0]);
                vtkf->pointdata->vector_data[k].type = data_array->type;
                strncpy(vtkf->pointdata->vector_data[k].name,comp->alias,DX_MAX_TOKEN_LENGTH);
                k++;
            }
        }
    }

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

    printf(" num objects %d\n",input.numObjects);
    for(i=0;i<input.numObjects;i++)
    {
        printf("name %d [%s]\n",i,input.objs[i].name);
    }
    
    if ((rc = DX_LoadAll(&input)) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Could not Load DX file contents [code: %d]\n",rc);
        exit(1);
    }

    DX_Close(&input);

    // do conversion

    if ((rc = dxFile2vtkDataFile(&input, &output)) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Conversion failed [code %d]\n",rc);
        exit(1);
    }

    vtkfilename = argv[2];
    VTK_Open(&output,vtkfilename);
    VTK_Write(&output);
    VTK_Close(&output);
}


