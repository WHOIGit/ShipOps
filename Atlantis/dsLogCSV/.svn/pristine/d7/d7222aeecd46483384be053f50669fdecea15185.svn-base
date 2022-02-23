# ---------------------------------------------------------------------- 
#
#   dsLogCsv Makefile
#
#   Modification History:
#   DATE 	AUTHOR 	COMMENT
#   2012-12-10  tt      Created and Written
#   2013-01-02  tt      - changed linker order of arguments due to linking failure in latest ubuntu release
#   2014-11-10  tt      - malor overhaul to properly accomodate two targets and different DSLOGCSV_MODE #define values
#
# ---------------------------------------------------------------------- 

CC = g++
LD = g++

# ----------------------------------------------------------------------
# 
# ----------------------------------------------------------------------

OSNAME=$(shell uname --)

ifeq ($(OSNAME),Linux)
OSDEF=-D_UNIX_ -D_LINUX_
endif

ifeq ($(OSNAME),QNX)
OSDEF=-D_UNIX_ -D_QNX_
endif

# ----------------------------------------------------------------------
#---- list of all header files
INCLUDES = Makefile  cmd_thread.h config_file.h launch_timer.h macros.h \
        msg_util.h nio_thread.h csvOutputThread.h csvLogThread.h sio_thread.h skel.h \
        stderr.h tcp_thread.h tcp_utilities.h time_util.h  \
        dsLogA2B.h dsLogCsv.h dsLogPostCsv.h dsLogTalk.h  readIniDsLogCSV.h \
        ssv_ChenMillero.h truewind_SmithBourassa.h

#---- list of all common object files for dsLogCSV and dsLogPostCSV
OBJECTS = cmd_thread.o config_file.o launch_timer.o msg_util.o \
        nio_thread.o csvOutputThread.o csvLogThread.o sio_thread.o \
        tcp_server_thread.o tcp_utilities.o time_util.o \
        dsLogA2B.o dsLogTalk.o \
        ssv_ChenMillero.o truewind_SmithBourassa.o

SRCS=$(OBJECTS:%.o=%.cpp)

# CFLAGS   = -g -Wall -pedantic -Wformat -W
#CFLAGS   = -g -Wall -pedantic -Wformat 
# note took out pedantic because of c++ warnings in sys/socket.h, howland, 1 oct 2001

# tt - 2010-06-13 add "-Wno-write-strings" to eliminate warnings about deprecated conversion
#    of string constant to 'char *'

CFLAGS   = -g -Wall -Wformat -Wno-write-strings -Wno-unused-but-set-variable
CPPFLAGS = $(CFLAGS)

#---- rule for compiling .cpp into .o
$(OBJECTS): %.o: %.cpp $(INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -c $< -o $@

#---- rule for compiling dsLogCsv.cpp into dsLogCsv.o
dsLogCsv.o: dsLogCsv.cpp $(INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -c $< -o $@

#---- rule for compiling dsLogPostCsv.cpp into dsLogPostCsv.o
dsLogPostCsv.o: dsLogPostCsv.cpp $(INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -c $< -o $@

#---- rule for compiling readIniDsLogCSV.cpp into postReadIniDsLogCSV.o
postReadIniDsLogCSV.o: readIniDsLogCSV.h readIniDsLogCSV.cpp $(INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -DDSLOGCSV_MODE=0 -c readIniDsLogCSV.cpp -o postReadIniDsLogCSV.o

#---- rule for compiling readIniDsLogCSV.cpp into realtimeReadIniDsLogCSV.o
realtimeReadIniDsLogCSV.o: readIniDsLogCSV.h readIniDsLogCSV.cpp $(INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -DDSLOGCSV_MODE=1 -c readIniDsLogCSV.cpp -o realtimeReadIniDsLogCSV.o

#---- rule for linking and creating executables
dsLogPostCSV: 	$(OBJECTS) $(INCLUDES) postReadIniDsLogCSV.o dsLogPostCsv.o Makefile
	$(LD) -g -Wall -Wformat -o $@ $(OBJECTS) postReadIniDsLogCSV.o dsLogPostCsv.o -lpthread

dsLogCSV: 	$(OBJECTS) $(INCLUDES) realtimeReadIniDsLogCSV.o dsLogCsv.o Makefile
	$(LD) -g -Wall -Wformat -o $@ $(OBJECTS) realtimeReadIniDsLogCSV.o dsLogCsv.o -lpthread

all: dsLogCSV dsLogPostCSV

default: all

#---- rule for cleaning up
clean:
	rm -f *~ *.o *.obj dsLogCSV dsLogPostCSV core*
