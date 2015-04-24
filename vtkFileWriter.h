/* dx2vtk: OpenDX to VTK file format converter
 * Copyright (C) 2015  David J. Warne
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file vtkFileWriter.h
 * @brief Writes a vtk data file using the legacy format
 * 
 * @details This library implements a light weight legacy vtk
 * format writer. This is intended to be used as a part of the 
 * dx2vtk program.
 *
 * @author David J. Warne (david.warne@qut.edu.au)
 * @author High Performance Computing and Research Support
 * @author Queensland University of Technology
 *
 */

#ifndef __VTKFILEWRITER_H
#define __VTKFILEWRITER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <endian.h>

/*data types*/
#define VTK_ASCII 0
#define VTK_BINARY 1

#define H2BE32(buf,size)                  \
{                                         \
    int iii;                              \
    uint32_t * buf32;                     \
    buf32 = (uint32_t *)(buf);            \
    for(iii=0;iii<(size);iii++)           \
    {                                     \
        buf32[iii] = htobe32(buf32[iii]); \
    }                                     \
}                                         \

#define BE2H32(buf,size)                  \
{                                         \
    int iii;                              \
    uint32_t * buf32;                     \
    buf32 = (uint32_t *)(buf);            \
    for(iii=0;iii<(size);iii++)           \
    {                                     \
        buf32[iii] = be32toh(buf32[iii]); \
    }                                     \
}                                         \


/*geometry*/
#define VTK_STRUCTURED_POINTS 0
#define VTK_STRUCTURED_GRID 1
#define VTK_UNSTRUCTURED_GRID 2
#define VTK_POLYDATA 3
#define VTK_RECTILINEAR_GRID 4
#define VTK_FIELD 5


#define VTK_VERTEX 1
#define VTK_POLY_VERTEX 2
#define VTK_LINE 3
#define VTK_POLY_LINE 4
#define VTK_TRIANGLE 5
#define VTK_TRIANGLE_STRIP 6
#define VTK_POLYGON 7
#define VTK_PIXEL 8
#define VTK_QUAD 9
#define VTK_TETRA 10
#define VTK_VOXEL 11
#define VTK_HEXAHEDRON 12
#define VTK_WEDGE 13
#define VTK_PYRAMID 14

#define VTK_QUADRATIC_EDGE 21
#define VTK_QUADRATIC_TRIANGLE 22
#define VTK_QUADRATIC_QUAD 23
#define VTK_QUADRATIC_TETRA 24
#define VTK_QUADRATIC_HEXAHEDRON 25

#define VTK_VERSION "4.2"

#define VTK_TITLE_LENGTH 256
#define VTK_DIM 3

#define VTK_INT 0
#define VTK_FLOAT 1


#define VTK_SUCCESS                  1
#define VTK_MEMORY_ERROR             0
#define VTK_FILE_NOT_FOUND_ERROR     -1
#define VTK_INVALID_FILE_ERROR       -2
#define VTK_NOT_SUPPORTED_ERROR      -3
#define VTK_INVALID_USAGE_ERROR      -4
#define VTK_NOT_IMPLEMENTED_YET_ERROR -5
#define VTK_FILE_ERROR -6

#ifndef VTK_TYPE_DEFAULT
#define VTK_TYPE_DEFAULT VTK_ASCII
#endif

typedef struct vtkDataFile_struct vtkDataFile;
typedef struct unstructuredGrid_struct unstructuredGrid;
typedef struct structuredPoints_struct structuredPoints;
typedef struct polydata_struct polydata;
typedef struct vtkData_struct vtkData;
typedef struct scalar_struct scalar;
typedef struct vector_struct vector;

struct scalar_struct {
    char name[32];
    int type;
    void * data;
};

struct vector_struct {
    char name[32];
    int type;
    void *data;
};

struct vtkData_struct {
    int numScalars;
    int numColorScalars;
    int numLookupTables;
    int numVectors;
    int numNormals;
    int numTextureCoords;
    int numTensors;
    int numFields;
    int size;
    /** @todo for now only support scalars and vectors */
    scalar *scalar_data;
    vector *vector_data;
};

struct vtkDataFile_struct {
    FILE *fp;
    char vtkVersion[4]; /*header version*/
    char title[VTK_TITLE_LENGTH]; /**/
    unsigned char dataType; 
    unsigned char geometry;
    void * dataset;
    vtkData * pointdata;
    vtkData * celldata;
};

struct structuredPoints_struct {
    int dimensions[VTK_DIM];
    float origin[VTK_DIM];
    float spacing[VTK_DIM];
};

struct structuredGrid_struct {
    int dimensions[VTK_DIM];
    int numPoints;
    float *points; // numpoints*3;
};

struct polydata_struct{
    int numPoints;
    float * points;
    int numPolygons;
    int *numVerts;
    int *polygons;
};

struct rectilinearGrid_struct{
    int dimensions[VTK_DIM];
    int numX;
    int numY;
    int numZ;
    float *X_coordinates;
    float *Y_coordinates;
    float *Z_coordinates;
};


struct unstructuredGrid_struct{
    int numPoints;
    float * points;
    int numCells;
    int cellSize;
    int *cells;
    int *numVerts;
    int *cellTypes;
};

/*function prototypes  */
int VTK_Open(vtkDataFile *file, char * filename);
int VTK_Write(vtkDataFile *file);
int VTK_WriteUnstructuredGrid(FILE *fp,unstructuredGrid *ug,char type);
int VTK_WritePolydata(FILE *fp,polydata *pd,char type);
int VTK_WriteStructuredPoints(FILE *fp,structuredPoints *sp,char type);
int VTK_WriteData(FILE *fp,vtkData *data,char type);
int VTK_Close(vtkDataFile*file);
#endif
