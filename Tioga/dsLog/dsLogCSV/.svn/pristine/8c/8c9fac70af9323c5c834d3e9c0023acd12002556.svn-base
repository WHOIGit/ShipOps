/* ----------------------------------------------------------------------

   Cmd thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000 LLW      Created and written.
   01-JAN-2002  LLW     Ported to RedHat Linux 7.2
   30-JAN-2002  JCH     merged with JCH version, which had abstraction of thread
                        table in it
  14 Feb 2002   JCH     implement Louis'new messaging api
  02 Apr 2002   LLW     Modified hos cmd responds to SAS message
                        to omit printing of header information

---------------------------------------------------------------------- */

/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* posix header files */
#define  POSIX_SOURCE 1

/* posix header files */
#define  POSIX_SOURCE 1
#include <pthread.h>

/* jason header files */
#include "dsLogTalk.h"		/* jasontalk protocol and structures */

#include "msg_util.h"		/* utility functions for messaging */

#include "dsLogCsv.h"

#include "cmd_thread.h"		/* cmd thread */
#include "dsLogA2B.h"		/* ascii to binary conversion library */


/* ----------------------------------------------------------------------

   Cmd Thread

   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000 LLW      Created and written.

---------------------------------------------------------------------- */
void *
cmd_thread (void *arg)
{
  // local vars
  
  int MY_THREAD_ADDR;
  const char *MY_THREAD_NAME;
  int SIO_THREAD_ADDR;
  int INTERACTIVE;

  msg_hdr_t in_hdr = { 0 };
  char in_data[MSG_DATA_LEN_MAX] = { 0 };

  msg_hdr_t out_hdr = { 0 };
  char out_data[MSG_DATA_LEN_MAX] = { 0 };
  int out_data_len;

  //  int i;

  int running = 1;


  // get my process address from the from the process table
  MY_THREAD_ADDR = (long int) arg;
  
  printf("%s (thread #%d) is starting. \n",
	 JASONTALK_THREAD_NAME(MY_THREAD_ADDR),
	 MY_THREAD_ADDR);

  // now look into the process table and figure out what thread I am

#ifdef DEBUG_CMD
  printf ("CMD THREAD[%d]: starting up a cmd thread \n",MY_THREAD_ADDR);
#endif

  MY_THREAD_NAME = JASONTALK_THREAD_NAME(MY_THREAD_ADDR);

  if (MY_THREAD_NAME <= 0)
    {
      fprintf (stderr, "CMD THREAD[%d]: thread %d  not found in process table!\n", MY_THREAD_ADDR,MY_THREAD_ADDR);
      fflush (stdout);
      fflush (stderr);
      abort ();
    }
  else
    {
      printf ("CMD THREAD[%d]: starting cmd thread for %s\n", MY_THREAD_ADDR, MY_THREAD_NAME);
    }
  
  // get the address of my SIO thread from the process table
  SIO_THREAD_ADDR = global_thread_table[MY_THREAD_ADDR].extra_arg_2;
  
  printf("CMD THREAD[%d]: my sio thread addr is %d\n",MY_THREAD_ADDR, SIO_THREAD_ADDR);

  // determine if I am interactive or not
  INTERACTIVE = global_thread_table[MY_THREAD_ADDR].extra_arg_3;

  // wakeup message
  printf ("CMD THREAD[%d]: %s (thread %d) initialized \n", MY_THREAD_ADDR, MY_THREAD_NAME, MY_THREAD_ADDR);
  printf ("CMD THREAD[%d]: %s File %s compiled at %s on %s\n", MY_THREAD_ADDR, MY_THREAD_NAME, __FILE__, __TIME__, __DATE__);

  // wait a second for all the threads to come up
  usleep (100000);
  
  int msg_success = msg_queue_new(MY_THREAD_ADDR, MY_THREAD_NAME);
  
  if(msg_success != MSG_OK)
    {
      fprintf(stderr, "CMD THREAD[%d]: %s: %s\n",MY_THREAD_ADDR, MY_THREAD_NAME,MSG_ERROR_CODE_NAME(msg_success) );
      fflush (stdout);
      fflush (stderr);
      abort ();
    } 


  // send message sio process to initilize router link and indicate that we have booted
  // sprintf an error message 
  {
    char time_str[128];


    rov_sprintf_dsl_time_string (time_str);

	 out_data_len = snprintf (out_data, 1024,"----------------------------------------------------------------------\n\r"
			    "                        ROV COMMAND CONSOLE \n\r" " %s (thread %d) initialized \n\r" 
			    " File %s compiled at %s on %s\n\r" " Time now is %s\n\r" 
			    "----------------------------------------------------------------------\n\r", 
			    MY_THREAD_NAME, MY_THREAD_ADDR, __FILE__, __TIME__, __DATE__, time_str);
  }

  //  send the message to the CMD serial I/O port
  if (INTERACTIVE)
    {
#ifdef DEBUG_CMD
      printf ("CMD is interactive\n");
#endif
      msg_send(SIO_THREAD_ADDR, MY_THREAD_ADDR, WAS, out_data_len, out_data);

    }



  // ----------------------------------------------------------------------
  // loop forever
  // ----------------------------------------------------------------------
  while (running)
    {

      // wait forever on the input channel
#ifdef DEBUG_CMD
      printf ("CMD calling get_message.\n");
#endif

      int msg_get_success = msg_get(MY_THREAD_ADDR,&in_hdr, in_data, MSG_DATA_LEN_MAX);
      if(msg_get_success != MSG_OK)
	{
          fprintf(stderr, "CMD THREAD[%d]: cmd thread--error on msg_get:  %s\n",MY_THREAD_ADDR, MSG_ERROR_CODE_NAME(msg_get_success));
	}
      else
	{

	  //	  #define DEBUG_RECIEVED_MESSAGES

#ifdef DEBUG_RECIEVED_MESSAGES
	  // print on stderr
	  //      printf
	  // ("%s: (thread %d) got msg type %d from proc %d to proc %d, %d bytes data\n",
	  //         MY_THREAD_NAME, MY_THREAD_ADDR, in_hdr.type, in_hdr.from, in_hdr.to, in_hdr.length);

	printf ("%s (%d):  got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
		MY_THREAD_NAME, MY_THREAD_ADDR,
		JASONTALK_MESSAGE_NAME(in_hdr.type),in_hdr.type, 
		JASONTALK_THREAD_NAME(in_hdr.from),in_hdr.from, 
		JASONTALK_THREAD_NAME(in_hdr.to),in_hdr.to,
		in_hdr.length );


#endif



	  // process the message
	  switch (in_hdr.type)
	    {

	      // ----------------------------------------------------------------------
	      // SERIAL STRING STATUS MESSAGE (co
	      // ----------------------------------------------------------------------

	    case SAS:
	      {

		//#define DEBUG_CMD
#ifdef DEBUG_CMD
		printf ("%s: (thread %d) got SAS msg type %d from proc %d, %d bytes, \"%.*s\"\n", MY_THREAD_NAME, MY_THREAD_ADDR, in_hdr.type, in_hdr.from, in_hdr.length, in_hdr.length, (char *) in_data);
#endif

		// got a SAS message from the CMD serial port process, so process it
		if (in_hdr.from == SIO_THREAD_ADDR)
		  {
		    int status;

		    // convert the ascii message to a binary message
          status = dsLogA2B (&in_hdr, in_data, &out_hdr, out_data);

#ifdef DEBUG_CMD
		    printf ("%s: (thread %d) ascii_to_binary status = %d\n", MY_THREAD_NAME, MY_THREAD_ADDR, status);
#endif

		    if (status == 0)	// ascii command message parsed and converted OK, so send the binary message
		      {
#ifdef DEBUG_CMD
			printf ("%s: (thread %d) sending msg type %d from thread %d to thread %d,  %d bytes\n", MY_THREAD_NAME, MY_THREAD_ADDR, out_hdr.type, out_hdr.from, out_hdr.to, out_hdr.length);
#endif
			if (out_hdr.type != WSX)
			  {
#ifdef DEBUG_CMD
			    printf ("%s: (thread %d) sending NON_WSX msg type %d from thread %d to thread %d,  %d bytes \n", MY_THREAD_NAME, MY_THREAD_ADDR, out_hdr.type, out_hdr.from, out_hdr.to, out_hdr.length);
#endif
			    //send_message (to_router, &out_hdr, out_data);
#ifdef   DEBUG_HOTEL_RHC
			    printf(" sending an RHC  to  thread %d\n",out_hdr.to);
#endif
			    msg_send(out_hdr.to, MY_THREAD_ADDR, out_hdr.type, out_hdr.length, out_data);
			    //send_msg (to_router, out_hdr.to, MY_THREAD_ADDR, RHC, 0, NULL);
#ifdef DEBUG_CMD
			    printf ("%s: (thread %d) sent NON_WSX msg type %d from thread %d to thread %d,  %d bytes\n", MY_THREAD_NAME, MY_THREAD_ADDR, out_hdr.type, out_hdr.from, out_hdr.to, out_hdr.length);
#endif

			  }
			else if (out_hdr.type == WSX)
			  {		// if this is a WSX message, send a copy to my own SIO thread also

			    // setup the default WSX mode termination character if user dis not specify it
			    if (out_hdr.length == 0)
			      {
				out_data[0] = '~';
				out_hdr.length = 1;
			      }


			    // send a newsy message to my serial port
			    char newsy_msg[1024];
				 snprintf (newsy_msg,1024, "------------------------------------------------------------------------------\n\r" "Entering WSX Terminal Mode on SIO thread %d. Termination character is \"%c\".  \n\r" "-----------------------------------------------------------------------------\n\r", out_hdr.to, out_data[0]);

			    // send the message to the CMD serial I/O port
			    msg_send(SIO_THREAD_ADDR, MY_THREAD_ADDR, WAS, strlen (newsy_msg), newsy_msg);
       

			    // put the specified serial port in wsx mode to my serial port
			    out_hdr.from = SIO_THREAD_ADDR;
			 
			    msg_send(&out_hdr, out_data);
			    // now put my port in WSX mode to the specified serial port
			    out_hdr.from = out_hdr.to;
			    out_hdr.to = SIO_THREAD_ADDR;
			 
			    msg_send(&out_hdr, out_data);

			  }
		      }
		    else		// error parsing the ascii message, send error to the user
		      {

			// sprintf an error message 
			out_data_len = snprintf (out_data, 1024,"\n\rERROR PARSING COMMAND LINE: %.*s\n\r", in_hdr.length - 1, in_data);

			// send the message to the CMD serial I/O port
			if (INTERACTIVE)
			  msg_send(SIO_THREAD_ADDR, MY_THREAD_ADDR, WAS, out_data_len, out_data);


		      }

		  }
		else		// got a SAS message from somewhere else, not the CMD serial port
		  {
		    // int sss_data_len;
		    // int out_data_len;
		    // format a SAS response and send it out the CMD serial port
		    // out_data_len = sprintf (out_data, "SAS %d %d ", in_hdr.from, in_hdr.length);
		    // keep track of bufer lengths
		    // sss_data_len = min (in_hdr.length, (MSG_DATA_LEN_MAX - out_data_len));
		    // memcpy (&out_data[out_data_len], in_data, sss_data_len);
		    // out_data_len += sss_data_len;
		    // out_data_len += sprintf (&out_data[out_data_len], "\n\r");
		    // send the message
		    // msg_send(SIO_THREAD_ADDR, MY_THREAD_ADDR, WAS, out_data_len, out_data); 

		    //  02 Apr 2002   LLW  Modified hos cmd responds to SAS message
		    //                     to omit printing of header information
		    msg_send(SIO_THREAD_ADDR, MY_THREAD_ADDR, WAS, in_hdr.length, in_data); 

		  }
		break;
	      }
	    case BYE:  // received a bye message--time to give up the ghost--
	      {
          

    		printf ("%s (thread %d) received BYE command. \n", MY_THREAD_NAME, MY_THREAD_ADDR);
		fflush(stdout);


		break;
	      }
	      // ----------------------------------------------------------------------
	      // PING message
	      // ----------------------------------------------------------------------

	    case PNG:		// recieved a PNG (Ping) message
	      {
		char msg[256];

		snprintf (msg, 256,"%s (thread %d) is Alive!", MY_THREAD_NAME, MY_THREAD_ADDR);

		// respond with a SPI (Status Ping) message
		msg_send(in_hdr.from, in_hdr.to, SPI, strlen (msg), msg);
	    

		break;
	      }


	      // ----------------------------------------------------------------------
	      // Default message processing binary -> ascii
	      // ----------------------------------------------------------------------

	    default:
	      {
		int status;

		// convert the ascii message to a binary message
      status = dsLogB2A (&in_hdr, in_data, &out_hdr, out_data);

#ifdef DEBUG_CMD
		printf ("%s: (thread %d) binary_to_ascii status = %d\n", MY_THREAD_NAME, MY_THREAD_ADDR, status);
#endif

		if (status == 0)	// ascii command message parsed and converted OK, so send the binary message
		  {
		    out_hdr.to = SIO_THREAD_ADDR;
		    out_hdr.from = MY_THREAD_ADDR;
		    out_hdr.type = WAS;

#ifdef DEBUG_CMD
		    printf ("%s: (thread %d) sending msg type %d from thread %d to thread %d, %d bytes\n", MY_THREAD_NAME, MY_THREAD_ADDR, out_hdr.type, out_hdr.from, out_hdr.to, out_hdr.length);
#endif

		    msg_send(&out_hdr, out_data);
		 
		  }
		else
		  {
		    printf ("%s: (thread %d) got unknown message type %d from proc %s (%d) to proc %s (%d), %d bytes data\n", 
			    MY_THREAD_NAME, MY_THREAD_ADDR, 
			    in_hdr.type, 
			    JASONTALK_THREAD_NAME(in_hdr.from), 
			    in_hdr.from, 
			    JASONTALK_THREAD_NAME(in_hdr.to), 
			    in_hdr.to, 
			    in_hdr.length);
		  }

		break;
	      }			// case


	    }			// switch
	} // else on msg_get
    }				// while

  return NULL;

}





