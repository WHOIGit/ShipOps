/* ----------------------------------------------------------------------
   
   Utility to launch a timer

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
#include <stdlib.h>   
#include <unistd.h>   
#include <stdio.h>

#include "launch_timer.h"


/* ----------------------------------------------------------------------

   timer_launch thread

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
static void * launched_timer_thread( void * arg)
{
  launched_timer_data_t * tmr_data;

  tmr_data = (launched_timer_data_t *) arg;
  
  while( tmr_data->control_flag != TIMER_KILL)
    {
      // disable afer kount cycles if kount is non negative 
      if( tmr_data->kount >= 0)
	{
	  if(tmr_data->kount>0)
	    tmr_data->kount--;
	  else
	    tmr_data->control_flag = TIMER_DISABLE;
	}

      // send the message if still enabled
      if(tmr_data->control_flag == TIMER_ENABLE)
         {
      
      
      
	msg_send(tmr_data->to, 0, tmr_data->type, 0, NULL);
  }

      // wait
      usleep( ((unsigned int) (tmr_data->dt * 1000000.0)));
    }

  // free the argument data atructure
  free(arg);

  return NULL;

}


/* ----------------------------------------------------------------------
   
   timer_launch.

   Timer will send a message of TYPE to TO every DT seconds.

   If kount is -1, then sends messages forever.

   If kount is positive, then sends exactly kount messages then disables itself.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
launched_timer_data_t * launch_timer_new( double     dt,
					  int        kount,
					  msg_addr_t to,
					  msg_type_t type)
{
  pthread_t st;
  pthread_attr_t DEFAULT_TIMER_THREAD_ATTR;
  launched_timer_data_t * tmr_data;


  
  // allocate and initialize the timer data struct
  tmr_data = (launched_timer_data_t *) malloc(sizeof(launched_timer_data_t));

  // if malloc failed, return null pointer to inficate failure
  if(tmr_data == NULL)
    return NULL;

  // assign the structure
  tmr_data->dt    = dt;
  tmr_data->kount = kount;
  tmr_data->to    = to;
  tmr_data->type  = type;
  tmr_data->control_flag = TIMER_ENABLE;

  // initialize the thread attributes
  pthread_attr_init (&DEFAULT_TIMER_THREAD_ATTR);

  // set for detached threads
  pthread_attr_setdetachstate (&DEFAULT_TIMER_THREAD_ATTR,
			       PTHREAD_CREATE_DETACHED),
    // launch stats thread
    pthread_create(  &st,  &DEFAULT_TIMER_THREAD_ATTR, launched_timer_thread, tmr_data);

  return tmr_data;

}

/* ----------------------------------------------------------------------
   
   timer_launch_disable()

   Disables the timer process messages.

   Does not kill the timer, just disables messages.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
void launch_timer_disable( launched_timer_data_t * tmr_data)
{

  tmr_data->control_flag = TIMER_DISABLE;

}

/* ----------------------------------------------------------------------
   
   timer_launch_enable()

   Reenables timer messages.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
void launch_timer_enable( launched_timer_data_t * tmr_data)
{

  tmr_data->control_flag = TIMER_ENABLE;

}

void launch_timer_enable( launched_timer_data_t * tmr_data,
			  double     dt,
			  int        kount,
			  msg_addr_t to,
			  msg_type_t type)
{

  // assign the structure
  tmr_data->dt    = dt;
  tmr_data->kount = kount;
  tmr_data->to    = to;
  tmr_data->type  = type;
  tmr_data->control_flag = TIMER_ENABLE;

}
		  

/* ----------------------------------------------------------------------
   
   timer_launch_free()

   Kills the timer process on the next DT cycle.

   Deallocates the launched_timer_data_t memory.

   Modification History: 
   DATE AUTHOR          COMMENT 
   14-FEB-2002 LLW      Created and Written

---------------------------------------------------------------------- */
extern void launch_timer_free( launched_timer_data_t * tmr_data)
{

  tmr_data->control_flag = TIMER_KILL;

}
		  
