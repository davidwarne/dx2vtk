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

int main(int argc, char ** argv)
{
    dxFile myfile;
    DX_Open(&myfile,argv[1]);
}
