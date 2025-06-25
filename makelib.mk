CC = gcc
CFLAGS = -DDEBUG
LIBNAME = libksocket.a
OBJFILES = ksocket.o

library: $(OBJFILES)
	ar rcs $(LIBNAME) $(OBJFILES)

$(OBJFILES) : ksocket.h ksocket.c

clean:
	rm -f $(OBJFILES) *.a