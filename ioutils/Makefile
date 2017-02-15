# This is a GNU Makefile.

# Creating a static and shared library
NAME = ioutils
TARGET = lib$(NAME)
STATIC = $(TARGET).a
SHARED = $(TARGET).so

CC = gcc
COPTS = -O2 -fPIC
AR = ar
AROPTS = -rcvs
# Libraries to use, objects to compile
SRC = ioutils.c
OBJ = $(SRC:.c=.o)

INSTALL_PREFIX = /usr/local

INCLUDE = 
LIB = 

all:
	make $(STATIC)
	make $(SHARED)

# Make it all!
$(STATIC): $(OBJ)
	$(AR) $(AROPTS) $@ $?
	
$(SHARED): $(OBJ)
	$(CC) -shared -o $@ $? $(LIB)

ioutils.o: ioutils.c
	$(CC) $(COPTS) -o $@ -c $< $(INCLUDE) 

install: $(STATIC) $(SHARED)
	mkdir -p $(INSTALL_PREFIX)/include/$(NAME)
	mkdir -p $(INSTALL_PREFIX)/lib/$(NAME)
	cp $(SHARED) $(INSTALL_PREFIX)/lib/$(NAME)/$(SHARED)
	cp $(STATIC) $(INSTALL_PREFIX)/lib/$(NAME)/$(STATIC)
	cp *.h $(INSTALL_PREFIX)/include/$(NAME)
	chmod 755 $(INSTALL_PREFIX)/include/$(NAME)
	chmod 755 $(INSTALL_PREFIX)/lib/$(NAME)
	chmod 644 $(INSTALL_PREFIX)/lib/$(NAME)/$(STATIC)
	chmod 755 $(INSTALL_PREFIX)/lib/$(NAME)/$(SHARED)
	chmod 644 $(INSTALL_PREFIX)/include/$(NAME)/*.h


# Standard make targets
clean:
	@rm -f *.o $(STATIC) $(SHARED)
