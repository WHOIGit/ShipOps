/*************************************************************************
csv log output thread for SSSG dsLog system

Modification History:
DATE        AUTHOR   COMMENT
2014-04-25  tt       create from csvLogThread.cpp

**************************************************************************/
/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <fcntl.h>

/* posix header files */
#define  POSIX_SOURCE 1

#include <pthread.h>
#include <termios.h>
#include <unistd.h>

/* jason header files */
#include "dsLogTalk.h"		/* jasontalk protocol and structures */

#include "msg_util.h"		/* utility functions for messaging */
#include "config_file.h"
#include "dsLogCsv.h"
#include "csvOutputThread.h"
#include "ssv_ChenMillero.h"
#include "truewind_SmithBourassa.h"

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;

/* ----------------------------------------------------------------------*/
//    csvOutputThread
//   Modification History:
//   DATE         AUTHOR  COMMENT
//   2014-04-25  tt       create from csvLogThread.cpp
//----------------------------------------------------------------------*/
void *
csvOutputThread (void *thread_num)
{
  int localThreadNumber;

  msg_hdr_t hdr = { 0 };
  unsigned char data[MSG_DATA_LEN_MAX] = { 0 };

  rov_time_t lastCsvMsgTime = rov_get_time();
  rov_time_t threadStartTime = rov_get_time();
  
  char *my_name = NULL;
  
  /* get my thread number */
  localThreadNumber = (long int) thread_num;

  int myLoggerNumber;
  // now look into the process table and figure out what thread I am
  for (int i = 0; i < MAX_NUMBER_OF_THREADS; i++) {
    if (global_thread_table[i].thread_num == localThreadNumber) {
      my_name = global_thread_table[i].thread_name;
      myLoggerNumber = global_thread_table[i].extra_arg_1;
    }
  }
#if 0  
  if (!my_name) {
    fprintf (stderr, "SIO_LOG_THREAD: thread %d  not found in process table!\n", localThreadNumber);
    fflush (stdout);
    fflush (stderr);
    abort ();
  }
#endif
  printf("***************************************************\n");
  printf("***************************************************\n");
  printf("*** csvOutputThread - localThreadNumber is %d\n", localThreadNumber);
  printf("*** csvOutputThread - thread_name is %s\n", my_name);
  printf("*** csvOutputThread - myLoggerNumber is %d\n", myLoggerNumber);
  printf("*** csvOutputThread - outputInterval is %d\n", csvOutputList[myLoggerNumber].outputInterval);

  // copy outputFileFormat and shipName to logging structure
  csvOutputList[myLoggerNumber].logging.outputFileFormat = csvOutputList[myLoggerNumber].outputFileFormat;
  csvOutputList[myLoggerNumber].logging.shipName = strdup(csvOutputList[myLoggerNumber].shipName);

  for(int destinationCount = 0; destinationCount < csvOutputList[myLoggerNumber].logging.nOfDestinations; destinationCount++) {
    if(UDP_SOCKET == csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationType) {
      csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.socket =  socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      int broadcastPermission = 1;
      if(setsockopt(csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.socket,SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0) {
	//fprintf (stderr, "***couldn't set broadcast on UDP : %s \n",csvOutputList[myLoggerNumber].sourceName );
      }
      memset(&(csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.Addr), 0, sizeof (csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.Addr));
      
      csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.Addr.sin_family = AF_INET;
      csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.Addr.sin_addr.s_addr = inet_addr(csvOutputList[myLoggerNumber].logging.destinations[destinationCount].networkDestination.ipAddress);
      csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].networkDestination.Addr.sin_port =  htons(csvOutputList[myLoggerNumber].logging.destinations[destinationCount].networkDestination.toSocketNumber);
    }
    else if(DISK == csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationType) {
      csvOutputList[myLoggerNumber].logging.destinations[ destinationCount].loggingDestination.asciiLogFileD = NULL;
    }
  }
  int msg_success = msg_queue_new(localThreadNumber, "CSV_OUTPUT_THREAD");
  if(msg_success != MSG_OK) {
    fprintf(stderr, "CSV_OUTPUT_THREAD creating msg_queue ---  %s \n", MSG_ERROR_CODE_NAME(msg_success) );
    fflush (stdout);
    fflush (stderr);
    abort ();
  }
   
  // ----------------------------------------------------------------------
  // display destination info
  // ----------------------------------------------------------------------
#ifdef DISPLAY_DESTINATION_DEBUG_INFO
  printf("--------------------------------------------------------------\n");
  printf("--------------------------------------------------------------\n");
  printf("Displaying destination info for csvOutputThread. \n");
  printf("myLoggerNumber = %d\n", myLoggerNumber);
  printf("csvOutputList[myLoggerNumber].logging.nOfDestinations = %d\n", csvOutputList[myLoggerNumber].logging.nOfDestinations);
  
  for(int destinationCount = 0; destinationCount < csvOutputList[myLoggerNumber].logging.nOfDestinations; destinationCount++) {
    printf("---------------\n");
    printf("csvOutputList[myLoggerNumber].logging.destinations[%d].destinationType = %d\n", destinationCount, csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationType);
    if(UDP_SOCKET == csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationType) {
      printf("destinationType is UDP_SOCKET\n");
    }
    if(DISK == csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationType) {
      printf("destinationType is DISK\n");
    }
  }
  printf("--------------------------------------------------------------\n");
  printf("--------------------------------------------------------------\n");
#endif
  // - above #endif closes section for displaying destination info

  // --- 
  // now setup Header string for log files according to the outputFileFormat
  switch(csvOutputList[myLoggerNumber].outputFileFormat) {
  case OUTFILE_FORMAT_SAMOS:
    csvOutputList[myLoggerNumber].logging.fileHeaderString = strdup("");
    break;
  case OUTFILE_FORMAT_NOAA:
    makeNoaaOutputHeaderString(myLoggerNumber);
    break;
  case OUTFILE_FORMAT_DSLOGCSV:
  default:
    makeCsvOutputHeaderString(myLoggerNumber);
    break;
  }  // end of switch over outputFileFormat

  // launch timer for generating csv message  
  launched_timer_data_t   *csvmsgTimer = launch_timer_new(csvOutputList[myLoggerNumber].outputInterval, -1, localThreadNumber, CSVMSG);

  // wakeup message
  printf ("CSV OUTPUT THREAD: Thread %d - initialized \n", localThreadNumber);
  printf ("CSV OUTPUT THREAD: File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);

  //**********************************************************************************
  //**********************************************************************************
  //**********************************************************************************
  // loop forever
  while (1) {
    // wait forever on the input channel
#ifdef DEBUG_CSV
    printf ("CSV calling get_message.\n");
#endif

    int msg_get_success = msg_get(localThreadNumber,&hdr, data, MSG_DATA_LEN_MAX);
    if(msg_get_success != MSG_OK) {
      fprintf(stderr, "csv_output_thread--error on msg_get:  %s\n",MSG_ERROR_CODE_NAME(msg_get_success));
    }
    else {
      // #define DEBUG_RECIEVED_MESSAGES
      
#ifdef DEBUG_RECIEVED_MESSAGES
      // print on stderr
      printf ("%s got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
	      JASONTALK_THREAD_NAME(localThreadNumber),
	      JASONTALK_MESSAGE_NAME(hdr.type),hdr.type,
	      JASONTALK_THREAD_NAME(hdr.from),hdr.from,
	      JASONTALK_THREAD_NAME(hdr.to),hdr.to,
	      hdr.length );
#endif
#ifdef DEBUG_CSV
      printf ("%s got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
	      JASONTALK_THREAD_NAME(localThreadNumber),
	      JASONTALK_MESSAGE_NAME(hdr.type),hdr.type,
	      JASONTALK_THREAD_NAME(hdr.from),hdr.from,
	      JASONTALK_THREAD_NAME(hdr.to),hdr.to,
	      hdr.length );
#endif
      // process new message
      switch (hdr.type) {
      case CSVMSG:		// recieved a CSVMSG (Create CSV record) Message
	{
	  char    outputRecord[MAX_LOGGING_STRING_LENGTH];
	  int dataLength;
	  char    timeString[48];
	  char    dateString[48];

	  if( (rov_get_time() - threadStartTime)  > csvOutputList[myLoggerNumber].outputInterval) {
	    //----------------------------------------------------
	    // build up the string to log to the csv files

	    // fill in datestring and timestring
	    // now write the end of line characters to the output record according to the outputFileFormat
	    switch(csvOutputList[myLoggerNumber].outputFileFormat) {
	    case OUTFILE_FORMAT_SAMOS:
	      samos_sprintf_date_string(dateString);
	      samos_sprintf_time_string(timeString);
	      break;
	    case OUTFILE_FORMAT_NOAA:
	      dateString[0] = '\0';
	      timeString[0] = '\0';
	      break;
	    case OUTFILE_FORMAT_DSLOGCSV:
	    default:
	      if(csvOutputList[myLoggerNumber].timestampEnable) {
		dslog_sprintf_date_string(dateString);
		dslog_sprintf_time_string(timeString);
	      }
	      else {
		dateString[0] = '\0';
		timeString[0] = '\0';
	      }
	      break;
	    }  // end of switch over outputFileFormat

	    dataLength = makeCsvOutputString(outputRecord, myLoggerNumber, lastCsvMsgTime, dateString, timeString, rov_get_time() );
	    lastCsvMsgTime = rov_get_time();
	    
	    //----------------------------------------------------
	    // now we've got the record we want to distribute and/or log
	    for(int destinationCount = 0; destinationCount < csvOutputList[myLoggerNumber].logging.nOfDestinations; destinationCount++) {
	      //printf("csvOutputThread - %d: my my -- \n", myLoggerNumber);
	      if(1 == csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationEnable) {
		switch( csvOutputList[myLoggerNumber].logging.destinations[destinationCount].destinationType) {
		case UDP_SOCKET:
		  {
		    sendto ( csvOutputList[myLoggerNumber].logging.destinations[destinationCount].networkDestination.socket, outputRecord, dataLength, 0, (struct sockaddr *) &(csvOutputList[myLoggerNumber].logging.destinations[destinationCount].networkDestination.Addr), sizeof (csvOutputList[myLoggerNumber].logging.destinations[destinationCount].networkDestination));
		    break;
		  }
		case DISK:
		  {
		    if( OUTFILE_FORMAT_NOAA == csvOutputList[myLoggerNumber].outputFileFormat ) {
		      // handle the NOAA output file
		      int currentHour;
		      currentHour = dslog_get_current_hour();
		      printf("currentHour = %d.  lastHour = %d.\n", currentHour, csvOutputList[myLoggerNumber].lastNoaaHour);
		      if(csvOutputList[myLoggerNumber].lastNoaaHour != currentHour) {
			csvOutputList[myLoggerNumber].lastNoaaHour = currentHour;
			logThisNow(&(csvOutputList[myLoggerNumber].logging.destinations[destinationCount]),&(csvOutputList[myLoggerNumber].logging),outputRecord);
		      }
		    }
		    else {
		      // handle all cases outside of NOAA output file
		      logThisNow(&(csvOutputList[myLoggerNumber].logging.destinations[destinationCount]),&(csvOutputList[myLoggerNumber].logging),outputRecord);
		    }
		    break;
		  }
		case SINGLE_DISK:
		  {
		    logThisNow(&theGlobalLogFile,&theGlobalLog,outputRecord);
		    break;
		  }
		default:
		  {
		    break;
		  }
		}  // end of switch(destinationType)
	      }  // end of if (destination is enabled)
	    }  // end of for(destinationCount)
	  }  // end of if this isnt the first time we get the timer msg CSVMSG
	  break;
	}  // end of case CSVMSG:

      case PNG:		// recieved a PNG (Ping) message
	{
	  char msg[256];
	  snprintf (msg, 256,"%s: (thread %d) is Alive!", global_thread_table[localThreadNumber].thread_name, localThreadNumber);
	  // respond with a SPI (Status Ping) message
	  msg_send (hdr.from, hdr.to, SPI, strlen (msg), msg);
	  break;
	}
      case BYE:  // received a bye message--time to give up the ghost--or at least my socket
	{
	  
	  launch_timer_disable(csvmsgTimer);
	  
	  return(NULL);
	  
	  break;
	}
      case SPI:		// recieved a SPI (Status Ping) message
	break;
        
      default:		// recieved an unknown message type
	{
	  printf ("%s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE - " "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[localThreadNumber].thread_name, localThreadNumber, hdr.type, hdr.from, hdr.to, hdr.length);
	  break;
	}  // end of hdr.type - default:
      } // end of switch(hdr.type)
    } // else
  } //while(1)
  
  return NULL;
}

//-------------------------------------------------------------------------
void makeCsvOutputHeaderString(int myLoggerNumber)
{
  // --- setup Header string for log files
  {
    char tmpstr[1024];
    int len=0;
    bool noCustomWidths;
    bool writeDelimiterFlag;

    //--- are there any custom widths defined?
    noCustomWidths = true;
    for(int outputFieldIndex=0; outputFieldIndex < csvOutputList[myLoggerNumber].numberOfOutputFields; ++outputFieldIndex) {
      if(1 != csvOutputList[myLoggerNumber].outfieldWidth[outputFieldIndex]) {
	noCustomWidths = false;
      }
    }

    //-- outputPrefix1
    if(strlen(csvOutputList[myLoggerNumber].outputPrefix1) > 0) {
      len = snprintf(tmpstr, MAX_LOGGING_STRING_LENGTH, "%s%s", csvOutputList[myLoggerNumber].outputPrefix1, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    else {
      len = 0;
      strcpy(tmpstr, "");
    }
    
    //-- date and time
    if(noCustomWidths) {
      len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, "%s%s%s%s", csvOutputList[myLoggerNumber].dateHeaderString, csvOutputList[myLoggerNumber].outputDelimiter, csvOutputList[myLoggerNumber].timeHeaderString, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    else {
      len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, "%10s%s%12s%s", csvOutputList[myLoggerNumber].dateHeaderString, csvOutputList[myLoggerNumber].outputDelimiter, csvOutputList[myLoggerNumber].dateHeaderString, csvOutputList[myLoggerNumber].outputDelimiter);
    }

    //-- outputPrefix2
    if(strlen(csvOutputList[myLoggerNumber].outputPrefix2) > 0) {
      len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, "%s%s", csvOutputList[myLoggerNumber].outputPrefix2, csvOutputList[myLoggerNumber].outputDelimiter);
    }
	    
    writeDelimiterFlag = false;
    for(int outputFieldIndex=0; outputFieldIndex < csvOutputList[myLoggerNumber].numberOfOutputFields; ++outputFieldIndex) {
      char formatString[16];

      if(csvOutputList[myLoggerNumber].outfieldEnable[outputFieldIndex]) {
	// only write the delimiter if a data field has already been written
	if(writeDelimiterFlag) {
	  len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, "%s", csvOutputList[myLoggerNumber].outputDelimiter);
	}
	
	if(strlen(csvOutputList[myLoggerNumber].outfieldHeaderString[outputFieldIndex]) > 0) {
	  //strcat(tmpstr, csvOutputList[myLoggerNumber].outfieldHeaderString[outputFieldIndex]);
	  sprintf(formatString, "%%%ds", csvOutputList[myLoggerNumber].outfieldWidth[outputFieldIndex]);
	  len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].outfieldHeaderString[outputFieldIndex]);
	}
	else {
	  sprintf(formatString, "%%%ds%%d", csvOutputList[myLoggerNumber].outfieldWidth[outputFieldIndex]);
	  len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, formatString, "FIELD_", outputFieldIndex);
	}
	writeDelimiterFlag = true;
      }  // end of if outfieldEnable
    }  // end of for loop over outputFieldIndex

    strcat(tmpstr, "\n");
    csvOutputList[myLoggerNumber].logging.fileHeaderString = strdup(tmpstr);
  }
}

//-------------------------------------------------------------------------
void makeNoaaOutputHeaderString(int myLoggerNumber)
{
  // --- setup Header string for log files
  {
    char tmpstr[1024];
    int len=0;
    bool writeDelimiterFlag;

    len = snprintf(tmpstr, MAX_LOGGING_STRING_LENGTH, "<,%s,none,", csvOutputList[myLoggerNumber].shipCallSign);
	    
    writeDelimiterFlag = false;
    for(int outputFieldIndex=0; outputFieldIndex < csvOutputList[myLoggerNumber].numberOfOutputFields; ++outputFieldIndex) {
      char formatString[16];

      if(csvOutputList[myLoggerNumber].outfieldEnable[outputFieldIndex]) {
	if(strlen(csvOutputList[myLoggerNumber].outfieldHeaderString[outputFieldIndex]) > 0) {
	  //strcat(tmpstr, csvOutputList[myLoggerNumber].outfieldHeaderString[outputFieldIndex]);
	  sprintf(formatString, "%%%ds,", csvOutputList[myLoggerNumber].outfieldWidth[outputFieldIndex]);
	  len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].outfieldHeaderString[outputFieldIndex]);
	}
	else {
	  sprintf(formatString, "%%%ds%%d,", csvOutputList[myLoggerNumber].outfieldWidth[outputFieldIndex]);
	  len += snprintf(&tmpstr[len], MAX_LOGGING_STRING_LENGTH, formatString, "FIELD_", outputFieldIndex);
	}
      }  // end of if outfieldEnable
    }  // end of for loop over outputFieldIndex

    strcat(tmpstr, "Version,>\n");
    csvOutputList[myLoggerNumber].logging.fileHeaderString = strdup(tmpstr);
  }
}

#define MAX_OUTFIELD_STRING_LENGTH 128
//-------------------------------------------------------------------------
int makeCsvOutputString(char *outputRecord, int myLoggerNumber, int lastCsvMsgTime, char *dateString, char *timeString, time_t timeNow)
{
  int len;
  bool writeDelimiterFlag;
  char outputFieldString[MAX_OUTFIELD_STRING_LENGTH];

  len = 0;  // it is set below - but let's be safe

  // write the leftmost fields of the data output record according to the outputFileFormat
  switch(csvOutputList[myLoggerNumber].outputFileFormat) {
  case OUTFILE_FORMAT_SAMOS:
    len = snprintf(outputRecord, MAX_LOGGING_STRING_LENGTH, "$SAMOS:001, CS:%s, YMD:%s, HMS:%s, ", csvOutputList[myLoggerNumber].shipCallSign, dateString, timeString);
    break;
  case OUTFILE_FORMAT_NOAA:
    len = snprintf(outputRecord, MAX_LOGGING_STRING_LENGTH, "<,,");
    break;
  case OUTFILE_FORMAT_DSLOGCSV:
  default:
    // write the outputPrefix1 and date and time
    if(strlen(csvOutputList[myLoggerNumber].outputPrefix1) > 0) {
      len = snprintf(outputRecord, MAX_LOGGING_STRING_LENGTH, "%s%s%s%s%s%s", csvOutputList[myLoggerNumber].outputPrefix1, csvOutputList[myLoggerNumber].outputDelimiter, dateString, csvOutputList[myLoggerNumber].outputDelimiter, timeString, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    else if( (strlen(dateString) > 0) && (strlen(timeString) > 0) ) {
      len = snprintf(outputRecord, MAX_LOGGING_STRING_LENGTH, "%s%s%s%s", dateString, csvOutputList[myLoggerNumber].outputDelimiter, timeString, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    else if(strlen(dateString) > 0) {
      len = snprintf(outputRecord, MAX_LOGGING_STRING_LENGTH, "%s%s", dateString, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    else if(strlen(timeString) > 0) {
      len = snprintf(outputRecord, MAX_LOGGING_STRING_LENGTH, "%s%s", timeString, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    
    if(strlen(csvOutputList[myLoggerNumber].outputPrefix2) > 0) {
      len += snprintf(&outputRecord[len], MAX_LOGGING_STRING_LENGTH, "%s%s", csvOutputList[myLoggerNumber].outputPrefix2, csvOutputList[myLoggerNumber].outputDelimiter);
    }
    break;
  }  // end of switch over outputFileFormat

  writeDelimiterFlag = false;
  if(csvOutputList[myLoggerNumber].numberOfOutputFields > 0) {
    for(int outputIndex=0; outputIndex < csvOutputList[myLoggerNumber].numberOfOutputFields; ++outputIndex) {
      if(csvOutputList[myLoggerNumber].outfieldEnable[outputIndex]) {
	char formatString[16];
	sprintf(formatString, "%%%ds", csvOutputList[myLoggerNumber].outfieldWidth[outputIndex]);

	switch(csvOutputList[myLoggerNumber].outfieldType[outputIndex]) 
	  {
	  case OUT_TYPE_NORMDIFF:
	    if( (calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex < 0) || (calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex < 0) ) {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, "BAD_NORM_DIFF_CONFIG");
	    }
	    else if(
		    ( (csvAbstractData[calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex].lastRxTime > lastCsvMsgTime) 
		      && (csvAbstractData[calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex].lastRxTime > lastCsvMsgTime) )
		    || ( ( csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex] > 0.0 ) 
			 && (timeNow - csvAbstractData[calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex].lastRxTime < csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex])
			 && (timeNow - csvAbstractData[calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex].lastRxTime  < csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex]) )
		    ) {
	      char normDiffString[128];
	      calculateNormDiff(normDiffString,
				128,
				csvAbstractData[calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex].dataString,
				csvAbstractData[calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex].dataString,
				1,
				calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].normThreshold,
				calcValuesConfig.normdiff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].normOffset );
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, normDiffString);
	    }
	    else {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	    }
	    break;
	  case OUT_TYPE_DIFF:
	    if( (calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex < 0) || (calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex < 0) ) {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, "BAD_DIFF_CONFIG");
	    }
	    else if(
		    ( (csvAbstractData[calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex].lastRxTime > lastCsvMsgTime) 
		      && (csvAbstractData[calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex].lastRxTime > lastCsvMsgTime) )
		    || ( ( csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex] > 0.0 ) 
			 && (timeNow - csvAbstractData[calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex].lastRxTime < csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex])
			 && (timeNow - csvAbstractData[calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex].lastRxTime  < csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex]) )
		    ) {
	      char diffString[128];
	      calculateDiff(diffString, 128, 
			   csvAbstractData[calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].minuendAbstractIndex].dataString, 
			    csvAbstractData[calcValuesConfig.diff[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].subtrahendAbstractIndex].dataString, 1);
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, diffString);
	    }
	    else {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	    }
	    break;
	  case OUT_TYPE_SSV:
	    if( (calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].temperatureAbstractIndex < 0) || (calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].salinityAbstractIndex < 0) ) {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, "BAD_SSV_CONFIG");
	      //printf("index=%d, tempAbIndex=%d, salinityAbIndex=%d\n", csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex], calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].temperatureAbstractIndex, calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].salinityAbstractIndex);
	    }
	    else if(
		    ( (csvAbstractData[calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].temperatureAbstractIndex].lastRxTime > lastCsvMsgTime) 
		      && (csvAbstractData[calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].salinityAbstractIndex].lastRxTime > lastCsvMsgTime) )
		    || ( ( csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex] > 0.0 ) 
			 && (timeNow - csvAbstractData[calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].temperatureAbstractIndex].lastRxTime < csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex])
			 && (timeNow - csvAbstractData[calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].salinityAbstractIndex].lastRxTime  < csvOutputList[myLoggerNumber].outfieldAgeLimit[outputIndex]) )
		    ) {
	      char ssvString[128];
	      calculateSsv(ssvString, 128, 
			   csvAbstractData[calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].temperatureAbstractIndex].dataString, 
			   csvAbstractData[calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].salinityAbstractIndex].dataString, 
			   calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].pressure,
			   calcValuesConfig.ssv[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].precision);
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, ssvString);
	    }
	    else {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	    }
	    break;
		      
	  case OUT_TYPE_BAROMETER:
	    if( calcValuesConfig.barometer[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].rawPressureAbstractIndex < 0 ) {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, "BAD_BAROMETER_CONFIG");
	    }
	    else if( (csvAbstractData[calcValuesConfig.barometer[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].rawPressureAbstractIndex].lastRxTime > lastCsvMsgTime) ) {
	      char barometerString[128];
	      calculateBarometer(barometerString, 128, 
				 csvAbstractData[calcValuesConfig.barometer[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].rawPressureAbstractIndex].dataString, 
				 calcValuesConfig.barometer[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].sensorHeight,
				 calcValuesConfig.barometer[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].precision);
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, barometerString);
	    }
	    else {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	    }
	    break;
		      
	  case OUT_TYPE_TRUEWIND_SPEED:
	  case OUT_TYPE_TRUEWIND_DIRECTION:
	  case OUT_TYPE_TRUEWIND_BOTH:
	  case OUT_TYPE_TRUEWIND_DEBUG:
	    if( (calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].sogAbstractIndex < 0) 
		|| (calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].cogAbstractIndex < 0)
		|| (calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].hdgAbstractIndex < 0) 
		|| (calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].windSpeedAbstractIndex < 0) 
		|| (calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].windDirectionAbstractIndex < 0) ) {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, "BAD_TRUEWIND_CONFIG");
	    }
	    else if( (csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].sogAbstractIndex].lastRxTime > lastCsvMsgTime) 
		     && (csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].cogAbstractIndex].lastRxTime > lastCsvMsgTime) 
		     && (csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].hdgAbstractIndex].lastRxTime > lastCsvMsgTime) 
		     && (csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].windSpeedAbstractIndex].lastRxTime > lastCsvMsgTime) 
		     && (csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].windDirectionAbstractIndex].lastRxTime > lastCsvMsgTime) ) {
	      char windString[256];
	      calculateTruewind(csvOutputList[myLoggerNumber].outfieldType[outputIndex], windString, 256, 
				csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].sogAbstractIndex].dataString, 
				csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].cogAbstractIndex].dataString, 
				csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].hdgAbstractIndex].dataString, 
				csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].windSpeedAbstractIndex].dataString, 
				csvAbstractData[calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].windDirectionAbstractIndex].dataString, 
				calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].zlr,
				calcValuesConfig.truewind[csvOutputList[myLoggerNumber].calculationControlIndex[outputIndex]].precision);
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, windString);
	    }
	    else {
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	    }
	    break;
		      
	  default:
	    // the default case will use plain old input fields
	    if(csvAbstractData[csvOutputList[myLoggerNumber].outfieldAbstractIndex[outputIndex]].lastRxTime > lastCsvMsgTime) {
	      //len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, formatString, csvAbstractData[csvOutputList[myLoggerNumber].outfieldAbstractIndex[outputIndex]].dataString);
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvAbstractData[csvOutputList[myLoggerNumber].outfieldAbstractIndex[outputIndex]].dataString);
	    }
	    else {
	      //len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	      snprintf(outputFieldString, MAX_OUTFIELD_STRING_LENGTH, formatString, csvOutputList[myLoggerNumber].badDataString);
	    }
	    break;
	  }  // end of switch for output type

	// now write the field to the actual output record according to the outputFileFormat
	switch(csvOutputList[myLoggerNumber].outputFileFormat) {
	case OUTFILE_FORMAT_SAMOS:
	  len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, "%s:%s, ", csvOutputList[myLoggerNumber].outfieldHeaderString[outputIndex], outputFieldString);
	  break;
	case OUTFILE_FORMAT_NOAA:
	  len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, "%s,", outputFieldString);
	  break;
	case OUTFILE_FORMAT_DSLOGCSV:
	default:
	  if(writeDelimiterFlag) {
	    len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, "%s", csvOutputList[myLoggerNumber].outputDelimiter);
	  }
	  writeDelimiterFlag = true;
		  
	  len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, "%s", outputFieldString);
	  break;
	}  // end of switch over outputFileFormat

      }  // end of if outfieldEnable
    }  // end for loop over outputIndex
  }  // end if(there is at least one defined output field)

  // now write the end of line characters to the output record according to the outputFileFormat
  switch(csvOutputList[myLoggerNumber].outputFileFormat) {
  case OUTFILE_FORMAT_SAMOS:
    len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH,"\n");
    break;
  case OUTFILE_FORMAT_NOAA:
    len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH,"%s,>\n", csvOutputList[myLoggerNumber].noaaVersionString);
    break;
  case OUTFILE_FORMAT_DSLOGCSV:
  default:
    len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, "%c", csvOutputList[myLoggerNumber].termChar1);
    if( -1 != csvOutputList[myLoggerNumber].termChar2) {
      len += snprintf(&outputRecord[len],MAX_LOGGING_STRING_LENGTH, "%c", csvOutputList[myLoggerNumber].termChar2);
    }
    break;
  }  // end of switch over outputFileFormat

  return(len);
}

//-------------------------------------------------------------------------
int calculateNormDiff(char *outString, int outStringSize, char *minuendString, char *subtrahendString, int outputPrecision, double normThreshold, double normOffset)
{
  double minuend;
  double subtrahend;
  double difference;
  char outputFormatString[16];

  //--- create outputFormatString
  sprintf(outputFormatString, "%%.%dlf", outputPrecision);

  //--- parse the minuend
  if(1 != sscanf(minuendString, "%lf", &minuend) ) {
    snprintf(outString, outStringSize, "SSV_BAD_MINUEND");
    return(-1);
  }

  //--- parse the subtrahend
  if(1 != sscanf(subtrahendString, "%lf", &subtrahend) ) {
    snprintf(outString, outStringSize, "SSV_BAD_SUBTRAHEND");
    return(-2);
  }

  //--- calculate the diff
  difference = (minuend - subtrahend);
  while(difference > normThreshold) { difference -= normOffset; }
  while(difference < ( -1.0 * normThreshold ) ) { difference += normOffset; }

  //--- fill the sring and return
  snprintf(outString, outStringSize, outputFormatString, difference );
  return(0);
}

//-------------------------------------------------------------------------
int calculateDiff(char *outString, int outStringSize, char *minuendString, char *subtrahendString, int outputPrecision)
{
  double minuend;
  double subtrahend;
  char outputFormatString[16];

  //--- create outputFormatString
  sprintf(outputFormatString, "%%.%dlf", outputPrecision);

  //--- parse the minuend
  if(1 != sscanf(minuendString, "%lf", &minuend) ) {
    snprintf(outString, outStringSize, "SSV_BAD_MINUEND");
    return(-1);
  }

  //--- parse the subtrahend
  if(1 != sscanf(subtrahendString, "%lf", &subtrahend) ) {
    snprintf(outString, outStringSize, "SSV_BAD_SUBTRAHEND");
    return(-2);
  }

  //--- calculate the diff
  //--- fill the sring and return
  snprintf(outString, outStringSize, outputFormatString, (minuend - subtrahend) );
  return(0);
}

//-------------------------------------------------------------------------
int calculateSsv(char *outString, int outStringSize, char *temperatureString, char *salinityString, double pressure, int outputPrecision)
{
  double temperature;
  double salinity;
  char outputFormatString[16];

  //--- create outputFormatString
  sprintf(outputFormatString, "%%.%dlf", outputPrecision);

  //--- parse the temperature
  if(1 != sscanf(temperatureString, "%lf", &temperature) ) {
    snprintf(outString, outStringSize, "SSV_BAD_TEMPERATURE");
    return(-1);
  }

  //--- parse the salinity
  if(1 != sscanf(salinityString, "%lf", &salinity) ) {
    snprintf(outString, outStringSize, "SSV_BAD_SALINITY");
    return(-2);
  }

  //--- calculate the ssv
  //--- fill the sring and return
  //snprintf(outString, outStringSize, "[T=%lf; S=%lf; P=%lf; ssv=%lf]", temperature, salinity, pressure, ssv_ChenMillero(salinity, temperature, pressure));
  //snprintf(outString, outStringSize, "%.3lf", ssv_ChenMillero(salinity, temperature, pressure));
  snprintf(outString, outStringSize, outputFormatString, ssv_ChenMillero(salinity, temperature, pressure));
  return(0);
}

//-------------------------------------------------------------------------
int calculateBarometer(char *outString, int outStringSize, char *rawPressureString, double sensorHeight, int outputPrecision)
{
  double rawPressure;
  char outputFormatString[16];

  //--- create outputFormatString
  sprintf(outputFormatString, "%%.%dlf", outputPrecision);

  //--- parse the pressure
  if(1 != sscanf(rawPressureString, "%lf", &rawPressure) ) {
    snprintf(outString, outStringSize, "BAROMETER_BAD_RAW_PRESSURE");
    return(-1);
  }

  //--- calculate the barometer
  //--- fill the sring and return
  //snprintf(outString, outStringSize, "%lf", (rawPressure + (0.1185 * sensorHeight) ) );
  snprintf(outString, outStringSize, outputFormatString, (rawPressure + (0.1185 * sensorHeight) ) );
  return(0);
}

//-------------------------------------------------------------------------
int calculateTruewind(int outfieldType, char *outString, int outStringSize, char *sogString, char *cogString, char *hdgString, char *measuredSpeedString, char *measuredDirString, double zlr, int outputPrecision)
{
  double sog;
  double cog;
  double hdg;
  double measuredSpeed;
  double measuredDirection;
  windspeed_result_t result;
  char outputFormatString[16];

  //--- create outputFormatString
  switch(outfieldType) {
  case OUT_TYPE_TRUEWIND_BOTH:
    sprintf(outputFormatString, "%%.%dlf %%.%dlf", outputPrecision, outputPrecision);
    break;
  default:
    sprintf(outputFormatString, "%%.%dlf", outputPrecision);
    break;
  };  // end of switch over outfieldType

  //--- parse the SOG
  if(1 != sscanf(sogString, "%lf", &sog) ) {
    snprintf(outString, outStringSize, "TRUEWIND_BAD_SOG");
    return(-1);
  }

  //--- parse the COG
  if(1 != sscanf(cogString, "%lf", &cog) ) {
    snprintf(outString, outStringSize, "TRUEWIND_BAD_COG");
    return(-2);
  }

  //--- parse the HDG
  if(1 != sscanf(hdgString, "%lf", &hdg) ) {
    snprintf(outString, outStringSize, "TRUEWIND_BAD_HDG");
    return(-3);
  }

  //--- parse the measured speed
  if(1 != sscanf(measuredSpeedString, "%lf", &measuredSpeed) ) {
    snprintf(outString, outStringSize, "TRUEWIND_BAD_MSRSPD");
    return(-4);
  }

  //--- parse the measured direction
  if(1 != sscanf(measuredDirString, "%lf", &measuredDirection) ) {
    snprintf(outString, outStringSize, "TRUEWIND_BAD_MSRDIR");
    return(-5);
  }

  //--- calculate the truewind
  truewind_SmithBourassa(&result, cog, sog, hdg, measuredDirection, measuredSpeed, zlr);
  //truewind_SmithBourassa(&result, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  //--- fill the sring and return
  switch(outfieldType) {
  case OUT_TYPE_TRUEWIND_SPEED:
    //snprintf(outString, outStringSize, "%.1lf", result.speed );
    snprintf(outString, outStringSize, outputFormatString, result.speed );
    break;
  case OUT_TYPE_TRUEWIND_DIRECTION:
    //snprintf(outString, outStringSize, "%.1lf", result.direction );
    snprintf(outString, outStringSize, outputFormatString, result.direction );
    break;
  case OUT_TYPE_TRUEWIND_BOTH:
    //snprintf(outString, outStringSize, "%.1lf %.1lf", result.speed, result.direction );
    snprintf(outString, outStringSize, outputFormatString, result.speed, result.direction );
    break;
  case OUT_TYPE_TRUEWIND_DEBUG:
    snprintf(outString, outStringSize, "[S=%lf; C=%lf; H=%lf; WS=%lf; WD=%lf; Z=%lf; TS=%lf; TD=%lf]", sog, cog, hdg, measuredSpeed, measuredDirection, zlr, result.speed, result.direction);
    break;
  };  // end of switch over outfieldType

  return(0);  
}

//-------------------------------------------------------------------------
/*
   sprintfs date string based on computer system clock

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   2014-11-09   tim thiel       Created from dslog_sprintf_date_string
   ---------------------------------------------------------------------- */
int samos_sprintf_date_string (char *str)
{
  int num_chars;

  // for date and hours
  struct tm tm;
  time_t current_time;

  // call time() and localtime for hour and date 
  // 18 July 2012 fix to use threadsafe localtime_r
  current_time = time (NULL);
  localtime_r (&current_time,&tm);

  num_chars = snprintf (str,32,"%04d%02d%02d", (int) tm.tm_year +1900, (int) tm.tm_mon + 1, (int) tm.tm_mday);

  return num_chars;
}

//-------------------------------------------------------------------------
// sprintfs dsl data time string based on computer system clock

int samos_sprintf_time_string (char *str)
{
  int num_chars;

  double total_secs;
  double secs_in_today;
  double hour;
  double min;
  double sec;

  // for date and hours
  struct tm tm;
  time_t current_time;

  // read gettimeofday() clock and compute min, and
  // sec with microsecond precision
  total_secs = rov_get_time ();
  secs_in_today = fmod (total_secs, 24.0 * 60.0 * 60.0);
  hour = secs_in_today / 3600.0;
  min = fmod (secs_in_today / 60.0, 60.0);
  sec = fmod (secs_in_today, 60.0);

  // call time() and localtime for hour and date 
  // 18 July 2012 fix to use threadsafe localtime_r
  current_time = time (NULL);
  localtime_r (&current_time,&tm);

  num_chars = snprintf (str,32,"%02d%02d%02d", (int) tm.tm_hour, (int) min, (int) sec);
  return num_chars;
}

//-------------------------------------------------------------------------
// returns hour as integer based on computer system clock
int dslog_get_current_hour()
{
  // for date and hours
  struct tm tm;
  time_t current_time;

  // call time() and localtime for hour and date 
  current_time = time (NULL);
  localtime_r (&current_time,&tm);

  return( (int) tm.tm_hour );
}

//-------------------------------------------------------------------------
