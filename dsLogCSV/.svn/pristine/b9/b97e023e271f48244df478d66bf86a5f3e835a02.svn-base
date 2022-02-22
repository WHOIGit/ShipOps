/* ----------------------------------------------------------------------

   Sio thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000  LLW      Created and written.
   19 Feb 2002   LLW     cleaned up obsolete message #define refs
   2013-05-21   TT       add include of nio_thread.h
                         add nio_port_table_entry to sio_thread_t
                         for integration of network ports into sio_thread.cpp

---------------------------------------------------------------------- */

#include "msg_util.h"
#include "nio_thread.h"

#ifndef SIO_PROCESS_INC
#define SIO_PROCESS_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbose debugging 
// ----------------------------------------------------------------------
//#define DEBUG_SIO
// ----------------------------------------------------------------------


#define MAX_SIO_DESTINATIONS  9


// these constands are for convenience when setting parity, use only one
#define SIO_PARITY_NONE  0
#define SIO_PARITY_ODD   1
#define SIO_PARITY_EVEN  2
#define SIO_PARITY_MARK  3	/* not presently supported, defaults to NONE */
#define SIO_PARITY_SPACE 4	/* not presently supported, defaults to NONE */

// these for setting flow control, can be 0, 1, 2, or 
#define SIO_FLOW_CONTROL_NONE           0
#define SIO_FLOW_CONTROL_XONXOFF        1
#define SIO_FLOW_CONTROL_RTSCTS         2
#define SIO_FLOW_CONTROL_XONOFF_RTSCTS  3






typedef struct
{
  long int thread_number;
  char *sio_com_port_name;
  char *name_of_thing_this_com_port_is_connected_to;
  int baud;
  int parity;
  int data_bits;
  int stop_bits;
  int flow_control;
  int echo;
  int n_of_destinations;
  int auto_mux_address[MAX_SIO_DESTINATIONS];	// -1 for no auto-muxing
  char auto_mux_term_char;	// -1 for any character is termination character
  // no, its not, (21 Dec 01) check the following switch instead
  int auto_mux_enabled;		// if this is set to 1 (true) then check the term character

}
sio_port_table_entry_t;

typedef struct
{
  pthread_mutex_t mutex;
  int sio_thread_num;
  nio_port_table_entry_t  my_nio_port_table_entry;
  sio_port_table_entry_t my_sio_port_table_entry;
  int sio_port_fid;
  pthread_t sio_rx_subthread;
  int wsx_mode;			// nonzero when in WSX mode
  char wsx_mode_term_char;	// normally '~'
  char wsx_mode_mux_address;
  unsigned int total_rx_chars;
  unsigned int total_tx_chars;
  char inchars[MSG_DATA_LEN_MAX + 1];
  int inchar_index;
}
sio_thread_t;


extern void *sio_thread (void *thread_num);
extern int open_serial (sio_thread_t * sio);

// externed here, declared in dsLog.h
extern sio_port_table_entry_t global_sio_port_table[];

#endif
