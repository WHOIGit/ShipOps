/* ----------------------------------------------------------------------
   
   Utility to launch a timer

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */

#ifndef LAUNCH_TIMER
#define LAUNCH_TIMER

// posix header files 
#include <pthread.h>   
#include <limits.h>


#include "msg_util.h"

typedef enum {TIMER_ENABLE, TIMER_DISABLE, TIMER_KILL} launched_timer_control_flag_t;

typedef struct
{
  double                         dt; 
  int                            kount;
  msg_addr_t                     to;
  msg_type_t                     type;
  pthread_t                      thread;
  launched_timer_control_flag_t  control_flag;
} launched_timer_data_t;


/* ----------------------------------------------------------------------
   
   timer_launch.

   Timer will send a message of TYPE to TO every DT seconds.

   If kount is -1, then sends messages forever.

   If kount is positive, then sends exactly kount messages then disables itself.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
extern launched_timer_data_t * launch_timer_new( double     dt,
						 int        kount,
						 msg_addr_t to,
						 msg_type_t type);
					     

/* ----------------------------------------------------------------------
   
   timer_launch_free()

   Kills the timer process on the next DT cycle.

   Deallocates the launched_timer_data_t memory.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
extern void launch_timer_free( launched_timer_data_t * data);
		  

/* ----------------------------------------------------------------------
   
   timer_launch_disable()

   Disables the timer process messages.

   Does not kill the timer, just disables messages.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
extern void launch_timer_disable( launched_timer_data_t * tmr_data);

/* ----------------------------------------------------------------------
   
   timer_launch_enable()

   Reenables an existing timer messages, optionally with new parameters.

   
   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
extern void launch_timer_enable( launched_timer_data_t * tmr_data);

extern void launch_timer_enable( launched_timer_data_t * tmr_data,
				 double     dt,
				 int        kount,
				 msg_addr_t to,
				 msg_type_t type);


#endif
