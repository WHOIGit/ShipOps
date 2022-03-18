
/* ----------------------------------------------------------------------
  Modification History:

  DATE         AUTHOR   COMMENT
  2014-09-11   tt       - create from dsLogCSV.cpp

---------------------------------------------------------------------- */

/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

// dsLog headers

#include "dsLogCsv.h"		/* main() header file */
#include "dsLogPostCsv.h"	/* header file for post-processing specific stuff */
#include "config_file.h"        /* ini file reading functions */
#include "csvLogThread.h"
#include "csvOutputThread.h"
#include "readIniDsLogCSV.h"

//2013-02-15  tt - added to prevent vector was not declared in this scope build error 
using namespace std; 

// some globals, moved from process.cpp

//----------------------------------------------------------------
//  this is the filename which is read as the argument to the main
//  program and stored as a double so everyvody can read it
//----------------------------------------------------------------

char *postGlobalInputDirectory;
char        *dsLogIniFilename;
startup_t   startup;



// ----------------------------------------------------------------------
// This is a structure that is initialized in rov.cpp to contain
// default thread attributes for launching threads. 
// It is provided as global variable as a convenience to other threads.
// ----------------------------------------------------------------------
pthread_attr_t DEFAULT_ROV_THREAD_ATTR;

// ----------------------------------------------------------------------
// This is a table of channel pairs that contains all the communication
// channels used in the entire system.  
// Each channel pair connects a single thread to the router.
// the router itself is process 0.
// This array is initialized in rov.cpp
// ----------------------------------------------------------------------

csv_abstract_data_t csvAbstractData[MAX_N_OF_ABSTRACT_DATA];

dslogPostCsv_t postCsv;

// the global thread table

thread_table_entry_t    global_thread_table[MAX_NUMBER_OF_THREADS];
logging_descriptor_t    theLoggingList[MAX_N_OF_INPUTS];
csv_output_descriptor_t csvOutputList[MAX_N_OF_CSV_OUTPUTS];
distribution_t          theGlobalLogFile;
logging_descriptor_t    theGlobalLog;

calculated_values_config_t calcValuesConfig;

pthread_mutex_t dsLogCsvMutex;

#define ROV_THREAD_DEFAULT_STACKSIZE 2000000


//---------------------------------------------------------------------------i
int main (int argc, char *argv[])
{
  int   nOfLogInputs;
  int   nOfLogOutputs;
  
  int bigWhileCount;
  FILE *inFD;

  int inputIndex;
  postInfileControl_t postIfc[MAX_N_OF_INPUTS];

  int outputIndex;
  char outputRecord[MAX_LOGGING_STRING_LENGTH];
  time_t lastOutputTime[MAX_N_OF_CSV_OUTPUTS];

  char oldDateStr[512];

  startup.running = 0;
  startup.startup_time = rov_get_time();
  startup.nOfLogInputs  = 0;
  startup.nOfLogOutputs = 0;
  
  fprintf (stderr, "File %s compiled on %s at %s \n", __FILE__, __DATE__, __TIME__);
  
  // time to read the startup file
  // first check for an argument
  if (argc > 1) {
    dsLogIniFilename = strdup (argv[1]);
    fprintf (stderr, "ini file argument is %s\n", dsLogIniFilename);
  }
  else {
    dsLogIniFilename = strdup (DEFAULT_INI_FILE);
    fprintf (stderr, "default ini file: %s\n", dsLogIniFilename);
  }

  // now try to open the file
  inFD = fopen (dsLogIniFilename, "r");
  if (!inFD) {
    fprintf (stderr, "%s ini file does not exist...exiting!\n", dsLogIniFilename);
    return -1;
  }
  
  
  if (inFD) {
    
    readIniDslogCsvProcess(inFD,&(theLoggingList[0]),&nOfLogInputs,&(csvOutputList[0]),&nOfLogOutputs,&theGlobalLogFile, &theGlobalLog);
    startup.running =  1;
    startup.nOfLogInputs = nOfLogInputs;
    startup.nOfLogOutputs = nOfLogOutputs;
    fclose (inFD);
  }

  // confirm that end time is after start time
  if(1 != startEndOrder(postCsv)) {
    fprintf(stderr, "**** ERROR ****  Specified end time is before start time.\n");
    return(-1);
  }
  
  //*** initialize the csvAbstractData array
  for(int ii=0; ii < MAX_N_OF_ABSTRACT_DATA; ++ii) {
    csvAbstractData[ii].lastRxTime = rov_get_time();
    csvAbstractData[ii].dataString = strdup("NODATA");
  }

  postCsv.postLogTime = postCsv.iniStartTime;

  postTimeToTimeT(postCsv.iniStartTime, &postCsv.startTime);
  postTimeToTimeT(postCsv.iniEndTime, &postCsv.endTime);

  postCsv.logTime = postCsv.startTime;

  // setup input side
  printf("\n----------\nInput file summary:\n");
  for(inputIndex = 0; inputIndex < nOfLogInputs; ++inputIndex) {
    // -- config stuff
    postIfc[inputIndex].infileDirectory = strdup(theLoggingList[inputIndex].infileDirectory);
    postIfc[inputIndex].infilePrefix = strdup(theLoggingList[inputIndex].infilePrefix);
    postIfc[inputIndex].infileExtension = strdup(theLoggingList[inputIndex].infileExtension);
    printf("%d - %s - %s - %s\n", inputIndex, postIfc[inputIndex].infileDirectory, postIfc[inputIndex].infilePrefix, postIfc[inputIndex].infileExtension);

    // -- state stuff
    postIfc[inputIndex].fileIsOpen = false; 
    postIfc[inputIndex].inputPendingFlag = false; 
    postIfc[inputIndex].filenameTime = postCsv.startTime; 
  }  // end of for loop over inputIndex

  // setup output side
  printf("\n----------\nOutput file summary:\n");
  for(outputIndex = 0; outputIndex < nOfLogOutputs; ++outputIndex) {
    lastOutputTime[outputIndex] = postCsv.startTime; 
    makeCsvOutputHeaderString(outputIndex);
    printf("outputIndex is %d - Header string is %s\n", outputIndex, csvOutputList[outputIndex].logging.fileHeaderString);
  }  // end of for loop over outputIndex
  printf("----------\n\n");

  timetDateString(oldDateStr, postCsv.logTime);
  printf("Processing data from %s\n", oldDateStr);

  // **** now walk through input files and process data
  bigWhileCount = 0;
  while(postCsv.logTime < postCsv.endTime) {
    //char tmpStr[512];
    char dateStr[512];
    char timeStr[512];

    timetDateString(dateStr, postCsv.logTime);
    timetTimeString(timeStr, postCsv.logTime);
    if(0 != strcmp(dateStr, oldDateStr) ) {
      printf("Processing data from %s\n", dateStr);
      timetDateString(oldDateStr, postCsv.logTime);
    }

    //postdebug printf("--------------------------------------------------------\n");
    //postdebug printf("bigWhileCount = %d - %s %s\n", bigWhileCount++, dateStr, timeStr);

    // process input side
    for(inputIndex = 0; inputIndex < nOfLogInputs; ++inputIndex) {
      int getNextInputStringResult;
      //int getInputTimeResult;
      time_t inputTime;
      bool processInputDone;

      postIfc[inputIndex].currentTime = postCsv.logTime;
      processInputDone = false;

      // read data from input files until current time is reached/exceeded?
      do {
	char inStr[4096];
	char str1[64];
	char str2[64];

	// read new input unless there is a previous one pending
	if( postIfc[inputIndex].inputPendingFlag ) {
	  //postdebug printf("Use pending input.\n");
	  getNextInputStringResult = 1;
	  strcpy(inStr, postIfc[inputIndex].pendingString);
	  postIfc[inputIndex].inputPendingFlag = false; 
	}
	else {
	  //postdebug printf("Get new input.\n");
	  getNextInputStringResult = getNextInputString(&postIfc[inputIndex], inStr, 4096);
	}

	// now process the new or pending input
	switch(getNextInputStringResult) {
	case 1:
	  // handle success
	  if( 1 == getInputTime(inStr, &inputTime) ) {
	    timetDateString(str1, inputTime);
	    timetTimeString(str2, inputTime);
	    //postdebug printf("inputIndex = %d; getNextInputStringResult = %d; %s %s; inStr - %s\n", inputIndex, getNextInputStringResult, str1, str2, inStr);

	    if(inputTime < postCsv.logTime) {
	      //postdebug printf("Time of record is ok - parse the values.\n");
	      //parse incoming string and set Abstract Values here
	      csvParseData(inStr, inputIndex, (rov_time_t)inputTime );
	    }
	    else {
	      //postdebug printf("Time of record is beyond current time - set as pending.\n");
	      // this is after the current time - save string and set pending flag
	      free(postIfc[inputIndex].pendingString);
	      postIfc[inputIndex].pendingString = strdup(inStr);
	      postIfc[inputIndex].inputPendingFlag = true; 
	      processInputDone = true;
	    }
	  }  // end of if( 1 == getInputTime(inStr, &inputTime) )

	  break;
	case -1:
	  //postdebug printf("EOF reached.\n");
	  // handle EOF reached - just try next file
	  break;
	case -2:
	  // handle buffer overflow
	  printf("***** ERROR ***** - Input line too large in %s.\n", postIfc[inputIndex].filename);
	  processInputDone = true;
	  break;
	case -3:
	  //postdebug printf("NO-MORE-FILES.\n");
	  // handle 'no more files' to try
	  processInputDone = true;
	  break;
	default:
	  // handle unknown return value
	  printf("***** ERROR ***** - Unknown return value from getNextInputString().\n");
	  processInputDone = true;
	  break;
	};
      } while( !processInputDone );

    }  // end of for loop over inputIndex

    // process output side
    for(outputIndex = 0; outputIndex < nOfLogOutputs; ++outputIndex) {
      if(postCsv.logTime - lastOutputTime[outputIndex] >= csvOutputList[outputIndex].outputInterval) {
	makeCsvOutputString(outputRecord, outputIndex, lastOutputTime[outputIndex], dateStr, timeStr, postCsv.logTime);
	//printf("%d - OUTPUT - %d - %s\n", outputIndex, csvOutputList[outputIndex].logging.nOfDestinations, outputRecord);
	lastOutputTime[outputIndex] = postCsv.logTime; 

	//----------------------------------------------------
	// now we've got the record we want to distribute and/or log
	for(int destinationCount = 0; destinationCount < csvOutputList[outputIndex].logging.nOfDestinations; destinationCount++) {
	  if(1 == csvOutputList[outputIndex].logging.destinations[destinationCount].destinationEnable) {
	    switch( csvOutputList[outputIndex].logging.destinations[destinationCount].destinationType) {
	    case DISK:
	      postLogThisNow(&(csvOutputList[outputIndex].logging.destinations[destinationCount]),&(csvOutputList[outputIndex].logging),outputRecord, postCsv.logTime);
	      break;
	    default:
	      break;
	    }  // end of switch(destinationType)
	  }  // end of if (destination is enabled)
	}  // end of for(destinationCount)
      }  // end of if(postCsv.logTime - lastOutputTime[outputIndex] > outputInterval[outputIndex])
    }  // end of for loop over outputIndex
    
    // advance logTime
    postCsv.logTime += 1;

  }  // end of while(postCsv.logTime < postCsv.endTime)

}

//---------------------------------------------------------------------------
// return values:
//    1 - success
//   -1 - sscanf fails
//---------------------------------------------------------------------------
int getInputTime(char *instr, time_t *inputTime)
{
  postTime_t postTime;
  int year, month, day, hour, minute, second, fracSec;

  if(7 != sscanf(instr, "%*s %d/%d/%d %d:%d:%d.%d", &year, &month, &day, &hour, &minute, &second, &fracSec) ) {
    return -1;
  }
  //printf("In getInputTime - %d/%d/%d %d:%d:%d.%d\n", year, month, day, hour, minute, second, fracSec);

  postTime.year = year;
  postTime.month = month;
  postTime.day = day;
  postTime.hour = hour;
  postTime.minute = minute;
  postTime.second = second;

  postTimeToTimeT(postTime, inputTime);

  return 1;
}

//---------------------------------------------------------------------------
// return values:
//    1 - success
//   -1 - EOF reached
//   -2 - char buffer overflow
//   -3 - no more files to try
//---------------------------------------------------------------------------
int getNextInputString(postInfileControl_t *ifc, char *instr, int instrSize)
{
  int count;
  char c;

  // if there is not an open file then open one
  if( !ifc->fileIsOpen ) {
    if( 0 == openNextInputFile(ifc) ) {
      return(-3);
    }
  }

  // if there is a line of data to return then send it else open next file

  // skip initial newline chars
  do {
    c = fgetc(ifc->infilePtr);
  } while( (c == '\n') || (c == '\r') );

  if( (c != '\n') && (c != '\r') && (c != EOF) ) {
    count = 0;
    instr[count++] = c;
    instr[count] = 0;
  }
  else {
    count = 0;
    instr[count] = 0;
  }

  // now get the next line
  do {
    c = fgetc(ifc->infilePtr);
    instr[count++] = c;
    instr[count] = 0;
  } while( (c != '\n') && (c != '\r') && (c != EOF) && (count < instrSize-1) );

  switch(c) {
  case '\n':
  case '\r':
    return 1;
    break;
  case EOF:
    fclose(ifc->infilePtr);
    ifc->fileIsOpen = false;
    // bump filenameTime by 60 seconds so we don't repeatedly  try to open the same file
    // program will loop endlessly without this - use caution if modifying
    ifc->filenameTime += 60;  // no seconds in filename - minute resolution
    return -1;
    break;
  default:
    //buffer overflow
    return -2;
    break;
  }  // end of switch(c)

  return(1);
}

//---------------------------------------------------------------------------
int openNextInputFile(postInfileControl_t *ifc)
{
  bool done;
  char tmpStr[4096];
  struct tm tm;

  // if there is not an open file then open one
  done = false;
  do {
    gmtime_r(&ifc->filenameTime, &tm);

    // create the new log file name
    snprintf (tmpStr, 512,"%s/%s%04d%02d%02d_%02d%02d.%s", ifc->infileDirectory, ifc->infilePrefix,(int) tm.tm_year + 1900, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) tm.tm_min, ifc->infileExtension);
    free(ifc->filename);
    ifc->filename = strdup(tmpStr);

    // try to open the new file 
    ifc->infilePtr = fopen (ifc->filename, "r");

    /* check results of fopen operation */
    if (NULL == ifc->infilePtr) {
      //postdebug printf ("error opening %s\n", ifc->filename);
      if( ifc->filenameTime + 60 <= ifc->currentTime ) {
	ifc->filenameTime += 60;  // no seconds in filename - minute resolution
      }
      else { 
	done = true;
      }
    }
    else {
      //postdebug printf ("success opening %s\n", ifc->filename);
      ifc->fileIsOpen = true;
      done = true;
      return(1);
    }
  } while( !done );

  return(0);
}

//---------------------------------------------------------------------------
void postTimeToString(char *str, postTime_t postTime) 
{
  sprintf(str, "%04d/%02d/%02d %02d:%02d:%02d", postTime.year, postTime.month, postTime.day, postTime.hour, postTime.minute, postTime.second);
}

//---------------------------------------------------------------------------
int timetDateString(char *str, time_t timetValue) 
{
  tm *tmTime;

  tmTime = gmtime(&timetValue);

  //sprintf(str, "%04d/%02d/%02d", tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday);
  sprintf(str, "%04d%02d%02d", tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday);
  return(0);
}

//---------------------------------------------------------------------------
int timetTimeString(char *str, time_t timetValue) 
{
  tm *tmTime;

  tmTime = gmtime(&timetValue);

  //sprintf(str, "%02d:%02d:%02d", tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);
  sprintf(str, "%02d%02d%02d", tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);
  return(0);
}

//---------------------------------------------------------------------------
int timetToPostTime(postTime_t *postTime, time_t timetValue) 
{
  tm *tmTime;

  tmTime = gmtime(&timetValue);

  postTime->year = tmTime->tm_year + 1900;
  postTime->month = tmTime->tm_mon + 1;
  postTime->day = tmTime->tm_mday;
  postTime->hour = tmTime->tm_hour;
  postTime->minute = tmTime->tm_min;
  postTime->second = tmTime->tm_sec;

  return(0);
}

//---------------------------------------------------------------------------
int postTimeToTimeT(postTime_t postTime, time_t *timetPtr) 
{
  tm *tmTime;
  time_t tTime;

  time(&tTime);
  tmTime = gmtime(&tTime);

  tmTime->tm_year = postTime.year - 1900;
  tmTime->tm_mon = postTime.month - 1;
  tmTime->tm_mday = postTime.day;
  tmTime->tm_hour = postTime.hour;
  tmTime->tm_min = postTime.minute;
  tmTime->tm_sec = postTime.second;

  *timetPtr = timegm(tmTime);

  return(0);
}

//---------------------------------------------------------------------------
int postTimeAddSeconds(postTime_t *postTime, int seconds) 
{
  tm *tmTime;
  time_t tTime;

  time(&tTime);
  tmTime = gmtime(&tTime);

  tmTime->tm_year = postTime->year - 1900;
  tmTime->tm_mon = postTime->month - 1;
  tmTime->tm_mday = postTime->day;
  tmTime->tm_hour = postTime->hour;
  tmTime->tm_min = postTime->minute;
  tmTime->tm_sec = postTime->second;

  tTime = timegm(tmTime);
  tTime += seconds;

  tmTime = gmtime(&tTime);

  postTime->year = tmTime->tm_year + 1900;
  postTime->month = tmTime->tm_mon + 1;
  postTime->day = tmTime->tm_mday;
  postTime->hour = tmTime->tm_hour;
  postTime->minute = tmTime->tm_min;
  postTime->second = tmTime->tm_sec;

  return(0);
}

//---------------------------------------------------------------------------
int startEndOrder(dslogPostCsv_t value)
{
  // test year
  if(value.iniStartTime.year < value.iniEndTime.year) return(1);
  if(value.iniStartTime.year > value.iniEndTime.year) return(0);

  // years are equal - test month
  if(value.iniStartTime.month < value.iniEndTime.month) return(1);
  if(value.iniStartTime.month > value.iniEndTime.month) return(0);

  // months are equal - test day
  if(value.iniStartTime.day < value.iniEndTime.day) return(1);
  if(value.iniStartTime.day > value.iniEndTime.day) return(0);

  // days are equal - test hour
  if(value.iniStartTime.hour < value.iniEndTime.hour) return(1);
  if(value.iniStartTime.hour > value.iniEndTime.hour) return(0);

  // hours are equal - test minute
  if(value.iniStartTime.minute < value.iniEndTime.minute) return(1);
  if(value.iniStartTime.minute > value.iniEndTime.minute) return(0);

  // minutes are equal - test second
  if(value.iniStartTime.second < value.iniEndTime.second) return(1);

  return(0);
}

//---------------------------------------------------------------------------
int lastDayOfMonth(int month, int year)
{
  switch(month) {
  case 2:  return(isLeapYear(year) ? 29 : 28);
  case 4:  return(30);
  case 6:  return(30);
  case 9:  return(30);
  case 11: return(30);
  };

  return(31);
}

//---------------------------------------------------------------------------
int isLeapYear(int year)
{
  if( (year % 4) != 0) { return(0); }
  if( (year % 100) != 0) { return(1); }
  if( (year % 400) != 0) { return(0); }
  return(1);
}

//---------------------------------------------------------------------------
int postLogOpenAsciiLogFile (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, time_t logTime)
{
   struct tm tm;
   time_t current_time;
   static char filename[512];
   int status = 0;

   current_time = time (NULL);
   //localtime_r (&current_time,&tm);   // 18 July 2012 fix to use threadsafe localtime_r
   gmtime_r (&logTime,&tm);

   // open a new log file under ANY of the following conditions:
   //   - there is no logFile currently open
   //   - - configuration indicates hourly logfiles and it is a new hour
   //   - - configuration indicates daily logfiles and it is a new day
   if ( (thisDestination->loggingDestination.asciiLogFileD == NULL)  ||
   	( (1 != loggingDescriptor->singleLogfileFlag) && ( (1 != loggingDescriptor->dailyLogfilesFlag) && (thisDestination->loggingDestination.lastHour  != tm.tm_hour) ) ) ||
   	( (1 != loggingDescriptor->singleLogfileFlag) && (1 == loggingDescriptor->dailyLogfilesFlag) && (thisDestination->loggingDestination.lastDay  != tm.tm_mday) ) ) 
      {
#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to set last hour.\n");
#endif
         thisDestination->loggingDestination.lastHour = tm.tm_hour;
         thisDestination->loggingDestination.lastDay  = tm.tm_mday;

         /* close existing log file */
         if (thisDestination->loggingDestination.asciiLogFileD != NULL)
            {
#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to close existing file.\n");
#endif
               if (0 != fclose (thisDestination->loggingDestination.asciiLogFileD))
                  printf ("LOGGING: ERROR closing log file %s !!!!!!!!!!!!!!!!!!!\n", thisDestination->loggingDestination.asciiLogFileName);
            }

#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to create new log file name.\n");
#endif
         /* create the new log file name */
	 snprintf (filename, 512,"%s/%s%04d%02d%02d_%02d%02d.%s",thisDestination->loggingDestination.loggingDirectory,thisDestination->loggingDestination.filenamePrefix,(int) tm.tm_year + 1900, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) tm.tm_min,thisDestination->loggingDestination.fileExtension);

         /* open the new file */
#ifdef DEBUG_LOGGING
         printf ("in open log file, opening a new file %s\n", filename);
#endif
         thisDestination->loggingDestination.asciiLogFileD = fopen (filename, "wa");

         /* check results of fopen operation */
         if (NULL == thisDestination->loggingDestination.asciiLogFileD)
            {
#ifdef DEBUG_LOGGING
               printf ("log.ascii_log_file_pointer was NULL\n");
#endif
               printf ("error opening %s\n", filename);
               status = 1;
            }
         else{
#ifdef DEBUG_LOGGING
               printf ("log->ascii_log_file_pointer was false\n");
               //printf ("ascii file pointer = %x\n", (int) log->ascii_log_file_pointer);
#endif
               strncpy(thisDestination->loggingDestination.asciiLogFileName, filename,256);
               //printf ("LOGGING: Opened log file %s OK\n", log->ascii_log_file_name);

	       // *** write the file header - added by tet - 2014-05-02
	       if(loggingDescriptor->useFileHeaderFlag) {
		 fprintf(thisDestination->loggingDestination.asciiLogFileD, "WHOI SSSG dsLogCsv\n");
		 fprintf(thisDestination->loggingDestination.asciiLogFileD, "%s", loggingDescriptor->fileHeaderString);
		 fflush(thisDestination->loggingDestination.asciiLogFileD);
	       }
            }
      }
   return (status);
}

//---------------------------------------------------------------------------
int logOpenAsciiLogFile (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor)
{

   struct tm tm;
   time_t current_time;
   static char filename[512];
   int status = 0;

   current_time = time (NULL);
   // 18 July 2012 fix to use threadsafe localtime_r

   localtime_r (&current_time,&tm);

   //old - replaced on 2013-04-24 if (((thisDestination->loggingDestination.lastHour  != tm.tm_hour) || (thisDestination->loggingDestination.asciiLogFileD == NULL)))

   // open a new log file under ANY of the following conditions:
   //   - there is no logFile currently open
   //   - - configuration indicates hourly logfiles and it is a new hour
   //   - - configuration indicates daily logfiles and it is a new day
   if ( (thisDestination->loggingDestination.asciiLogFileD == NULL)  ||
   	( (1 != loggingDescriptor->dailyLogfilesFlag) && (thisDestination->loggingDestination.lastHour  != tm.tm_hour) ) ||
   	( (1 == loggingDescriptor->dailyLogfilesFlag) && (thisDestination->loggingDestination.lastDay  != tm.tm_mday) ) ) 
      {
#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to set last hour.\n");
#endif
         thisDestination->loggingDestination.lastHour = tm.tm_hour;
         thisDestination->loggingDestination.lastDay  = tm.tm_mday;

         /* close existing log file */
         if (thisDestination->loggingDestination.asciiLogFileD != NULL)
            {
#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to close existing file.\n");
#endif
               if (0 != fclose (thisDestination->loggingDestination.asciiLogFileD))
                  printf ("LOGGING: ERROR closing log file %s !!!!!!!!!!!!!!!!!!!\n", thisDestination->loggingDestination.asciiLogFileName);
            }

#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to create new log file name.\n");
#endif
         /* create the new log file name */
	 snprintf (filename, 512,"%s/%s%04d%02d%02d_%02d%02d.%s",thisDestination->loggingDestination.loggingDirectory,thisDestination->loggingDestination.filenamePrefix,(int) tm.tm_year + 1900, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) tm.tm_min,thisDestination->loggingDestination.fileExtension);

         /* open the new file */
#ifdef DEBUG_LOGGING
         printf ("in open log file, opening a new file %s\n", filename);
#endif
         thisDestination->loggingDestination.asciiLogFileD = fopen (filename, "wa");

         /* check results of fopen operation */
         if (NULL == thisDestination->loggingDestination.asciiLogFileD)
            {
#ifdef DEBUG_LOGGING
               printf ("log.ascii_log_file_pointer was NULL\n");
#endif
               //strcpy(log.ascii_log_file_name, "ERROR OPENING: ");
               //strcat(log.ascii_log_file_name, filename);
               printf ("error opening %s\n", filename);
               status = 1;
            }
         else{
#ifdef DEBUG_LOGGING
               printf ("log->ascii_log_file_pointer was false\n");
               //printf ("ascii file pointer = %x\n", (int) log->ascii_log_file_pointer);
#endif
               strncpy(thisDestination->loggingDestination.asciiLogFileName, filename,256);
               //printf ("LOGGING: Opened log file %s OK\n", log->ascii_log_file_name);

	       // *** write the file header - added by tet - 2014-05-02
	       if(loggingDescriptor->useFileHeaderFlag) {
		 fprintf(thisDestination->loggingDestination.asciiLogFileD, "WHOI SSSG dsLogCsv\n");
		 fprintf(thisDestination->loggingDestination.asciiLogFileD, "%s", loggingDescriptor->fileHeaderString);
		 fflush(thisDestination->loggingDestination.asciiLogFileD);
	       }
            }
      }
   return (status);


}

//---------------------------------------------------------------------------
/*
  MODIFICATION HISTORY
  DATE         WHO             WHAT
  -----------  --------------  ----------------------------
  2014-09-14   tt              Created and Written based on logThisNow()
  
*/
//---------------------------------------------------------------------------
int postLogThisNow(distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData, time_t logTime)
{
#ifdef DEBUG_LOGGING
   printf (" just entered postLogThisNow!\n");
   printf (" string to log is --- %s\n", recordData);
#endif

   // open a log file if required
   postLogOpenAsciiLogFile (thisDestination,loggingDescriptor, logTime);

#ifdef DEBUG_LOGGING
   printf (" back from call to logOpenAsciiLogFile.\n");
#endif

   if (NULL != thisDestination->loggingDestination.asciiLogFileD) {
#ifdef DEBUG_LOGGING
     printf (" about to call fprintf from postLogThisNow.\n");
#endif
     fprintf (thisDestination->loggingDestination.asciiLogFileD, "%s", recordData);
     fflush (thisDestination->loggingDestination.asciiLogFileD);
   }

#ifdef DEBUG_LOGGING
   printf (" just left log this now!\n");
#endif

   return 0;
}

//---------------------------------------------------------------------------
int logThisNow(distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData)
      /*


     MODIFICATION HISTORY
     DATE         WHO             WHAT
     -----------  --------------  ----------------------------
     18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl
     15 OCT 2001  Jon Howland     hack to use with rov_54.1, remove timestamping
     05 Nov 2009  Jon Howland     hack again to work with dsLogger

     ---------------------------------------------------------------------- */
{


#ifdef DEBUG_LOGGING
   printf (" just entered log this now!\n");
   printf (" string to log is --- %s\n", recordData);
#endif
   // lock the data structure mutex so that we chan diddle with the file
   // commented out because in this architecture, this is the only access
   // to the file.  did the unlock too
   // unknown author and date - was it me?  (t. thiel 2014-06-23)

   // was seeing instances where the files were getting stepped on by one another.  
   // reinstated calls to mutex lock/unlock
   // log.mutex was gone so created new mutex - dsLogCsvMutex (global)
   // t. thiel 2014-06-23
   pthread_mutex_lock(&dsLogCsvMutex);

   // open a log file if required
   logOpenAsciiLogFile (thisDestination,loggingDescriptor);

#ifdef DEBUG_LOGGING
   printf (" back from call to logOpenAsciiLogFile.\n");
#endif

   if (NULL != thisDestination->loggingDestination.asciiLogFileD)
      {
#ifdef DEBUG_LOGGING
   printf (" about to call fprintf from logThisNow.\n");
#endif
         fprintf (thisDestination->loggingDestination.asciiLogFileD, "%s", recordData);
         fflush (thisDestination->loggingDestination.asciiLogFileD);
      }

   pthread_mutex_unlock(&dsLogCsvMutex);

#ifdef DEBUG_LOGGING
   printf (" just left log this now!\n");
#endif

   return 0;

}

/* ---------------------------------------------------------------------- */
void flushAsciiLogAndClose (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData)
      /*
     MODIFICATION HISTORY
     DATE         WHO             WHAT
     -----------  --------------  ----------------------------
     18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl
     15 Oct 2001  Jon Howland     modified to work with data structure, not globals
     ---------------------------------------------------------------------- */
{
   // open a log file if required
   logOpenAsciiLogFile (thisDestination,loggingDescriptor);

   if(NULL != thisDestination->loggingDestination.asciiLogFileD )
      {
         fflush (thisDestination->loggingDestination.asciiLogFileD);
         fclose (thisDestination->loggingDestination.asciiLogFileD);
      }
}

