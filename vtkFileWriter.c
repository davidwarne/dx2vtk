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
    int numPoints;
    int numCells;
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
        {
            int i,j,t;
            int size;
            fprintf(file->fp,"POLYDATA\n");
            polydata * data = (polydata *)file->dataset;
            /**@todo assert that points are floats */
            fprintf(file->fp,"POINTS %d float\n",data->numPoints);
            for (i=0;i<data->numPoints;i++)
            {
                fprintf(file->fp,"%f %f %f\n",data->points[i*3],data->points[i*3+1],data->points[i*3 +2]);
            }

            size = data->numPolygons;
            for (i=0;i<data->numPolygons;i++)
            {
                size += data->numVerts[i];
            }
            
            fprintf(file->fp,"POLYGONS %d %d\n",data->numPolygons,size);
            t=0;
            for (i=0;i<data->numPolygons;i++)
            {
                fprintf(file->fp,"%d",data->numVerts[i]);
                for (j=t;j<(t+data->numVerts[i]);j++)
                {
                    fprintf(file->fp," %d",data->polygons[j]);
                }
                fprintf(file->fp,"\n");
                t=j;
            }
            numPoints = data->numPoints;
            numCells = data->numPolygons;
        }
            break;
        case VTK_UNSTRUCTURED_GRID:
        {
            int i,j,t;
            int size;
            fprintf(file->fp,"UNSTRUCTURED_GRID\n");
            unstructuredGrid *data = (unstructuredGrid *) file->dataset;
            fprintf(file->fp,"POINTS %d float\n",data->numPoints);
            for (i=0;i<(data->numPoints);i++)
            {
                fprintf(file->fp,"%f %f %f\n",data->points[i*3],data->points[i*3+1],data->points[i*3+2]);
            }

            size = data->numCells;
            for (i=0;i<(data->numCells);i++)
            {
                size += data->numVerts[i];
            }

            fprintf(file->fp,"CELLS %d %d\n",data->numCells,size);
            t=0;
            for (i=0;i<(data->numCells);i++)
            {
                fprintf(file->fp,"%d",data->numVerts[i]);
                for (j=t;j<(t+data->numVerts[i]);j++)
                {
                    fprintf(file->fp," %d",data->cells[j]);
                }
                fprintf(file->fp,"\n");
                t=j;
            }
            fprintf(file->fp,"CELL_TYPES %d\n",data->numCells);
            for (i=0;i<(data->numCells);i++)
            {
                fprintf(file->fp,"%d\n",data->cellTypes[i]);
            }
            numPoints = data->numPoints;
            numCells = data->numCells;
        }
            break;
    }
    
    // now write the point data and cell data
    if (file->pointdata != NULL)
    {
        int i;
        fprintf(file->fp,"POINT_DATA %d\n",numPoints);
        // write scalar data
        for (i=0;i<(file->pointdata->numScalars);i++)
        {
            int j;
            scalar * sd;
            sd = &(file->pointdata->scalar_data[i]);
            // print the header
            switch (sd->type)
            {
                case VTK_INT:
                {
                    int * datai = (int *)(sd->data);
                    fprintf(file->fp,"SCALARS %s int\nLOOKUP_TABLE default\n",sd->name);
                    for (j=0;j<numPoints;j++)
                    {
                        fprintf(file->fp,"%d\n",datai[j]);
                    }
                }
                    break;
                case VTK_FLOAT:
                {
                    float * dataf = (float *)(sd->data);
                    fprintf(file->fp,"SCALARS %s float\nLOOKUP_TABLE default\n",sd->name);
                    for (j=0;j<numPoints;j++)
                    {
                        fprintf(file->fp,"%f\n",dataf[j]);
                    }
                }
                    break;                    
            }
        }

        // write vector data
        for (i=0;i<(file->pointdata->numVectors);i++)
        {
            int j;
            vector *vd;
            vd = &(file->pointdata->vector_data[i]);

            switch (vd->type)
            {
                case VTK_INT:
                {
                    int * datai = (int *)(vd->data);
                    fprintf(file->fp,"VECTORS %s int\n",vd->name);
                    for (j=0;j<numPoints;j++)
                    {
                        fprintf(file->fp,"%d %d %d\n",datai[j*3],datai[j*3+1],datai[j*3+2]);
                    }
                }
                    break;
                case VTK_FLOAT:
                {
                    float * dataf = (float *)(vd->data);
                    fprintf(file->fp,"VECTORS %s float\n",vd->name);
                    for (j=0;j<numPoints;j++)
                    {
                        fprintf(file->fp,"%f %f %f\n",dataf[j*3],dataf[j*3+1],dataf[j*3+2]);
                    }
                }
                    break;                    
            }

        }
    }

    if (file->celldata != NULL)
    {
    }
}

/**
 * @brief closes the vtk file
 */
int VTK_Close(vtkDataFile*file)
{
    fclose(file->fp);
}
