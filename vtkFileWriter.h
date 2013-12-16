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
/*data types*/
#define VTK_ASCII 0
#define VTK_BINARY 1

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

typedef struct vtkDataFile_struct vtkDataFile;

struct vtkDataFile_struct {
    char[4] vtkVersion; /*header version*/
    char[256] title; /**/
    unsigned char dataType; 
    unsigned char Geometry;
    void * dataset;
    void * pointdata;
    void * celldata;
};

struct structuredPoints_struct {
    int dimensions[3];
    float origin[3];
    float spacing[3];
};

struct structuredGrid_struct {
    int dimensions[3];
    int numPoints;
    float *points; // numpoints*3;
};

struct rectilinearGrid_struct{
    int dimensions[3];
    int numX;
    int numY;
    int numZ;
    float *X_coordinates;
    float *Y_coordinates;
    float *Z_coordinates;
};

/*
struct polydata_struct {
    int numPoints;
    float *points;
    int

};
*/

struct unstructuredGrid_struct{
    int numPoints;
    float * points;
    int numCells;
    int cellSize;
    int *cells;
    int *cellTypes;
};
#endif
