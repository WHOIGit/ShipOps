/* ----------------------------------------------------------------------

nio log thread for alvin system

Modification History:
DATE        AUTHOR COMMENT
4 Nov 2009  jch    created and written, derived from nio_thread.cpp

2012-12-21   tt       - start of mods in nioLogThread.cpp to support SSSG data logging platform needs
2012-12-21   tt       - mods to support single prompt sequence rather than multiple prompt sequences
                             - theLoggingList[myLoggerNumber].promptSequences[0].***  -> theLoggingList[myLoggerNumber].promptSequence.***
                             - remove code related to nOfPromptSequences
2013-05-30   tt       - mods to support argument of open_network() function

**************************************************************************/

/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
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

#include "dsLog.h"


#include "msg_util.h"		/* utility functions for messaging */


#include "nio_thread.h"		/* nio thread */
#include "nioLogThread.h"
#include "config_file.h"

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;


/* ----------------------------------------------------------------------

Function to process incoming records 


Modification History:
DATE         AUTHOR  COMMENT
1-OCT-2001  JCH      Created and written.

---------------------------------------------------------------------- */

static void *
      nioLogRXThread (void *arg)
{

   nio_thread_t *nio = (nio_thread_t *) arg;
   char big_buffer[BUFFERMAX];
   int rcv_size;
   nio = (nio_thread_t *) arg;
   struct sockaddr from_addr;
   unsigned int from_len;

   while (1)
      {

         if ((rcv_size = recvfrom (nio->my_nio_port_table_entry.my_sock, big_buffer, BUFFERMAX, 0, &from_addr, &from_len)) < 0)
            {
               fprintf (stderr, "nioLogRXThread - error on recvfrom, socket from %s\n", nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to);
            }
         else
            {			// we got some good data

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
               for (int i = 0; i < nio->my_nio_port_table_entry.n_of_destinations; i++)
                  {
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

//   NIO nioLogThread

//   Modification History:
//   DATE         AUTHOR  COMMENT
//  4 nov 2009    jch     created and writen, derived from nio_thread.cpp


//----------------------------------------------------------------------*/
void * nioLogThread (void *thread_num)
{

   nio_thread_t nio = { PTHREAD_MUTEX_INITIALIZER, -1};

   char *my_name;
   loggerStatus_t myStatus;
   myStatus.bytesReceived = 0;
   myStatus.msgsRecieved = 0;
   myStatus.readErrors = 0;
   myStatus.timeSinceLastData = 999.0;

   DestinationStatus_t  destinationStatus;
   destinationStatus.nOfDestinations = 0;
   destinationStatus.statusTime = rov_get_time();
   for(int dN = 0; dN < MAX_N_OF_DESTINATIONS; dN++)
      {
         destinationStatus.statuses[dN].errorCount = 0;
         destinationStatus.statuses[dN].bytesSent = 0;
         destinationStatus.statuses[dN].availableSpace = 0;
         destinationStatus.statuses[dN].freeSpace = 0;
         destinationStatus.statuses[dN].percentageFree = 100.0;
      }

   prompt_sequence_t    myPromptSequence;


   msg_hdr_t hdr =  {0};
   unsigned char data[MSG_DATA_LEN_MAX] = {0};

#ifdef DEBUG_NIO
   printf ("NIO THREAD: entering nio thread\n\n");
#endif
   /* get my thread number */
   nio.nio_thread_num = (long int) thread_num;
#ifdef DEBUG_NIO
   printf ("NIO THREAD: nio thread num = %d\n\n", nio.nio_thread_num);
#endif

   int myLoggerNumber;
   // now look into the process table and figure out what thread I am
   for (int i = 0; i < MAX_NUMBER_OF_THREADS; i++)
      {

         if (global_thread_table[i].thread_num == nio.nio_thread_num)
            {

               my_name = global_thread_table[i].thread_name;
               myLoggerNumber = global_thread_table[i].extra_arg_1;
               //#ifdef DEBUG_NIO
               //printf("NIO THREAD: NIO my name is %s\n",  my_name);
               //#endif
            }
      }

#ifdef DEBUG_NIO
   printf ("NIO THREAD: made it through thread table search in nio thread\n\n");
#endif
   if (!my_name)
      {
         fprintf (stderr, "NIO log THREAD: thread %d  not found in process table!\n", nio.nio_thread_num);
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   else
      {
         //printf ("NIO THREAD: starting nio thread for %s\n", my_name);
      }



   nio.my_nio_port_table_entry.socket_number  = theLoggingList[myLoggerNumber].inSocketNumber;
   nio.my_nio_port_table_entry.ip_address = strdup(NULL_IP_ADDRESS);
   nio.my_nio_port_table_entry.to_socket_number = NULL_SOCKET_NUMBER;

   printf(" num of destinations = %d\n",theLoggingList[myLoggerNumber].nOfDestinations);

   for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++)
      {
         if(UDP_SOCKET == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType)
            {
               theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.socket =  socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
               int broadcastPermission = 1;
               if(setsockopt(theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.socket,SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0)
                  {
                     fprintf (stderr, "***couldn't set broadcast on UDP : %s \n",theLoggingList[myLoggerNumber].sourceName );

                  }
               memset(&(theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr), 0, sizeof (theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr));

               theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr.sin_family = AF_INET;
               theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr.sin_addr.s_addr = inet_addr(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.ipAddress);
               theLoggingList[myLoggerNumber].destinations[ destinationCount].networkDestination.Addr.sin_port =  htons(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.toSocketNumber);
            }
         else if(DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType)
            {
            }
      }

   myPromptSequence.numberOfPrompts = theLoggingList[myLoggerNumber].promptSequence.numberOfPrompts;
   myPromptSequence.secondsBetweenSequences = theLoggingList[myLoggerNumber].promptSequence.secondsBetweenSequences;
   myPromptSequence.secondsBetweenPrompts = theLoggingList[myLoggerNumber].promptSequence.secondsBetweenPrompts;
   
   for(int promptN = 0; promptN < MAX_N_OF_PROMPTS_IN_SEQUENCE; promptN++)
     {
       myPromptSequence.prompts[promptN].promptCharLength =  theLoggingList[myLoggerNumber].promptSequence.prompts[promptN].promptCharLength;
       for(int charN = 0; charN < MAX_PROMPT_LENGTH; charN++)
	 {
	   myPromptSequence.prompts[promptN].promptChars[charN] = theLoggingList[myLoggerNumber].promptSequence.prompts[promptN].promptChars[charN];
	 }
     }

   destinationStatus.nOfDestinations = theLoggingList[myLoggerNumber].nOfDestinations;
//left off here putting destination status into this thread
   // init the msg queue
   int msg_success = msg_queue_new(nio.nio_thread_num, my_name);

   if(msg_success != MSG_OK)
      {
         fprintf(stderr, "NIO THREAD: NIO thread for %s: %s\n",nio.my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to,MSG_ERROR_CODE_NAME(msg_success) );
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   // wakeup message
   //#ifdef DEBUG_NIO
   printf ("NIO LOG THREAD: NIO (thread %d) initialized \n", nio.nio_thread_num);
   printf ("NIO LOG THREAD: NIO File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);

   // #endif

   // ----------------------------------------------------------------------
   // open the network port
   // ----------------------------------------------------------------------
#ifdef DEBUG_NIO_LOGGER
   printf ("NIO THREAD: about to open port %d for input\n",nio.my_nio_port_table_entry.socket_number);
#endif
   nio.nio_port_fid = open_network (& nio.my_nio_port_table_entry);
#ifdef DEBUG_NIO_LOGGER
   printf ("NIO THREAD: got by open port, FID=%d\n",nio.nio_port_fid);
#endif
   nio.my_nio_port_table_entry.n_of_destinations = 1;
   nio.my_nio_port_table_entry.auto_send_addresses[0] = nio.nio_thread_num;
   // launch the thread to process incoming characters from the network port
   pthread_create (&nio.nio_rx_subthread, &DEFAULT_ROV_THREAD_ATTR, nioLogRXThread, (void *) (&nio));

   launched_timer_data_t   *promptTimer = launch_timer_new(LOGGING_STATUS_INTERVAL,-1,nio.nio_thread_num,STPR);


   // set up the prompting timers
   int mySequenceMessage;
   int myPromptMessage;
   int whichPrompt, activePrompt;


   double sequenceInterval = myPromptSequence.secondsBetweenSequences;
   mySequenceMessage = nio.nio_thread_num*10000;
   myPromptMessage = nio.nio_thread_num*10000 + 100;
   myPromptSequence.betweenSequenceTimer = launch_timer_new(sequenceInterval,-1,nio.nio_thread_num, mySequenceMessage);
   myPromptSequence.betweenPromptTimer = launch_timer_new(sequenceInterval,0,nio.nio_thread_num, myPromptMessage);
   whichPrompt = 0;
   activePrompt = 0;

   // loop forever
   while (1)
      {

         // wait forever on the input channel
#ifdef DEBUG_NIO_LOGGER
         printf ("NIO THREAD: NIO calling get_message.\n");
#endif
         int msg_get_success = msg_get(nio.nio_thread_num,&hdr, data, MSG_DATA_LEN_MAX);
         if(msg_get_success != MSG_OK)
            {
               fprintf(stderr, "NIO THREAD: nio thread--error on msg_get:  %s\n",MSG_ERROR_CODE_NAME(msg_get_success));
            }
         else
            {

#ifdef DEBUG_RECIEVED_MESSAGES
               // print on stderr
               printf ("NIO THREAD: NIO THREAD (thread %d)  got msg type %d from proc %d to proc %d, %d bytes data\n", nio.nio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
               // process new message
               switch (hdr.type)
                  {

                  case WAS:		// recieved a WAS (Write Serial String) Message
                        {
                           // send the characters to the port, keep stats
                           // note that WAS messages need NOT be null terminated
                           if (hdr.length > 0)
                              {

                                 int bytes_sent;
                                 // lock the mutex
                                 pthread_mutex_lock (&nio.mutex);
                                 // send the bytes

                                 bytes_sent = sendto(nio.my_nio_port_table_entry.to_sock,
                                                     data,
                                                     hdr.length,
                                                     0,
                                                     (struct sockaddr *) (&(nio.my_nio_port_table_entry.ToAddr)),
                                                     sizeof (nio.my_nio_port_table_entry.ToAddr));

                                 if (bytes_sent == hdr.length)
                                    {

                                       // keep stats
                                       nio.total_tx_chars += hdr.length;
                                    }
                                 else
                                    {
                                       fprintf (stderr, "NIO THREAD: ERROR sending %d byte WAS to %s port %d - %d bytes sent.\n",
                                                hdr.length,
                                                nio.my_nio_port_table_entry.ip_address,
                                                nio.my_nio_port_table_entry.to_socket_number,
                                                bytes_sent
                                                );

                                       fprintf (stderr, "NIO THREAD: Data is: \"%*s\"\n", hdr.length, data);

                                    }

                                 // unlock the mutex
                                 pthread_mutex_unlock (&nio.mutex);
                              }

                           break;
                        }
                  case SAS:
                        {			// received a SAS, probably from my subthread
#ifdef DEBUG_NIO
                           printf ("NIO THREAD: %s: (thread %d) NIO RECIEVED SAS -" "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[nio.nio_thread_num].thread_name, nio.nio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
                           // first, null terminate
                           data[hdr.length] = '\0';
                           if(hdr.from == nio.nio_thread_num)
                              {
                                 myStatus.timeOfLastMsg = rov_get_time();
                                 myStatus.msgsRecieved++;
                                 myStatus.bytesReceived += hdr.length;
                              }
                           // first check to see if we should timestamp this record
                           char    outputRecord[MAX_LOGGING_STRING_LENGTH];
                           int dataLength;
                           //memset(outputRecord,'\0',MAX_LOGGING_STRING_LENGTH);
                           if(theLoggingList[myLoggerNumber].timeStampFlag)
                              {
                                 char    timeString[48];

                                 rov_sprintf_dsl_time_string(timeString);
                                 int len = snprintf(outputRecord,MAX_LOGGING_STRING_LENGTH,"%s %s %s %s" ,theLoggingList[myLoggerNumber].dataType,timeString,theLoggingList[myLoggerNumber].sourceName,data);
                                 // null terminate just in case
                                 outputRecord[len] = '\0';
                                 dataLength = len;

                              }
                           else
                              {
                                 strncpy(outputRecord,(char *)data,MAX_LOGGING_STRING_LENGTH);
                                 dataLength = hdr.length;
                              }
                           // does it have a /n on it?  check and see
                           if(outputRecord[dataLength-1] != '\n')
                              {
                                 outputRecord[dataLength] = '\n';
                                 outputRecord[dataLength+1] = '\0';
                                 dataLength += 1;
                              }
                           // now we've got the record we wnat to distribute and/or log
                           for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++)
                              {
                                 switch( theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType)
                                    {
                                    case UDP_SOCKET:
                                          {
                                             // here is where I should check the filters
                                             int send = true;

                                             if(theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.implemented)
                                                {
                                                   char *filterPointer =  theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.startsWith;
                                                   if(filterPointer)
                                                      {
                                                         if(!strncmp((char *)data,filterPointer,strlen(filterPointer)))
                                                            {
                                                               send = true;
                                                            }
                                                         else
                                                            {
                                                               send = false;
                                                            }
                                                      }
                                                   filterPointer =  theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.greps;
                                                   if(filterPointer)
                                                      {
                                                         if(strstr((char *)data,filterPointer))
                                                            {
                                                               send = true;
                                                            }
                                                         else
                                                            {
                                                               send = false;
                                                            }
                                                      }
                                                   filterPointer =  theLoggingList[myLoggerNumber].destinations[destinationCount].destinationFilter.grepMinusVs;
                                                   if(filterPointer)
                                                      {
                                                         if(strstr( (char *)data, filterPointer))
                                                            {
                                                               send = false;
                                                            }
                                                         else
                                                            {
                                                               send = true;
                                                            }

                                                      }
                                                }

                                             if(send)
                                                {
                                                   sendto ( theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.socket, outputRecord, dataLength, 0, (struct sockaddr *) &(theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination.Addr), sizeof (theLoggingList[myLoggerNumber].destinations[destinationCount].networkDestination));
                                                }
                                             break;
                                          }
                                    case DISK:
                                          {
                                             //printf(" output record length %d rec %s \n",strlen(outputRecord),outputRecord);
                                             logThisNow(&(theLoggingList[myLoggerNumber].destinations[destinationCount]),&(theLoggingList[myLoggerNumber]),outputRecord);
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

                                    }

                              }
                           break;

                        }
                  case STPR:
                        {
                           // send a status msg to the status thread

                           rov_time_t timeNow = rov_get_time();
                           myStatus.timeSinceLastData = timeNow - myStatus.timeOfLastMsg;

                           myStatus.statusTime = timeNow;

                           //printf("myStatus %d msgs %d\n",nio.nio_thread_num,myStatus.msgsRecieved);
                           msg_send(DSLOG_STATUS_THREAD,nio.nio_thread_num,LSTM, sizeof(myStatus), &myStatus);

                           break;
                        }

                   case RDSP:
                      {
                         struct statvfs sfs;
                         char *logging_directory;
                         for(int destinationCount = 0; destinationCount < theLoggingList[myLoggerNumber].nOfDestinations; destinationCount++)
                            {
                               if(DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType)
                                  {
                                     logging_directory = theLoggingList[myLoggerNumber].destinations[destinationCount].loggingDestination.loggingDirectory;
                                     destinationStatus.statuses[destinationCount].destinationType = DISK;
                                  }
                               else if(SINGLE_DISK == theLoggingList[myLoggerNumber].destinations[destinationCount].destinationType)
                                  {
                                     logging_directory = theGlobalLogFile.loggingDestination.loggingDirectory;
                                     destinationStatus.statuses[destinationCount].destinationType = SINGLE_DISK;
                                  }
                               else
                                  {
                                     continue;
                                  }
                               if (statvfs(logging_directory, &sfs) == -1)
                                  {
                                     continue;  // problem on the statfs
                                  }
                               else
                                  {
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

                         msg_send(DSLOG_STATUS_THREAD,nio.nio_thread_num,LSDM, sizeof(destinationStatus), &destinationStatus);
                         break;




                     }
                  case SPI:
                        {	// recieved a SPI (Status Ping) message

                           char msg[256];
                           snprintf (msg,256, "NIO LOG  THREAD: %s: (thread %d) is Alive!", global_thread_table[nio.nio_thread_num].thread_name, nio.nio_thread_num);
                           break;
                        }
                  case BYE:
                        { // received a bye message--time to give up the ghost--or at least my socket

                           close(nio.my_nio_port_table_entry.my_sock);
                           launch_timer_disable(promptTimer);
			   launch_timer_disable(myPromptSequence.betweenSequenceTimer);
			   launch_timer_disable(myPromptSequence.betweenPromptTimer);
                           return(NULL);


                           break;
                        }
                  default:		// recieved an unknown message type
                        {
			  if(hdr.type >= LOG_THREAD_OFFSET*10000) {
			    if(mySequenceMessage == hdr.type) { // time to initiate a sequence
			      //printf("nioLogThread - mySequenceMessage received.\n");
			    }
			  }
			  else {
			    printf ("NIO LOG THREAD: %s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE -" "got msg type %d from proc %d to proc %d, %d bytes data\n",
				    global_thread_table[nio.nio_thread_num].thread_name,
				    nio.nio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
			  }
			  break;
                        }
                  }
            } // end else

      }


}


