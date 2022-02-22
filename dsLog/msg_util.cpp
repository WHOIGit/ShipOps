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
			Using pipes.
			Did not use System V ipc message queues becasue
                        of implementation limits.  See "man msgop".

   12-FEB-2002 LLW      Implemented revised API
   13-FEB-2002 LLW      Replaced mutexes with semaphores.
   13-FEB-2002 LLW      Replaced pipes with simple static shared memory
                        rung buffer.  Not optimized for efficient
                        memory use at present.
   16-FEB-2002 JCH      fixed <= to < in initilaizing msg queue
   17-Feb-2002 JCH      put in msg_send vice msg_send with two arguments
                        the other wasnt working, dont know why

---------------------------------------------------------------------- */

// standard ansi C header files 
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

// for anonymous pipes
#include <unistd.h>
#include <fcntl.h>

// pthread header files
#ifndef __WIN32__
  #include <pthread.h>
  #include <semaphore.h>
#else
  #include "pthread.h"
  #include "implement.h"
  #include <semaphore.h>
#endif

// local includes
#include "msg_util.h"
#include "macros.h"
#include "dsLogTalk.h"


// ----------------------------------------------------------------------
// define type for message queue queue table entry
// ----------------------------------------------------------------------
#define MSG_QUEUE_DEPTH 80

typedef struct
{
  sem_t              access_semaphore;
  sem_t              msg_count_semaphore;
  int                active;
  char               name[24];
  int                index_in;
  int                index_out;
  msg_hdr_t       *  hdr;
  unsigned char   *  data;
  msg_queue_stats_t  stats;  
}
queue_entry_t;

// declare the queue table, initialize to zero
queue_entry_t  queue_table[MSG_ADDRESS_MAX];


/* ----------------------------------------------------------------------

   initializes the queue table

   need to call once before using any msg queue functions 


   Modification History: 
   12-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */

int msg_queue_initialize( void )
{
  int num;
  msg_queue_stats_t zero_stats = {0};

  // initialize messahe queue structs
  for(num=MSG_ADDRESS_MIN; num<MSG_ADDRESS_MAX; num++)
    {
      int status; 

      // initialize the access semaphore to a nonzero value
      status = sem_init (&queue_table[num].access_semaphore, 0, 1);

      if(status != 0)
	{
	  fprintf(stderr, "MSG_UTIL: Error initializing message table entry number %d, status = %d\n", num, status);
	  fflush(stdout);
	  return MSG_ERR_ALLOC;
	}

      // initialize the count semaphore to a zero value
      status = sem_init (&queue_table[num].msg_count_semaphore, 0, 0);

      if(status != 0)
	{
	  fprintf(stderr, "MSG_UTIL: Error initializing message table entry number %d, status = %d\n", num, status);
	  fflush(stdout);
	  return MSG_ERR_ALLOC;
	}

      // init the rest of the data
      queue_table[num].active                    = 0;
      strcpy(queue_table[num].name, "inactive");
      queue_table[num].index_in                  = 0;
      queue_table[num].index_out                 = 0;
      queue_table[num].stats           = zero_stats;
      queue_table[num].hdr                       = NULL;
      queue_table[num].data                      = NULL;     

    } 

#ifdef DEBUG_MSG_UTIL
  fprintf(stderr,"MSG_UTIL: Initialized message table with %d entries OK., \n", num);
#endif

  return MSG_OK;
}




/* ----------------------------------------------------------------------
   char * MSG_ERROR_CODE_NAME(int)
 
   Accepts an integer argument and returns a pointer to a char string 
   containing the ascii name of the error code.  Useful when printing
   out error codes.

   Modification History: 
   12-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */
const char * MSG_ERROR_CODE_NAME(int error_code)
{
    const char * name;

#define str(flag) case flag : name = #flag; break

  switch(error_code)
    {
      str( MSG_OK              );
      str( MSG_ERR_SEND_SIZE   );
      str( MSG_ERR_GET_SIZE    );
      str( MSG_ERR_ALLOC       );
      str( MSG_ERR_FREE        );
      str( MSG_ERR_ADDR_INUSE  );
      str( MSG_ERR_ADDR_UNUSED );
      str( MSG_ERR_ADDR_NFG    );
      str( MSG_ERR_OTHER       );
      str( MSG_ERR_READ        );
      str( MSG_ERR_WRITE       );
      str( MSG_ERR_QUEUE_FULL  );
    default:
      str( MSG_ERR_UNKNOWN     );
    }

  return name;

}


/* ----------------------------------------------------------------------
   int msg_queue_new(int, char *)
  
   Takes an integer address and an ascii name and allocates a message queue
   with the specified integer address.

   Present implementatin limits addresses to unsigned shorts.

   The address is important and should be unique system-wide.

   The ascii name is not critical - it is  only used for newsy error 
   messages, and can be NULL.

   If you ask to allocate an already existing, you get a bad status
   MSG_ERR_ADDR_INUSE but no harm is done.

   Modification History: 
   06-FEB-2002 LLW      Created and written.
   ---------------------------------------------------------------------- */
int msg_queue_new (int  my_queue_number,
		   const char * my_ascii_name)
{


  // check queue number to ensure it is in valid range
  if (0 == inrange(my_queue_number, MSG_ADDRESS_MIN, MSG_ADDRESS_MAX) )
    return MSG_ERR_ADDR_NFG;

  // if queue is already initialized, exit with an error
  if( queue_table[my_queue_number].active != 0)
    return MSG_ERR_ADDR_INUSE;

  // wait on the semaphore to get exclusive access to the data struct
  sem_wait(&queue_table[my_queue_number].access_semaphore);

  // allocate the static queues
  queue_table[my_queue_number].hdr = (msg_hdr_t *) malloc(MSG_QUEUE_DEPTH * sizeof(msg_hdr_t));

  // if memory allocation error, set return code accordingly
  if( queue_table[my_queue_number].hdr == NULL)
    return MSG_ERR_ALLOC;

  // allocate the static queues
  queue_table[my_queue_number].data = (unsigned char *) malloc(MSG_QUEUE_DEPTH * MSG_DATA_LEN_MAX);

  // if memory allocation error, set return code accordingly
  if( queue_table[my_queue_number].hdr == NULL)
    return MSG_ERR_ALLOC;

  // if memory allocation OK, setup stuff
  queue_table[my_queue_number].active = 1;    
  if( my_ascii_name != NULL)
    strcpy(queue_table[my_queue_number].name, my_ascii_name);
  else
    strcpy(queue_table[my_queue_number].name, "No name for this queue!");

  // post the semaphore to release exclusive access to the data struct
  sem_post(&queue_table[my_queue_number].access_semaphore);

  return MSG_OK;
}


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
int msg_queue_free(int my_queue_number)
{
  int status;

  // check queue number to ensure it is in valid range
  status =  inrange(my_queue_number, MSG_ADDRESS_MIN, MSG_ADDRESS_MAX);

  if (status == 0)
    return MSG_ERR_ADDR_NFG;

  // if queue is NOT already initialized, exit with an error
  if( queue_table[my_queue_number].active == 0)
    return MSG_ERR_ADDR_UNUSED;

  // wait on the semaphore to get exclusive access to the data struct
  sem_wait(&queue_table[my_queue_number].access_semaphore);

  // free the file descriptors
  //  status = close(queue_table[my_queue_number].filedes[0]);

  // if pipe close error, set return code accordingly
  if( status != 0)
    return MSG_ERR_FREE;

  // free the malloced memory
  free(queue_table[my_queue_number].hdr);
  free(queue_table[my_queue_number].data);

  // clean up table entry
  strcpy(queue_table[my_queue_number].name, "inactive and deallocated");
  queue_table[my_queue_number].active = 0;    

  // post the semaphore to release exclusive access to the data struct
  sem_post(&queue_table[my_queue_number].access_semaphore);

  return MSG_OK;

}


/* ----------------------------------------------------------------------
   int msg_send()
 
   sends a message. 

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
int msg_send(msg_hdr_t * hdr, const void * data)
{

  // check to see if this is a dev_null address
  
  if(hdr->to == NULL_THREAD_ADDRESS)
      {
    return MSG_OK;
  }
  
  // check queue number to ensure it is in valid range
  if (0 == inrange(hdr->to, MSG_ADDRESS_MIN, MSG_ADDRESS_MAX))
    {
      
#ifdef DEBUG_MSG_UTIL
      printf("MSG_SEND(): Dropping msg from %3d, to %3d, type %3d, length %6d, "
	     "data=%10.10s, status=%s\n", 
	     hdr->from, hdr->to, hdr->type, hdr->length, (char *) data, 
	     MSG_ERROR_CODE_NAME(MSG_ERR_ADDR_NFG)
	     );

#endif
  
    return MSG_ERR_ADDR_NFG;
    }


  // if queue is NOT already initialized, exit with an error
  if( queue_table[hdr->to].active == 0)
    {
      
#ifdef DEBUG_MSG_UTIL
      printf("MSG_SEND(): Dropping msg from %3d, to %3d, type %3d, length %6d, "
	     "data=%10.10s, status=%s\n", 
	     hdr->from, hdr->to, hdr->type, hdr->length, (char *) data, 
	     MSG_ERROR_CODE_NAME(MSG_ERR_ADDR_UNUSED)
	     );

#endif
  
    return MSG_ERR_ADDR_UNUSED;

    }



  // check to see if the data can fit in the queue
  if( hdr->length > MSG_DATA_LEN_MAX)
    {
      
#ifdef DEBUG_MSG_UTIL
      printf("MSG_SEND(): Dropping msg from %3d, to %3d, type %3d, length %6d, "
	     "data=%10.10s, status=%s\n", 
	     hdr->from, hdr->to, hdr->type, hdr->length, (char *) data, 
	     MSG_ERROR_CODE_NAME(MSG_ERR_SEND_SIZE)
	     );

#endif
  
      return MSG_ERR_SEND_SIZE;

    }

  // wait on the semaphore to get exclusive access to the data struct
  sem_wait(&queue_table[hdr->to].access_semaphore);

  // check to see if there is room in the queue
  if(( (queue_table[hdr->to].index_in+1) % MSG_QUEUE_DEPTH) 
     == queue_table[hdr->to].index_out)
    { // queue is full, so drop the message, update stats, and return error status
      // update stats
      queue_table[hdr->to].stats.msg_count_in++;
      queue_table[hdr->to].stats.msg_count_dropped++;
      queue_table[hdr->to].stats.msg_bytes_dropped += sizeof(*hdr) + hdr->length;

      // post the semaphore to release exclusive access to the data struct
      sem_post(&queue_table[hdr->to].access_semaphore);

#ifdef DEBUG_MSG_UTIL
      printf("MSG_SEND(): Dropping msg from %3d, to %3d, type %3d, length %6d, "
	     "data=%10.10s, status=%s\n", 
	     hdr->from, hdr->to, hdr->type, hdr->length, (char *) data, 
	     MSG_ERROR_CODE_NAME(MSG_ERR_QUEUE_FULL)
	     );
#endif
  
      return MSG_ERR_QUEUE_FULL;
    }

  // copy the header to the queue
  queue_table[hdr->to].hdr[queue_table[hdr->to].index_in] = *hdr;  

  // copy the data to the queue
  if(hdr->length > 0)
    memcpy( &(queue_table[hdr->to].data[queue_table[hdr->to].index_in * MSG_DATA_LEN_MAX]),
	    data,
	    hdr->length);

  // update the pointers 
  queue_table[hdr->to].index_in = (queue_table[hdr->to].index_in +1) % MSG_QUEUE_DEPTH;
 
  // update stats
  queue_table[hdr->to].stats.msg_count_in++;
  queue_table[hdr->to].stats.msg_bytes_in += sizeof(*hdr) + hdr->length;
  queue_table[hdr->to].stats.msgs_in_queue_now++;

  queue_table[hdr->to].stats.msgs_in_queue_highwater_mark = 
    max(queue_table[hdr->to].stats.msgs_in_queue_highwater_mark,
	queue_table[hdr->to].stats.msgs_in_queue_now);

  queue_table[hdr->to].stats.msg_max_size =
    max(hdr->length, queue_table[hdr->to].stats.msg_max_size);

  // post the semaphore to release exclusive access to the data struct
  sem_post(&queue_table[hdr->to].access_semaphore);

  // increment the msg count semaphore to enable msg_read()'s on this queue
  sem_post(&queue_table[hdr->to].msg_count_semaphore);

  return MSG_OK;
    
}


/* ----------------------------------------------------------------------
   int msg_send()
 
   sends a message. 

   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   06-OCT-1992  LLW     Created and written.
   11-MAR-2000  LLW     Ported to posix
   25-MAR-2000  LLW     Cleaned up sloppy code
   06-FEB-2002  LLW     Revised to simplify and omit channel structures.

   ---------------------------------------------------------------------- */
int msg_send (msg_addr_t to,
	      msg_addr_t from,
	      msg_type_t type, 
	      msg_len_t  length, 
	      const void *data)
{



  msg_hdr_t hdr;
  int       status;
  
  if(to == NULL_THREAD_ADDRESS){
    return MSG_OK;
  }
  // assign the message header
  hdr.to   = to;
  hdr.from = from;
  hdr.type = type;  
  hdr.length = length;
  
  

  // send it
  status = msg_send(&hdr, data);

  return status;
}

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
extern int msg_send_broadcast(msg_hdr_t * hdr_ptr, const void * data)
{
  int status;
  int queue_num;
  msg_hdr_t hdr = *hdr_ptr;

  // send a copy of the message to every active queue
  for(queue_num=MSG_ADDRESS_MIN; queue_num<=MSG_ADDRESS_MAX; queue_num++)
    {
      if(queue_table[queue_num].active)
	{
	  hdr.to = queue_num;
	  status = msg_send(&hdr, data);
	  
	  if(status != MSG_OK)
	    return status;
	}
    }

  return status;

}


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
extern int msg_send_broadcast(msg_addr_t from,
			      msg_type_t type, 
			      msg_len_t  length, 
			      const void *data)
{

  msg_hdr_t hdr;
  int       status;

  // assign the message header
  hdr.to   = 0;
  hdr.from = from;
  hdr.type = type;  
  hdr.length = length;

  // send it
  status = msg_send_broadcast(&hdr, data);

  return status;
}




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
int msg_get(msg_addr_t  my_queue_number,
	    msg_hdr_t * hdr, 
	    void      * data, 
	    msg_len_t   max_size)
{

  // check queue number to ensure it is in valid range
  if (0 == inrange(my_queue_number, MSG_ADDRESS_MIN, MSG_ADDRESS_MAX))
    return MSG_ERR_ADDR_NFG;

  // if queue is NOT already initialized, exit with an error
  if( queue_table[my_queue_number].active == 0)
    return MSG_ERR_ADDR_UNUSED;

  // wait until there is a message in the queue
  sem_wait(&queue_table[my_queue_number].msg_count_semaphore);

  // now wait on the semaphore to get exclusive access to the data struct
  sem_wait(&queue_table[my_queue_number].access_semaphore);

  // copy the header
  *hdr = queue_table[my_queue_number].hdr[queue_table[my_queue_number].index_out];

  // check to see if the buffer is large enough to hold the data
  if (hdr->length > max_size)
    {
      sem_post(&queue_table[my_queue_number].access_semaphore);

#ifdef DEBUG_MSG_UTIL
      printf("MSG_GET(): Error data buffer too small, msg from %3d, to %3d. type %3d, length %6d, "
	     "data=%10.10s, status=%s\n", 
	     hdr->from, hdr->to, hdr->type, hdr->length, (char *) data, 
	     MSG_ERROR_CODE_NAME(MSG_ERR_GET_SIZE)
	     );
#endif

      return( MSG_ERR_GET_SIZE );
    }

  // copy the data
  if( hdr->length > 0)
    memcpy( data, 
	    &(queue_table[my_queue_number].data[MSG_DATA_LEN_MAX* queue_table[my_queue_number].index_out]),
	    hdr->length);

  // update pointers
  queue_table[my_queue_number].index_out = (queue_table[my_queue_number].index_out +1) % MSG_QUEUE_DEPTH;

  // update stats
  queue_table[my_queue_number].stats.msg_count_out++;
  queue_table[my_queue_number].stats.msg_bytes_out += sizeof(*hdr) + hdr->length;
  queue_table[my_queue_number].stats.msgs_in_queue_now--;

  // post the semaphore to release exclusive access to the data struct
  sem_post(&queue_table[my_queue_number].access_semaphore);

  return MSG_OK;
    
}



/* ----------------------------------------------------------------------
   stats_t msg_get_queue_stats(int queue_num)
 
   Gets statistics on the message queue.  

   This will normally only be used for debugging.

   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   11-FEB-2002  LLW     Created and written.

   ---------------------------------------------------------------------- */
int  msg_get_queue_stats(msg_addr_t queue_number,
			 msg_queue_stats_t * stats)
{

  // check queue number to ensure it is in valid range
  if (0 == inrange(queue_number, MSG_ADDRESS_MIN, MSG_ADDRESS_MAX))
    return MSG_ERR_ADDR_NFG;

  // if queue is NOT already initialized, exit with an error
  if( queue_table[queue_number].active == 0)
    return MSG_ERR_ADDR_UNUSED;

  // now wait on the semaphore to get exclusive access to the data struct
  sem_wait(&queue_table[queue_number].access_semaphore);

  // count the number of active queues
  int i;
  queue_table[queue_number].stats.num_active_queues = 0;
  for(i=MSG_ADDRESS_MIN; i<=MSG_ADDRESS_MAX; i++)
    {
      if(queue_table[i].active)
	queue_table[queue_number].stats.num_active_queues++;
    }

  // get the stats
  *stats = queue_table[queue_number].stats;

  // post the semaphore to release exclusive access to the data struct
  sem_post(&queue_table[queue_number].access_semaphore);

  // return good status
  return MSG_OK;

}


/* ----------------------------------------------------------------------
   int msg_get_all_stats(stats * stats);  
 
   Gets total statistics on all the message queues.

   This will normally only be used for debugging.

   Modification History: 
   Modification History:
   DATE         AUTHOR  COMMENT
   11-FEB-2002  LLW     Created and written.

   ---------------------------------------------------------------------- */
int  msg_get_queue_stats_all(msg_queue_stats_t * total_stats)
{

  int queue_num;
  msg_queue_stats_t stats = {0};

  for(queue_num=MSG_ADDRESS_MIN; queue_num<=MSG_ADDRESS_MAX; queue_num++)
    {
      stats.msg_count_in         += queue_table[queue_num].stats.msg_count_in;
      stats.msg_count_in_last    += queue_table[queue_num].stats.msg_count_in_last;
      stats.msg_count_out        += queue_table[queue_num].stats.msg_count_out;
      stats.msg_count_dropped    += queue_table[queue_num].stats.msg_count_dropped;

      stats.msg_bytes_in         += queue_table[queue_num].stats.msg_bytes_in;
      stats.msg_bytes_in_last    += queue_table[queue_num].stats.msg_bytes_in_last;
      stats.msg_bytes_out        += queue_table[queue_num].stats.msg_bytes_out;
      stats.msg_bytes_dropped    += queue_table[queue_num].stats.msg_bytes_dropped;

      stats.msgs_in_queue_now    += queue_table[queue_num].stats.msgs_in_queue_now;

      stats.msg_max_size         = max(stats.msg_max_size, queue_table[queue_num].stats.msg_max_size);
      stats.msgs_in_queue_highwater_mark  = max(stats.msgs_in_queue_highwater_mark, queue_table[queue_num].stats.msgs_in_queue_highwater_mark);
      
      stats.msgs_in_per_sec      += queue_table[queue_num].stats.msgs_in_per_sec;
      stats.bytes_in_per_sec     += queue_table[queue_num].stats.bytes_in_per_sec;

      if(queue_table[queue_num].active)
	stats.num_active_queues++;
    }

  // assign the total stats
  *total_stats = stats;

  // return good status
  return MSG_OK;
  
}  


/* ----------------------------------------------------------------------

   msg stats sprintf

   Call this guy at one hertz

   Modification History: 
   13-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */
int msg_stats_sprintf(msg_queue_stats_t stats, char * str)
{

  int len = 0;

  len += sprintf(&str[len], "Message Queue Statistics:\n"); 
  len += sprintf(&str[len], "msgs in        = %lu\n", stats.msg_count_in);
  len += sprintf(&str[len], "msgs out       = %lu\n", stats.msg_count_out);
  len += sprintf(&str[len], "msgs dropped   = %lu\n", stats.msg_count_dropped);

  len += sprintf(&str[len], "bytes in       = %lu\n", stats.msg_bytes_in);
  len += sprintf(&str[len], "bytes out      = %lu\n", stats.msg_bytes_out);
  len += sprintf(&str[len], "bytes dropped  = %lu\n", stats.msg_bytes_dropped);

  len += sprintf(&str[len], "msgs in queue  = %d\n", stats.msgs_in_queue_now);
  len += sprintf(&str[len], "msgs highwater = %d\n", stats.msgs_in_queue_highwater_mark);
  len += sprintf(&str[len], "msgs max size  = %d\n", stats.msg_max_size);

  len += sprintf(&str[len], "msgs  per sec  = %d\n", stats.msgs_in_per_sec);
  len += sprintf(&str[len], "bytes per sec  = %d\n", stats.bytes_in_per_sec);

  len += sprintf(&str[len], "total  queues  = %u\n", MSG_ADDRESS_MAX-MSG_ADDRESS_MIN+1);
  len += sprintf(&str[len], "active queues  = %u\n", stats.num_active_queues);

  return len;
}


/* ----------------------------------------------------------------------

   message queue stats one hertz update

   Call this guy at one hertz

   Modification History: 
   12-FEB-2001 LLW      Created and written
   ---------------------------------------------------------------------- */
void msg_queue_stats_one_hertz_update(void)
{

  int num;

  for(num=MSG_ADDRESS_MIN; num<MSG_ADDRESS_MAX; num++)
    {
      // now wait on the semaphore to get exclusive access to the data struct
      sem_wait(&queue_table[num].access_semaphore);

      // compute messages per second
      queue_table[num].stats.msgs_in_per_sec = 
	queue_table[num].stats.msg_count_in -
	queue_table[num].stats.msg_count_in_last;

      // update histroy
      queue_table[num].stats.msg_count_in_last = 
	queue_table[num].stats.msg_count_in;

      // compute bytes per second
      queue_table[num].stats.bytes_in_per_sec = 
	queue_table[num].stats.msg_bytes_in -
	queue_table[num].stats.msg_bytes_in_last;

      // update histroy
      queue_table[num].stats.msg_bytes_in_last = 
	queue_table[num].stats.msg_bytes_in;

      // post the semaphore to release exclusive access to the data struct
      sem_post(&queue_table[num].access_semaphore);

    } 
  
}

