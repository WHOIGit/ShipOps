#ifndef DSLOGPOSTCSV_H
#define DSLOGPOSTCSV_H
/* ----------------------------------------------------------------------

header file for dsLogPostCSV.h

  Modification History:
  DATE         AUTHOR 	COMMENT
  2014-09-11   tt       - created
---------------------------------------------------------------------- */

typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
} postTime_t;

#if 0
typedef struct {
  char prefix1[];
  char prefix2[];
  time_t dataTime;
} postParseResult_t;
#endif

typedef struct {

  // state values
  bool fileIsOpen;
  time_t filenameTime;
  time_t currentTime;
  char *filename;
  FILE *infilePtr;
  bool inputPendingFlag;
  char *pendingString;

  // configuration values
  char *infileDirectory;
  char *infilePrefix;
  char *infileExtension;
} postInfileControl_t;

typedef struct {
  postTime_t iniStartTime;
  postTime_t iniEndTime;
  postTime_t postLogTime;  //**** REMOVE this

  time_t startTime;
  time_t endTime;
  time_t logTime;

  int sampleSecond;
} dslogPostCsv_t;


//----------------------------------------------------
// function prototypes
int timetToPostTime(postTime_t *postTime, time_t timetValue);
int postTimeToTimeT(postTime_t postTime, time_t *timetPtr);
int timetDateString(char *str, time_t timetValue);
int timetTimeString(char *str, time_t timetValue);

int isLeapYear (int year);
int lastDayOfMonth(int month, int year);
int startEndOrder(dslogPostCsv_t value);
int postTimeAddSeconds(postTime_t *postTime, int seconds);
void postTimeToString(char *str, postTime_t postTime);

int getInputTime(char *instr, time_t *inputTime);
int getNextInputString(postInfileControl_t *ifc, char *instr, int instrSize);
int openNextInputFile(postInfileControl_t *ifc);

int postLogThisNow(distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData, time_t logTime);
int postLogOpenAsciiLogFile (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, time_t logTime);

#endif // DSLOGPOSTCSV_H
