/* ----------------------------------------------------------------------
** Data Thread Routine
** data_thread.cpp
**
** Description: the thread which sends messages (navigation and attitude) to the hmrg
**              data acquisition system
**
**
** History:
**       Date       Who      Description
**      ------      ---      --------------------------------------------
**      7/2002      jch      Create from DATA_THREAD.cpp
**      9/02/2002   jch      fix big bugs:  rad/deg in geoconversion
**     11/18/2006   jch      add OPR, fix CSV, fix MDS, reorder things
**     2013-01-03   tt       changes to support new destination specific prefix and extensions.
**
**     2013-05-01   tt       changes to support new logging messages
**                           - also fixed bad format string in info msg "compiled on...."
**                           - added ISLS case to send ISLS and PSLS messages
**                           - added subroutines to create ISLS and PSLS messages
**
**     2013-05-01   tt       changes to support new logging messages
**                           - added code to generate SUDS message
**                           - added code to initialize and update local copy of promptLoggerStatus
**                           - 
**                           - 
**                           - 
**                           - 
**                           - 
**
**
**
**
---------------------------------------------------------------------- */
/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/statfs.h>


/* posix header files */
#define  POSIX_SOURCE 1
#include <pthread.h>

/* jason header files */
#include "dsLog.h"

#include "dsLogTalk.h"		/* jasontalk protocol and structures */
#include "launch_timer.h"
#include "msg_util.h"		/* utility functions for messaging */

#include "time_util.h"
#include "dsLogTalk.h"
#include "dsLogStatusThread.h"


/* ----------------------------------------------------------------------

   dsLog status Thread

   Modification History:
   DATE                AUTHOR  COMMENT
   11  september 2011  jch     Created and written.to talk to daq system
   
   

---------------------------------------------------------------------- */
void *
      dsLogStatusThread (void *)
{


   msg_hdr_t hdr = { 0 };
   unsigned char data[MSG_DATA_LEN_MAX] = { 0 };
   loggerStatus_t loggerStatus[MAX_N_OF_INPUTS];
   promptLoggerStatus_t promptLoggerStatus[MAX_N_OF_INPUTS];
   DestinationStatus_t destinationStatus[MAX_N_OF_INPUTS];

   // wakeup message
   printf ("dslog status thread (thread %d) initialized \n", DSLOG_STATUS_THREAD);
   printf ("dslog status thread File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);

   int msg_success = msg_queue_new(DSLOG_STATUS_THREAD, "data thread");
   if(msg_success != MSG_OK)
      {
         fprintf(stderr, "dslog status thread: %s\n",MSG_ERROR_CODE_NAME(msg_success) );
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   launched_timer_data_t   *configurationTimer = launch_timer_new(CONFIGURATION_PROMPT_INTERVAL,-1,DSLOG_STATUS_THREAD,CFPR);

   launched_timer_data_t   *promptTimer = launch_timer_new(STATUS_PROMPT_INTERVAL,-1,DSLOG_STATUS_THREAD,STPR);

   launched_timer_data_t   *diskCheckTimer= launch_timer_new(DISK_PROMPT_INTERVAL,-1,DSLOG_STATUS_THREAD,DSPR);

   logging_descriptor_t    statusDescriptor;
   statusDescriptor.active = true;
   statusDescriptor.nOfDestinations = 2;
   statusDescriptor.destinations[0].destinationType = DISK;
   statusDescriptor.destinations[0].loggingDestination.fileExtension = strdup("LGS");
   statusDescriptor.destinations[0].loggingDestination.filenamePrefix = strdup(startup.defaultFilenamePrefix);
   statusDescriptor.destinations[0].loggingDestination.loggingDirectory = strdup(startup.rootDirectory);
   statusDescriptor.destinations[1].destinationType = UDP_SOCKET;
   statusDescriptor.destinations[1].networkDestination.active = true;
   statusDescriptor.destinations[1].networkDestination.socket =  socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   int broadcastPermission = 1;
   if(setsockopt(statusDescriptor.destinations[1].networkDestination.socket,SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0)
      {
         fprintf (stderr, "***couldn't set broadcast on status UDP  \n");

      }
   memset(&(statusDescriptor.destinations[1].networkDestination.Addr), 0, sizeof (statusDescriptor.destinations[1].networkDestination.Addr));

   statusDescriptor.destinations[1].networkDestination.Addr.sin_family = AF_INET;
   statusDescriptor.destinations[1].networkDestination.Addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   statusDescriptor.destinations[1].networkDestination.Addr.sin_port =  htons(10500);


   statusDescriptor.sourceName = strdup("LGR");
   statusDescriptor.dataType = strdup("LGS");
   statusDescriptor.timeStampFlag = false;

   // initialize the promptLoggerStatus array
   //promptLoggerStatus_t promptLoggerStatus[MAX_N_OF_INPUTS];
   for(int plsIndex=0; plsIndex<MAX_N_OF_INPUTS; ++plsIndex) {
     promptLoggerStatus[plsIndex].sourceNumber = 999;
     strncpy(promptLoggerStatus[plsIndex].sourceName, "UNKNOWN", sizeof(promptLoggerStatus[plsIndex].sourceName));
     promptLoggerStatus[plsIndex].numberOfPrompts = 0;
     promptLoggerStatus[plsIndex].bytesReceived = 0;
     promptLoggerStatus[plsIndex].msgsRecieved = 0;
     promptLoggerStatus[plsIndex].readErrors = 0;
     promptLoggerStatus[plsIndex].timeSinceLastData = 999.0;
     for(int myPromptCount=0; myPromptCount<MAX_N_OF_PROMPTS_IN_SEQUENCE; ++myPromptCount) {
       sprintf(promptLoggerStatus[plsIndex].promptSourceName[myPromptCount], "UNKNOWN");
     }
     for(int myPromptCount=0; myPromptCount<MAX_N_OF_PROMPTS_IN_SEQUENCE; ++myPromptCount) {
       promptLoggerStatus[plsIndex].promptStatus[myPromptCount].bytesReceived = 0;
       promptLoggerStatus[plsIndex].promptStatus[myPromptCount].msgsRecieved = 0;
       promptLoggerStatus[plsIndex].promptStatus[myPromptCount].readErrors = 0;
       promptLoggerStatus[plsIndex].promptStatus[myPromptCount].timeSinceLastData = 999.0;
     }
   }  // end of for loop over plsIndex

   // loop forever
   while (1)
      {
         // wait forever on the input channel
         int msg_get_success = msg_get(DSLOG_STATUS_THREAD,&hdr, data, MSG_DATA_LEN_MAX);
         if(msg_get_success != MSG_OK)
            {
               fprintf(stderr, "data thread--error on msg_get:  %s\n",MSG_ERROR_CODE_NAME(msg_get_success));
            }
         else
            {
#ifdef DEBUG_RECIEVED_MESSAGES
               // print on stderr
               printf ("DSLOG_STATUS_THREAD got msg type %d from proc %d to proc %d, %d bytes data\n", hdr.type, hdr.from, hdr.to, hdr.length);
#endif

               // process new message
               switch (hdr.type)
                  {
                  case PNG:		// recieved a PNG (Ping) message
                        {
                           const char *msg = "DSLOG_STATUS_THREAD is Alive!";

                           // respond with a SPI (Status Ping) message
                           msg_send( hdr.from, hdr.to, SPI, strlen (msg), msg);

                           break;
                        }
                  case SPI:		// recieved a SPI (Status Ping) message
                     break;
                  case CFPR:
                        {
                           break;
                        }
                  case LSDM:
                     {
                        DestinationStatus_t *newStatus = (DestinationStatus_t *)data;
                        for(int thN = 0; thN < startup.nOfLogInputs; thN++)
                           {
                              if(hdr.from == theLoggingList[thN].threadNumber)
                                 {
                                    destinationStatus[thN] = *newStatus;
                                    break;
                                 }
                           }
                        break;
                     }


                  case LSTM:
                        {
                           loggerStatus_t *newStatus = (loggerStatus_t *)data;

                           for(int thN = 0; thN < startup.nOfLogInputs; thN++)
                              {
                                 if(hdr.from == theLoggingList[thN].threadNumber)
                                    {
                                       loggerStatus[thN] = *newStatus;
                                       //printf(" in status thread, %d received %d msgs \n",hdr.from,newStatus->msgsRecieved);
                                       break;
                                    }
                              }
                           break;
                        }
                  case STPR:
                        {
                           char  msg[STATUS_MSG_LENGTH];
                           // got a status prompt.  interrogate the logging sio and nio threads
                           rov_time_t  statusTime = rov_get_time();
                           int msglen = makeLoggingStatusMessage(loggerStatus,statusTime,msg);
                           //printf(" the status msg:  %s\n",msg);
                           if(msglen)
                              {
                                // msg_send(GUI_THREAD,DSLOG_STATUS_THREAD, WAS,msglen,msg  );
                                 logThisNow(&(statusDescriptor.destinations[0]),&statusDescriptor,msg);
                                 sendto ( statusDescriptor.destinations[1].networkDestination.socket, msg, msglen, 0, (struct sockaddr *) &(statusDescriptor.destinations[1].networkDestination.Addr), sizeof (statusDescriptor.destinations[1].networkDestination));

                              }

                           msglen = makeSudsStatusMessage(promptLoggerStatus,statusTime,msg);
                           if(msglen) {
			     logThisNow(&(statusDescriptor.destinations[0]),&statusDescriptor,msg);
			     sendto ( statusDescriptor.destinations[1].networkDestination.socket, msg, msglen, 0, (struct sockaddr *) &(statusDescriptor.destinations[1].networkDestination.Addr), sizeof (statusDescriptor.destinations[1].networkDestination));
			   }
                           break;
                        }
                  case ISLS:
                        {
                           char  msg[STATUS_MSG_LENGTH];
                           rov_time_t  statusTime = rov_get_time();
			   promptLoggerStatus_t *newPromptLoggerStatus = (promptLoggerStatus_t *) data;

			   // keep a copy odf the info for the SUDS message
                           for(int thN = 0; thN < startup.nOfLogInputs; thN++) {
			     if(hdr.from == theLoggingList[thN].threadNumber) {
			       promptLoggerStatus[thN] = *newPromptLoggerStatus;
			       break;
			     }
			   }

                           // send an input specific logging status message
                           int msglen = makeIsLoggingStatusMessage( (promptLoggerStatus_t *) data, statusTime, msg);
                           if(msglen) {
			     logThisNow(&(statusDescriptor.destinations[0]),&statusDescriptor,msg);
			     sendto ( statusDescriptor.destinations[1].networkDestination.socket, msg, msglen, 0, (struct sockaddr *) &(statusDescriptor.destinations[1].networkDestination.Addr), sizeof (statusDescriptor.destinations[1].networkDestination));
			     
			   }

                           // send a series of prompt specific logging status messages if appropriate
			   if(newPromptLoggerStatus->numberOfPrompts > 0) {
			     for(int promptIndex=0; promptIndex<newPromptLoggerStatus->numberOfPrompts; ++promptIndex) {
			       int msglen = makePsLoggingStatusMessage( (promptLoggerStatus_t *) data, promptIndex, statusTime, msg);
			       if(msglen) {
				 logThisNow(&(statusDescriptor.destinations[0]),&statusDescriptor,msg);
				 sendto ( statusDescriptor.destinations[1].networkDestination.socket, msg, msglen, 0, (struct sockaddr *) &(statusDescriptor.destinations[1].networkDestination.Addr), sizeof (statusDescriptor.destinations[1].networkDestination));
			       }  // end of if(msglen)
			     }  // end of for(promptIndex...)
			   }  // end of if(newPromptLoggerStatus->numberOfPrompts > 0)
                           break;
                        }
                  case DSPR:
                     {
                        char  msg[STATUS_MSG_LENGTH];
                        for(int thN = 0; thN < startup.nOfLogInputs; thN++)
                           {
                              msg_send(theLoggingList[thN].threadNumber, DSLOG_STATUS_THREAD,RDSP,0,NULL);
                           }
                         rov_time_t  statusTime = rov_get_time();
                        // // now make a status msg of what we have from before
                        int msglen = makeDestinationStatusMessage(destinationStatus,statusTime,msg);
                       // printf(" the DISK msg:  %s\n",msg);
                        logThisNow(&(statusDescriptor.destinations[0]),&statusDescriptor,msg);
                        sendto ( statusDescriptor.destinations[1].networkDestination.socket, msg, msglen, 0, (struct sockaddr *) &(statusDescriptor.destinations[1].networkDestination.Addr), sizeof (statusDescriptor.destinations[1].networkDestination));
                        break;

                     }

                  case BYE:
                        {
                           launch_timer_disable(promptTimer);
                           launch_timer_disable(configurationTimer);
                           launch_timer_disable(diskCheckTimer);
                           return(NULL);

                           break;
                        }
                  default:		// recieved an unknown message type
                        {
                           printf ("DSLOG_STATUS_THREAD: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE - " "got msg type %d from proc %d to proc %d, %d bytes data\n", DSLOG_STATUS_THREAD, hdr.type, hdr.from, hdr.to, hdr.length);
                           break;
                        }
                  }			// switch
            } // end else msg_get OK

      }

}

int   makeLoggingStatusMessage(loggerStatus_t   *statusData, rov_time_t statusTime, char *msg)
{
   int length = 0;
   char  timeString[256];
   rov_convert_dsl_time_string(statusTime, timeString);
   length = snprintf(msg,264,"LGS %s LGR",timeString);
   for(int srcN = 0; srcN < startup.nOfLogInputs; srcN++)
      {
         length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," S%d %0.2f %d %d %d",srcN+1,statusData[srcN].timeSinceLastData,
                           statusData[srcN].msgsRecieved,
                           statusData[srcN].bytesReceived,
                           statusData[srcN].readErrors);


      }
   length += snprintf(&(msg[length]),5,"\n");
   return length;
}

int makeSudsStatusMessage(promptLoggerStatus_t *statusData, rov_time_t statusTime, char *msg)
{
  int length = 0;
  char  timeString[256];
  rov_convert_dsl_time_string(statusTime, timeString);
  length = snprintf(msg,264,"SUDS %s ",timeString);
  for(int srcN = 0; srcN < startup.nOfLogInputs; srcN++) {
    length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," S%d %s %0.2f %d %d %d",
		       srcN+1,
		       statusData[srcN].sourceName,
		       statusData[srcN].timeSinceLastData,
		       statusData[srcN].msgsRecieved,
		       statusData[srcN].bytesReceived,
		       statusData[srcN].readErrors);
  }
  length += snprintf(&(msg[length]),5,"\n");
  return length;
}

int   makeIsLoggingStatusMessage(promptLoggerStatus_t *statusData, rov_time_t statusTime, char *msg)
{
   int length = 0;
   char  timeString[256];
   rov_convert_dsl_time_string(statusTime, timeString);

   // make ISLS message
   length = snprintf(msg,264,"ISLS %s",timeString);
   length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," S%d %s %0.2f %d %d %d",
		      statusData->sourceNumber,
		      statusData->sourceName,
		      statusData->timeSinceLastData,
		      statusData->msgsRecieved,
		      statusData->bytesReceived,
		      statusData->readErrors);

   length += snprintf(&(msg[length]),5,"\n");
   return length;
}

int   makePsLoggingStatusMessage(promptLoggerStatus_t *statusData, int index, rov_time_t statusTime, char *msg)
{
   int length = 0;
   char  timeString[256];
   rov_convert_dsl_time_string(statusTime, timeString);

   // make ISLS message
   length = snprintf(msg,264,"PSLS %s",timeString);
   length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," S%d %s P%d %s %0.2f %d %d %d",
		      statusData->sourceNumber,
		      statusData->sourceName,
		      index,
		      statusData->promptSourceName[index],
		      statusData->promptStatus[index].timeSinceLastData,
		      statusData->promptStatus[index].msgsRecieved,
		      statusData->promptStatus[index].bytesReceived,
		      statusData->promptStatus[index].readErrors);

   length += snprintf(&(msg[length]),5,"\n");
   return length;
}

int makeDestinationStatusMessage(DestinationStatus_t   *destinationData, rov_time_t statusTime, char *msg)
{
    int length = 0;
    char  timeString[256];
    rov_convert_dsl_time_string(statusTime, timeString);
    length = snprintf(msg,264,"DSR %s LGR",timeString);
    for(int srcN = 0; srcN < startup.nOfLogInputs; srcN++)
       {
            length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," S%d %d",srcN+1,destinationData[srcN].nOfDestinations);
            for(int dN = 0; dN < destinationData[srcN].nOfDestinations; dN++)
                {
                    if((DISK == destinationData[srcN].statuses[dN].destinationType) || (SINGLE_DISK == destinationData[srcN].statuses[dN].destinationType))
                        {
                            length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," %d %.1f %lld  ",  destinationData[srcN].statuses[dN].errorCount,destinationData[srcN].statuses[dN].percentageFree,destinationData[srcN].statuses[dN].availableSpace);
                        }
                    else
                        {
                            length += snprintf(&(msg[length]),STATUS_MSG_LENGTH," NOT_DISK 0.0 0");
                        }

                }

       }
    length += snprintf(&(msg[length]),5,"\n");

    return length;
}
