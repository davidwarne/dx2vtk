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
        return -1;
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
            rc = VTK_WritePolydata(file->fp,(polydata *)file->dataset);
            break;
        case VTK_UNSTRUCTURED_GRID:
            rc = VTK_WriteUnstructuredGrid(file->fp,(unstructuredGrid *)file->dataset);
            break;
        case VTK_STRUCTURED_POINTS:
            rc = VTK_WriteStructuredPoints(file->fp,(structuredPoints *)file->dataset);
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
        rc = VTK_WriteData(file->fp,file->pointdata);
    }

    if (file->celldata->size > 0)
    {
        fprintf(file->fp,"\nCELL_DATA ");
        rc = VTK_WriteData(file->fp,file->celldata);
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
 */
int VTK_WriteUnstructuredGrid(FILE *fp,unstructuredGrid *ug)
{
    int i,j,t;
    int size;
    fprintf(fp,"UNSTRUCTURED_GRID\n");
    fprintf(fp,"POINTS %d float\n",ug->numPoints);
    for (i=0;i<(ug->numPoints);i++)
    {
        fprintf(fp,"%f %f %f\n",ug->points[i*3],ug->points[i*3+1],ug->points[i*3+2]);
    }

    size = ug->numCells;
    for (i=0;i<(ug->numCells);i++)
    {
        size += ug->numVerts[i];
    }

    fprintf(fp,"CELLS %d %d\n",ug->numCells,size);
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
    fprintf(fp,"CELL_TYPES %d\n",ug->numCells);
    for (i=0;i<(ug->numCells);i++)
    {
        fprintf(fp,"%d\n",ug->cellTypes[i]);
    }
    return VTK_SUCCESS;
}

/**
 * @brief writes a VTK poly data mesh to the file output stream
 * @param fp the file output stream
 * @param pd the polydata mesh
 */
int VTK_WritePolydata(FILE *fp,polydata *pd)
{
    int i,j,t;
    int size;
    fprintf(fp,"POLYDATA\n");
    /**@todo assert that points are floats */
    fprintf(fp,"POINTS %d float\n",pd->numPoints);
    for (i=0;i<pd->numPoints;i++)
    {
        fprintf(fp,"%f %f %f\n",pd->points[i*3],pd->points[i*3+1],pd->points[i*3 +2]);
    }

    size = pd->numPolygons;
    for (i=0;i<pd->numPolygons;i++)
    {
        size += pd->numVerts[i];
    }
            
    fprintf(fp,"POLYGONS %d %d\n",pd->numPolygons,size);
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

    return VTK_SUCCESS;
}

/**
 * @brief wriets a VTK structured point mesh
 * @param fp the output file stream
 * @param the structured point mesh
 */
int VTK_WriteStructuredPoints(FILE *fp,structuredPoints *sp)
{
    int i;
    fprintf(fp,"STRUCTURED_POINTS\n");
    fprintf(fp,"DIMENSIONS");
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
 */
int VTK_WriteData(FILE *fp,vtkData *data)
{
    int i;
    fprintf(fp,"%d\n",data->size);
#ifdef DEBUG
    printf("writing %d numScalars %d numVectors %d\n",data->size,data->numScalars,data->numVectors);
#endif
    // write scalar data
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

    return VTK_SUCCESS;

}
/**
 * @brief closes the vtk file
 */
int VTK_Close(vtkDataFile*file)
{
    fclose(file->fp);
}
