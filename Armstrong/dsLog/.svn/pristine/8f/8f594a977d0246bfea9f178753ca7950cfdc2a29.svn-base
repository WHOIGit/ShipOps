/*************************************************************************
csv log thread for SSSG dsLog system

Modification History:
DATE        AUTHOR COMMENT
2013-12-31   tt    created from sioLogThread.cpp

**************************************************************************/
/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

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
#include "sio_thread.h"		/* sio thread */
#include "config_file.h"
#include "dsLogCsv.h"
#include "csvLogThread.h"
// #define DEBUG_CSV

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;

/* ----------------------------------------------------------------------
   Function to process incoming UDP characters
   
   Modification History:
   DATE         AUTHOR  COMMENT
   19-JUL-2000  LLW      Created and written.
   2012-12-18   tt       - I believe the entry above in the mod history refers to when the sio_thread was created and written
   ---------------------------------------------------------------------- */
static void * udpLogRXThread (void *arg)
{
  nio_thread_t *nio = (nio_thread_t *) arg;
  char big_buffer[BUFFERMAX];
  int rcv_size;
  nio = (nio_thread_t *) arg;
  struct sockaddr from_addr;
  unsigned int from_len;
  
  while (1) {
    if ((rcv_size = recvfrom (nio->my_nio_port_table_entry.my_sock, big_buffer, BUFFERMAX, 0, &from_addr, &from_len)) < 0) {
      fprintf (stderr, "udpLogRXThread - error on recvfrom, socket from %s\n", nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to);
    }
    else {			// we got some good data
#ifdef DEBUG_NIO
      printf ("%s (thread %d) device %s  got %d characters\n", global_thread_table[nio->nio_thread_num].thread_name, nio->nio_thread_num,
	      nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to, rcv_size);
      printf(" the data: %s\n",big_buffer);
      fflush (stdout);
#endif
      // lock the data structure mutex so that we chan diddle with the data
      pthread_mutex_lock (&nio->mutex);
      // keep stats
      nio->total_rx_chars += rcv_size;
#ifdef DEBUG_NIO
      printf (" n of destinations for this data is % d \n ", nio->my_nio_port_table_entry.n_of_destinations);
#endif
      for (int i = 0; i < nio->my_nio_port_table_entry.n_of_destinations; i++) {
#ifdef DEBUG_NIO
	printf (" sending %d bytes to thread %d\n", rcv_size, nio->my_nio_port_table_entry.auto_send_addresses[i]);
#endif
	msg_send( nio->my_nio_port_table_entry.auto_send_addresses[i], nio->nio_thread_num, SAS, rcv_size, big_buffer);
      }
      pthread_mutex_unlock (&nio->mutex);
    }
  }
  return NULL;
}

/* ----------------------------------------------------------------------*/
//    csvLogThread
//   Modification History:
//   DATE         AUTHOR  COMMENT
//  5 nov 2009    jch     created and writen, derived from sio_thread.cpp
//----------------------------------------------------------------------*/
void *
csvLogThread (void *thread_num)
{
  sio_thread_t sio = { PTHREAD_MUTEX_INITIALIZER, -1 };
   
  msg_hdr_t hdr = { 0 };
  unsigned char data[MSG_DATA_LEN_MAX] = { 0 };

  int csvDataJunk = -999;
  
  loggerStatus_t myStatus;
  myStatus.bytesReceived = 0;
  myStatus.msgsRecieved = 0;
  myStatus.readErrors = 0;
  myStatus.timeSinceLastData = 999.0;

  DestinationStatus_t  destinationStatus;
  destinationStatus.nOfDestinations = 0;
  destinationStatus.statusTime = rov_get_time();
  for(int dN = 0; dN < MAX_N_OF_DESTINATIONS; dN++) {
    destinationStatus.statuses[dN].errorCount = 0;
    destinationStatus.statuses[dN].bytesSent = 0;
    destinationStatus.statuses[dN].availableSpace = 0;
    destinationStatus.statuses[dN].freeSpace = 0;
    destinationStatus.statuses[dN].percentageFree = 100.0;
  }
  char *my_name = NULL;
  
  /* get my thread number */
  sio.sio_thread_num = (long int) thread_num;
  
  int myLoggerNumber;
  // now look into the process table and figure out what thread I am
  for (int i = 0; i < MAX_NUMBER_OF_THREADS; i++) {
    if (global_thread_table[i].thread_num == sio.sio_thread_num) {
      my_name = global_thread_table[i].thread_name;
      myLoggerNumber = global_thread_table[i].extra_arg_1;
    }
  }
  if (!my_name) {
    fprintf (stderr, "SIO_LOG_THREAD: thread %d  not found in process table!\n", sio.sio_thread_num);
    fflush (stdout);
    fflush (stderr);
    abort ();
  }

  //----- read operational settings for serial or udp port as appropriate
  if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType) {
    // settings for a udp port source
    sio.my_nio_port_table_entry.socket_number  = theLoggingList[myLoggerNumber].inSocketNumber;
    sio.my_nio_port_table_entry.to_socket_number  = theLoggingList[myLoggerNumber].outSocketNumber;
    sio.my_nio_port_table_entry.ip_address = strdup(theLoggingList[myLoggerNumber].outIpAddress);
    printf("******* socket_number is %d\n", sio.my_nio_port_table_entry.socket_number);
    printf("******* to_socket_number is %d\n", sio.my_nio_port_table_entry.to_socket_number);
    printf("******* ip_address is %s\n", sio.my_nio_port_table_entry.ip_address);
  }
  
  promptLoggerStatus_t myPromptStatus;
  myPromptStatus.sourceNumber = theLoggingList[myLoggerNumber].inputNumber;
  strncpy(myPromptStatus.sourceName, theLoggingList[myLoggerNumber].sourceName, sizeof(myPromptStatus.sourceName));
  myPromptStatus.numberOfPrompts = theLoggingList[myLoggerNumber].promptSequence.numberOfPrompts;
  myPromptStatus.bytesReceived = 0;
  myPromptStatus.msgsRecieved = 0;
  myPromptStatus.readErrors = 0;
  myPromptStatus.timeSinceLastData = 999.0;
  for(int myPromptCount=0; myPromptCount<MAX_N_OF_PROMPTS_IN_SEQUENCE; ++myPromptCount) {
    sprintf(myPromptStatus.promptSourceName[myPromptCount], "UNDEFINED");
  }
  for(int myPromptCount=0; myPromptCount<myPromptStatus.numberOfPrompts; ++myPromptCount) {
    strncpy(myPromptStatus.promptSourceName[myPromptCount], theLoggingList[myLoggerNumber].promptSourceName[myPromptCount], sizeof(myPromptStatus.promptSourceName[myPromptCount]));
  }
  for(int myPromptCount=0; myPromptCount<MAX_N_OF_PROMPTS_IN_SEQUENCE; ++myPromptCount) {
    myPromptStatus.promptStatus[myPromptCount].bytesReceived = 0;
    myPromptStatus.promptStatus[myPromptCount].msgsRecieved = 0;
    myPromptStatus.promptStatus[myPromptCount].readErrors = 0;
    myPromptStatus.promptStatus[myPromptCount].timeSinceLastData = 999.0;
  }
  
  destinationStatus.nOfDestinations = theLoggingList[myLoggerNumber].nOfDestinations;
  //printf(" num of destinations = %d\n",theLoggingList[myLoggerNumber].nOfDestinations);
  
  for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++) {
    if(UDP_SOCKET == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
      theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.socket =  socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      int broadcastPermission = 1;
      if(setsockopt(theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.socket,SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0) {
	fprintf (stderr, "***couldn't set broadcast on UDP : %s \n",theLoggingList[myLoggerNumber].sourceName );
      }
      memset(&(theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr), 0, sizeof (theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr));
      
      theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr.sin_family = AF_INET;
      theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr.sin_addr.s_addr = inet_addr(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.ipAddress);
      theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr.sin_port =  htons(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.toSocketNumber);
    }
    else if(DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
      theLoggingList[myLoggerNumber].destinations[ destinationCount].loggingDestination.asciiLogFileD = NULL;
    }
  }
  int msg_success = msg_queue_new(sio.sio_thread_num, sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
  if(msg_success != MSG_OK) {
    if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType) {
      fprintf(stderr, "SIO_LOG_THREAD creating msg_queue for udp port %d: %s\n", theLoggingList[myLoggerNumber].inSocketNumber, MSG_ERROR_CODE_NAME(msg_success) );
    }
    else {
      fprintf(stderr, "SIO_LOG_THREAD creating msg_queue for %s: %s\n",sio.my_sio_port_table_entry.sio_com_port_name,MSG_ERROR_CODE_NAME(msg_success) );
    }
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
  printf("Displaying destination info for csvLogThread.  COM port name is %s\n", sio.my_sio_port_table_entry.sio_com_port_name);
  printf("myLoggerNumber = %d\n", myLoggerNumber);
  printf("theLoggingList[myLoggerNumber].nOfDestinations = %d\n", theLoggingList[myLoggerNumber].nOfDestinations);
  
  for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++) {
    printf("---------------\n");
    printf("theLoggingList[myLoggerNumber].destinations[%d].destinationType = %d\n", destinationCount, theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType);
    if(UDP_SOCKET == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
      printf("destinationType is UDP_SOCKET\n");
    }
    if(DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
      printf("destinationType is DISK\n");
    }
    //printf("");
    //printf("");
  }
  printf("--------------------------------------------------------------\n");
  printf("--------------------------------------------------------------\n");
#endif
  // - above #endif closes section for displaying destination info
  
  
  if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType) {
    // ----------------------------------------------------------------------
    // open the network port
    // ----------------------------------------------------------------------
#ifdef DEBUG_NIO_LOGGER
    printf ("NIO THREAD: about to open port %d for input\n",sio.my_nio_port_table_entry.socket_number);
#endif
    sio.sio_port_fid = open_network (&sio.my_nio_port_table_entry);
#ifdef DEBUG_NIO_LOGGER
    printf ("NIO THREAD: got by open port, FID=%d\n", sio.nio_port_fid);
#endif
    sio.my_nio_port_table_entry.n_of_destinations = 1;
    sio.my_nio_port_table_entry.auto_send_addresses[0] = sio.sio_thread_num;
    // launch the thread to process incoming characters from the network port
    pthread_create (&sio.sio_rx_subthread, &DEFAULT_ROV_THREAD_ATTR, udpLogRXThread, (void *) (&sio));
  }  // end of if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType)

  // launch timer for generating csv message  
  launched_timer_data_t   *csvmsgTimer = launch_timer_new(1.0, -1, sio.sio_thread_num, CSVMSG);

  // wakeup message
  printf ("CSV LOG THREAD: SIO (thread %d) initialized \n", sio.sio_thread_num);
  printf ("CSV LOG THREAD: CSV File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);

  //**********************************************************************************
  //**********************************************************************************
  //**********************************************************************************
  // loop forever
  while (1) {
    // wait forever on the input channel
#ifdef DEBUG_CSV
    printf ("CSV calling get_message.\n");
#endif

    int msg_get_success = msg_get(sio.sio_thread_num,&hdr, data, MSG_DATA_LEN_MAX);
    if(msg_get_success != MSG_OK) {
      fprintf(stderr, "sio thread--error on msg_get:  %s\n",MSG_ERROR_CODE_NAME(msg_get_success));
    }
    else {
      // #define DEBUG_RECIEVED_MESSAGES
      
#ifdef DEBUG_RECIEVED_MESSAGES
      // print on stderr
      printf ("%s got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
	      JASONTALK_THREAD_NAME(sio.sio_thread_num),
	      JASONTALK_MESSAGE_NAME(hdr.type),hdr.type,
	      JASONTALK_THREAD_NAME(hdr.from),hdr.from,
	      JASONTALK_THREAD_NAME(hdr.to),hdr.to,
	      hdr.length );
#endif
#ifdef DEBUG_CSV
      printf ("%s got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
	      JASONTALK_THREAD_NAME(sio.sio_thread_num),
	      JASONTALK_MESSAGE_NAME(hdr.type),hdr.type,
	      JASONTALK_THREAD_NAME(hdr.from),hdr.from,
	      JASONTALK_THREAD_NAME(hdr.to),hdr.to,
	      hdr.length );
#endif
      // process new message
      switch (hdr.type) {
      case CSVMSG:		// recieved a CSVMSG (Create CSV record) Message
	{
#if 1
	  char    outputRecord[MAX_LOGGING_STRING_LENGTH];
	  int len, dataLength;
	  char    timeString[48];

	  rov_sprintf_dsl_time_string(timeString);
	  //len = snprintf(outputRecord,MAX_LOGGING_STRING_LENGTH,"%s %s %s %s" ,dataTypeString,timeString,sourceNameString,data);
	  len = snprintf(outputRecord,MAX_LOGGING_STRING_LENGTH,"CSVTEST %s 11 22 33 %d 0x%04X\n", timeString, csvDataJunk, csvDataJunk);
	  dataLength = len;

	  printf("hey hey -- %d  \n", theLoggingList[myLoggerNumber].nOfDestinations);

	  // now we've got the record we want to distribute and/or log
	  for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++) {
	  printf("my my -- \n");
	    if(1 == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationEnable) {
	      switch( theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
	      case UDP_SOCKET:
		{
		  sendto ( theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.socket, outputRecord, dataLength, 0, (struct sockaddr *) &(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.Addr), sizeof (theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination));
		  destinationStatus.statuses[destinationCount].bytesSent += dataLength;
		  break;
		}
	      case DISK:
		{
		  logThisNow(&(theLoggingList[myLoggerNumber].destinations[destinationCount]),&(theLoggingList[myLoggerNumber]),outputRecord);
		  destinationStatus.statuses[destinationCount].bytesSent += dataLength;
		  break;
		}
	      case SINGLE_DISK:
		{
		  logThisNow(&theGlobalLogFile,&theGlobalLog,outputRecord);
		  destinationStatus.statuses[destinationCount].bytesSent += dataLength;
		  break;
		}
	      default:
		{
		  break;
		}
	      }  // end of switch(destinationType)
	    }  // end of if (destination is enabled)
	  }  // end of for(destinationCount)
#endif
	  break;
	}  // end of case CSVMSG:

      case SAS:
	{			// received a SAS, probably from my subthread
	  int tmpVal;

#ifdef DEBUG_CSV
	  printf ("CSV THREAD: %s: (thread %d) CSV RECIEVED SAS -" "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
	  // first, null terminate
	  data[hdr.length] = '\0';

	  for(int parseIndex=0; parseIndex < theLoggingList[myLoggerNumber].numberOfCsvParse; ++parseIndex) {
	    printf("parseIndex = %d \n", parseIndex);
	    if(0 != strcmp(theLoggingList[myLoggerNumber].csvParse[parseIndex].prefixString, "INVALID_PREFIX")) {
	      char *instring;
	      char formatString[64];
	      instring = strstr((char*)data, theLoggingList[myLoggerNumber].csvParse[parseIndex].prefixString);
	      if(NULL != instring) {
		sprintf(formatString, "%s %%d", theLoggingList[myLoggerNumber].csvParse[parseIndex].prefixString);
		if(1 == sscanf( instring, formatString, &tmpVal)) {
		  csvDataJunk = tmpVal;
		}
	      }  // end of if prefixString found
	    }  // end of if prefixString is valid
	  }
	  //if(0) { 
	  if(1 == sscanf( (char *)data, "NAV %d", &tmpVal)) {
	    //csvDataJunk = tmpVal;
	  }
	  break;
	}  // end of case SAS:

      case 98765:
	{			// received a SAS, probably from my subthread
#ifdef DEBUG_CSV
	  printf ("CSV THREAD: %s: (thread %d) CSV RECIEVED SAS -" "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
#if 0
	  // first, null terminate
	  data[hdr.length] = '\0';
	  // check for terminators and get rid of them
	  if(('\r' == data[hdr.length-1]) || ('\n' == data[hdr.length-1])) {
	    data[hdr.length-1] = '\0';
	  }
	  if(hdr.from == sio.sio_thread_num) {
	    myStatus.timeOfLastMsg = rov_get_time();
	    myStatus.msgsRecieved++;
	    myStatus.bytesReceived += hdr.length;

	    myPromptStatus.timeOfLastMsg = rov_get_time();
	    myPromptStatus.msgsRecieved++;
	    myPromptStatus.bytesReceived += hdr.length;
	    myPromptStatus.promptStatus[activePrompt].timeOfLastMsg = rov_get_time();
	    myPromptStatus.promptStatus[activePrompt].msgsRecieved++;
	    myPromptStatus.promptStatus[activePrompt].bytesReceived += hdr.length;
	  }
	  // first check to see if we should timestamp this record
	  char    outputRecord[MAX_LOGGING_STRING_LENGTH];
	  int dataLength;
	  if(theLoggingList[myLoggerNumber].timeStampFlag) {
	    char    timeString[48];
	    char *dataTypeString;
	    char *sourceNameString;
	    
	    if(theLoggingList[myLoggerNumber].usePromptSpecificDataType) {
	      dataTypeString = strdup(theLoggingList[myLoggerNumber].promptDataType[activePrompt]);
	    }
	    else {
	      dataTypeString = strdup(theLoggingList[myLoggerNumber].dataType);
	    }
	    if(theLoggingList[myLoggerNumber].usePromptSpecificSourceName) {
	      sourceNameString = strdup(theLoggingList[myLoggerNumber].promptSourceName[activePrompt]);
	    }
	    else {
	      sourceNameString = strdup(theLoggingList[myLoggerNumber].sourceName);
	    }
	    rov_sprintf_dsl_time_string(timeString);
	    int len = snprintf(outputRecord,MAX_LOGGING_STRING_LENGTH,"%s %s %s %s" ,dataTypeString,timeString,sourceNameString,data);
	    // null terminate just in case
	    outputRecord[len] = '\0';
	    dataLength = len;
	    
	    free(dataTypeString);
	    free(sourceNameString);
	  }
	  else {
	    strncpy(outputRecord,(char *)data,hdr.length);
	    dataLength = hdr.length;
	  }
	  // does it have a /n on it?  check and see
	  if(outputRecord[dataLength-1] != '\n') {
	    outputRecord[dataLength] = '\n';
	    outputRecord[dataLength+1] = '\0';
	    dataLength += 1;
	  }
	  // now we've got the record we want to distribute and/or log
	  for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++) {
	    if(1 == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationEnable) {
	      if( (0 == theLoggingList[myLoggerNumber].usePromptSpecificDestinationsFlag) || (theLoggingList[myLoggerNumber].promptDestinationEnable[activePrompt][destinationCount]) ) {
		//printf("--->>>  --->>>  whichPrompt is %d; activePrompt is %d; destinationCount is %d; rxdata is %s\n", whichPrompt, activePrompt, destinationCount, data);
		switch( theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
		case UDP_SOCKET:
		  {
		    // here is where I should check the filters
		    int send = true;
		    
		    if(theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.implemented) {
		      char *filterPointer =  theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.startsWith;
		      
		      if(filterPointer) {
			// printf(" the filter pointer %s the data %s",filterPointer,data);
			if(!strncmp((char *)data,filterPointer,strlen(filterPointer))) {
			  send = true;
			}
			else {
			  send = false;
			}
		      }
		      filterPointer =  theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.greps;
		      if(filterPointer) {
			if(strstr((char *)data,filterPointer)) {
			  send = true;
			}
			else {
			  send = false;
			}
		      }
		      filterPointer =  theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.grepMinusVs;
		      if(filterPointer) {
			if(strstr( (char *)data, filterPointer)) {
			  send = false;
			}
			else {
			  send = true;
			}
		      }
		    }
		    
		    if(send) {
		      sendto ( theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.socket, outputRecord, dataLength, 0, (struct sockaddr *) &(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.Addr), sizeof (theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination));
		      destinationStatus.statuses[destinationCount].bytesSent += dataLength;
		    }
		    break;
		  }
		case DISK:
		  {
		    logThisNow(&(theLoggingList[myLoggerNumber].destinations[destinationCount]),&(theLoggingList[myLoggerNumber]),outputRecord);
		    destinationStatus.statuses[destinationCount].bytesSent += dataLength;
		    break;
		  }
		case SINGLE_DISK:
		  {
		    logThisNow(&theGlobalLogFile,&theGlobalLog,outputRecord);
		    destinationStatus.statuses[destinationCount].bytesSent += dataLength;
		    break;
		  }
		default:
		  {
		    break;
		  }
		}  // end of switch(destinationType)
	      }  // end of if(use this destination for this prompt)
	    }  // end of if (destination is enabled)
	  }  // end of for(destinationCount)
#endif
	  break;
	}  // end of case SAS:
      case STPR:
	{
	  rov_time_t timeNow = rov_get_time();

	  // send a status msg to the status thread
	  myStatus.timeSinceLastData = timeNow - myStatus.timeOfLastMsg;	  
	  myStatus.statusTime = timeNow;
	  msg_send(DSLOG_STATUS_THREAD,sio.sio_thread_num,LSTM, sizeof(myStatus), &myStatus);
	  
	  // send prompt specific status msgs to the status thread
	  myPromptStatus.timeSinceLastData = timeNow - myStatus.timeOfLastMsg;	  
	  myPromptStatus.statusTime = timeNow;
	  for(int myPromptCount=0; myPromptCount<MAX_N_OF_PROMPTS_IN_SEQUENCE; ++myPromptCount) {
	    myPromptStatus.promptStatus[myPromptCount].timeSinceLastData = timeNow - myPromptStatus.promptStatus[myPromptCount].timeOfLastMsg;
	    myPromptStatus.promptStatus[myPromptCount].statusTime = timeNow;
	  }	  
	  msg_send(DSLOG_STATUS_THREAD,sio.sio_thread_num, ISLS, sizeof(myPromptStatus), &myPromptStatus);
	  
	  break;
	}
      case RDSP:
	{
	  struct statvfs sfs;
	  char *logging_directory;
	  for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++) {
	    if(DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
	      logging_directory = theLoggingList[myLoggerNumber].destinations[destinationCount].loggingDestination.loggingDirectory;
	      destinationStatus.statuses[destinationCount].destinationType = DISK;
	    }
	    else if(SINGLE_DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType) {
	      logging_directory = theGlobalLogFile.loggingDestination.loggingDirectory;
	      destinationStatus.statuses[destinationCount].destinationType = SINGLE_DISK;
	    }
	    else {
	      continue;
	    }
	    if (statvfs(logging_directory, &sfs) == -1) {
	      continue;  // problem on the statfs
	    }
	    else {
	      // calculate the file system size
	      unsigned long long blksize;
	      unsigned long long blocks;
	      unsigned long long freeblks;
	      unsigned long long frgSize;
	      unsigned long long used;
	      
	      blksize = sfs.f_bsize;
	      blocks = sfs.f_blocks;
	      freeblks = sfs.f_bavail;
	      frgSize = sfs.f_frsize;
	      
	      unsigned long long diskSize = blocks * frgSize;
	      unsigned long long freeSize = freeblks * blksize;
	      used = diskSize - freeSize;
	      double percentage = 100.0 * ((double)used)/((double)diskSize);
	      destinationStatus.statuses[destinationCount].availableSpace = freeSize;
	      destinationStatus.statuses[destinationCount].freeSpace = freeSize;
	      destinationStatus.statuses[destinationCount].percentageFree = 100.0 - percentage;
	    }
	  }
	  destinationStatus.statusTime = rov_get_time();
	  
	  msg_send(DSLOG_STATUS_THREAD,sio.sio_thread_num,LSDM, sizeof(destinationStatus), &destinationStatus);
	  break;
	}
                     
      case RAS:		// recieved a RAS (Read Serial String) Message
	{
	  // lock the mutex
	  pthread_mutex_lock (&sio.mutex);
	  
	  // send a SAS status message to the designated process
	  msg_send(hdr.from, sio.sio_thread_num, SAS, sio.inchar_index + 1, sio.inchars);
	  
	  // zero the character array index
	  sio.inchar_index = 0;
	  
	  // unlock the mutex
	  pthread_mutex_unlock (&sio.mutex);
	  break;
	}
        
      case PNG:		// recieved a PNG (Ping) message
	{
	  char msg[256];
	  snprintf (msg, 256,"%s: (thread %d) is Alive!", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num);
	  // respond with a SPI (Status Ping) message
	  msg_send (hdr.from, hdr.to, SPI, strlen (msg), msg);
	  break;
	}
      case BYE:  // received a bye message--time to give up the ghost--or at least my socket
	{
	  
	  close(sio.sio_port_fid);
	  launch_timer_disable(csvmsgTimer);
	  
	  return(NULL);
	  
	  break;
	}
      case SPI:		// recieved a SPI (Status Ping) message
	break;
        
      default:		// recieved an unknown message type
	{
	  printf ("%s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE - " "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
	  break;
	}  // end of hdr.type - default:
      } // end of switch(hdr.type)
    } // else
  } //while(1)
  
  return NULL;
}
