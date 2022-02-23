/* ----------------------------------------------------------------------
   ascii_to_binary() - jasontlk ascii to binary message

   binary_to_ascii() - binary message to jasontlk ascii

   Modified: 

   Modification History:
   DATE        AUTHOR  COMMENT
   20-JUL-00   LLW     Created and Written
   28-AUG-00   LLW&DAS Convert PNS heading field from degrees to radians
   01-DEC-00   LLW     Added DEPth and KVH jasontalk strings
                       Bug fixes to WRV, PNS, and PAS  
   14-May  2001 DAS Modifed to accept DEp string using Paro data
   15 May  2001 DAS Modified PNS string format to include sharps ducer hdg from nav program
   xx NOV 2001  JCH  added a lot of hotel messages
   14 Feb 2001  JCH  some different include files, new msging api
   13 Mar 2002  JCH  change WMV msg
   18 Apr 2002  JCH  add hydraulics msgs, S05
   7 Feb 2003   jch  add tms messages
   1 Mar 2003  jch   make strcasecmp in command parser instead of strncasecmp
    MAY 9, 2004 LLW Added WAAO for Argus   
    NOV 7, 2006 LLW   added WTT to a2b.cpp
	 November 18, 2006  jch  added PMO (power manager override)
   ---------------------------------------------------------------------- */
/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dsLogTalk.h"
#include "dsLogCsv.h"

#include "dsLogA2B.h"

#define SORTED_JASONTALK_LIST_SIZE (1+MSG_ADDRESS_MAX-MSG_ADDRESS_MIN)
#define MAX_FORMAT_STRING_LENGTH  1024
#define	MAX_LOWERING_NAME_LENGTH	16

// this is an arrray of structures which is used by binary_to_ascii.
// each element of the array corresponds to a jasontalk command.
// the arrray is initilaized the first time binary_to_ascii is called
jasontalk_message_table_entry_t sorted_jasontalk_list[SORTED_JASONTALK_LIST_SIZE] =
{
      {
            0}
};

// here is the flag that is used to flag if the above array in initialized 
int sorted_jasontalk_list_initialized = 0;

// this is an un-ordered array of jasontalk commands
static const jasontalk_message_table_entry_t unsorted_jasontalk_list[] = {

      {PNG, "PNG", 0,
       "PNG interrogates a thread to see if it is alive.   usage: \"PNG <thread number>\" example: \"PNG 0\"\n\r"},
{SPI, "SPI", 0},
{RST, "RST", 0},
{VER, "VER", 0},
{RTD, "RTD", 0,
 "RTD (Read Thread Data) requests the full data structure of a thread. \n\r  usage: RTD <thread number> <cr>\n\r"},



/* Serial I/O Commands  */
{WAS, "WAS", 0},
{RAS, "RAS", 0},
{SAS, "SAS", 0,"Status Ascii String"},
{WSX, "WSX", 0},
{WSY, "WSY", 0},



{BYE, "BYE", ANY_THREAD, "BYE: flush logs and exit\\n\\r"},
{0, NULL, 0}
};


/* ----------------------------------------------------------------------

   ascii_to_binary() - jasontalk data logging ascii to binary message

   Modified: 

   Modification History:
   DATE        AUTHOR  COMMENT
   20-JUL-00   LLW     Created and Written
   09-11-11    jch     data logging only

   ---------------------------------------------------------------------- */

int dsLogA2B (msg_hdr_t * in_hdr, char *in_data, msg_hdr_t * out_hdr, char *out_data)
{
   int i;
	char command[16];

   // null terminate the incoming ascii string so scand will work
   in_data[in_hdr->length] = '\0';

   // Scan the global jasontalk list for the 3 character command name

   // should I really only use three characters?
   // probably not, but test this carefully!
	// doesnt work unless I scanf the command first

   int items = sscanf(in_data,"%s",&(command[0]));
   if(items != 1)
      {
         return(-1);
      }
   int clen = strlen(command);
   if(clen > MAX_CMDSTRING_LEN)
      {
         return(-1);
      }

   i = 0;
   while (unsorted_jasontalk_list[i].msg_cmd_name != NULL)
      {
         if (!strncasecmp (in_data, unsorted_jasontalk_list[i].msg_cmd_name, clen))

            {

               break;
            }
         else
            {
               i++;
            }
      }

   /* Couldn't Find it... Not valid Jasontalk      */
   if (unsorted_jasontalk_list[i].msg_cmd_name == NULL)
      {
#ifdef DEBUG_A2B	 
         printf("ERROR %s Line %d MSG From %s To %s Type %s\nERROR Data: %s \n",
                __FILE__,
                __LINE__,
                JASONTALK_THREAD_NAME(in_hdr->from),
                JASONTALK_THREAD_NAME(in_hdr->to),
                JASONTALK_MESSAGE_NAME(in_hdr->type),
                in_data);
#endif
         return (-1);
      }

   /* Found it, so load the header with the correct command type and destination  */
   out_hdr->type = unsorted_jasontalk_list[i].msg_type;
   out_hdr->to = unsorted_jasontalk_list[i].msg_default_destination;
   out_hdr->from = in_hdr->to;
   out_hdr->length = 0;

   // now process the specific message


   switch (unsorted_jasontalk_list[i].msg_type)
      {

      case SSP:			/* serial port status */
      case SAS:
            {				/* Serial String Status */

               // this command not implemented so send an error message back to the sender
               out_hdr->length = snprintf (out_data, 1024,"ERROR: COMMAND TYPE %d NOT IMPLEMENTED.\n\r", in_hdr->type);
               out_hdr->to = in_hdr->from;
               break;
            }

         // ------------------------------------------------------------
         // the following comands have no argument data
         // the destination thread is implicit in the command type
         // ------------------------------------------------------------

      case BYE:			// sends msg to stderr and exits

         // ------------------------------------------------------------
         // the following comands have a thread destination argument, but no other data
         // the destination thread is NOT implicit in the command type
         // ------------------------------------------------------------
      case PNG:			// generate a PNG (Ping) message
      case RAS:			// Read Serial String
      case WSY:			/* Serial Port Terminal Mode STOP       */
      case SPI:
         // ------------------------------------------------------------
         // the following comands have a destination and string data
         // ------------------------------------------------------------
      case WSX:			/* Serial Port Terminal Mode START      */

      case WAS:
            {				/* Write Serial String  */

               int to;
               if (sscanf (in_data, "%*s %d %[^\\0]s", &to, out_data) == 2)
                  {
                     out_hdr->length = strlen (out_data);
                     out_hdr->to = to;
                  }
               else if (sscanf (in_data, "%*s %d", &to) == 1)
                  {
                     out_hdr->length = 0;
                     out_hdr->to = to;
                  }
               else
                  {
                     return -1;
                  }
               break;
            }


         // ------------------------------------------------------------
         // ver has a destination and optional integer
         // ------------------------------------------------------------

      case VER:
            {				// generate a message of varying verbosity

               int to;

               // must get the first argument
               if (1 != sscanf (in_data, "%*s %d", &to))
                  {
                     return (-1);
                  }
               out_hdr->to = to;

               // optionally get a second argument
               if (1 == sscanf (in_data, "%*s %*d %d", (int *) out_data))
                  {
                     out_hdr->length = sizeof (int);
                  }

               break;
            }
      }
   return 0;
}


/* ----------------------------------------------------------------------

   binary_to_ascii() - binary message to jasontalk ascii

   Modified: 

   Modification History:
   DATE        AUTHOR  COMMENT
   20-JUL-00   LLW     Created and Written

   ---------------------------------------------------------------------- */

int dsLogB2A (msg_hdr_t * in_hdr, char *in_data, msg_hdr_t * out_hdr, char *out_data)
{
   int i;
   int len = 0;

   // initialize the indexed list of jasontalk commands on first call

   if (!sorted_jasontalk_list_initialized)
      {
         // initialize the entire sorted list to default values
         for (i = 0; i < MSG_ADDRESS_MAX; i++)
            {
               sorted_jasontalk_list[i].msg_type = i;
               sorted_jasontalk_list[i].msg_cmd_name = "UNKNOWN MESSAGE TYPE ";
               sorted_jasontalk_list[i].msg_default_destination = 0;
            }

         // walk through the unsorted list, initializing the corresponding sorted entries as we go
         for (i = 0; i < (int) (sizeof (unsorted_jasontalk_list) / sizeof (jasontalk_message_table_entry_t)); i++)
            {
               int sorted_list_index = unsorted_jasontalk_list[i].msg_type;

               if ((sorted_list_index >= MSG_ADDRESS_MIN) && (sorted_list_index <= MSG_ADDRESS_MAX))
                  {
                     // ansii C lets us assign data structures
                     sorted_jasontalk_list[sorted_list_index] = unsorted_jasontalk_list[i];
                  }
            }

         // flag the sorted list as initialized
         sorted_jasontalk_list_initialized = 1;

         // print the entire sorted list to default values
         /*      for(i=0; i< MSG_ADDRESS_MAX; i++)
	      printf("JASONTALK CMD %d, ascii %s, default address is %d\n\r",
	      sorted_jasontalk_list[i].msg_type,
	      sorted_jasontalk_list[i].msg_cmd_name,
	      sorted_jasontalk_list[i].msg_default_destination);
      */

      }

   // if the incoming message type is out of range then this message
   // type is illegal so return immediately with error status
   if (in_hdr->type > MSG_ADDRESS_MAX)
      {
         out_hdr->length = 0;
         return (-1);
      }

   //  load the outgoing message header with the correct command type and
   // return address. Caller must set destinatin address.
   out_hdr->type = WAS;
   out_hdr->from = in_hdr->to;
   out_hdr->length = 0;

   // create the beginning of the ascii string by sprintfing the ascii command
   // for this message type
   len = snprintf (&out_data[len],128, "%s", sorted_jasontalk_list[in_hdr->type].msg_cmd_name);

   // now process the specific message
   switch (in_hdr->type)
      {
         // ----------------------------------------
         // the following comands are not implemented
         // ----------------------------------------

      case SSP:

      case RTD:			/* Read Thruster Scale     */

      case WSX:			/* Serial Port Terminal Mode START      */
      case WSY:			/* Serial Port Terminal Mode STOP       */



            {				// command not implemented so send an error message back to the sender
               len += snprintf (&out_data[len], 128," message received from %s (thread %d) with %d bytes data.\n\r", global_thread_table[in_hdr->from].thread_name, global_thread_table[in_hdr->from].thread_num, in_hdr->length);
               break;
            }


         len += snprintf (&out_data[len], 8," \n\r");

         break;


         // ------------------------------------------------------------
         // the following comands have a thread destination argument, but no other data
         // the destination thread is NOT implicit in the command type
         // ------------------------------------------------------------
      case PNG:			// generate a PNG (Ping) message
      case RAS:			//  Read Serial String
            {
               len += snprintf (&out_data[len], 128," %d:\n\r", in_hdr->to);
               break;
            }


         // ------------------------------------------------------------
         // ver has a destination and optional integer
         // ------------------------------------------------------------

      case VER:			// generate a VER (Verbose) message
            {
               if (in_hdr->length == 0)
                  len += snprintf (&out_data[len], 32," %d:\n\r", in_hdr->to);
               else if (in_hdr->length == sizeof (int))
                  len += snprintf (&out_data[len], 32," %d %d:\n\r", in_hdr->to, *((int *) in_data));

               break;
            }

         // ------------------------------------------------------------
         // the following comands have an address and string data
         // ------------------------------------------------------------
      case SPI:			// Reply to a PNG
      case SAS:			// serial string status
            {
               len += snprintf (&out_data[len], 64," %d %.*s\n\r", in_hdr->from, in_hdr->length, in_data);
               break;
            }

         // ------------------------------------------------------------
         // ascii string
         // ------------------------------------------------------------
      case WAS:			// write string status
            {
               memcpy (out_data, in_data, in_hdr->length);
               len = out_hdr->length = in_hdr->length;
               break;
            }



      default:
         return (-1);

      }				//switch


   out_hdr->length = len;

   return 0;

}





