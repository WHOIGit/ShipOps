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
//   2014-05-02   tet     this thread was derived from the logging thread for dsLog
//----------------------------------------------------------------------*/
void *
csvLogThread (void *thread_num)
{
  sio_thread_t sio = { PTHREAD_MUTEX_INITIALIZER, -1 };
   
  msg_hdr_t hdr = { 0 };
  unsigned char data[MSG_DATA_LEN_MAX] = { 0 };

  char *my_name = NULL;
  
  /* get my thread number */
  sio.sio_thread_num = (long int) thread_num;
  printf("*** csvLogThread - thread_number is %d\n", sio.sio_thread_num);
  
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
  printf("***************************************************\n");
  printf("***************************************************\n");
  printf("*** csvLogThread - localThreadNumber is %d\n", sio.sio_thread_num);
  printf("*** csvLogThread - thread_name is %s\n", my_name);
  printf("*** csvLogThread - myLoggerNumber is %d\n", myLoggerNumber);

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
      case SAS:
	{			// received a SAS, probably from my subthread

#ifdef DEBUG_CSV
	  printf ("CSV THREAD: %s: (thread %d) CSV RECIEVED SAS -" "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
	  // first, null terminate
	  data[hdr.length] = '\0';

	  //-------------------  cut --------------------------
	  csvParseData( (char *) data, myLoggerNumber );
	  //-------------------  cut --------------------------
	  break;
	}  // end of case SAS:

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

//----------------------------------------------------------------------------------------------------------------------------------------------------
int csvParseData(char *data, int myLoggerNumber)
{
  char *instring;

  //-------------------  cut --------------------------
  for(int parseIndex=0; parseIndex < theLoggingList[myLoggerNumber].numberOfCsvParse; ++parseIndex) {
    //--- if a mustContain is defined and not in the string then skip this string
    if( (0 != strcmp(theLoggingList[myLoggerNumber].csvParse[parseIndex].mustContain, "INVALID_STRING") ) 
	&& (NULL == strstr((char*)data, theLoggingList[myLoggerNumber].csvParse[parseIndex].mustContain)) ) {
      //printf("parseIndex=%d - %s - mustContain FAIL\n", parseIndex, theLoggingList[myLoggerNumber].csvParse[parseIndex].mustContain);
      continue;
    }
    
    //--- if a mustContain2 is defined and not in the string then skip this string
    if( (0 != strcmp(theLoggingList[myLoggerNumber].csvParse[parseIndex].mustContain2, "INVALID_STRING") ) 
	&& (NULL == strstr((char*)data, theLoggingList[myLoggerNumber].csvParse[parseIndex].mustContain2)) ) {
      //printf("parseIndex=%d - %s - mustContain2 FAIL\n", parseIndex, theLoggingList[myLoggerNumber].csvParse[parseIndex].mustContain2);
      continue;
    }

    //--- if a mustNotContain is defined and it is in the string then skip this string
    if( (0 != strcmp(theLoggingList[myLoggerNumber].csvParse[parseIndex].mustNotContain, "INVALID_STRING") ) 
	&& (NULL != strstr((char*)data, theLoggingList[myLoggerNumber].csvParse[parseIndex].mustNotContain)) ) {
      //printf("parseIndex=%d - %s - mustNotContain FAIL\n", parseIndex, theLoggingList[myLoggerNumber].csvParse[parseIndex].mustNotContain);
      continue;
    }

    //--debug-csv-log--printf("parseIndex = %d \n", parseIndex);
    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
      //---------------------------------
    case CPD_GPRMC_UTC_TIME:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int hour, minute, second, fraction;
	if(4 == sscanf(instring, "$GPRMC,%2d%2d%2d.%2d", &hour, &minute, &second, &fraction)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d:%02d:%02d.%02d", hour, minute, second, fraction);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_HOUR:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_MINUTE:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_SECOND:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%*2d%*2d%2d.", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_FRACTION:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%*2d%*2d%*2d.%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_RCVR_STATUS:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	char rcvrStatusString[16];
	if(1 == sscanf(instring, "$GPRMC,%*[^','],%15s", rcvrStatusString)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", rcvrStatusString);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_LAT_DECDEG:
    case CPD_GPRMC_LAT_DECMIN:
    case CPD_GPRMC_LAT_DECSEC:
    case CPD_GPRMC_LAT_NOAA:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int latDegreesInt;
	int latMinutesInt;
	double latDegreesDbl;
	double latMinutesDbl;
	double latSecondsDbl;
	char latHemiChar;
	if(3 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%2d%lf,%c", &latDegreesInt, &latMinutesDbl, &latHemiChar)) {
	  latDegreesDbl = (double) latDegreesInt + ( latMinutesDbl / 60.0 );
	  latMinutesInt = (int) latMinutesDbl;
	  latSecondsDbl = 60.0 * (latMinutesDbl - (double) latMinutesInt);

	  if( ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    latDegreesDbl = -1.0 * latDegreesDbl;
	    latDegreesInt =   -1 * latDegreesInt;
	  }
	  if( ('N' == latHemiChar) || ('N' == latHemiChar) || ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    //--debug-csv-log--printf("GPRMC_LAT_DECDEG ----  parseAbstractIndex = %d  ----  latDegreesInt = %d  ---  latMinutesDbl = %.4lf ---  latHemiChar = %c = %d   ---  latDegreesDbl = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, latDegreesInt, latMinutesDbl, latHemiChar, latHemiChar, latDegreesDbl);
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GPRMC_LAT_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", latDegreesDbl);
	      break;
	    case CPD_GPRMC_LAT_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", latDegreesInt, latMinutesDbl);
	      break;
	    case CPD_GPRMC_LAT_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", latDegreesInt, latMinutesInt, latSecondsDbl);
	      break;
	    case CPD_GPRMC_LAT_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d%06.3lf%c", abs(latDegreesInt), latMinutesDbl, latHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid latHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GPRMC_LON_DECDEG:
    case CPD_GPRMC_LON_DECMIN:
    case CPD_GPRMC_LON_DECSEC:
    case CPD_GPRMC_LON_NOAA:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int lonDegreesInt;
	int lonMinutesInt;
	double lonDegreesDbl;
	double lonMinutesDbl;
	double lonSecondsDbl;
	char lonHemiChar;
	if(3 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%3d%lf,%c", &lonDegreesInt, &lonMinutesDbl, &lonHemiChar)) {
	  lonDegreesDbl = (double) lonDegreesInt + ( lonMinutesDbl / 60.0 );
	  lonMinutesInt = (int) lonMinutesDbl;
	  lonSecondsDbl = 60.0 * (lonMinutesDbl - (double) lonMinutesInt);
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) ) {
	    lonDegreesDbl = -1.0 * lonDegreesDbl;
	    lonDegreesInt =   -1 * lonDegreesInt;
	  }
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) || ('E' == lonHemiChar) || ('e' == lonHemiChar) ) {
	    //--debug-csv-log--printf("GPRMC_LON_DECDEG ----  parseAbstractIndex = %d  ----  lonDegreesInt = %d  ---  lonMinutesDbl = %.4lf ---  lonHemiChar = %c = %d   ---  lonDegreesDbl = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, lonDegreesInt, lonMinutesDbl, lonHemiChar, lonHemiChar, lonDegreesDbl);
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GPRMC_LON_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", lonDegreesDbl);
	      break;
	    case CPD_GPRMC_LON_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", lonDegreesInt, lonMinutesDbl);
	      break;
	    case CPD_GPRMC_LON_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", lonDegreesInt, lonMinutesInt, lonSecondsDbl);
	      break;
	    case CPD_GPRMC_LON_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%03d%06.3lf%c", abs(lonDegreesInt), lonMinutesDbl, lonHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid lonHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GPRMC_SOG:
      //--debug-csv-log--printf("CPD_PARSE -  GPRMC_SOG case\n");
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	double sogValue;
	if(1 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &sogValue)) {
	  //--debug-csv-log--printf("GPRMC_SOG ----  parseAbstractIndex = %d  ----  sogValue = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, sogValue);
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", sogValue);
	}
	else {
	  //--debug-csv-log--printf("GPRMC_SOG ----  sscanf fails!! \n");
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_COG:
    case CPD_GPRMC_COG_SAMOS:
      //--debug-csv-log--printf("CPD_PARSE -  GPRMC_COG case\n");
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	double cogValue;
	if(1 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &cogValue)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	  case CPD_GPRMC_COG:
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", cogValue);
	    break;
	  case CPD_GPRMC_COG_SAMOS:
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%05.1lf", cogValue);
	    break;
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_DATE:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int year, month, day;
	if(3 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%2d%2d%2d", &day, &month, &year)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  // output format changed  t. thiel 2014-11-06
	  //sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d%02d%02d", day, month, year);
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%04d-%02d-%02d", year+2000, month, day);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_YEAR:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*2d%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_MONTH:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*2d%2d%*2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_UTC_DAY:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%2d%*2d%*2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMC_MAGVAR:
      instring = strstr((char*)data, "$GPRMC");
      if(NULL != instring) {
	double variation;
	char eastWest;
	if(2 == sscanf(instring, "$GPRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf,%c*", &variation, &eastWest)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf %c", variation, eastWest);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_STATUS:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	char value;
	if(1 == sscanf(instring, "$GPRMB,%c", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_CTE_MAG:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_CTE_DIR:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	char value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%c", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_ORIGIN_ID:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_DEST_ID:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_DEST_LAT:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	int latDegreesInt;
	double latMinutesDbl;
	double tmpval;
	char latHemiChar;
	if(3 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%2d%lf,%c", &latDegreesInt, &latMinutesDbl, &latHemiChar)) {
	  tmpval = (double) latDegreesInt + ( latMinutesDbl / 60.0 );
	  if( ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    tmpval = -1.0 * tmpval;
	  }
	  if( ('N' == latHemiChar) || ('N' == latHemiChar) || ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_DEST_LON:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	int lonDegrees;
	double lonMinutes;
	double tmpval;
	char lonHemiChar;
	if(3 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%3d%lf,%c", &lonDegrees, &lonMinutes, &lonHemiChar)) {
	  tmpval = (double) lonDegrees + ( lonMinutes / 60.0 );
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) ) {
	    tmpval = -1.0 * tmpval;
	  }
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) || ('E' == lonHemiChar) || ('e' == lonHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_RANGE:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_BEARING:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_DEST_SPEED:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPRMB_ARRIVAL_ALARM:
      instring = strstr((char*)data, "$GPRMB");
      if(NULL != instring) {
	char value;
	if(1 == sscanf(instring, "$GPRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%c*", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_UTC_HOUR:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_UTC_MINUTE:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_UTC_SECOND:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%*2d%*2d%2d.", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_UTC_FRACTION:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%*2d%*2d%*2d.%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_UTC_TIME:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int hour, minute, second, fraction;
	if(4 == sscanf(instring, "$GPGGA,%2d%2d%2d.%2d", &hour, &minute, &second, &fraction)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d:%02d:%02d.%02d", hour, minute, second, fraction);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_LAT_DECDEG:
    case CPD_GPGGA_LAT_DECMIN:
    case CPD_GPGGA_LAT_DECSEC:
    case CPD_GPGGA_LAT_NOAA:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int latDegreesInt;
	int latMinutesInt;
	double latDegreesDbl;
	double latMinutesDbl;
	double latSecondsDbl;
	char latHemiChar;
	if(3 == sscanf(instring, "$GPGGA,%*[^','],%2d%lf,%c", &latDegreesInt, &latMinutesDbl, &latHemiChar)) {
	  latDegreesDbl = (double) latDegreesInt + ( latMinutesDbl / 60.0 );
	  latMinutesInt = (int) latMinutesDbl;
	  latSecondsDbl = 60.0 * (latMinutesDbl - (double) latMinutesInt);
	  if( ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    latDegreesDbl = -1.0 * latDegreesDbl;
	  }
	  if( ('N' == latHemiChar) || ('N' == latHemiChar) || ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GPGGA_LAT_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", latDegreesDbl);
	      break;
	    case CPD_GPGGA_LAT_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", latDegreesInt, latMinutesDbl);
	      break;
	    case CPD_GPGGA_LAT_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", latDegreesInt, latMinutesInt, latSecondsDbl);
	      break;
	    case CPD_GPGGA_LAT_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d%06.3lf%c", abs(latDegreesInt), latMinutesDbl, latHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid latHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GPGGA_LON_DECDEG:
    case CPD_GPGGA_LON_DECMIN:
    case CPD_GPGGA_LON_DECSEC:
    case CPD_GPGGA_LON_NOAA:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int lonDegreesInt;
	int lonMinutesInt;
	double lonDegreesDbl;
	double lonMinutesDbl;
	double lonSecondsDbl;
	char lonHemiChar;
	if(3 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%3d%lf,%c", &lonDegreesInt, &lonMinutesDbl, &lonHemiChar)) {
	  lonDegreesDbl = (double) lonDegreesInt + ( lonMinutesDbl / 60.0 );
	  lonMinutesInt = (int) lonMinutesDbl;
	  lonSecondsDbl = 60.0 * (lonMinutesDbl - (double) lonMinutesInt);
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) ) {
	    lonDegreesDbl = -1.0 * lonDegreesDbl;
	    lonDegreesInt =   -1 * lonDegreesInt;
	  }
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) || ('E' == lonHemiChar) || ('e' == lonHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GPGGA_LON_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", lonDegreesDbl);
	      break;
	    case CPD_GPGGA_LON_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", lonDegreesInt, lonMinutesDbl);
	      break;
	    case CPD_GPGGA_LON_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", lonDegreesInt, lonMinutesInt, lonSecondsDbl);
	      break;
	    case CPD_GPGGA_LON_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%03d%06.3lf%c", abs(lonDegreesInt), lonMinutesDbl, lonHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid lonHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GPGGA_QUALITY:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_NUM_SAT:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_DILUTION:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_ALTITUDE:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_GEOID_HEIGHT:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],M,%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_FIX_AGE:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],M,,%*[^','],M,%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPGGA_STATION_ID:
      instring = strstr((char*)data, "$GPGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],M,,%*[^','],M,%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_HOUR:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_MINUTE:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_SECOND:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*2d%*2d%2d.", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_FRACTION:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*2d%*2d%*2d.%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_TIME:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int hour, minute, second, fraction;
	if(4 == sscanf(instring, "$GPZDA,%2d%2d%2d.%2d", &hour, &minute, &second, &fraction)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d:%02d:%02d.%02d", hour, minute, second, fraction);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_DATE:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int year, month, day;
	if(3 == sscanf(instring, "$GPZDA,%*[^','],%d,%d,%d", &day, &month, &year)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%04d-%02d-%02d", year, month, day);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_DAY:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_MONTH:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_UTC_YEAR:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_LOCALZONE_HOURS:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPZDA_LOCALZONE_MINUTES:
      instring = strstr((char*)data, "$GPZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GPZDA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPVTG_TRUE_TRACK:
      instring = strstr((char*)data, "$GPVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPVTG,%lf,T", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPVTG_MAGNETIC_TRACK:
      instring = strstr((char*)data, "$GPVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPVTG,%*[^','],%*[^','],%lf,M", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPVTG_SPEED_KNOTS:
      instring = strstr((char*)data, "$GPVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPVTG,%*[^','],%*[^','],%*[^','],%*[^','],%lf,N", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GPVTG_SPEED_KPH:
      instring = strstr((char*)data, "$GPVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GPVTG,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf,K", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;


      //--------------------------------- START of section to parse $GNGGA, $GNRMB, $GNRMC, $GNVTG, $GNZDA, --------------------------------- 

      //---------------------------------
    case CPD_GNRMC_UTC_TIME:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int hour, minute, second, fraction;
	if(4 == sscanf(instring, "$GNRMC,%2d%2d%2d.%2d", &hour, &minute, &second, &fraction)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d:%02d:%02d.%02d", hour, minute, second, fraction);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_HOUR:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_MINUTE:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_SECOND:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%*2d%*2d%2d.", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_FRACTION:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%*2d%*2d%*2d.%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_RCVR_STATUS:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	char rcvrStatusString[16];
	if(1 == sscanf(instring, "$GNRMC,%*[^','],%15s", rcvrStatusString)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", rcvrStatusString);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_LAT_DECDEG:
    case CPD_GNRMC_LAT_DECMIN:
    case CPD_GNRMC_LAT_DECSEC:
    case CPD_GNRMC_LAT_NOAA:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int latDegreesInt;
	int latMinutesInt;
	double latDegreesDbl;
	double latMinutesDbl;
	double latSecondsDbl;
	char latHemiChar;
	if(3 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%2d%lf,%c", &latDegreesInt, &latMinutesDbl, &latHemiChar)) {
	  latDegreesDbl = (double) latDegreesInt + ( latMinutesDbl / 60.0 );
	  latMinutesInt = (int) latMinutesDbl;
	  latSecondsDbl = 60.0 * (latMinutesDbl - (double) latMinutesInt);

	  if( ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    latDegreesDbl = -1.0 * latDegreesDbl;
	    latDegreesInt =   -1 * latDegreesInt;
	  }
	  if( ('N' == latHemiChar) || ('N' == latHemiChar) || ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    //--debug-csv-log--printf("GNRMC_LAT_DECDEG ----  parseAbstractIndex = %d  ----  latDegreesInt = %d  ---  latMinutesDbl = %.4lf ---  latHemiChar = %c = %d   ---  latDegreesDbl = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, latDegreesInt, latMinutesDbl, latHemiChar, latHemiChar, latDegreesDbl);
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GNRMC_LAT_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", latDegreesDbl);
	      break;
	    case CPD_GNRMC_LAT_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", latDegreesInt, latMinutesDbl);
	      break;
	    case CPD_GNRMC_LAT_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", latDegreesInt, latMinutesInt, latSecondsDbl);
	      break;
	    case CPD_GNRMC_LAT_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d%06.3lf%c", abs(latDegreesInt), latMinutesDbl, latHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid latHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GNRMC_LON_DECDEG:
    case CPD_GNRMC_LON_DECMIN:
    case CPD_GNRMC_LON_DECSEC:
    case CPD_GNRMC_LON_NOAA:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int lonDegreesInt;
	int lonMinutesInt;
	double lonDegreesDbl;
	double lonMinutesDbl;
	double lonSecondsDbl;
	char lonHemiChar;
	if(3 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%3d%lf,%c", &lonDegreesInt, &lonMinutesDbl, &lonHemiChar)) {
	  lonDegreesDbl = (double) lonDegreesInt + ( lonMinutesDbl / 60.0 );
	  lonMinutesInt = (int) lonMinutesDbl;
	  lonSecondsDbl = 60.0 * (lonMinutesDbl - (double) lonMinutesInt);
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) ) {
	    lonDegreesDbl = -1.0 * lonDegreesDbl;
	    lonDegreesInt =   -1 * lonDegreesInt;
	  }
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) || ('E' == lonHemiChar) || ('e' == lonHemiChar) ) {
	    //--debug-csv-log--printf("GNRMC_LON_DECDEG ----  parseAbstractIndex = %d  ----  lonDegreesInt = %d  ---  lonMinutesDbl = %.4lf ---  lonHemiChar = %c = %d   ---  lonDegreesDbl = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, lonDegreesInt, lonMinutesDbl, lonHemiChar, lonHemiChar, lonDegreesDbl);
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GNRMC_LON_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", lonDegreesDbl);
	      break;
	    case CPD_GNRMC_LON_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", lonDegreesInt, lonMinutesDbl);
	      break;
	    case CPD_GNRMC_LON_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", lonDegreesInt, lonMinutesInt, lonSecondsDbl);
	      break;
	    case CPD_GNRMC_LON_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%03d%06.3lf%c", abs(lonDegreesInt), lonMinutesDbl, lonHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid lonHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GNRMC_SOG:
      //--debug-csv-log--printf("CPD_PARSE -  GNRMC_SOG case\n");
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	double sogValue;
	if(1 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &sogValue)) {
	  //--debug-csv-log--printf("GNRMC_SOG ----  parseAbstractIndex = %d  ----  sogValue = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, sogValue);
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", sogValue);
	}
	else {
	  //--debug-csv-log--printf("GNRMC_SOG ----  sscanf fails!! \n");
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_COG:
    case CPD_GNRMC_COG_SAMOS:
      //--debug-csv-log--printf("CPD_PARSE -  GNRMC_COG case\n");
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	double cogValue;
	if(1 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &cogValue)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	  case CPD_GNRMC_COG:
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", cogValue);
	    break;
	  case CPD_GNRMC_COG_SAMOS:
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%05.1lf", cogValue);
	    break;
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_DATE:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int year, month, day;
	if(3 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%2d%2d%2d", &day, &month, &year)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  // output format changed  t. thiel 2014-11-06
	  //sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d%02d%02d", day, month, year);
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%04d-%02d-%02d", year+2000, month, day);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_YEAR:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*2d%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_MONTH:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*2d%2d%*2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_UTC_DAY:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%2d%*2d%*2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMC_MAGVAR:
      instring = strstr((char*)data, "$GNRMC");
      if(NULL != instring) {
	double variation;
	char eastWest;
	if(2 == sscanf(instring, "$GNRMC,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf,%c*", &variation, &eastWest)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf %c", variation, eastWest);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_STATUS:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	char value;
	if(1 == sscanf(instring, "$GNRMB,%c", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_CTE_MAG:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_CTE_DIR:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	char value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%c", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_ORIGIN_ID:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_DEST_ID:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_DEST_LAT:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	int latDegreesInt;
	double latMinutesDbl;
	double tmpval;
	char latHemiChar;
	if(3 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%2d%lf,%c", &latDegreesInt, &latMinutesDbl, &latHemiChar)) {
	  tmpval = (double) latDegreesInt + ( latMinutesDbl / 60.0 );
	  if( ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    tmpval = -1.0 * tmpval;
	  }
	  if( ('N' == latHemiChar) || ('N' == latHemiChar) || ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_DEST_LON:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	int lonDegrees;
	double lonMinutes;
	double tmpval;
	char lonHemiChar;
	if(3 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%3d%lf,%c", &lonDegrees, &lonMinutes, &lonHemiChar)) {
	  tmpval = (double) lonDegrees + ( lonMinutes / 60.0 );
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) ) {
	    tmpval = -1.0 * tmpval;
	  }
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) || ('E' == lonHemiChar) || ('e' == lonHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_RANGE:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_BEARING:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_DEST_SPEED:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNRMB_ARRIVAL_ALARM:
      instring = strstr((char*)data, "$GNRMB");
      if(NULL != instring) {
	char value;
	if(1 == sscanf(instring, "$GNRMB,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%c*", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_UTC_HOUR:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_UTC_MINUTE:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_UTC_SECOND:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%*2d%*2d%2d.", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_UTC_FRACTION:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%*2d%*2d%*2d.%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_UTC_TIME:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int hour, minute, second, fraction;
	if(4 == sscanf(instring, "$GNGGA,%2d%2d%2d.%2d", &hour, &minute, &second, &fraction)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d:%02d:%02d.%02d", hour, minute, second, fraction);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_LAT_DECDEG:
    case CPD_GNGGA_LAT_DECMIN:
    case CPD_GNGGA_LAT_DECSEC:
    case CPD_GNGGA_LAT_NOAA:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int latDegreesInt;
	int latMinutesInt;
	double latDegreesDbl;
	double latMinutesDbl;
	double latSecondsDbl;
	char latHemiChar;
	if(3 == sscanf(instring, "$GNGGA,%*[^','],%2d%lf,%c", &latDegreesInt, &latMinutesDbl, &latHemiChar)) {
	  latDegreesDbl = (double) latDegreesInt + ( latMinutesDbl / 60.0 );
	  latMinutesInt = (int) latMinutesDbl;
	  latSecondsDbl = 60.0 * (latMinutesDbl - (double) latMinutesInt);
	  if( ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    latDegreesDbl = -1.0 * latDegreesDbl;
	  }
	  if( ('N' == latHemiChar) || ('N' == latHemiChar) || ('S' == latHemiChar) || ('s' == latHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GNGGA_LAT_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", latDegreesDbl);
	      break;
	    case CPD_GNGGA_LAT_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", latDegreesInt, latMinutesDbl);
	      break;
	    case CPD_GNGGA_LAT_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", latDegreesInt, latMinutesInt, latSecondsDbl);
	      break;
	    case CPD_GNGGA_LAT_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d%06.3lf%c", abs(latDegreesInt), latMinutesDbl, latHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid latHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GNGGA_LON_DECDEG:
    case CPD_GNGGA_LON_DECMIN:
    case CPD_GNGGA_LON_DECSEC:
    case CPD_GNGGA_LON_NOAA:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int lonDegreesInt;
	int lonMinutesInt;
	double lonDegreesDbl;
	double lonMinutesDbl;
	double lonSecondsDbl;
	char lonHemiChar;
	if(3 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%3d%lf,%c", &lonDegreesInt, &lonMinutesDbl, &lonHemiChar)) {
	  lonDegreesDbl = (double) lonDegreesInt + ( lonMinutesDbl / 60.0 );
	  lonMinutesInt = (int) lonMinutesDbl;
	  lonSecondsDbl = 60.0 * (lonMinutesDbl - (double) lonMinutesInt);
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) ) {
	    lonDegreesDbl = -1.0 * lonDegreesDbl;
	    lonDegreesInt =   -1 * lonDegreesInt;
	  }
	  if( ('W' == lonHemiChar) || ('w' == lonHemiChar) || ('E' == lonHemiChar) || ('e' == lonHemiChar) ) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	    case CPD_GNGGA_LON_DECDEG:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", lonDegreesDbl);
	      break;
	    case CPD_GNGGA_LON_DECMIN:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %.3lf", lonDegreesInt, lonMinutesDbl);
	      break;
	    case CPD_GNGGA_LON_DECSEC:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d %d %.3lf", lonDegreesInt, lonMinutesInt, lonSecondsDbl);
	      break;
	    case CPD_GNGGA_LON_NOAA:
	      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%03d%06.3lf%c", abs(lonDegreesInt), lonMinutesDbl, lonHemiChar);
	      break;
	    }  // end of switch()
	  }  // end of if(valid lonHemiChar)
	}  // end of if(3 == sscanf())
      }  // end of if(NULL != instring)
      break;

      //---------------------------------
    case CPD_GNGGA_QUALITY:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_NUM_SAT:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_DILUTION:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_ALTITUDE:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_GEOID_HEIGHT:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],M,%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_FIX_AGE:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],M,,%*[^','],M,%lf", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNGGA_STATION_ID:
      instring = strstr((char*)data, "$GNGGA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNGGA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],M,,%*[^','],M,%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_HOUR:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_MINUTE:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*2d%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_SECOND:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*2d%*2d%2d.", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_FRACTION:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*2d%*2d%*2d.%2d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_TIME:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int hour, minute, second, fraction;
	if(4 == sscanf(instring, "$GNZDA,%2d%2d%2d.%2d", &hour, &minute, &second, &fraction)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%02d:%02d:%02d.%02d", hour, minute, second, fraction);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_DATE:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int year, month, day;
	if(3 == sscanf(instring, "$GNZDA,%*[^','],%d,%d,%d", &day, &month, &year)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%04d-%02d-%02d", year, month, day);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_DAY:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_MONTH:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_UTC_YEAR:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_LOCALZONE_HOURS:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNZDA_LOCALZONE_MINUTES:
      instring = strstr((char*)data, "$GNZDA");
      if(NULL != instring) {
	int value;
	if(1 == sscanf(instring, "$GNZDA,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNVTG_TRUE_TRACK:
      instring = strstr((char*)data, "$GNVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNVTG,%lf,T", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNVTG_MAGNETIC_TRACK:
      instring = strstr((char*)data, "$GNVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNVTG,%*[^','],%*[^','],%lf,M", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNVTG_SPEED_KNOTS:
      instring = strstr((char*)data, "$GNVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNVTG,%*[^','],%*[^','],%*[^','],%*[^','],%lf,N", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //---------------------------------
    case CPD_GNVTG_SPEED_KPH:
      instring = strstr((char*)data, "$GNVTG");
      if(NULL != instring) {
	double value;
	if(1 == sscanf(instring, "$GNVTG,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf,K", &value)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%lf", value);
	}
      }
      break;

      //--------------------------------- END of section to parse $GNGGA, $GNRMB, $GNRMC, $GNVTG, $GNZDA, --------------------------------- 


      //---------------------------------
    case CPD_HEHDT_HDG:
    case CPD_HEHDT_HDG_SAMOS:
      instring = strstr((char*)data, "GYRO");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "GYRO $HEHDT,%lf", &tmpval)) {
	  //--debug-csv-log--printf("GYRO_HDG ----  parseAbstractIndex = %d  ----  tmpval = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, tmpval);
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].parseDataType) {
	  case CPD_HEHDT_HDG:
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	    break;
	  case CPD_HEHDT_HDG_SAMOS:
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%05.1lf", tmpval);
	    break;
	  }
	}
      }
      break;

      //---------------------------------
    case CPD_GPHDT_HDG:
      instring = strstr((char*)data, "GYRO");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "GYRO $GPHDT,%lf", &tmpval)) {
	  //--debug-csv-log--printf("GYRO_HDG ----  parseAbstractIndex = %d  ----  tmpval = %.4lf\n", theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex, tmpval);
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SBE45_TEMPERATURE:
      instring = strstr((char*)data, "SBE45");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "SBE45 %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SBE45_CONDUCTIVITY:
      instring = strstr((char*)data, "SBE45");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "SBE45 %*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.5lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SBE45_SALINITY:
      instring = strstr((char*)data, "SBE45");
      if(NULL != instring) {
	double tmpval;
	//if(1 == sscanf(instring, "SBE45 %*f %*f %lf", &tmpval)) {
	if(1 == sscanf(instring, "SBE45 %*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SBE45_SOUNDVELOCITY:
      instring = strstr((char*)data, "SBE45");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "SBE45 %*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SBE48_TEMPERATURE:
      instring = strstr((char*)data, "SBE48");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "SBE48 # %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLR_MILLIVOLTS:
      instring = strstr((char*)data, "FLR");
      if(NULL != instring) {
	double tmpval;
	//if(1 == sscanf(instring, "FLR %*[^'*']*%lf", &tmpval)) {
	if(1 == sscanf(instring, "FLR *%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VAI_ADDRESS:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "WXT%*c %cR0", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VAI_WIND_DIRECTION_AVG:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],Dm=%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VAI_WIND_SPEED_MIN:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],Sn=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_WIND_SPEED_AVG:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],Sm=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_WIND_SPEED_MAX:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],%*[^','],Sx=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_TEMPERATURE:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],Ta=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_HUMIDITY:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],Ua=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_PRESSURE:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],Pa=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_RAIN_ACCUMULATION:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],Rc=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break; 

      //---------------------------------
    case CPD_VAI_RAIN_INTENSITY:
      instring = strstr((char*)data, "WXT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "WXT%*c %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],Ri=%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VDVBW_WATER_SPEED_AHEAD:
      instring = strstr((char*)data, "$VDVBW");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$VDVBW,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VDVBW_WATER_SPEED_STBD:
      instring = strstr((char*)data, "$VDVBW");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$VDVBW,%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VDVBW_WATER_SPEED_STATUS:
      instring = strstr((char*)data, "$VDVBW");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "$VDVBW,%*[^','],%*[^','],%c", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VDVBW_GROUND_SPEED_AHEAD:
      instring = strstr((char*)data, "$VDVBW");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$VDVBW,%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VDVBW_GROUND_SPEED_STBD:
      instring = strstr((char*)data, "$VDVBW");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$VDVBW,%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_VDVBW_GROUND_SPEED_STATUS:
      instring = strstr((char*)data, "$VDVBW");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "$VDVBW,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%c", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_TIME:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_HEADING:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ROLL:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_PITCH:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_HEAVE:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ROLL_ACCURACY:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_PITCH_ACCURACY:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_HEADING_ACCURACY:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_GPS_FLAG:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%c", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_IMU_FLAG:
      instring = strstr((char*)data, "$PASHR");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "$PASHR,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%c", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_SECONDS:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_HEADING:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_PITCH:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_ROLL:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_MRMS:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_BRMS:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PASHR_ATT_REAQUISITION:
      instring = strstr((char*)data, "$PASHR,ATT");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "PASHR,ATT,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_RAW_STRING:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", instring);
      }
      break;

      //---------------------------------
    case CPD_TSS1_RAW_ACCEL_HOR:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	char tmpstr[16];
	if(1 == sscanf(instring, ":%2s", tmpstr)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", tmpstr);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_RAW_ACCEL_VER:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	char tmpstr[16];
	if(1 == sscanf(instring, ":%*2s%4s", tmpstr)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", tmpstr);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_RAW_HEAVE:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	char tmpstr[16];
	if(1 == sscanf(instring, ":%*2s%*4s %5s", tmpstr)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", tmpstr);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_STATUS:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	char tmpchar;
	if(1 == sscanf(instring, ":%*2s%*4s %*4s%c", &tmpchar)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpchar);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_RAW_ROLL:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	char tmpstr[16];
	if(1 == sscanf(instring, ":%*2s%*4s %*5s%*c%5s", tmpstr)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", tmpstr);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_RAW_PITCH:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	char tmpstr[16];
	if(1 == sscanf(instring, ":%*2s%*4s %*5s%*c%*5s %5s", tmpstr)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%s", tmpstr);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_ACCEL_HOR:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	unsigned int tmpval;
	if(1 == sscanf(instring, ":%2X", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.6lf", 0.0383 * (double) tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_ACCEL_VER:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	unsigned int tmpval;
	if(1 == sscanf(instring, ":%*2s%4X", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.6lf", 0.000625 * (double) tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_HEAVE:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, ":%*2s%*4s %5d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", 0.01 * (double) tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_ROLL:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, ":%*2s%*4s %*5s%*c%5d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", 0.01 * (double) tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_TSS1_PITCH:
      instring = strstr((char*)data, ":");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, ":%*2s%*4s %*5s%*c%*5s %5d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", 0.01 * (double) tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PKEL_DEPTH_1:
      instring = strstr((char*)data, "$PKEL99");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PKEL99,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PKEL_STATUS_1:
      instring = strstr((char*)data, "$PKEL99");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "$PKEL99,%*f,%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PKEL_DUCER_DEPTH_1:
      instring = strstr((char*)data, "$PKEL99");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PKEL99,%*f,%*d,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PKEL_DEPTH_2:
      instring = strstr((char*)data, "$PKEL99");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PKEL99,%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
	else if(1 == sscanf(instring, "$PKEL99,,,,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PKEL_STATUS_2:
      instring = strstr((char*)data, "$PKEL99");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "$PKEL99,%*f,%*d,%*f,%*f,%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
	else if(1 == sscanf(instring, "$PKEL99,,,,%*[^','],%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PKEL_DUCER_DEPTH_2:
      instring = strstr((char*)data, "$PKEL99");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PKEL99,%*f,%*d,%*f,%*f,%*d,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
	else if(1 == sscanf(instring, "$PKEL99,,,,%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHINS_HEADING:
      instring = strstr((char*)data, "$HEHDT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$HEHDT,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHTRO_PITCH:
      instring = strstr((char*)data, "$PHTRO");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHTRO,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHTRO_PITCH_FRAME:
      instring = strstr((char*)data, "$PHTRO");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "$PHTRO,%*f,%c", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHTRO_ROLL:
      instring = strstr((char*)data, "$PHTRO");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHTRO,%*f,%*c,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHTRO_ROLL_FRAME:
      instring = strstr((char*)data, "$PHTRO");
      if(NULL != instring) {
	char tmpval;
	if(1 == sscanf(instring, "$PHTRO,%*f,%*c,%*f,%c", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%c", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHLIN_SURGE_LIN:
      instring = strstr((char*)data, "$PHLIN");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHLIN,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHLIN_SWAY_LIN:
      instring = strstr((char*)data, "$PHLIN");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHLIN,%*f,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHLIN_HEAVE_LIN:
      instring = strstr((char*)data, "$PHLIN");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHLIN,%*f,%*f,%lf*", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHSPD_SURGE_SPD:
      instring = strstr((char*)data, "$PHSPD");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHSPD,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHSPD_SWAY_SPD:
      instring = strstr((char*)data, "$PHSPD");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHSPD,%*f,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_PHSPD_HEAVE_SPD:
      instring = strstr((char*)data, "$PHSPD");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$PHSPD,%*f,%*f,%lf*", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.4lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_KIDPT_DEPTH:
      instring = strstr((char*)data, "KIDPT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "KIDPT,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_KIDPT_BEAMS_GOOD:
      instring = strstr((char*)data, "KIDPT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "KIDPT,%*f,%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_KIDPT_FREQUENCY:
      instring = strstr((char*)data, "KIDPT");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "KIDPT,%*f,%*f,%lf*", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SWR_RADIATION:
      instring = strstr((char*)data, "SWR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "SWR %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_LWR_DOME_TEMPERATURE:
      instring = strstr((char*)data, "LWR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "LWR %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_LWR_BODY_TEMPERATURE:
      instring = strstr((char*)data, "LWR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "LWR %*f %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_LWR_THERMOPILE_VOLTAGE:
      instring = strstr((char*)data, "LWR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "LWR %*f %*f %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_LWR_RADIATION:
      instring = strstr((char*)data, "LWR");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "LWR %*f %*f %*f %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_XCALC_SSV:
      instring = strstr((char*)data, "calcSSV");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "calcSSV %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_XCALC_TRUEWIND_SPEED:
      instring = strstr((char*)data, "NOT_IMPLEMENTED");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_XCALC_TRUEWIND_DIRECTION:
      instring = strstr((char*)data, "NOT_IMPLEMENTED");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_RAW_COUNTS:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_CUMULATIVE_COUNTS:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %*[^','],%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_FLOW_LAST_SECOND:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_FLOW_LAST_MINUTE:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_FLOW_LAST_HOUR:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_FLOW_LAST_24HOURS:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_FLOW_ELAPSED_SECONDS:
      instring = strstr((char*)data, "$FLOWRATE, ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "$FLOWRATE, %*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.1lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_AML_SSV_TEMPERATURE:
      instring = strstr((char*)data, "AML_SSV ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "AML_SSV %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_AML_SSV_SSV:
      instring = strstr((char*)data, "AML_SSV ");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "AML_SSV %*[^' '] %lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_SERIAL_NUMBER:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	int tmpval;
	if(1 == sscanf(instring, "$WIR%d", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_NUMBER:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_PIR:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_LW:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_TCASE:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_TDOME:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_SW:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_T_AVR:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_RAD_BATT:
      instring = strstr((char*)data, "$WIR");
      if(NULL != instring) {
	double tmpval;
	//--- skip date and time
	if(1 == sscanf(instring, "$WIR%*d,%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	}
      }
      break;

      //---------------------------------
    case CPD_SSSG_GENERIC:
      if(theLoggingList[myLoggerNumber].csvParse[parseIndex].genericUseHeaderString) {
        instring = strstr((char*)data, theLoggingList[myLoggerNumber].csvParse[parseIndex].genericHeaderString);
        if(NULL != instring) {
	  double tmpDblVal;
	  int tmpIntVal;
	  int tmpIndex;
	  char tmpString[16];
	  char formatString[2048];

	  if(-1 == theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterSkipCount) {
	    //---- build sscanf format string
	    tmpIndex = theLoggingList[myLoggerNumber].csvParse[parseIndex].genericIndex;
	    sprintf(tmpString, "%%*[^'%c']%c", theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterChar, theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterChar);
	    sprintf(formatString, "%s", theLoggingList[myLoggerNumber].csvParse[parseIndex].genericHeaderString);

	    while(tmpIndex-- > 0) {
	      strcat(formatString, tmpString);
	    }
	    if(theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDataType) {
	      strcat(formatString, "%lf");
	      //printf("\n\n------------------------------\n");
	      //printf("formatString is <%s>\n", formatString);
	      //printf("instring is <%s>\n", instring);
	      if(1 == sscanf(instring, formatString, &tmpDblVal)) {
		sprintf(tmpString, "%%.%dlf", theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDataPrecision);
		csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
		sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, tmpString, tmpDblVal);
	      }
	    }  // end of parsing for type double values
	    else {
	      strcat(formatString, "%d");
	      if(1 == sscanf(instring, formatString, &tmpIntVal)) {
		csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
		sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpIntVal);
	      }
	    }  // end of parsing for type integer values
	  }  // end of parsing for NON_SKIP_COUNT parsing
	  else {
	    char *ptr;
	    int skipCount;

	    // skip ahead past N delimiters
	    //printf("\n===============================\n");
	    //printf("parseIndex is %d.  skipCount is %d.\n\n", parseIndex, theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterSkipCount);
	    ptr = instring;
	    ptr += strlen(theLoggingList[myLoggerNumber].csvParse[parseIndex].genericHeaderString);
	    skipCount=0;
	    while( (*ptr != 0) && (skipCount < theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterSkipCount) ) {
	      if(*ptr++ == theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterChar) { ++skipCount; }
	      //printf("skipCount=%d.  string is<%s>\n", skipCount, ptr);
	    }

	    // if success then try to parse remaining string
	    if(skipCount == theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDelimiterSkipCount) {
	      if(theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDataType) {
		if(1 == sscanf(ptr, "%lf", &tmpDblVal)) {
		  sprintf(tmpString, "%%.%dlf", theLoggingList[myLoggerNumber].csvParse[parseIndex].genericDataPrecision);
		  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
		  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, tmpString, tmpDblVal);
		}
	      }  // end of parsing for type double values
	      else {
		if(1 == sscanf(ptr, "%d", &tmpIntVal)) {
		  csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
		  sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%d", tmpIntVal);
		}
	      }  // end of parsing for type integer values
	    }  // end of if(skipCount is correct value)
	  }  // end of parsing for SKIP_COUNT parsing
	}  // end of if(NULL != instring)
      }  // end of if - if(genericUseHeaderString)
      else {
      }  // end of else - if(genericUseHeaderString)
      break;

#if 0
      //  %*[^','],
      //---------------------------------
    case CPD_:
      instring = strstr((char*)data, "XXX");
      if(NULL != instring) {
	double tmpval;
	if(1 == sscanf(instring, "SBE45 %*[^','],%*[^','],%*[^','],%lf", &tmpval)) {
	  if(1 == sscanf(instring, "", &tmpval)) {
	    csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
	    sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "%.3lf", tmpval);
	  }
	}
      }
      break;
#endif

    default:
      csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].lastRxTime = rov_get_time();
      sprintf(csvAbstractData[theLoggingList[myLoggerNumber].csvParse[parseIndex].parseAbstractIndex].dataString, "BAD_INI_FILE");
      printf("CPD_PARSE - default case\n");
      break;
    };  // end of switch(theLoggingList[myLoggerNumber].csvParse[parseIndex].csvParseDatatype)
  }  // end of loop over parseIndex
  //-------------------  cut --------------------------
  return 0;
}
