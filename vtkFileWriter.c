#include "vtkFileWriter.h"

/**
 * @brief Opens a vtk file for writing
 * @param file the vtk file object
 * @param filename the filename to open the object under
 *
 */
int VTK_Open(vtkDataFile *file, char * filename)
{
    file->fp = fopen(filename,"w");
    if (file->fp == NULL)
    {
        return VTK_FILE_ERROR;
    }
}

/**
 * @brief exports the vtk data file object to a file
 */
int VTK_Write(vtkDataFile *file)
{
    int rc;
    // write the header and title
    fprintf(file->fp,"# vtk DataFile Version %s\n",file->vtkVersion);
    fprintf(file->fp,"%s",file->title);
    // print data type
    if (file->dataType == VTK_ASCII)
    {
        fprintf(file->fp,"ASCII\n");
    }
    else
    {
        fprintf(file->fp,"BINARY\n");
    }
    // print data set
    fprintf(file->fp,"DATASET ");
    /** @todo abstract to sub functions */
    switch(file->geometry)
    {
        case VTK_POLYDATA:
            rc = VTK_WritePolydata(file->fp,(polydata *)file->dataset,file->dataType);
            break;
        case VTK_UNSTRUCTURED_GRID:
            rc = VTK_WriteUnstructuredGrid(file->fp,(unstructuredGrid *)file->dataset,file->dataType);
            break;
        case VTK_STRUCTURED_POINTS:
            rc = VTK_WriteStructuredPoints(file->fp,(structuredPoints *)file->dataset,file->dataType);
            break;
    }

    if (rc != VTK_SUCCESS)
    {
        return rc;
    }
    
    // now write the point data and cell data
    if (file->pointdata->size > 0)
    {
        fprintf(file->fp,"\nPOINT_DATA ");
        rc = VTK_WriteData(file->fp,file->pointdata,file->dataType);
    }

    if (file->celldata->size > 0)
    {
        fprintf(file->fp,"\nCELL_DATA ");
        rc = VTK_WriteData(file->fp,file->celldata,file->dataType);
    }
    if (rc != VTK_SUCCESS)
    {
        return rc;
    }

    return VTK_SUCCESS;
}
/**
 * @brief writes a VTK unstructured grid to the file ouput stream
 * @param fp the file output stream
 * @param ug the unstructured grid
 * @param type specifies write mode, either VTK_ASCII or VTK_BINARY
 */
int VTK_WriteUnstructuredGrid(FILE *fp,unstructuredGrid *ug, char type)
{
    int i,j,t;
    int size;
    fprintf(fp,"UNSTRUCTURED_GRID\n");
    fprintf(fp,"POINTS %d float\n",ug->numPoints);
    if (type == VTK_ASCII)
    {
        for (i=0;i<(ug->numPoints);i++)
        {
            fprintf(fp,"%f %f %f\n",ug->points[i*3],ug->points[i*3+1],ug->points[i*3+2]);
        }
    }
    else
    {
        size_t n;
        H2BE32(ug->points,ug->numPoints*3);
        n = fwrite((void *)ug->points,sizeof(float),ug->numPoints*3,fp);
        BE2H32(ug->points,ug->numPoints*3);
        if (n != ug->numPoints*3)
        {
            return VTK_FILE_ERROR;
        }
    }

    size = ug->numCells;
    for (i=0;i<(ug->numCells);i++)
    {
        size += ug->numVerts[i];
    }

    fprintf(fp,"CELLS %d %d\n",ug->numCells,size);
    if (type == VTK_ASCII)
    {
        t=0;
        for (i=0;i<(ug->numCells);i++)
        {
            fprintf(fp,"%d",ug->numVerts[i]);
            for (j=t;j<(t+ug->numVerts[i]);j++)
            {
                fprintf(fp," %d",ug->cells[j]);
            }
            fprintf(fp,"\n");
            t=j;
        }
    }
    else 
    {
        int *tmp_buffer = (int*)malloc(size*sizeof(int));
        int ii;
        size_t n;
        if (tmp_buffer == NULL)
        {
            return VTK_MEMORY_ERROR;
        }

        t=0;
        ii=0;
        for (i=0;i<(ug->numCells);i++)
        {
            tmp_buffer[ii] = ug->numVerts[i];
            ii++;
            for (j=t;j<(t+ug->numVerts[i]);j++)
            {
                tmp_buffer[ii] = ug->cells[j];
                ii++;   
            }
            t=j;
        }
        H2BE32(tmp_buffer,size);
        n = fwrite((void*)tmp_buffer,sizeof(int),size,fp);
        BE2H32(tmp_buffer,size);
        free(tmp_buffer);
        tmp_buffer = NULL;
        if (n != size)
        {
            return VTK_FILE_ERROR;
        }

    }
    fprintf(fp,"CELL_TYPES %d\n",ug->numCells);
    if (type == VTK_ASCII)
    {
        for (i=0;i<(ug->numCells);i++)
        {
            fprintf(fp,"%d\n",ug->cellTypes[i]);
        }
    }
    else
    {
        size_t n;
        H2BE32(ug->cellTypes,ug->numCells);
        n = fwrite((void*)ug->cellTypes,sizeof(int),ug->numCells,fp);
        H2BE32(ug->cellTypes,ug->numCells);
        if (n != ug->numCells)
        {
            return VTK_FILE_ERROR;
        }
    }
    return VTK_SUCCESS;
}

/**
 * @brief writes a VTK poly data mesh to the file output stream
 * @param fp the file output stream
 * @param pd the polydata mesh
 * @param type specifies write mode, either VTK_ASCII or VTK_BINARY
 */
int VTK_WritePolydata(FILE *fp,polydata *pd,char type)
{
    int i,j,t;
    int size;
    fprintf(fp,"POLYDATA\n");
    /**@todo assert that points are floats */
    fprintf(fp,"POINTS %d float\n",pd->numPoints);
    if (type == VTK_ASCII)
    {
        for (i=0;i<pd->numPoints;i++)
        {
            fprintf(fp,"%f %f %f\n",pd->points[i*3],pd->points[i*3+1],pd->points[i*3 +2]);
        }
    }
    else
    {
        size_t n;
        uint32_t * buf32be;
        H2BE32(pd->points,pd->numPoints*3);
        n = fwrite((void*)pd->points,sizeof(float),pd->numPoints*3,fp);
        BE2H32(pd->points,pd->numPoints*3);

        if (n != pd->numPoints*3)
        {
            return VTK_FILE_ERROR;
        }
    }
    size = pd->numPolygons;
    for (i=0;i<pd->numPolygons;i++)
    {
        size += pd->numVerts[i];
    }
            
    fprintf(fp,"POLYGONS %d %d\n",pd->numPolygons,size);
    if (type == VTK_ASCII)
    {
        t=0;
        for (i=0;i<pd->numPolygons;i++)
        {
            fprintf(fp,"%d",pd->numVerts[i]);
            for (j=t;j<(t+pd->numVerts[i]);j++)
            {
                fprintf(fp," %d",pd->polygons[j]);
            }
            fprintf(fp,"\n");
            t=j;
        }
    }
    else
    {
        int *tmp_buffer;
        int ii;
        size_t n;
        uint32_t *buffer32;
        tmp_buffer = (int*)malloc(size*sizeof(int));
        if (tmp_buffer == NULL)
        {
            return VTK_MEMORY_ERROR;
        }
        
        ii=0;
        t=0;
        for (i=0;i<pd->numPolygons;i++)
        {
            tmp_buffer[ii] = pd->numVerts[i];
            ii++;
            for (j=t;j<(t+pd->numVerts[i]);j++)
            {
                tmp_buffer[ii] = pd->polygons[j];
                ii++;
            }
            t=j;
        }

        H2BE32(tmp_buffer,size);
        n = fwrite((void*)tmp_buffer,sizeof(int),size,fp);
        BE2H32(tmp_buffer,size);
        free(tmp_buffer);
        tmp_buffer = NULL;
        if (n != size)
        {
            return VTK_FILE_ERROR;
        }
    }
    return VTK_SUCCESS;
}

/**
 * @brief wriets a VTK structured point mesh
 * @param fp the output file stream
 * @param the structured point mesh
 * @param type specifies write mode, either VTK_ASCII or VTK_BINARY
 */
int VTK_WriteStructuredPoints(FILE *fp,structuredPoints *sp,char type)
{
    int i;
    fprintf(fp,"STRUCTURED_POINTS\n");
    fprintf(fp,"DIMENSIONS");
    // structured points (also called ImageData) are written the same for binary or ascii
    for (i=0;i<VTK_DIM;i++)
    {
        fprintf(fp," %d",sp->dimensions[i]);
    }
    fprintf(fp,"\nORIGIN");
    for (i=0;i<VTK_DIM;i++)
    {
        fprintf(fp," %f",sp->origin[i]);
    }
    fprintf(fp,"\nSPACING");
    for (i=0;i<VTK_DIM;i++)
    {
        fprintf(fp," %f",sp->spacing[i]);
    }
    return VTK_SUCCESS;
}

/**
 * @brief writes VTK data attributes (i.e., point or cell data)
 * @param fp the ouptut file stream
 * @param data the vtkData struct
 * @param type specifies write mode, either VTK_ASCII or VTK_BINARY
 */
int VTK_WriteData(FILE *fp,vtkData *data,char type)
{
    int i;
    fprintf(fp,"%d\n",data->size);
#ifdef DEBUG
    printf("writing %d numScalars %d numVectors %d\n",data->size,data->numScalars,data->numVectors);
#endif
    // write scalar data
    if (type == VTK_ASCII)
    {
        for (i=0;i<(data->numScalars);i++)
        {
            int j;
            scalar * sd;
            sd = &(data->scalar_data[i]);
            // print the header
            switch (sd->type)
            {
                case VTK_INT:
                {
                    int * datai = (int *)(sd->data);
                    fprintf(fp,"SCALARS %s int\nLOOKUP_TABLE default\n",sd->name);
                    for (j=0;j<data->size;j++)
                    {
                        fprintf(fp,"%d\n",datai[j]);
                    }
                }
                    break;
                case VTK_FLOAT:
                {
                    float * dataf = (float *)(sd->data);
                    fprintf(fp,"SCALARS %s float\nLOOKUP_TABLE default\n",sd->name);
                    for (j=0;j<data->size;j++)
                    {
                        fprintf(fp,"%f\n",dataf[j]);
                    }
                }
                    break;                    
            }
        }
    }
    else
    {
        for (i=0;i<(data->numScalars);i++)
        {   
            scalar * sd;
            sd = &(data->scalar_data[i]);
            size_t n;
            switch (sd->type)
            {
                case VTK_INT:
                    fprintf(fp,"SCALARS %s int\nLOOKUP_TABLE default\n",sd->name);
                    H2BE32(sd->data,data->size);
                    n = fwrite(sd->data,sizeof(int),data->size,fp);
                    BE2H32(sd->data,data->size);
                    if (n != data->size)
                    {
                        return VTK_FILE_ERROR;
                    }
                    break;
                case VTK_FLOAT:
                    fprintf(fp,"SCALARS %s float\nLOOKUP_TABLE default\n",sd->name);
                    H2BE32(sd->data,data->size);
                    n = fwrite(sd->data,sizeof(float),data->size,fp);
                    BE2H32(sd->data,data->size);
                    if (n != data->size)
                    {
                        return VTK_FILE_ERROR;
                    }
                    break;
            }
        }
    }

    if (type == VTK_ASCII)
    {
        // write vector data
        for (i=0;i<(data->numVectors);i++)
        {
            int j;
            vector *vd;
            vd = &(data->vector_data[i]);
            switch (vd->type)
            {
                case VTK_INT:
                {
                    int * datai = (int *)(vd->data);
                    fprintf(fp,"VECTORS %s int\n",vd->name);
                    for (j=0;j<data->size;j++)
                    {
                        fprintf(fp,"%d %d %d\n",datai[j*3],datai[j*3+1],datai[j*3+2]);
                    }
                }
                    break;
                case VTK_FLOAT:
                {
                    float * dataf = (float *)(vd->data);
                    fprintf(fp,"VECTORS %s float\n",vd->name);
                    for (j=0;j<data->size;j++)
                    {
                        fprintf(fp,"%f %f %f\n",dataf[j*3],dataf[j*3+1],dataf[j*3+2]);
                    }
                }
                    break;                    
            }
        }
    }
    else 
    {
         // write vector data
        for (i=0;i<(data->numVectors);i++)
        {
            vector *vd;
            vd = &(data->vector_data[i]);
            size_t n;
            switch (vd->type)
            {
                case VTK_INT:
                    fprintf(fp,"VECTORS %s int\n",vd->name);
                    H2BE32(vd->data,data->size);
                    n = fwrite((void*)vd->data,sizeof(int),data->size*VTK_DIM,fp);
                    BE2H32(vd->data,data->size);
                    if (n != data->size*VTK_DIM)
                    {
                        return VTK_FILE_ERROR;
                    }
                    break;
                case VTK_FLOAT:
                    fprintf(fp,"VECTORS %s float\n",vd->name);
                    H2BE32(vd->data,data->size);
                    n = fwrite((void*)vd->data,sizeof(float),data->size*VTK_DIM,fp);
                    BE2H32(vd->data,data->size);
                    if (n != data->size*VTK_DIM)
                    {
                        return VTK_FILE_ERROR;
                    }
                    break;                    
            }
        }
       
    }

    return VTK_SUCCESS;

}
/**
 * @brief closes the vtk file
 */
int VTK_Close(vtkDataFile*file)
{
    fclose(file->fp);
}
