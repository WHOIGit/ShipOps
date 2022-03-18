/* -------------------------------------------------------------------
nio thread for ROV control system

Modification History:
DATE        AUTHOR COMMENT
1-OCT-2001  JCH     Created and written.
22-Jan-2002 JCH     make work with sending, not just receiving
15 feb 2002 JCH     change for new messaging api
26 apr 2002 JCH     make work for broadcast
19 Jul 2002 LLW     Revised to transmit as well as receive, presently supports
                    only one TX destination
                    
---------------------------------------------------------------------- */
/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
#include "config_file.h"

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;
/* ----------------------------------------------------------------------
   
Function to open a network port.  

Returns -1 if unsuccesful, valid file id if successful 

Modification History:
DATE         AUTHOR  COMMENT
1-OCT-2001  JCH      Created and written.
2013-05-23  tet      changes to support merging UDP logging into sioLogThread() mostly in open_network()
---------------------------------------------------------------------- */

// 2013-05-23  tet      changes to support merging UDP logging into sioLogThread()
// previously passed a pointer to the nio_thead_t structure now pass a pointer to the port table entry (which is now part of the sio_thread_t)
int open_network (nio_port_table_entry_t * nio_port_table_entry)
{
   
   if ((nio_port_table_entry->my_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
      {
         fprintf (stderr, "NIO THREAD: ***Aborting UDP FROM Port: %s \n", nio_port_table_entry->name_of_thing_this_nio_port_is_connected_to);
         return 0;
      }
   
#ifdef DEBUG_NIO
   printf ("NIO THREAD: got by nio socket call\n");
#endif
   
   
   bzero(&(nio_port_table_entry->MyAddr), sizeof (nio_port_table_entry->MyAddr));	/* Zero out structure */
   
#ifdef DEBUG_NIO
   printf ("NIO THREAD: got by nio memset call\n");
#endif
   
   nio_port_table_entry->MyAddr.sin_family = AF_INET;	/* Internet address family */
   
#ifdef DEBUG_NIO
   printf ("NIO THREAD: got by nio sin family \n");
#endif
   
   nio_port_table_entry->MyAddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* Server IP Address */
   
#ifdef DEBUG_NIO
   printf ("NIO THREAD: about to set port in nio\n");
   printf ("NIO THREAD: socket number is %d\n", nio_port_table_entry->socket_number);
#endif
   nio_port_table_entry->MyAddr.sin_port = htons(nio_port_table_entry->socket_number);	/* Broadcast port */
#ifdef DEBUG_NIO
   printf ("NIO THREAD: about to bind in nio\n");
#endif
   
   /* Bind to the broadcast port */
   int myretCode = bind (nio_port_table_entry->my_sock,
                         (struct sockaddr *) &(nio_port_table_entry->MyAddr), 
                         sizeof (nio_port_table_entry->MyAddr)) ;
   
   
   if(myretCode < 0)
      {
         printf( "Error binding  socket: %s\n", strerror( errno ) );
         fprintf (stderr, "NIO THREAD: ***Aborting UDP Port: %s \n", nio_port_table_entry->name_of_thing_this_nio_port_is_connected_to);
         return 0;
      }
   
   // 4 nov 09  jch
   // add in a capability to not have an address to talk to.  Shouldn't need one for some tasks
   if(strcmp(nio_port_table_entry->ip_address,NULL_IP_ADDRESS))
      {
         
         // now put in the port that we are going to talk to
         nio_port_table_entry->to_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);;
         
         if (nio_port_table_entry->to_sock < 0)
            {
               fprintf (stderr, "NIO THREAD: ***Aborting UDP  TO Port: %s \n", nio_port_table_entry->name_of_thing_this_nio_port_is_connected_to);
               return 0;
            }
         
         
#ifdef DEBUG_NIO
         printf ("NIO THREAD: got by nio socket call\n");
#endif
         // add in broadcast capability
         
         
         int broadcastPermission = 1;
         if(setsockopt(nio_port_table_entry->to_sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,
                       sizeof(broadcastPermission)) < 0)
            {
               fprintf (stderr, "NIO THREAD: ***couldn't set broadcast on UDP : %s \n", nio_port_table_entry->name_of_thing_this_nio_port_is_connected_to);
               return 0;
            }
         
         
         //  printf ("NIO THREAD: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx\n");
         /*
      fprintf(stderr,"NIO THREAD: set broadcast on UDP port %d (%s)\n",
                   nio_port_table_entry->to_socket_number,
                   nio_port_table_entry->name_of_thing_this_nio_port_is_connected_to
                   );
    */
         memset(&(nio_port_table_entry->ToAddr), 0, sizeof (nio_port_table_entry->ToAddr));	/* Zero out structure */
         
#ifdef DEBUG_NIO
         printf ("NIO THREAD: got by nio TO memset call\n");
#endif
         
         nio_port_table_entry->ToAddr.sin_family = AF_INET;	/* Internet address family */
         
#ifdef DEBUG_NIO
         printf ("NIO THREAD: got by nio TO sin family \n");
#endif
         
         nio_port_table_entry->ToAddr.sin_addr.s_addr = inet_addr (nio_port_table_entry->ip_address);	/* Destination IP Address */
         
         
         
         if(INADDR_NONE == nio_port_table_entry->ToAddr.sin_addr.s_addr)
            {
               printf ("NIO THREAD: ERROR Invalid IP Address %s\n",nio_port_table_entry->ip_address);
            }
         //else
         //printf ("NIO THREAD: Valid IP Address %s is OK.\n",nio_port_table_entry->ip_address);
         
#ifdef DEBUG_NIO
         printf ("NIO THREAD: about to set port in nio\n");
         printf ("NIO THREAD: socket number is %d\n", nio_port_table_entry->to_socket_number);
#endif
         
         nio_port_table_entry->ToAddr.sin_port = (nio_port_table_entry->to_socket_number);	/* Broadcast port */
         
#ifdef DEBUG_NIO
         printf ("NIO THREAD: the sin_port is %d\n", nio_port_table_entry->to_socket_number);
#endif
      }
   
   return 1;
}


/* ----------------------------------------------------------------------
   
Function to process incoming records 


Modification History:
DATE         AUTHOR  COMMENT
1-OCT-2001  JCH      Created and written.

---------------------------------------------------------------------- */

static void *
      nio_rx_thread (void *arg)
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
               fprintf (stderr, "nio_rx_thread - error on recvfrom, socket from %s\n", nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to);
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
               nio->total_tx_chars += rcv_size;
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

//   NIO Thread

//   Modification History:
//   DATE         AUTHOR  COMMENT
//   1-OCT-2001  JCH      Created and written.

//----------------------------------------------------------------------*/
void * nio_thread (void *thread_num)
{
   
   nio_thread_t nio = { PTHREAD_MUTEX_INITIALIZER, -1};
   
   char *my_name;
   
   FILE *ini_fd;
   
   int status;
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
   
   
   // now look into the process table and figure out what thread I am
   for (int i = 0; i < MAX_NUMBER_OF_THREADS; i++)
      {
         
         if (global_thread_table[i].thread_num == nio.nio_thread_num)
            {
               
               my_name = global_thread_table[i].thread_name;
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
         fprintf (stderr, "NIO THREAD: thread %d  not found in process table!\n", nio.nio_thread_num);
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   else
      {
         //printf ("NIO THREAD: starting nio thread for %s\n", my_name);
      }
   
   // get the ini file name from the global variable and open the ini file
   
   ini_fd = fopen (dsLogIniFilename, "r");
   if (!ini_fd)
      {
         fprintf (stderr, "NIO THREAD: %s ini file does not exist...exiting!\n", dsLogIniFilename);
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   
   else
      {
#ifdef DEBUG_NIO
         printf ("NIO THREAD: entering ini reading process in %s nio thread\n", my_name);
#endif
         status = read_ini_nio_process (ini_fd, my_name, &nio);
#ifdef DEBUG_NIO
         printf ("NIO THREAD: just left ini reading process in %s nio thread\n", my_name);
#endif
         fclose (ini_fd);
         
         if (status != 0)
            {
               fflush (stdout);
               fflush (stderr);
               abort ();
            }
      }
   
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
   printf ("NIO THREAD: NIO (thread %d) initialized \n", nio.nio_thread_num);
   printf ("NIO THREAD: NIO File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);
   
   // #endif
   
   // ----------------------------------------------------------------------
   // open the network port
   // ----------------------------------------------------------------------
#ifdef DEBUG_NIO
   printf ("NIO THREAD: about to open port\n");
#endif
   nio.nio_port_fid = open_network (& nio.my_nio_port_table_entry);
#ifdef DEBUG_NIO
   printf ("NIO THREAD: got by open port, FID=%d\n",nio.nio_port_fid);
#endif
   
   // launch the thread to process incoming characters from the network port
   pthread_create (&nio.nio_rx_subthread, &DEFAULT_ROV_THREAD_ATTR, nio_rx_thread, (void *) (&nio));
   
   // loop forever
   while (1)
      {
         
         // wait forever on the input channel

#ifdef DEBUG_NIO
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
                           break;
                        }
                  case RAS:		// recieved a RAS (Read Serial String) Message
                        {
                           
                           
                           break;
                        }
                     
                  case SPI:		// recieved a SPI (Status Ping) message
                        {
                           char msg[256];
                           snprintf (msg,256, "NIO THREAD: %s: (thread %d) is Alive!", global_thread_table[nio.nio_thread_num].thread_name, nio.nio_thread_num);
                           break;
                        }
                  case BYE:  // received a bye message--time to give up the ghost--or at least my socket
                        {
                           
                           close(nio.my_nio_port_table_entry.my_sock);
                           return(NULL);
                           
                           
                           break;
                        }
                  default:		// recieved an unknown message type
                        {
                           printf ("NIO THREAD: %s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE -" "got msg type %d from proc %d to proc %d, %d bytes data\n", 
                                   global_thread_table[nio.nio_thread_num].thread_name, 
                                   nio.nio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
                           break;
                        }
                  }
            } // end else
         
      }
   
   
}

/* ----------------------------------------------------------------------
   
read_ini_nio_process

Modification History:
DATE         AUTHOR   COMMENT
1  OCT 2001  JCH  Created and written.
19 Jul 2002  LLW  Revised to transmit as well as receive, presently supports
only one TX destination

---------------------------------------------------------------------- */

//#define DEBUG_CONFIG

int read_ini_nio_process (FILE * ini_fd, char *my_name, nio_thread_t * my_nio)
{
   
   char destination_string[MAX_DESTINATION_STRING_LENGTH];
   char *new_destination;
   int   status;
   
   
   // read in the socket number
   my_nio->my_nio_port_table_entry.socket_number    = rov_read_int    (ini_fd, my_name,  "PORT",          ERRONEOUS_NIO_PORT);
   my_nio->my_nio_port_table_entry.ip_address       = rov_read_string (ini_fd, my_name,  "TO_IP_ADDRESS", ERRONEOUS_IP_ADDRESS);
   my_nio->my_nio_port_table_entry.to_socket_number = rov_read_int    (ini_fd, my_name,  "TO_PORT",       ERRONEOUS_NIO_PORT);
   
    printf ("NIO THREAD: my name is %s\n", my_name);
   printf ("NIO THREAD: ini nio read RX PORT          : %d\n", my_nio->my_nio_port_table_entry.socket_number);
   printf ("NIO THREAD: ini nio read TX TO_IP_ADDRESS : %s\n", my_nio->my_nio_port_table_entry.ip_address);
   printf ("NIO THREAD: ini nio read TX T0_PORT       : %d\n", my_nio->my_nio_port_table_entry.to_socket_number);
   
   
   status = (my_nio->my_nio_port_table_entry.socket_number  == ERRONEOUS_NIO_PORT) ||
            (0 == strcmp( my_nio->my_nio_port_table_entry.ip_address, "ERRONEOUS_IP_ADDRESS")) ||
            ( my_nio->my_nio_port_table_entry.to_socket_number == ERRONEOUS_NIO_PORT);
   
   
#ifdef DEBUG_CONFIG
   printf ("NIO THREAD: status = %d\n", status);
#endif
   
   // now read in the destinations for the nios
   if (status == 0)
      {
         
         //      my_nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to = "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ";
         
         //      my_nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to = 0;
         
         snprintf(my_nio->my_nio_port_table_entry.name_of_thing_this_nio_port_is_connected_to,512,"UDP port %d at IP Address %s",
                 my_nio->my_nio_port_table_entry.to_socket_number,
                 my_nio->my_nio_port_table_entry.ip_address
                 );
         
         for (int i = 0; i < MAX_NIO_DESTINATIONS; i++)
            {
               snprintf (destination_string,MAX_DESTINATION_STRING_LENGTH, "destination_%d", i);
               new_destination = rov_read_string (ini_fd, my_name, destination_string, "NONE");
               
#ifdef DEBUG_CONFIG
               printf ("NIO THREAD: new destination read, it is %s\n", new_destination);
#endif
               
               if (strcmp(new_destination, "NONE"))
                  {			// this means there is a destination thread for this string
                     // now search through the global process table and look for the process name
                     
                     
                     for (int j = 0; j < MAX_NUMBER_OF_THREADS; j++)
                        {
                           if(global_thread_table[j].thread_name)
                              {
#ifdef DEBUG_CONFIG
                                 printf ("NIO THREAD: checking thread name %s for %s\n", global_thread_table[j].thread_name, new_destination);
#endif
                                 
                                 if (0 == strcasecmp (global_thread_table[j].thread_name, new_destination))
                                    {
                                       
                                       my_nio->my_nio_port_table_entry.auto_send_addresses[my_nio->my_nio_port_table_entry.n_of_destinations++] = global_thread_table[j].thread_num;
                                       
                                       /*printf ("NIO THREAD: %s added thread destination %d (%s) total=%d\n", 
				  my_name,
				  global_thread_table[j].thread_num,
				  global_thread_table[j].thread_name,
				  my_nio->my_nio_port_table_entry.n_of_destinations);*/
                                       
                                       
                                       break;
                                    }
                                 else
                                    {
#ifdef DEBUG_CONFIG
                                       printf ("NIO THREAD: Could not find destination %s\n", new_destination);
#endif
                                    }
                              }
                           
                        }
                  }
               
            }
      }
   
   
   return status;
   
   
}

