/* ----------------------------------------------------------------------

   csv thread for dsLog system

   Modification History:
   DATE         AUTHOR  COMMENT
   1-OCT-2001   jch      Created and writte from LLW's sio 
   15 FEB 2002  JCH      change for new messaging api
   2013-12-31   tt       create from sioLogThread.h

---------------------------------------------------------------------- */
#ifndef CSV_LOG_THREAD_INC
#define CSV_LOG_THREAD_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbose debugging 
// ----------------------------------------------------------------------
#define DEBUG_CSV_LOGGER

// ----------------------------------------------------------------------

#include "sio_thread.h"

extern void *csvLogThread (void *thread_num);
int csvParseData(char *data, int myLoggerNumber, rov_time_t dataRecordTime);

#endif
