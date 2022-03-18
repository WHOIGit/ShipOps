/* ----------------------------------------------------------------------
   
   Jason Message Passing Utility Functions External Declarations.

   Functions to be used for all message sending and recieving.  The
   programmer should exclusively use these functions for message passing,
   thus allowing us to tranparently upgrade the message passing layer in
   the future.  

   Modification History: 
   DATE AUTHOR          COMMENT 
   06-OCT-1992 LLW      Created and written.
   23-OCT-1992 LEF      Changed defs for send and receive msg functions to
                        add header.
   11-MAR-2000 LLW      Ported to posix
   10-JUL-2000 LLW      Ported to Linux
                        uses __WIN32__ and __LINUX__ precompiler #defines
   06-FEB-2002 LLW      Revised message passing API to simplify.
                        Channels no longer exist.
   11-FEB-2002 LLW      Minor spec revisions
   13-FEB-2002 LLW      Works
   13-FEB-2002 LLW      Added message broadcast

---------------------------------------------------------------------- */
#ifndef MSG_INCLUDE
#define MSG_INCLUDE

/*  ---------------------------------------------------------------------- */
/*  DEBUG FLAG:  Uncomment this and recompile to get verbose debugging     */
/*  ---------------------------------------------------------------------- */
//#define DEBUG_MSG_UTIL


/* ----------------------------------------------------------------------
   message_header_t: Complete header for interprocess communication.
   Containing from and to adresses, type of message, and size of
   variable-length data which follows the header.  The structure of the
   variable length data`s content is determined by the message type.
   ---------------------------------------------------------------------- */
typedef int msg_addr_t;
typedef int msg_type_t;
typedef int msg_len_t;

typedef struct
{
  msg_addr_t  to;
  msg_addr_t  from;
  msg_type_t  type;
  msg_len_t   length;
}
msg_hdr_t;


/* ---------------------------------------------------------------------- */
/* some useful message related constants                                  */
/* ---------------------------------------------------------------------- */
#define MSG_ADDRESS_MAX   1024
#define MSG_ADDRESS_MIN   0

#define MSG_TYPE_MAX      USHRT_MAX /* defined in <limits.h> */
#define MSG_TYPE_MIN      0

#define MSG_DATA_LEN_MAX  USHRT_MAX /* defined in <limits.h> */
#define MSG_DATA_LEN_MIN  0

/*  ---------------------------------------------------------------------- */
/*  message function return status error codes */
/*  ---------------------------------------------------------------------- */
#define MSG_OK         	     0   /* operation completed OK */
#define MSG_ERR_SEND_SIZE   -1	 /* attempt to send message data too large for message system */
#define MSG_ERR_GET_SIZE    -2	 /* attempt to read mesage data too large for output buffer */
#define MSG_ERR_ALLOC  	    -3	 /* error allocating memory for incoming message */
#define MSG_ERR_FREE   	    -4	 /* error freeing memory for outgoing message */
#define MSG_ERR_ADDR_INUSE  -5   /* request for new message address failed - address already in use */
#define MSG_ERR_ADDR_UNUSED -6   /* request for new message address failed - address already in use */
#define MSG_ERR_ADDR_NFG    -7   /* request to free a message address failed - address does not exist */
#define MSG_ERR_OTHER       -8   /* other unspecified error */
#define MSG_ERR_UNKNOWN     -9   /* unknown error */
#define MSG_ERR_WRITE      -10   /* error writing a message to a queue */
#define MSG_ERR_READ       -11   /* error reading a message from a queue */
#define MSG_ERR_QUEUE_FULL -12   /* message queue is full, msg_send() failed, message dropped */


/* ----------------------------------------------------------------------
   char * MSG_ERROR_CODE_NAME(int)
 
   Accepts an integer argument and returns a pointer to a char string 
   containing the ascii name of the error code.  Useful when printing
   out error codes.

   Modification History: 
   06-FEB-2001 LLW      Created.
   ---------------------------------------------------------------------- */
extern const char * MSG_ERROR_CODE_NAME(int error_code);  


/* ----------------------------------------------------------------------

   initializes the queue table

   need to call once before using any msg queue functions 


   Modification History: 
   12-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */
extern int msg_queue_initialize(void);


/* ----------------------------------------------------------------------
   int msg_queue_new(int, const char *)
  
   Takes an integer address and an ascii name and allocates a message queue
   with the specified integer address.

   Present implementatin limits addresses to unsigned shorts.

   The address is important and should be unique system-wide.

   The ascii name is not critical - it is  only used for newsy error 
   messages, and can be NULL.

   If you ask to allocate an already existing, you get a bad status
   MSG_ERR_ADDR_INUSE but no harm is done.

   Modification History: 
   06-FEB-2002 LLW      Created.
   ---------------------------------------------------------------------- */
extern int msg_queue_new (int    my_queue_number,
			  const char * my_ascii_name);


/* ----------------------------------------------------------------------
   int msg_queue_free(int)
  
   Takes an integer address frees (de-allocates) the message queue
   with the specified integer address.

   Present implementatin limits addresses to unsigned shorts.

   The address is important and should be unique system-wide.

   If you de-allocate a non-existent mailbox queue, you get a bad status
   MSG_ERR_ADDR_NFG but no harm is done.
 
   Modification History: 
   06-FEB-2002 LLW      Created.
   ---------------------------------------------------------------------- */
extern int msg_queue_free(int);


/* ----------------------------------------------------------------------
   int msg_send()
 
   sends a message. Two different calling conventions.

   Returns zero on success

   Returns negative number if error - see header file for details.

   Returns MSG_ERR_SEND_SIZE, and drops the message, if message data
   exceeds message queue buffer size,

   Returns MSG_ERR_QUEUE_FULL, and drops the message, if the message
   queue is full.


   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   06-OCT-1992  LLW     Created and written.
   11-MAR-2000  LLW     Ported to posix
   25-MAR-2000  LLW     Cleaned up sloppy code
   06-FEB-2002  LLW     Revised to simplify and omit channel structures.
   13-FEB-2002  LLW     Works.

   ---------------------------------------------------------------------- */
extern int msg_send(msg_hdr_t * hdr, const void * data);

extern int msg_send (msg_addr_t to,
		     msg_addr_t from,
		     msg_type_t type, 
		     msg_len_t  length, 
		     const void *data);



/* ----------------------------------------------------------------------
   int msg_send_broadcast()
 
   sends a message to all active queues. 

   Returns zero on success

   Returns negative number if error - see header file for details.

   Returns MSG_ERR_SEND_SIZE, and drops the message, if message data
   exceeds message queue buffer size,

   Returns MSG_ERR_QUEUE_FULL, and drops the message, if the message
   queue is full.


   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   14-FEB-2002  LLW     Created and written.

   ---------------------------------------------------------------------- */
extern int msg_send_broadcast(msg_hdr_t * hdr, const void * data);

extern int msg_send_broadcast(msg_addr_t from,
			      msg_type_t type, 
			      msg_len_t  length, 
			      const void *data);


/* ----------------------------------------------------------------------
   int msg_get()
 
   Gets a message.  

   Blocks forever if no message is waiting.

   Returns zero on success, with message details in hdr structure.

   Returns negative number if error - see header file for details.

   Returns MSG_ERR_GET_SIZE if message data exceeds output buffer size,
   data is not valid.


   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   06-OCT-1992  LLW     Created and written.
   11-MAR-2000  LLW     Ported to posix
   25-MAR-2000  LLW     Cleaned up sloppy code
   06-FEB-2002  LLW     Revised to simplify and omit channel structures.
   13-FEB-2002  LLW     Works.

   ---------------------------------------------------------------------- */
extern int msg_get(msg_addr_t  my_queue_number,
		   msg_hdr_t * hdr, 
                   void      * data, 
                   msg_len_t   max_size);



/* ----------------------------------------------------------------------

   Message que statistics data structure

   Modification History:
   DATE         AUTHOR  COMMENT
   11-FEB-2002  LLW     Created and written.

   ---------------------------------------------------------------------- */

typedef struct 
{
  unsigned long msg_count_in;
  unsigned long msg_count_in_last;
  unsigned long msg_count_out;
  unsigned long msg_count_dropped;

  unsigned long msg_bytes_in;
  unsigned long msg_bytes_in_last;
  unsigned long msg_bytes_out;
  unsigned long msg_bytes_dropped;

  int           msgs_in_queue_now;
  int           msgs_in_queue_highwater_mark;
  int           msg_max_size;

  int           num_active_queues;

  int           msgs_in_per_sec;
  int           bytes_in_per_sec;

} msg_queue_stats_t;



/* ----------------------------------------------------------------------
   msg_queue_stats_t msg_get_queue_stats(int queue_num)
 
   Gets statistics on the message queue.  

   This will normally only be used for debugging.

   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   11-FEB-2002  LLW     Created and written.

   ---------------------------------------------------------------------- */
extern int  msg_get_queue_stats(msg_addr_t queue_queue_number,
				msg_queue_stats_t * stats);


/* ----------------------------------------------------------------------
   int msg_get_queue_stats_all(msg_queue_stats * stats);  
 
   Gets total statistics on all the message queues.

   This will normally only be used for debugging.

   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   11-FEB-2002  LLW     Created and written.

   ---------------------------------------------------------------------- */
extern int  msg_get_queue_stats_all(msg_queue_stats_t * stats);



/* ----------------------------------------------------------------------

   message queue stats one hertz update

   Call this guy at one hertz

   Modification History: 
   12-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */
extern void msg_queue_stats_one_hertz_update(void);


/* ----------------------------------------------------------------------

   msg stats sprintf

   Call this guy at one hertz

   Modification History: 
   13-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */
extern int msg_stats_sprintf(msg_queue_stats_t stats, char * str);


#endif
