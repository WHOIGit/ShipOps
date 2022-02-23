/* ----------------------------------------------------------------------

   nio thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   1-OCT-2001   jch      Created and writte from LLW's sio 
   15 FEB 2002  JCH      change for new messaging api

2013-05-30   tt   changed argument of open_network() function
---------------------------------------------------------------------- */
#ifndef NIO_PROCESS_INC
#define NIO_PROCESS_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbose debugging 
// ----------------------------------------------------------------------
//#define DEBUG_NIO
//#define DEBUG_CONFIG
// ----------------------------------------------------------------------

#define BUFFERMAX 512
#define MAX_NIO_DESTINATIONS  9
#define NULL_IP_ADDRESS "99.99.99.99"
#define NULL_SOCKET_NUMBER  99


typedef struct
{
  long int    	      thread_number;
  char   	     *ip_address;
  int                 socket_number;
  int                 to_socket_number;

  int    	      my_sock;   
  struct sockaddr_in  MyAddr;	/* Source Addr  */
  int                 to_sock;
  struct sockaddr_in  ToAddr;	/* Sendto Address */
  char   	      name_of_thing_this_nio_port_is_connected_to[512];
  int    	      n_of_destinations;
  int    	      auto_send_addresses[MAX_NIO_DESTINATIONS];
}
nio_port_table_entry_t;



typedef struct
{
  pthread_mutex_t         mutex;
  int                     nio_thread_num;
  nio_port_table_entry_t  my_nio_port_table_entry;
  int                     nio_port_fid;
  pthread_t               nio_rx_subthread;
  unsigned int            total_rx_chars;
  unsigned int            total_tx_chars;
  char                    inchars[MSG_DATA_LEN_MAX + 1];
  int                     inchar_index;
}
nio_thread_t;

extern void *nio_thread (void *thread_num);
extern int open_network (nio_port_table_entry_t *nio_port_table_entry);

#endif
