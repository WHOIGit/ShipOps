/* ----------------------------------------------------------------------

   TCP thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   1-OCT-2001   jch      Created and writte from LLW's sio 
   15 FEB 2002  JCH      change for new messaging api

---------------------------------------------------------------------- */
#ifndef TCP_PROCESS_INC
#define TCP_PROCESS_INC

#define MAX_NUMBER_OF_TCP_THREADS   16

// ----------TCP_STATUS_THREAD------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbose debugging 
// ----------------------------------------------------------------------
#define DEBUG_TCP
//#define DEBUG_CONFIG
// ----------------------------------------------------------------------

//#define TCP_BUFFERMAX 512
#define MAX_NIO_DESTINATIONS  9

#define TCP_CONNECT_INTERVAL	1.0
#include "launch_timer.h"

typedef struct
{
  int    	      thread_number;
  char   	     *ip_address;
  int                 socket_number;
  int                 to_socket_number;

  int    	      my_sock;   
  struct sockaddr_in  MyAddr;	/* Source Addr  */
  int                 to_sock;
  struct sockaddr_in  ToAddr;	/* Sendto Address */
  
  
  int    	      auto_send_addresses[MAX_NIO_DESTINATIONS];
  int					connected;
  int					reallyConnected;
  launched_timer_data_t * tcp_server_connect_timer;
}
tio_port_table_entry_t;

typedef struct {
	
	int	connected;
}	tcp_connected_t;

typedef struct {
   pthread_mutex_t         mutex;
	int	verbosity;
	tio_port_table_entry_t  my_tio_port_table_entry;
	int	tcp_thread_number;
	int	in_socket_number;
	int   tio_port_fid;
	int	socket;
	unsigned int            total_tx_chars;
} im_tcp_thread_t;



extern void *tcp_server_thread (void *thread_num);
extern void *ThreadMain(void *threadArgs);

extern int read_ini_tcp_server_process (FILE * ini_fd, char *my_name, im_tcp_thread_t * my_nio);
extern char *rov_ini_file_name;
extern tcp_connected_t	global_connected_table[MAX_NUMBER_OF_TCP_THREADS];
#endif
