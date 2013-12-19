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
    dxFile file;
    char * dxfilename;
    char * vtkfilename;
    int i;

    if (argc != 3)
    {
        fprintf(stderr,"Usage: dx2vtk filename.dx filename.vtk\n");
        exit(1);
    }

    dxfilename = argv[1];
    if (DX_Open(&file,dxfilename) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Could not Open DX file\n");
        exit(1);
    }

    printf(" num objects %d\n",file.numObjects);
    for(i=0;i<file.numObjects;i++)
    {
        printf("name %d [%s]\n",i,file.objs[i].name);
    }
    
    if (DX_LoadAll(&file) != DX_SUCCESS)
    {
        fprintf(stderr,"Error: Could not Load DX file contents\n");
        exit(1);
    }
}


