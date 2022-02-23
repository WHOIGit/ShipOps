/* ----------------------------------------------------------------------

   csvOutput thread for dsLogCsv system

   Modification History:
   DATE         AUTHOR  COMMENT
   2014-04-25   tt      create from csvLogThread.h

---------------------------------------------------------------------- */
#ifndef CSV_OUTPUT_THREAD_INC
#define CSV_OUTPUT_THREAD_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbose debugging 
// ----------------------------------------------------------------------
#define DEBUG_CSV_OUTPUT

// ----------------------------------------------------------------------

#include "sio_thread.h"

extern void *csvOutputThread (void *thread_num);

void makeNoaaOutputHeaderString(int myLoggerNumber);
void makeCsvOutputHeaderString(int myLoggerNumber);
int makeCsvOutputString(char *outputRecord, int myLoggerNumber, int lastCsvMsgTime, char *dateString, char *timeString, time_t timeNow);

int calculateNormDiff(char *outString, int outStringSize, char *minuendString, char *subtrahendString, int outputPrecision, double normThreshold, double normOffset);
int calculateDiff(char *outString, int outStringSize, char *minuendString, char *subtrahendString, int outputPrecision);
int calculateScaleOffset(char *outString, int outStringSize, char *inputString, double scaleValue, double offsetValue, int outputPrecision);
int calculateSsv(char *outString, int outStringSize, char *temperatureString, char *salinityString, double pressure, int outputPrecision);
int calculateBarometer(char *outString, int outStringSize, char *rawPressureString, double sensorHeight, int outputPrecision);
int calculateTruewind(int outfieldType, char *outString, int outStringSize, char *sogString, char *cogString, char *hdgString, char *measuredSpeedString, char *measuredDirString, double zlr, int outputPrecision);

int samos_sprintf_date_string (char *str);
int samos_sprintf_time_string (char *str);

int dslog_get_current_hour();

#endif
