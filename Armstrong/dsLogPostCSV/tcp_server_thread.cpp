/* ----------------------------------------------------------------------

tcp thread for configuration transfer

Modification History:
DATE        AUTHOR COMMENT
17 oct 2006  jch    create and write
8 june 2009  jch    fit into jason/alvin code
3 march 2011 jch    modify for navest

---------------------------------------------------------------------- */
/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h> 
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdarg.h>

/* posix header files */
#define  POSIX_SOURCE 1

#include <pthread.h>
#include <termios.h>
#include <unistd.h>



/* jason header files */
//#include "jasontalk.h"		/* jasontalk protocol and structures */
#include "dsLogTalk.h"
#include "dsLogCsv.h"
#include "macros.h"

#include "msg_util.h"		/* utility functions for messaging */

#include "launch_timer.h"

//#include "stderr_thread.h"	/* utility functions for messaging */
#include "tcp_thread.h"		/* TCP thread */
#include "config_file.h"
#include "tcp_utilities.h"
#include "skel.h"

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;

struct ThreadArgs
{
  int clntSock;                      /* Socket descriptor for client */
  im_tcp_thread_t	*my_thread;
};



int open_tcp_server( im_tcp_thread_t * tio )
{
  //struct sockaddr_in local;
  SOCKET s;
  const int on = 1;
  int  sndbufsize = 128*1024;

  s = socket( AF_INET, SOCK_STREAM, 0 );
  if ( !isvalidsock( s ) )
    error( 1, errno, "socket call failed" );
  tio->my_tio_port_table_entry.my_sock = s;
  if ( setsockopt( s, SOL_SOCKET, SO_REUSEADDR,
                   ( char * )&on, sizeof( on ) ) )
    {
      error( 1, errno, "setsockopt failed" );
      return 0;
	  }
	if ( setsockopt( s, SOL_SOCKET, SO_SNDBUF,
									 ( char * )&sndbufsize, sizeof( sndbufsize ) ) )
		{
			error( 1, errno, "setsockopt sndbufsize failed" );
			return 0;
	  }
	if ( setsockopt( s, IPPROTO_TCP, TCP_NODELAY,
									 ( char * )&on, sizeof( on ) ) )
		{
			error( 1, errno, "setsockopt failed no delay" );
			return 0;
	  }

	if ( bind( s, ( struct sockaddr * ) &(tio->my_tio_port_table_entry.MyAddr),
						 sizeof( tio->my_tio_port_table_entry.MyAddr ) ) )
		{
			error( 1, errno, "bind failed" );
			return 0;
	  }

	if ( listen( s, NLISTEN ) )
		{
			error( 1, errno, "listen failed" );
			return 0;
	  }

	return 1;
}


/* ----------------------------------------------------------------------*/

//   tcp Thread

//   Modification History:
//   DATE         AUTHOR  COMMENT
//   1-OCT-2001  JCH      Created and written.

//----------------------------------------------------------------------*/
void * tcp_server_thread (void *thread_num)
{

  im_tcp_thread_t tio = { PTHREAD_MUTEX_INITIALIZER, -1};

  char *my_name;

  FILE *ini_fd;
  SOCKET	s1;
  struct sockaddr_in	peer;
  int	peerlen;
  int status;
  int my_thread_table_entry;
  msg_hdr_t hdr =  {0};
  unsigned char data[MSG_DATA_LEN_MAX] = {0};
  pthread_t threadID;

  struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */
#ifdef DEBUG_TCP
  printf ("TCP THREAD: entering TCP server thread\n\n");
#endif
  /* get my thread number */
  tio.tcp_thread_number = (long int) thread_num;  // 2011-09-26 SS - ported to 64bit
#ifdef DEBUG_TCP
  printf ("TCP THREAD: TCP thread num = %d\n\n", tio.tcp_thread_number);
#endif


  // now look into the process table and figure out what thread I am
  for (int i = 0; i < MAX_NUMBER_OF_THREADS; i++)
    {
      if (global_thread_table[i].thread_num == tio.tcp_thread_number)
        {
          my_name = global_thread_table[i].thread_name;
          my_thread_table_entry = i;
#ifdef DEBUG_TCP
          printf("TCP THREAD: TCP my name is %s\n",  my_name);
#endif  
        }
    }



#ifdef DEBUG_TCP
  printf ("TCP THREAD: made it through thread table search in TCP thread\n\n");
#endif
  if (!my_name)
    {
      fprintf (stderr, "tcp THREAD: thread %d  not found in process table!\n", tio.tcp_thread_number);
      fflush (stdout);
      fflush (stderr);
      abort ();
	  }
	else
		{
			//printf ("tcp THREAD: starting tcp thread for %s\n", my_name);
	  }

	// get the config  file names from the defaults and open the config files

	ini_fd = fopen (dsLogIniFilename, "r");
	if (!ini_fd)
		{
			fprintf (stderr, "TCP THREAD: %s ini file does not exist...exiting!\n", dsLogIniFilename);
			fflush (stdout);
			fflush (stderr);
			abort ();
		}
	else
		{
#ifdef DEBUG_TCP
			printf ("tcp THREAD: entering ini reading process in %s tcp thread\n", my_name);
#endif
			status = read_ini_tcp_server_process (ini_fd, my_name, &tio);
#ifdef DEBUG_TCP
			printf ("tcp THREAD: just left ini reading process in %s tcp thread status = %d\n", my_name,status);
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
	int msg_success = msg_queue_new(tio.tcp_thread_number, my_name);

	if(msg_success != MSG_OK)
		{
			fprintf(stderr, "tcp THREAD: tcp thread : %s\n",MSG_ERROR_CODE_NAME(msg_success) );
			fflush (stdout);
			fflush (stderr);
			abort ();
	  }

	printf ("TCP THREAD: TCP (thread %d) initialized \n", tio.tcp_thread_number);
	printf ("tcp THREAD: tcp File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);

	// now open up the server

	set_address("INADDR_ANY",tio.my_tio_port_table_entry.socket_number, &(tio.my_tio_port_table_entry.MyAddr));
	// wakeup message
	int server_open =  open_tcp_server( &tio);
	tio.my_tio_port_table_entry.tcp_server_connect_timer = launch_timer_new(TCP_CONNECT_INTERVAL,0, tio.tcp_thread_number,  TCPC);
	if(server_open){
			peerlen = sizeof(peer);
			s1 = accept(tio.my_tio_port_table_entry.my_sock, (struct sockaddr * )&peer,(socklen_t *) &peerlen);
			printf(" got a connection in tcp server thread!\n");
			if ( !isvalidsock( s1 ) ){
					error( 0, errno, "accept failed" );
		    }
			if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs)))
		    == NULL){
					error( 0, errno, "malloc failed" );
		    }
			threadArgs->clntSock = s1;
			threadArgs->my_thread = &(tio);


			/* Create client thread */
			if (pthread_create(&threadID, NULL, ThreadMain, (void *) threadArgs) != 0){
					error( 0, errno, "thread creation failed" );
		    }
			tio.my_tio_port_table_entry.connected = TRUE;
			tio.my_tio_port_table_entry.reallyConnected = TRUE;
			//global_connected_table[tio.tcp_thread_number].connected = TRUE;
			tio.my_tio_port_table_entry.to_sock = s1;


	  }

	//tio.my_tio_port_table_entry.tcp_server_connect_timer = launch_timer_new(TCP_CONNECT_INTERVAL,0, tio.tcp_thread_number,  TCPC);


	if(!tio.my_tio_port_table_entry.connected){
			launch_timer_enable(tio.my_tio_port_table_entry.tcp_server_connect_timer,TCP_CONNECT_INTERVAL,-1, tio.tcp_thread_number,  TCPC);
	  }

	// loop forever
	while (1){

			// wait forever on the input channel
#ifdef DEBUG_TCP
			printf ("TCP THREAD: TCP calling get_message.\n");
#endif
			int msg_get_success = msg_get(tio.tcp_thread_number,&hdr, data, MSG_DATA_LEN_MAX);

			if(msg_get_success != MSG_OK){
					fprintf(stderr, "TCP THREAD: TCP thread--error on msg_get:  %s\n",MSG_ERROR_CODE_NAME(msg_get_success));
		    }
			else{

					//#ifdef DEBUG_RECIEVED_MESSAGES
#ifdef DEBUG_TCP
					// print on stderr
					printf ("TCP THREAD: TCP THREAD (thread %d)  got msg type %d from proc %d to proc %d, %d bytes data\n", tio.tcp_thread_number, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
					// process new message
					switch (hdr.type){
					case TCPC:{

								printf(" got a TCPC!!!!\n");
								printf(" the connected flag: %d\n",tio.my_tio_port_table_entry.connected);
								printf(" the reallyConnected flag: %d\n",tio.my_tio_port_table_entry.reallyConnected);

#if 1
								if(!tio.my_tio_port_table_entry.connected){
										printf(" not connected, accept another connection\n");
										launch_timer_disable(tio.my_tio_port_table_entry.tcp_server_connect_timer);
										s1 = accept(tio.my_tio_port_table_entry.my_sock, (struct sockaddr * )&peer,(socklen_t *) &peerlen);
										printf(" got a second connection!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
										if ( !isvalidsock( s1 ) ){
												error( 0, errno, "accept failed" );
											}
										if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL){
												error( 0, errno, "malloc failed" );
											}
										threadArgs->clntSock = s1;
										tio.my_tio_port_table_entry.to_sock = s1;
										threadArgs->my_thread = &(tio);
										/* Create client thread */
										if (pthread_create(&threadID, NULL, ThreadMain, (void *) threadArgs) != 0){
												error( 0, errno, "thread creation failed" );
											}
										tio.my_tio_port_table_entry.connected = TRUE;


									}
								else{
										printf(" still connected, dont do anything\n");

									}
#endif
								break;

							}
					case WAS:{		// recieved a WAS (Write Serial String) Message

								// send the characters to the port, keep stats
								// note that WAS messages need NOT be null terminated
								if (hdr.length > 0){
										int bytes_sent=0;
										// lock the mutex
										pthread_mutex_lock (&tio.mutex);
										// send the bytes // have to change the send here
#if 0
										bytes_sent = sendto(nio.my_nio_port_table_entry.to_sock,
																				data,
																				hdr.length,
																				0,
																				(struct sockaddr *) (&(nio.my_nio_port_table_entry.ToAddr)),
																				sizeof (tio.my_nio_port_table_entry.ToAddr));
#endif
										if (bytes_sent == hdr.length){

												// keep stats
												tio.total_tx_chars += hdr.length;

											}
										else{
												fprintf (stderr, "TCP THREAD: ERROR sending %d byte WAS to %s port %d - %d bytes sent.\n",
																 hdr.length,
																 tio.my_tio_port_table_entry.ip_address,
																 tio.my_tio_port_table_entry.to_socket_number,
																 bytes_sent
																 );

												fprintf (stderr, "TCP THREAD: Data is: \"%*s\"\n", hdr.length, data);

											}

										// unlock the mutex
										pthread_mutex_unlock (&tio.mutex);
									}

								break;
							}
					case INIR:{
								int bytes_sent=0;
								printf(" received ini request in tcp server thread!\n");
								char my_line[MAX_CHARACTER_COUNT], *ch;

								// first count the bytes in the file
								int fileSize;
								printf(" about to open the file\n");
								ini_fd = fopen (dsLogIniFilename, "r");

								if(!ini_fd){
										break;
									}
								fseek(ini_fd,0,SEEK_END);
								fileSize = ftell(ini_fd);
								fseek(ini_fd,0,SEEK_SET);
								int totalBytesSent =0;
								int line = 0;
								int len;
								while (!feof (ini_fd))
									{
										ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, ini_fd);
										if (ch){
												len = strlen(&(my_line[0]));
												bytes_sent = sendto(tio.my_tio_port_table_entry.to_sock,
																						&(my_line[0]),
																						len,
																						0,
																						(struct sockaddr *) (&(tio.my_tio_port_table_entry.ToAddr)),
																						sizeof (tio.my_tio_port_table_entry.ToAddr));
												totalBytesSent += bytes_sent;
												line++;
												//printf(" line %d bytes sent = %d total bytes = %d\n",line,bytes_sent,totalBytesSent);
											}
									}
								fclose(ini_fd);
								len = snprintf(&(my_line[0]),MAX_CHARACTER_COUNT,"END_OF_DSLOG_INI_FILE");
								bytes_sent = sendto(tio.my_tio_port_table_entry.to_sock,
																		&(my_line[0]),
																		len,
																		0,
																		(struct sockaddr *) (&(tio.my_tio_port_table_entry.ToAddr)),
																		sizeof (tio.my_tio_port_table_entry.ToAddr));


								break;
							}

					case SAS:{			// received a SAS, probably from my subthread
#ifdef DEBUG_TCP
								printf ("TCP THREAD: %s: (thread %d) TCP RECIEVED SAS -" "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[tio.tcp_thread_number].thread_name, tio.tcp_thread_number, hdr.type, hdr.from, hdr.to, hdr.length);
#endif
								break;
							}

					case SPI:{		// recieved a SPI (Status Ping) message

								char msg[256];
								snprintf (msg, 256,"TCP THREAD: %s: (thread %d) is Alive!", global_thread_table[tio.tcp_thread_number].thread_name,tio.tcp_thread_number);
								break;
							}
					case BYE:{  // received a bye message--time to give up the ghost--or at least my socket


								return(NULL);

								break;
							}
					default:{		// recieved an unknown message type

								printf ("TCP THREAD: %s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE -" "got msg type %d from proc %d to proc %d, %d bytes data\n",
												global_thread_table[tio.tcp_thread_number].thread_name,
												tio.tcp_thread_number, hdr.type, hdr.from, hdr.to, hdr.length);
								break;
							}
					}
		    } // end else

		}


}

/* ----------------------------------------------------------------------

read_ini_TCP_process

Modification History:
DATE         AUTHOR   COMMENT
1  OCT 2001  JCH  Created and written.
19 Jul 2002  LLW  Revised to transmit as well as receive, presently supports
only one TX destination

---------------------------------------------------------------------- */

//#define DEBUG_CONFIG

int read_ini_tcp_server_process (FILE * ini_fd, char *my_name, im_tcp_thread_t * my_tio)
{

  int status = 1;
  my_tio->my_tio_port_table_entry.socket_number    = rov_read_int    (ini_fd, my_name,  "TCP_SERVER_THREAD_SOCKET",          ERRONEOUS_NIO_PORT);
  //my_tio->my_tio_port_table_entry.ip_address       = rov_read_string (ini_fd, my_name,  "TO_IP_ADDRESS", ERRONEOUS_IP_ADDRESS);
  //my_tio->my_tio_port_table_entry.to_socket_number = rov_read_int    (ini_fd, my_name,  "TO_PORT",       ERRONEOUS_NIO_PORT);
  // printf(" the port:  %d\n",my_tio->my_tio_port_table_entry.socket_number);
  status = (my_tio->my_tio_port_table_entry.socket_number  == ERRONEOUS_NIO_PORT) ;


#ifdef DEBUG_TCP
  printf ("TIO server THREAD: status on leaving ini read= %d\n", status);
#endif
  return status;


}

void *ThreadMain(void *threadArgs)
{
  int clntSock;                   /* Socket descriptor for client connection */

  /* Guarantees that thread resources are deallocated upon return */
  pthread_detach(pthread_self());

  /* Extract socket file descriptor from argument */
  clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;
  im_tcp_thread_t	*the_thread;
  the_thread = ((struct ThreadArgs *) threadArgs) -> my_thread;

  free(threadArgs);              /* Deallocate memory for argument */

  // now listen for new data on the port

  unsigned char	buf[1024];
  int rc;
  while(1){
      rc = recv(clntSock,buf,sizeof(buf),0);
      printf(" rc = %d\n",rc);
      if(rc == 0){
          printf(" ############client disconnected!##################\n");
          //printf(" the thread number is %d\n",the_thread->tcp_thread_number);
          //the_thread->my_tio_port_table_entry.connected = FALSE;
          //global_connected_table[the_thread->tcp_thread_number].connected = FALSE;
          // enable the timer
          printf(" enabled the timer to send a TCPC\n");
          launch_timer_enable(the_thread->my_tio_port_table_entry.tcp_server_connect_timer,TCP_CONNECT_INTERVAL,-1, the_thread->tcp_thread_number,  TCPC);

          the_thread->my_tio_port_table_entry.connected = FALSE;
          return (NULL);
		    }
			if(rc == -1){
					printf(" got error on port:  \n");
					//global_connected_table[the_thread->tcp_thread_number].connected = FALSE;
					// enable the timer
					//printf(" enabled the timer to send a TCPC\n");
					launch_timer_enable(the_thread->my_tio_port_table_entry.tcp_server_connect_timer,TCP_CONNECT_INTERVAL,-1, the_thread->tcp_thread_number,  TCPC);
					the_thread->my_tio_port_table_entry.connected = FALSE;
					return (NULL);



		    }
			else{
					printf(" got data:  %d bytes\n",rc);
					// tear it apart


					buf[rc] = '\0';
					printf(" the data is %s\n",buf);

					if(!strncmp((char *)buf,"REQUEST_INI",11)){
							printf(" ini file requested!\n");
							printf(" thread numbers:  %d\n",the_thread->tcp_thread_number);
							msg_send(the_thread->tcp_thread_number, the_thread->tcp_thread_number,INIR,0,NULL);
							printf(" the msg is sent!\n");
						}
					printf(" about to launch the timer...\n");
					if(the_thread->my_tio_port_table_entry.tcp_server_connect_timer){
							//printf(" the timer is not null\n");
							launch_timer_enable(the_thread->my_tio_port_table_entry.tcp_server_connect_timer,TCP_CONNECT_INTERVAL,-1, the_thread->tcp_thread_number,  TCPC);
						}
					//printf(" thread timer  enabled!\n");
					the_thread->my_tio_port_table_entry.connected = FALSE;
					the_thread->my_tio_port_table_entry.connected = FALSE;
					the_thread->my_tio_port_table_entry.connected = FALSE;
					the_thread->my_tio_port_table_entry.connected = FALSE;
					the_thread->my_tio_port_table_entry.connected = FALSE;
					the_thread->my_tio_port_table_entry.reallyConnected = FALSE;


					printf("in thread main, connected = %d\n",the_thread->my_tio_port_table_entry.connected);
					return (NULL);




		    }
	  }

}

