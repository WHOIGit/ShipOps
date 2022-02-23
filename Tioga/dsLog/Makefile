# ---------------------------------------------------------------------- 
#
#   dsLog Makefile
#
#   Modification History:
#   DATE 	AUTHOR 	COMMENT
#   2012-12-10  tt      Created and Written
#   2013-01-02  tt      - changed linker order of arguments due to linking failure in latest ubuntu release
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
        msg_util.h nioLogThread.h nio_thread.h sioLogThread.h sio_thread.h skel.h \
        stderr.h tcp_thread.h tcp_utilities.h time_util.h  \
        dsLogA2B.h dsLog.h dsLogStatusThread.h dsLogTalk.h dsLogTypes.h \

#---- list of all object files
OBJECTS = cmd_thread.o config_file.o launch_timer.o msg_util.o \
        nioLogThread.o nio_thread.o sioLogThread.o sio_thread.o \
        tcp_server_thread.o tcp_utilities.o time_util.o \
        dsLogA2B.o dsLog.o dsLogStatusThread.o dsLogTalk.o 

# ----------------------------------------------------------------------
#---- list of all header files
DSLOGCSV_INCLUDES = Makefile  cmd_thread.h config_file.h launch_timer.h macros.h \
        msg_util.h nioLogThread.h nio_thread.h csvLogThread.h sio_thread.h skel.h \
        stderr.h tcp_thread.h tcp_utilities.h time_util.h  \
        dsLogA2B.h dsLogCsv.h dsLogStatusThread.h dsLogTalk.h dsLogTypes.h \

#---- list of all object files
DSLOGCSV_OBJECTS = cmd_thread.o config_file.o launch_timer.o msg_util.o \
        nioLogThread.o nio_thread.o csvLogThread.o sio_thread.o \
        tcp_server_thread.o tcp_utilities.o time_util.o \
        dsLogA2B.o dsLogCsv.o dsLogStatusThread.o dsLogTalk.o 

# ----------------------------------------------------------------------
SRCS=$(OBJECTS:%.o=%.cpp)

# CFLAGS   = -g -Wall -pedantic -Wformat -W
#CFLAGS   = -g -Wall -pedantic -Wformat 
# note took out pedantic because of c++ warnings in sys/socket.h, howland, 1 oct 2001

# tt - 2010-06-13 add "-Wno-write-strings" to eliminate warnings about deprecated conversion
#    of string constant to 'char *'

CFLAGS   = -g -Wall -Wformat -Wno-write-strings

CPPFLAGS = $(CFLAGS)

#---- rule for compiling .cpp into .o
$(OBJECTS): %.o: %.cpp $(INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -c $< -o $@

#---- rule for compiling .cpp into .o
$(DSLOGCSV_OBJECTS): %.o: %.cpp $(DSLOGCSV_INCLUDES) Makefile
	$(CC) $(CPPFLAGS) -c $< -o $@

#---- rule for linking and creating dsLog
dsLog: 	$(OBJECTS) $(INCLUDES) Makefile

#	$(LD) -g -Wall -Wformat -lpthread -o $@ $(OBJECTS) 
	$(LD) -g -Wall -Wformat -o $@ $(OBJECTS)  -lpthread

#---- rule for linking and creating dsLog
dsLogCsv: 	$(DSLOGCSV_OBJECTS) $(DSLOGCSV_INCLUDES) Makefile

	$(LD) -g -Wall -Wformat -o $@ $(DSLOGCSV_OBJECTS)  -lpthread

all: dsLog dsLogCsv

default: dsLog

#---- rule for cleaning up
clean:
	rm -f *~ *.o *.obj dsLog core*
