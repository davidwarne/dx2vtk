#!/bin/make

INSTALLDIR = /usr/local/bin

CC = gcc
#COPTS = -g -DDEBUG
COPTS = -O2
SRC = dxFileReader.c vtkFileWriter.c dx2vtk.c
OBJS = $(SRC:.c=.o)
INC = -I./ioutils
LIB = -lm -L./ioutils -lioutils
BINARY = dx2vtk

all:
	make $(BINARY)

dxFileReader.o: dxFileReader.c
	$(CC) $(COPTS) -o $@ -c $< $(INC)

vtkFileWriter.o: vtkFileWriter.c
	$(CC) $(COPTS) -o $@ -c $< 

dx2vtk.o: dx2vtk.c
	$(CC) $(COPTS) -o $@ -c $< $(INC) 

$(BINARY): $(OBJS)
	$(CC) $(COPTS) -o $@ $(OBJS) $(LIB)

install: $(BINARY)
	cp $(BINARY) $(INSTALLDIR)
	chmod 755 $(INSTALLDIR)/$(BINARY)

clean:
	rm -f *.o $(BINARY)

