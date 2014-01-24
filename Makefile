#!/bin/make

INSTALLDIR = /usr/local/bin

CC = gcc
#COPTS = -g -DDEBUG
COPTS = -O2
SRC = dxFileReader.c vtkFileWriter.c dx2vtk.c ioutils.c
OBJS = $(SRC:.c=.o)
INC = 
LIB = -lm 
BINARY = dx2vtk

all:
	make $(BINARY)

ioutils.o: ioutils.c
	$(CC) $(COPTS) -o $@ -c $< 

dxFileReader.o: dxFileReader.c
	$(CC) $(COPTS) -o $@ -c $< 

vtkFileWriter.o: vtkFileWriter.c
	$(CC) $(COPTS) -o $@ -c $< 

dx2vtk.o: dx2vtk.c
	$(CC) $(COPTS) -o $@ -c $< 

$(BINARY): $(OBJS)
	$(CC) $(COPTS) -o $@ $(OBJS) $(LIB)

install: $(BINARY)
	cp $(BINARY) $(INSTALLDIR)
	chmod 755 $(INSTALLDIR)/$(BINARY)

clean:
	rm -f *.o $(BINARY)

