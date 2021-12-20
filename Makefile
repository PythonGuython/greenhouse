# Compiler and linker

CC = gcc
CCLD = $(CC)

# Compiler flags. We assemble them from (a) a set of general flags and (b) from
# the return strings of pkg-config for all packages we use
# -c instructs gcc toproduce a .o file, which must be linked spearately
# -Ox produces optimized code. Use -O0 for easier debugging, but -O2 or -O3
# for production code

DEBUGFLG = -g -Wall -fbounds-check
GTK_CFLAGS = -c -O0 -pthread
LDFLAGS = -lm -lrt -lpigpiod_if2

# Here, invoke pkg-config to get the specific package flags Note that neither gsl nor fftw
# add any cflags. Make sure we have Aravis included.
# pigpio seems to have no pkg-config representation, include it separately.
# Note also that pkg-config --list-all provides a list of all istalled packages

GTK_LIBS_INVOKE = $(shell pkg-config --cflags glib-2.0 gobject-2.0)
ARAVIS_FLAGS_INVOKE = $(shell pkg-config --cflags aravis-0.8)
IMGSAVE_FLAGS_INVOKE = $(shell pkg-config --cflags libpng libtiff-4)

GTK_LIBS = $(GTK_LIBS_INVOKE) $(ARAVIS_FLAGS_INVOKE) $(IMGSAVE_FLAGS_INVOKE)


# Libraries needed for the linker. We need to do the same shpiel, but with --libs
# And... pigpio is already included with the LDFLAGS

GTK_LDFLAGS_INVOKE = $(shell pkg-config --libs glib-2.0 gobject-2.0)
ARAVIS_LDFLAGS_INVOKE = $(shell pkg-config --libs aravis-0.8)
IMGSAVE_LDFLAGS_INVOKE = $(shell pkg-config --libs libpng libtiff-4)

LDADD = $(LDFLAGS) $(GTK_LDFLAGS_INVOKE) $(ARAVIS_LDFLAGS_INVOKE) $(IMGSAVE_LDFLAGS_INVOKE)




#-----------------------------------------------------------------------------
# So far, so good. Now define the Makefile targets.
# The first (and default) target is 'all', which is a list of, well,
# all targets.

all:	acquire


# 'all' is followed by the individual targets that are listed therein
# The syntax is (note the mandatory indentation):
#
#	target: dependency dependency ...
#	<indent>	command
#	<indent>	command
#	<indent>	command with \
#	<indent>	   continuation line

acquire: acquire.c
	$(CC)    $(DEBUGFLG) $(GTK_CFLAGS) $(GTK_LIBS) acquire.c
	$(CCLD)  $(LDFLAGS) $(LDADD) -o acquire acquire.o


# The 'clean' target: It removes all intermediate files, such as .o files

clean:
	rm -f *.o

