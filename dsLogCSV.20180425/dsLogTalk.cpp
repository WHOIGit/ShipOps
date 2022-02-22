/* ----------------------------------------------------------------------

   Funcitons for Jason Interprocess Communication

   Modification History:
   DATE        AUTHOR   COMMENT
   21 Oct 2003 LLW      Created
   27 OCT 2003 JCK&LLW   XVISION  
   14 OCT 2004 LLW      Added SRC_ARGUS_PARO,  SRC_ARGUS_ALTIMETER
                        and SRC_DVLNAV_ALTITUDE to function
                        STATE_SOURCE_NAME( )
   ---------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>


#include "dsLogTalk.h"
#include "dsLogCsv.h"


/* ----------------------------------------------------------------------

   Jasontalk thread name to int

   returns -1 if name is not known

   Modification History:
   DATE         AUTHOR  COMMENT
   23-OCT-2003  LLW     Created

---------------------------------------------------------------------- */
int JASONTALK_THREAD_NUM( char * name)
{

  int i;
  int thread_num = -1;
  
  for(i =0; i< MAX_NUMBER_OF_THREADS; i++)
    {
      if(global_thread_table[i].thread_name != NULL)
	if( 0 == strcasecmp(name, global_thread_table[i].thread_name))
	  {
	    thread_num = i;

	    i = MAX_NUMBER_OF_THREADS;
	  }
    }


  if(thread_num == -1)
    {
      printf("ERROR: JASONTALK_THREAD_NUM unable to find thread \"%s\"\n", name);
      printf("\a\a\a\a\a");
    }

  return thread_num;
  
}


/* ----------------------------------------------------------------------

   Jasontalk thread number int to string.

   Note that this lists all thread names, not just the ones
   that are loaded and run by rov.cpp in the thread table

   Modification History:
   DATE         AUTHOR  COMMENT
   21-OCT-2003  LLW     Created

---------------------------------------------------------------------- */
const char * JASONTALK_THREAD_NAME (int thread_num)
{

  static char errmsg[] = "UNKNOWN THREAD NUMBER";
  const char * name; 

  if( (thread_num >= 0) && (thread_num < MAX_NUMBER_OF_THREADS))
    name = global_thread_table[thread_num].thread_name;
  else
    name = errmsg;

  return name;

}



/* ----------------------------------------------------------------------

   Jasontalk message type int to string

   Modification History:
   DATE         AUTHOR  COMMENT
   21-OCT-2003  LLW     Created

---------------------------------------------------------------------- */
const char * JASONTALK_MESSAGE_NAME (int num)
{

  const char *name;

#define make_entry(flag) case flag : name = #flag; break

  switch (num)
    {
      /* ---------    SYSTEM STATUS MESSAGES TYPES ----------*/
      make_entry( HLP );
      make_entry( PNG );
      make_entry( SPI );
      make_entry( RST );
      make_entry( VER );

      make_entry( BYE );


      /* ---------    SERIAL I/O MESSAGE TYPES ----------*/
      make_entry( WAS );
      make_entry( RAS );
      make_entry( SAS );
      make_entry( WHS );
      make_entry( RHS );
      make_entry( SHS );
      make_entry( WSX );
      make_entry( WSY );
      make_entry( WSP );
      make_entry( RSP );
      make_entry( SSP );


    default:
      name = "UNKOWN MESSAGE TYPE";
    }

  return name;

}









  
