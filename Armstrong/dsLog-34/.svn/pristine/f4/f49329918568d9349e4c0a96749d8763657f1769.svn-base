/* --------------------------------------------------------------------
sio log thread for alvin system

Modification History:
DATE        AUTHOR COMMENT
5 Nov 2009  jch    created and written, derived from nio_thread.cpp
2013-05-30   tt    major changes to support servicing serial and udp sources with a single thread - eliminating nioLogThread.cpp
                     - added udpLogRxThread() function
                     - changes to code to use ini file values
                     - changes to code where port is opened

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
#include "dsLog.h"
#include "sioLogThread.h"
// #define DEBUG_SIO

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;



/* ----------------------------------------------------------------------

   Function to process incoming UDP characters
   
   Modification History:
   DATE         AUTHOR  COMMENT
   19-JUL-2000  LLW      Created and written.
   2012-12-18   tt       - I believe the entry above in the mod history refers to when the sio_thread was created and written
  2013-05-01   tt       - changes to support new status messages (input specific and prompt specific)

   
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

/* ----------------------------------------------------------------------

   Function to process incoming characters
   
   Modification History:
   DATE         AUTHOR  COMMENT
   19-JUL-2000  LLW      Created and written.
   2012-12-18   tt       - I believe the entry above in the mod history refers to when the sio_thread was created and written
                         - this code was written by jon howland (based on sio_thread) in 2011 or more likely 2012
   2012-12-18   tt       - major mods to use a single sequence of prompts rather than multiple sequences of prompts
                                - change myPromptSequences from an array of prompt_sequence_t to just a single prompt_sequence_t
                                - remove for loop around initialization of myPromptSequences
                                - mySequenceMessage, myPromptMessage, and whichPrompt no longer arrays
                                - big changes in default case of switch(hdr.type) - deal with non-array type data
                                - promptTimer was expiring immediately when reenabled on start of sequence.  
                                    - So I do not send any data on the sequence timer and instead just zero whichPrompt and reenable the prompt timer.  
                                    - The promptTimer fires immediately and the first prompt is sent.
   2012-12-20   tt       - mods to have prompt specific destinations
   2012-12-21   tt       - mods to support single prompt sequence rather than multiple prompt sequences
                                - theLoggingList[myLoggerNumber].promptSequences[0].***  -> theLoggingList[myLoggerNumber].promptSequence.***
                                - remove nOfPromptSequences from logging_descriptor_t
   2012-12-22   tt       - mods to have prompt specific termination characters
   2012-12-22   tt       - changes to support prompt specific data types and source names
  2012-12-22   tt       - changes to support prompt specific syncopation
  2012-12-26   tt       - changes to support prompt specific reply window - this is not 100% working yet

  2013-04-07   tt       - changes to indenting - makes comparison to earlier versions difficult.
  2013-04-07   tt       - changes to support disabling specific prompts so they don't need to be renumbered
  2013-04-20   tt       - changes to support free running prompt sequences
  2013-04-20   tt       - changes to provide safeguards against user disabling ALL prompts

  2013-05-01   tt       - changes to support new status messages (input specific and prompt specific)

   
---------------------------------------------------------------------- */

static void * sioLogRxThread (void *arg)
{
#ifdef DEBUG_SIO
  printf ("initiating rx subthread\n");
#endif
  sio_thread_t *sio = (sio_thread_t *) arg;
  int fid;
  ssize_t num;
  char new_chars[256] = { 0 };
  
  sio = (sio_thread_t *) arg;
  fid = sio->sio_port_fid;
  
  if (sio->sio_thread_num == CMD_0_SIO_THREAD) {
    printf ("%s: Reminder to clean up single character read()\n", global_thread_table[sio->sio_thread_num].thread_name);
  }
  
  while (1) {
    // read a character
#ifdef DEBUG_SIO
    printf ("%s (thread %d) device %s about to read\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name);
#endif
    
    num = read(fid, new_chars, 1);
    
    if (num > 0) {
#ifdef DEBUG_SIO
      printf ("%s (thread %d) device %s fid %d got %d characters, " "ascii %c, decimal %d, hex 0x%02X, errno=%d (%s) from %s\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, num, new_chars[0], new_chars[0], new_chars[0], errno, strerror (errno), sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
      fflush (stdout);
#endif
    }
    else {
#ifdef DEBUG_SIO
      printf ("%s ERROR (thread %d) device %s fid %d got %d characters, " "ascii %c, decimal %d, hex 0x%02X, errno=%d (%s) from %s\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, num, new_chars[0], new_chars[0], new_chars[0], errno, strerror (errno), sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
      fflush (stdout);
#endif
      abort ();
    }
    
    // lock the data structure mutex so that we chan diddle with the data
    if(startup.running) {
      pthread_mutex_lock (&sio->mutex);
      
      // ----------------------------------------------------------------------
      // if we are in WSX mode, handle this specially
      // ----------------------------------------------------------------------
      if (sio->wsx_mode) {
	// check to see if the character is the WSX term character -
	// if so, then terminate WSX mode
	if (new_chars[0] == sio->wsx_mode_term_char) {
	  // send the character to the WSX port
	  printf ("%s (thread %d) device %s fid %d terminating WSX mode to %s on character, " "ascii %c, decimal %d, hex 0x%02X, - sending WSY to address %d\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to, new_chars[0], new_chars[0], new_chars[0], sio->wsx_mode_mux_address);
          
	  fflush (stdout);
          
	  // send a message to the other WSX port to kill WSX mode
	  msg_send( sio->wsx_mode_mux_address, sio->sio_thread_num, WSY, 0, NULL);
          
	  // terminate wsx mode
	  sio->wsx_mode = 0;
	}
	else {
	  // send the character to the WSX port
#ifdef DEBUG_SIO
	  printf ("%s (thread %d) device %s fid %d sending wsx wss for %d characters, " "ascii %c, decimal %d, hex 0x%02x, to address %d\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, num, new_chars[0], new_chars[0], new_chars[0], sio->wsx_mode_mux_address);
	  fflush (stdout);
#endif
	  msg_send( sio->wsx_mode_mux_address, sio->sio_thread_num, WAS, num, new_chars);
	}
        
	// need to goto bottom of loop to unlock the mutex
	goto bottom_of_loop;
        
      }
      
      // ----------------------------------------------------------------------
      // echo the character(s) if needed
      // ----------------------------------------------------------------------
      if (sio->my_sio_port_table_entry.echo) {
	
	// prepend a linefeed to echoed carriage returns
	if (new_chars[0] == '\r')
	  write (fid, "\n", 1);
	// echo the char
	write (fid, new_chars, num);
	// keep stats
	sio->total_tx_chars++;
      }
      
      // add the character to the serial I/O array
      sio->inchars[sio->inchar_index] = new_chars[0];
      
      // keep rx stats
      sio->total_rx_chars += num;
      
      // PROCESS THE NEW CHARACTER(S)!
      // a complete string is recieved iff ant of the following is
      // true 1. Auto-muxing is enabled and the termination character
      // is recieved.  2. Auto-muxing is enabled and the auto-muxing
      // termination character is "-1", then ANY recieved character
      // constitutes a complete string.  3.  Auto-muxing is enabled
      // and buffer is full, then send the string.
      
      if ((sio->my_sio_port_table_entry.auto_mux_address >= 0) && ((!sio->my_sio_port_table_entry.auto_mux_enabled) || (sio->my_sio_port_table_entry.auto_mux_term_char == new_chars[0]) || (sio->inchar_index >= MSG_DATA_LEN_MAX))) {
	// got a complete string, so
	// send a SAS status message to the designated process
#ifdef DEBUG_SIO
	printf (" about to enter destination loop for %s (thread %d) device %s\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name);
#endif
	for (int i = 0; i < sio->my_sio_port_table_entry.n_of_destinations; i++) {
#ifdef DEBUG_SIO
	  printf ("%s (thread %d) device %s fid %d auto-muxing %d char string to thread %d.\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, sio->inchar_index + 1, sio->my_sio_port_table_entry.auto_mux_address[i]);
#endif
	  msg_send((msg_addr_t) sio->sio_thread_num, (msg_addr_t) sio->my_sio_port_table_entry.auto_mux_address[i], SAS, sio->inchar_index + 1, sio->inchars);
	}
        
	// zero the character array index
	sio->inchar_index = 0;
#ifdef DEBUG_SIO
	printf ("%s (thread %d) device %s  getting ready to look for a new character\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name);
#endif
	
      }
      else {
	// do not yet have a complete string, so
	// increment the buffer pointer if the buffer is not full
	// if the buffer is full and auto-muxing is not enabled characters are lost
	sio->inchar_index = (sio->inchar_index + 1) % MSG_DATA_LEN_MAX;
      }
      
      // unlock the data structure mutex so the other thread can diddle with it
    bottom_of_loop:
      pthread_mutex_unlock (&sio->mutex);
      
    }  //if rov.running
    else {  // not running
      return(NULL);
    }
  }				// while
  
  return NULL;
}


/* ----------------------------------------------------------------------*/
//    sioLogThread
//   Modification History:
//   DATE         AUTHOR  COMMENT
//  5 nov 2009    jch     created and writen, derived from sio_thread.cpp
//----------------------------------------------------------------------*/
void *
sioLogThread (void *thread_num)
{
  sio_thread_t sio = { PTHREAD_MUTEX_INITIALIZER, -1 };
   
  msg_hdr_t hdr = { 0 };
  unsigned char data[MSG_DATA_LEN_MAX] = { 0 };
  prompt_sequence_t    myPromptSequences;
  
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
  
  int have_a_port = false;
  
  int syncopationCount[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  
  //--- initialize syncopationCount array to zero
  for(int syncopationIndex=0; syncopationIndex < MAX_N_OF_PROMPTS_IN_SEQUENCE; ++syncopationIndex) {
    syncopationCount[syncopationIndex] = 0;
  }
  
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
  else {
    // settings for a serial port source
    sio.my_sio_port_table_entry.parity = theLoggingList[myLoggerNumber].theSerialPort.parity;
    sio.my_sio_port_table_entry.flow_control = theLoggingList[myLoggerNumber].theSerialPort.flow_control;
    sio.my_sio_port_table_entry.echo = theLoggingList[myLoggerNumber].theSerialPort.echo;
    sio.my_sio_port_table_entry.data_bits = theLoggingList[myLoggerNumber].theSerialPort.data_bits;
    sio.my_sio_port_table_entry.baud = theLoggingList[myLoggerNumber].theSerialPort.baud;
    sio.my_sio_port_table_entry.stop_bits = theLoggingList[myLoggerNumber].theSerialPort.stop_bits;
    sio.my_sio_port_table_entry.parity = theLoggingList[myLoggerNumber].theSerialPort.parity;
    sio.my_sio_port_table_entry.auto_mux_enabled = theLoggingList[myLoggerNumber].theSerialPort.auto_mux_enabled;
    sio.my_sio_port_table_entry.auto_mux_term_char = theLoggingList[myLoggerNumber].theSerialPort.auto_mux_term_char;
    sio.my_sio_port_table_entry.sio_com_port_name = strdup(theLoggingList[myLoggerNumber].theSerialPort.sio_com_port_name);
    sio.my_sio_port_table_entry.n_of_destinations = 1;
    sio.my_sio_port_table_entry.auto_mux_address[0] = sio.sio_thread_num;
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
  
  myPromptSequences.numberOfPrompts = theLoggingList[myLoggerNumber].promptSequence.numberOfPrompts;
  myPromptSequences.secondsBetweenSequences = theLoggingList[myLoggerNumber].promptSequence.secondsBetweenSequences;
  myPromptSequences.secondsBetweenPrompts = theLoggingList[myLoggerNumber].promptSequence.secondsBetweenPrompts;
  
  for(int promptN = 0; promptN < MAX_N_OF_PROMPTS_IN_SEQUENCE; promptN++) {
    myPromptSequences.prompts[promptN].promptCharLength =  theLoggingList[myLoggerNumber].promptSequence.prompts[promptN].promptCharLength;
    for(int charN = 0; charN < MAX_PROMPT_LENGTH; charN++) {
      myPromptSequences.prompts[promptN].promptChars[charN] = theLoggingList[myLoggerNumber].promptSequence.prompts[promptN].promptChars[charN];
    }
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
  printf("Displaying destination info for sioLogThread.  COM port name is %s\n", sio.my_sio_port_table_entry.sio_com_port_name);
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
  }
  else {
    // ----------------------------------------------------------------------
    // open the serial port
    // ----------------------------------------------------------------------
    if (sio.my_sio_port_table_entry.sio_com_port_name == NULL) {
      printf (" sio device isnt available!\n");
    }
    else if(strcmp(sio.my_sio_port_table_entry.sio_com_port_name,"/dev/null")) {
      have_a_port = true;
      sio.sio_port_fid = open_serial (&sio);
#ifdef DEBUG_SIO
      printf ("got by open port\n");
#endif
      char parity_string[32];
      switch (sio.my_sio_port_table_entry.parity)
	{
	case SIO_PARITY_NONE:
	  snprintf(parity_string,32,"NONE");
	  break;
	case SIO_PARITY_ODD:
	  snprintf(parity_string,32,"ODD");
	  break;
	case SIO_PARITY_EVEN:
	  snprintf(parity_string,32,"EVEN");
	  break;
	case SIO_PARITY_MARK:
	  snprintf(parity_string,32,"MARK");
	  break;
	case SIO_PARITY_SPACE:
	  snprintf(parity_string,32,"SPACE");
	  break;
	default:
	  snprintf(parity_string,32,"BAD PARITY CONFIGURATION!");
	  break;
	}
      
      char flow_string[32];
      switch (sio.my_sio_port_table_entry.flow_control)
	{
	case SIO_FLOW_CONTROL_NONE:
	  snprintf(flow_string,32,"NONE");
	  break;
	case SIO_FLOW_CONTROL_XONXOFF:
	  snprintf(flow_string,32,"XON/XOFF");
	  break;
	case SIO_FLOW_CONTROL_RTSCTS:
	  snprintf(flow_string,32,"RTS/CTS");
	  break;
	case SIO_FLOW_CONTROL_XONOFF_RTSCTS:
	  snprintf(flow_string,32,"XON/XOFF+RTS/CTS");
	  break;
	default:
	  snprintf(flow_string,32,"BAD FLOW CONFIGURATION!");
	  break;
	}
      
      // print newsy information
      if (sio.sio_port_fid != -1) {
	/*printf("%s: (thread %d) %s opened OK, fid %d, %d %s %d %d, flow=%s\n\r"
	  "%s: echo=%s, mux=%d destinations, eol=0x%X, %s is connected to %s  \n",
	  global_thread_table[sio.sio_thread_num].thread_name,
	  sio.sio_thread_num,
	  sio.my_sio_port_table_entry.sio_com_port_name,
	  sio.sio_port_fid,
	  sio.my_sio_port_table_entry.baud,
	  parity_string,
	  sio.my_sio_port_table_entry.data_bits,
	  sio.my_sio_port_table_entry.stop_bits,
	  flow_string,
	  global_thread_table[sio.sio_thread_num].thread_name,
	  sio.my_sio_port_table_entry.echo ? "ON" : "OFF",
	  sio.my_sio_port_table_entry.n_of_destinations,
	  //sio.my_sio_port_table_entry.auto_mux_address[0] < 0 ? "OFF" : global_thread_table[sio.my_sio_port_table_entry.auto_mux_address].thread_name,
	  sio.my_sio_port_table_entry.auto_mux_term_char,
	  sio.my_sio_port_table_entry.sio_com_port_name,
	  sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
	*/
      }
      else {
	printf ("%s ERROR (thread %d) device %s FAILED TO OPEN with fid %d connected to %s\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, sio.my_sio_port_table_entry.sio_com_port_name, sio.sio_port_fid, sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
	fflush (stdout);
	have_a_port = false;
	//abort ();
      }
      
      // launch the thread to process incoming characters from the serial port
#ifdef DEBUG_SIO
      printf ("SIO about to  launch rx thread.\n");
#endif
      if (have_a_port) {
	pthread_create (&sio.sio_rx_subthread, &DEFAULT_ROV_THREAD_ATTR, sioLogRxThread, (void *) (&sio));
	
#ifdef DEBUG_SIO
	printf ("SIO just launched rx thread.\n");
#endif
      }
    }				// end if there's actually a device
  }  // end of else for if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType)
  
  launched_timer_data_t   *promptTimer = launch_timer_new(LOGGING_STATUS_INTERVAL,-1,sio.sio_thread_num,STPR);

  // set up the prompting timers
  int startPromptTimersFlag;
  int mySequenceMessage;
  int myPromptMessage;
  int whichPrompt, activePrompt;
  
  double sequenceInterval = myPromptSequences.secondsBetweenSequences;
  mySequenceMessage = sio.sio_thread_num*10000;   
  myPromptMessage = sio.sio_thread_num*10000 + 100;
  whichPrompt = 0;
  activePrompt = 0;

  // if all prompts are disabled then don't start the prompt timers
  startPromptTimersFlag = 0;
  for(int promptIndex=0; promptIndex < myPromptSequences.numberOfPrompts; ++promptIndex) {
    if(1 == theLoggingList[myLoggerNumber].promptEnable[promptIndex]) {
      startPromptTimersFlag = 1;
    }
  }

  printf("**** startPromptTimersFlag = %d\n", startPromptTimersFlag);

  if(1 == startPromptTimersFlag) {
    if(theLoggingList[myLoggerNumber].freeRunPromptFlag == 0 ) {
      myPromptSequences.betweenSequenceTimer = launch_timer_new(sequenceInterval, -1, sio.sio_thread_num, mySequenceMessage);
      myPromptSequences.betweenPromptTimer = launch_timer_new(sequenceInterval, 0, sio.sio_thread_num, myPromptMessage);
    }
    else {
      // if in free run prompt mode we can just start the myPromptMessage timer
      // there will not be a sequenceMessageTimer
      myPromptSequences.betweenPromptTimer = launch_timer_new(sequenceInterval, 0, sio.sio_thread_num, myPromptMessage);
      launch_timer_enable(myPromptSequences.betweenPromptTimer, 0, 1, sio.sio_thread_num, myPromptMessage);
    }
  }
  
   // wakeup message
   printf ("SIO LOG THREAD: SIO (thread %d) initialized \n", sio.sio_thread_num);
   printf ("SIO LOG THREAD: SIO File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);

  // loop forever
  while (1) {
    // wait forever on the input channel
#ifdef DEBUG_SIO
    printf ("SIO calling get_message.\n");
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
#ifdef DEBUG_SIO
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
#ifdef DEBUG_SIO
	  printf ("SIO THREAD: %s: (thread %d) SIO RECIEVED SAS -" "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
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
	  break;
	}  // end of case SAS:
      case WAS:		// recieved a WAS (Write Serial String) Message
	{
	  // send the characters to the port, keep stats
	  // note that WAS messages need NOT be null terminated
	  if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType) {
	    printf("in UDP section of WAS - %d \n", hdr.length);
	    if (hdr.length > 0) {                                 
	      int bytes_sent;
	      // lock the mutex
	      pthread_mutex_lock (&sio.mutex);

	      // send the bytes
	      bytes_sent = sendto(sio.my_nio_port_table_entry.to_sock, 
				  data, 
				  hdr.length, 
				  0, 
				  (struct sockaddr *) (&(sio.my_nio_port_table_entry.ToAddr)), 
				  sizeof (sio.my_nio_port_table_entry.ToAddr));
	      
	      // keep stats
	      if (bytes_sent == hdr.length) {                                       
		sio.total_tx_chars += hdr.length;
	      }
	      else {
	      }
	      {
		fprintf (stderr, "SIO_LOG_THREAD: ERROR sending %d byte WAS to %s port %d - %d bytes sent.\n", 
			 hdr.length,
			 sio.my_nio_port_table_entry.ip_address,
			 sio.my_nio_port_table_entry.to_socket_number,
			 bytes_sent
			 );                                       
		fprintf (stderr, "SIO_LOG_THREAD: Data is: \"%*s\"\n", hdr.length, data);                                       
	      }
              
	      // unlock the mutex
	      pthread_mutex_unlock (&sio.mutex);
	    }
	  }
	  else {
	    if (!have_a_port) {
	      break;
	    }
	    if (hdr.length > 0) {
	      // lock the mutex
	      pthread_mutex_lock (&sio.mutex);
	      
	      // write the bytes
	      write (sio.sio_port_fid, data, hdr.length);
	      
	      // keep stats
	      sio.total_tx_chars += hdr.length;
	      
	      // unlock the mutex
	      pthread_mutex_unlock (&sio.mutex);
	      
	    }
	  }
	  break;
	}  // end of case WAS
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
                     
      case WSX:		// recieved a WSX (Begin Write Serial Xtra Transparent Mode)
	{
	  if (!have_a_port) {
	    break;
	  }
	  // if this port is already in WSX mode, ignore this command
	  if (sio.wsx_mode != 0)
	    break;
	  
	  // lock the mutex
	  pthread_mutex_lock (&sio.mutex);
	  
	  // set the WSX mode flag
	  sio.wsx_mode = 1;
	  
	  // program the auto-mux to copy all characters to the calling process
	  sio.wsx_mode_mux_address = hdr.from;
	  
	  // if the data is nonzero, use the first character as the WSX term character
	  if (hdr.length > 0) {
	    sio.wsx_mode_term_char = data[0];
	  }
	  else
	    sio.wsx_mode_term_char = '~';
	  
	  // unlock the mutex
	  pthread_mutex_unlock (&sio.mutex);
	  
	  
#ifdef DEBUG_SIO
	  printf ("%s (thread %d) device %s fid %d entering WSX mode " "term char is ascii %c, decimal %d, hex 0x%02X.\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, sio.my_sio_port_table_entry.sio_com_port_name, sio.sio_port_fid, sio.my_sio_port_table_entry.auto_mux_term_char, sio.my_sio_port_table_entry.auto_mux_term_char, sio.my_sio_port_table_entry.auto_mux_term_char);
	  fflush (stdout);
#endif
	  break;
	}
                     
      case WSY:		// recieved a WSY (Terminate Write Serial Xtra Transparent Mode)
	{
	  if (!have_a_port) {
	    break;
	  }			// if this port is not in WSX mode, ignore this command
	  if (sio.wsx_mode == 0)
	    break;
	  
	  // lock the mutex
	  pthread_mutex_lock (&sio.mutex);
	  
	  // set the WSX mode flag
	  sio.wsx_mode = 0;
	  
	  // newsy message
	  printf ("%s (thread %d) device %s fid %d recieved WSY message, terminating WSX mode to %s (thread %d).\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, sio.my_sio_port_table_entry.sio_com_port_name, sio.sio_port_fid, sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to, sio.wsx_mode_mux_address);
	  
	  fflush (stdout);

	  // unlock the mutex
	  pthread_mutex_unlock (&sio.mutex);
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
	  launch_timer_disable(promptTimer);
	  
	  launch_timer_disable(myPromptSequences.betweenSequenceTimer);
	  launch_timer_disable(myPromptSequences.betweenPromptTimer);
	  
	  return(NULL);
	  
	  break;
	}
      case SPI:		// recieved a SPI (Status Ping) message
	break;
        
      default:		// recieved an unknown message type
	{
	  int syncopationDoneFlag;
	  
	  if(UDP_ASCII == theLoggingList[myLoggerNumber].sourceType) {
	    //printf("udp - got a default message\n");
	  }
	  else {
	    //printf("serial - got a default message\n");
	  }

	  if(hdr.type >= LOG_THREAD_OFFSET*10000) {
	    if(mySequenceMessage == hdr.type) { // time to initiate a sequence
	      //tttodo printf("*** mySequenceMessage ***\n");
	      whichPrompt = 0;
	      launch_timer_enable(myPromptSequences.betweenPromptTimer, myPromptSequences.secondsBetweenPrompts, 1, sio.sio_thread_num, myPromptMessage);
	      break;
	    }
	    else if(myPromptMessage == hdr.type) {
	      //tttodo printf("*** myPromptMessage ***\n");
	      
	      // --- determine which message to send next
	      syncopationDoneFlag = 0;
	      while(0 == syncopationDoneFlag) {
		//tttodo printf("whichPrompt is %d, theLoggingList[myLoggerNumber].promptSyncopation[whichPrompt] is %d, syncopationCount[whichPrompt] is %d\n", whichPrompt, theLoggingList[myLoggerNumber].promptSyncopation[whichPrompt], syncopationCount[whichPrompt]);
		if(whichPrompt >= myPromptSequences.numberOfPrompts) {
		  // -- all messages have been sent
		  whichPrompt = 0;

		  // if we are in free run just keep going - else wait for next mySequenceMessage
		  if(theLoggingList[myLoggerNumber].freeRunPromptFlag == 0 ) {
		    syncopationDoneFlag = 1;
		  }
		}
		else {
		  ++syncopationCount[whichPrompt];
		  if( (1 == theLoggingList[myLoggerNumber].promptEnable[whichPrompt]) && ( (0 == theLoggingList[myLoggerNumber].usePromptSpecificSyncopation) || (syncopationCount[whichPrompt] > theLoggingList[myLoggerNumber].promptSyncopation[whichPrompt]) ) ) {
		    // send this message now
		    // tttodo printf("whichPrompt is %d - promptCharlength is %d - prompt is %s \n", whichPrompt, myPromptSequences.prompts[whichPrompt].promptCharLength, myPromptSequences.prompts[whichPrompt].promptChars);
		    msg_send(sio.sio_thread_num, sio.sio_thread_num, WAS, myPromptSequences.prompts[whichPrompt].promptCharLength, myPromptSequences.prompts[whichPrompt].promptChars);
		    if(theLoggingList[myLoggerNumber].usePromptSpecificTermChar) {
		      sio.my_sio_port_table_entry.auto_mux_term_char = theLoggingList[myLoggerNumber].promptTermChar[whichPrompt];
		    }
		    syncopationCount[whichPrompt] = 0;
		    activePrompt = whichPrompt; // so that rx data is handled appropriately
		    whichPrompt++;
		    syncopationDoneFlag = 1;
		  }
		  else {
		    // try the next message
		    whichPrompt++;
		  }
		}
	      }  // end of while(0 == syncopationDoneFlag)
	      
	      // --- Now setup the timer to send the next prompt - or disable if done with this sequence
	      if(whichPrompt != 0) {
		if(theLoggingList[myLoggerNumber].usePromptSpecificReplyWindows) {
		  launch_timer_enable(myPromptSequences.betweenPromptTimer, theLoggingList[myLoggerNumber].promptReplyWindows[activePrompt], 1, sio.sio_thread_num,myPromptMessage);
		}
		else {
		  launch_timer_enable(myPromptSequences.betweenPromptTimer, myPromptSequences.secondsBetweenPrompts, 1, sio.sio_thread_num, myPromptMessage);
		}
	      }  // end of if(whichPrompt != 0)
	      else {
		launch_timer_disable(myPromptSequences.betweenPromptTimer);
	      }
	      break;
	    }  // end of else if(myPromptMessage == hdr.type)
	    break;
	  } // end of if(hdr.type >= LOG_THREAD_OFFSET*10000)
	  
	  else {
	    printf ("%s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE - " "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
	  }
	  
	  break;
	}  // end of hdr.type - default:
      } // end of switch(hdr.type)
    } // else
  } //while
  
  return NULL;
}
