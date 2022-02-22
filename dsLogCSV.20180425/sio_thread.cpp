/* --------------------------------------------------------------------
   Sio thread for ROV control system
   
   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000  LLW      Created and written.
   31 Jan 2002  JCH      merge in jch changes to abstract sio parameters into ini file
   02 Feb 2002  JCH      put in check to see if device name ever existed--if it doesn't, try to
                         avoid seg faults, simply block on messages which won't come
   04 Mar 2002  JCH      put incheck to see if running--avoiding blocking on mutex or checking
                          sio for when quit message has been recieved--
   08 Apr 2002  JCH     fix slight bug in setting parity
   
---------------------------------------------------------------------- */
/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* posix header files */
#define  POSIX_SOURCE 1

#include <pthread.h>
#include <termios.h>
#include <unistd.h>


/* jason header files */
#include "dsLogTalk.h"		/* jasontalk protocol and structures */

#include "msg_util.h"		/* utility functions for messaging */


#include "sio_thread.h"		/* sio thread */
#include "config_file.h"
#include "dsLogCsv.h"

// #define DEBUG_SIO

extern pthread_attr_t DEFAULT_ROV_THREAD_ATTR;



/* ----------------------------------------------------------------------

   Function to open a serial port.
   
   Returns -1 if unsuccesful, valid file id if successful
   
   Modification History:
   DATE         AUTHOR  COMMENT
   19-JUL-2000  LLW      Created and written.
   
---------------------------------------------------------------------- */

int
open_serial (sio_thread_t * sio)
{
   
   struct termios tio = { 0 };
   
   // open the serial port
   sio->sio_port_fid = open (sio->my_sio_port_table_entry.sio_com_port_name, O_RDWR | O_NOCTTY);
   
   // return immediately if device fails to open
   if (sio->sio_port_fid < 0)
      return (sio->sio_port_fid);
   
   // set tio.cflag
   tio.c_cflag = CLOCAL;		// ignore modem control lines like carrier detect
   tio.c_cflag |= CREAD;		// enable uart reciever
   
   // switch statement to set the baud rate bits in tio.cflag
   // use a macro here to save typing and reduce errors
   // the serial I/O macros B9600, etc, are defined in /usr/include/bits/termios.h
   // note that we use the macro text concatenation operator ## here
#define setbaud( BAUD ) case  BAUD :  tio.c_cflag |= B ## BAUD ; break
   
   switch (sio->my_sio_port_table_entry.baud)
      {
      setbaud (0);
      setbaud (50);
      setbaud (75);
      setbaud (110);
      setbaud (134);
      setbaud (150);
      setbaud (200);
      setbaud (300);
      setbaud (600);
      setbaud (1200);
      setbaud (1800);
      setbaud (2400);
      setbaud (4800);
      setbaud (9600);
      setbaud (19200);
      setbaud (38400);
      setbaud (57600);
      setbaud (115200);
      setbaud (230400);
      setbaud (460800);
      setbaud (500000);
      setbaud (576000);
      setbaud (921600);
      setbaud (1000000);
      setbaud (1152000);
      setbaud (1500000);
      setbaud (2000000);
      setbaud (2500000);
      setbaud (3000000);
      setbaud (3500000);
      setbaud (4000000);
      default:
         tio.c_cflag |= B9600;
         break;
      }
   
   // switch statement to set the data bit width in tio.cflag
   switch (sio->my_sio_port_table_entry.data_bits)
      {
      case 5:
         tio.c_cflag |= CS5;
         break;
      case 6:
         tio.c_cflag |= CS6;
         break;
      case 7:
         tio.c_cflag |= CS7;
         break;
      case 8:
         tio.c_cflag |= CS8;
         break;
      default:
         tio.c_cflag |= CS8;
         break;
      }
   
   
   // ignore incoming BREAK events
   tio.c_iflag |= IGNBRK;
   
   // switch statement to set the parity tio.cflag
   // default is no parity
   switch (sio->my_sio_port_table_entry.parity)
      {
      case SIO_PARITY_ODD:
         tio.c_cflag |= PARENB;	// enable output parity generation, default to odd
         tio.c_cflag |= PARODD;
         tio.c_iflag |= INPCK;	// enable input parity checking
         break;
      case SIO_PARITY_EVEN:
         tio.c_cflag |= PARENB ;	// enable output parity generation, even
         tio.c_cflag &= ~PARODD;
         tio.c_iflag |= INPCK;	// enable input parity checking
         break;
      case SIO_PARITY_MARK:
      case SIO_PARITY_SPACE:
      case SIO_PARITY_NONE:
      default:
         break;
      }
   
   // set stop bit parametrs in tio.cflag
   switch (sio->my_sio_port_table_entry.stop_bits)
      {
      case 2:
         tio.c_cflag |= CSTOPB;
         break;
      case 1:
      default:
         break;
      }
   
   
   // set flow control parametrs
   switch (sio->my_sio_port_table_entry.flow_control)
      {
      case SIO_FLOW_CONTROL_XONXOFF:
         tio.c_iflag |= IXON | IXOFF;	// enable XON/XOFF
         //printf(" set an xon off parameter!\n");
         break;
      case SIO_FLOW_CONTROL_RTSCTS:
         tio.c_cflag |= CRTSCTS;	// enable hardware RTC CTS flow control
         break;
      case SIO_FLOW_CONTROL_XONOFF_RTSCTS:
         tio.c_iflag |= IXON | IXOFF;	// enable XON/XOFF
         tio.c_cflag |= CRTSCTS;	// enable hardware RTC CTS flow control
         break;
      case SIO_FLOW_CONTROL_NONE:
      default:
         break;
      }
   
   // set tio.oflag
   tio.c_oflag = 0;
   
   // set input mode (non-canonical, no echo,...)
   // echoing will be done in manually in this thread, not by UNIX
   tio.c_lflag = 0;
   tio.c_cc[VTIME] = 0;		/* inter-character timer unused */
   tio.c_cc[VMIN] = 1;		/* blocking read until at least 1 chars received */
   
   // flush the serial port
   tcflush (sio->sio_port_fid, TCIFLUSH);
   
   // set the attributes
   tcsetattr (sio->sio_port_fid, TCSANOW, &tio);
   
   return sio->sio_port_fid;
}

/* ----------------------------------------------------------------------

   Function to process incoming characters
   
   This is a first generation version.
   Needs to be modified to read() a block of characters
   at a time and process them more efficiently.
   
   Modification History:
   DATE         AUTHOR  COMMENT
   19-JUL-2000  LLW      Created and written.
   
---------------------------------------------------------------------- */

static void * sio_rx_thread (void *arg)
{
#ifdef DEBUG_SIO
   printf ("initiating rx subthread\n");
#endif
   sio_thread_t *sio = (sio_thread_t *) arg;
   int fid;
   ssize_t num;
   char new_chars[256] = { 0 };
   
   sio = (sio_thread_t *) arg;
   fid = sio->sio_port_fid;
   
   
   
   if (sio->sio_thread_num == CMD_0_SIO_THREAD)
      {
         printf ("%s: Reminder to clean up single character read()\n", global_thread_table[sio->sio_thread_num].thread_name);
      }
   while (1)
      {
         // read a character
#ifdef DEBUG_SIO
         printf ("%s (thread %d) device %s about to read\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name);
#endif
         
         
         num = read(fid, new_chars, 1);
         
         if (num > 0)
            {
#ifdef DEBUG_SIO
               printf ("%s (thread %d) device %s fid %d got %d characters, " "ascii %c, decimal %d, hex 0x%02X, errno=%d (%s) from %s\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, num, new_chars[0], new_chars[0], new_chars[0], errno, strerror (errno), sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
               fflush (stdout);
#endif
            }
         else
            {
#ifdef DEBUG_SIO
               printf ("%s ERROR (thread %d) device %s fid %d got %d characters, " "ascii %c, decimal %d, hex 0x%02X, errno=%d (%s) from %s\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, num, new_chars[0], new_chars[0], new_chars[0], errno, strerror (errno), sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
               fflush (stdout);
#endif
               abort ();
            }
         
         // lock the data structure mutex so that we chan diddle with the data
         if(startup.running)
            {
               pthread_mutex_lock (&sio->mutex);
               
               // ----------------------------------------------------------------------
               // if we are in WSX mode, handle this specially
               // ----------------------------------------------------------------------
               if (sio->wsx_mode)
                  {
                     // check to see if the character is the WSX term character -
                     // if so, then terminate WSX mode
                     if (new_chars[0] == sio->wsx_mode_term_char)
                        {
                           // send the character to the WSX port
                           printf ("%s (thread %d) device %s fid %d terminating WSX mode to %s on character, " "ascii %c, decimal %d, hex 0x%02X, - sending WSY to address %d\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to, new_chars[0], new_chars[0], new_chars[0], sio->wsx_mode_mux_address);
                           
                           fflush (stdout);
                           
                           // send a message to the other WSX port to kill WSX mode
                           msg_send( sio->wsx_mode_mux_address, sio->sio_thread_num, WSY, 0, NULL);
                           
                           // terminate wsx mode
                           sio->wsx_mode = 0;
                        }
                     else
                        {
                           // send the character to the WSX port
#ifdef DEBUG_SIO
                           printf ("%s (thread %d) device %s fid %d sending wsx wss for %d characters, " "ascii %c, decimal %d, hex 0x%02x, to address %d\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, num, new_chars[0], new_chars[0], new_chars[0], sio->wsx_mode_mux_address);
                           fflush (stdout);
#endif
                           msg_send( sio->wsx_mode_mux_address, sio->sio_thread_num, WAS, num, new_chars);
                        }
                     
                     // need to goto bottom of loop to unlock the mutex
                     goto bottom_of_loop;
                     
                  }
               
               // ----------------------------------------------------------------------
               // echo the character(s) if needed
               // ----------------------------------------------------------------------
               if (sio->my_sio_port_table_entry.echo)
                  {
                     
                     // prepend a linefeed to echoed carrage returns
                     if (new_chars[0] == '\r')
                        write (fid, "\n", 1);
                     // echo the char
                     write (fid, new_chars, num);
                     // keep stats
                     sio->total_tx_chars++;
                  }
               
               // add the character to the serial I/O array
               sio->inchars[sio->inchar_index] = new_chars[0];
               
               // keep rx stats
               sio->total_rx_chars += num;
               
               // PROCESS THE NEW CHARACTER(S)!
               // a complete string is recieved iff ant of the following is
               // true 1. Auto-muxing is enabled and the termination character
               // is recieved.  2. Auto-muxing is enabled and the auto-muxing
               // termination character is "-1", then ANY recieved character
               // constitutes a complete string.  3.  Auto-muxing is enabled
               // and buffer is full, then send the string.
               
               if ((sio->my_sio_port_table_entry.auto_mux_address >= 0) && ((!sio->my_sio_port_table_entry.auto_mux_enabled) || (sio->my_sio_port_table_entry.auto_mux_term_char == new_chars[0]) || (sio->inchar_index >= MSG_DATA_LEN_MAX)))
                  {
                     // got a complete string, so
                     // send a SAS status message to the designated process
#ifdef DEBUG_SIO
                     printf (" about to enter destination loop for %s (thread %d) device %s\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name);
#endif
                     for (int i = 0; i < sio->my_sio_port_table_entry.n_of_destinations; i++)
                        {
#ifdef DEBUG_SIO
                           printf ("%s (thread %d) device %s fid %d auto-muxing %d char string to thread %d.\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name, fid, sio->inchar_index + 1, sio->my_sio_port_table_entry.auto_mux_address[i]);
#endif
                           msg_send( sio->my_sio_port_table_entry.auto_mux_address[i], sio->sio_thread_num, SAS, sio->inchar_index + 1, sio->inchars);
                        }
                     
                     // zero the character array index
                     sio->inchar_index = 0;
#ifdef DEBUG_SIO
                     printf ("%s (thread %d) device %s  getting ready to look for a new character\n", global_thread_table[sio->sio_thread_num].thread_name, sio->sio_thread_num, sio->my_sio_port_table_entry.sio_com_port_name);
#endif
                     
                  }
               else
                  {
                     // do not yet have a complete string, so
                     // increment the buffer pointer if the buffer is not full
                     // if the buffer is full and auto-muxing is not enabled characters are lost
                     sio->inchar_index = (sio->inchar_index + 1) % MSG_DATA_LEN_MAX;
                  }
               
               // unlock the data structure mutex so the other thread can diddle with it
bottom_of_loop:
               pthread_mutex_unlock (&sio->mutex);
               
            }  //if rov.running
         else{  // not running
               return(NULL);
            }
      }				// while
   
   return NULL;
}


/* ----------------------------------------------------------------------

   Sio Thread
   
   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000  LLW      Created and written.
   
---------------------------------------------------------------------- */
void *
sio_thread (void *thread_num)
{
   
   sio_thread_t sio = { PTHREAD_MUTEX_INITIALIZER, -1 };
   int i;
   msg_hdr_t hdr = { 0 };
   unsigned char data[MSG_DATA_LEN_MAX] = { 0 };
   
   
   FILE *ini_fd;
   char *my_name = NULL;
   int success;
   int have_a_port = false;
   
   /* get my thread number */
   sio.sio_thread_num = (long int) thread_num;
   
   // now look into the process table and figure out what thread I am
   for (i = 0; i < MAX_NUMBER_OF_THREADS; i++)
      {
         
         if (global_thread_table[i].thread_num == sio.sio_thread_num)
            my_name = global_thread_table[i].thread_name;
      }
   if (!my_name)
      {
         fprintf (stderr, "thread %d  not found in process table!\n", sio.sio_thread_num);
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   else
      {
#ifdef DEBUG_SIO
         printf ("starting sio thread for %s\n", my_name);
#endif
      }
   
   
   // get the ini file name from the global variable and open the ini file
   
   ini_fd = fopen (dsLogIniFilename, "r");
   if (!ini_fd)
      {
         fprintf (stderr, "%s ini file does not exist...exiting!\n", dsLogIniFilename);
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   
   else
      {
#ifdef DEBUG_SIO
         printf (" entering ini reading process in %s sio thread\n", my_name);
#endif
         success = read_ini_sio_process (ini_fd, my_name, &sio);
#ifdef DEBUG_SIO
         printf (" just left ini reading process in %s sio thread\n", my_name);
#endif
         fclose (ini_fd);
         if (!success)
            {
               fflush (stdout);
               fflush (stderr);
               abort ();
            }
      }
   
   
   int msg_success = msg_queue_new(sio.sio_thread_num, sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
   if(msg_success != MSG_OK)
      {
         fprintf(stderr, "sio thread for %s: %s\n",sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to,MSG_ERROR_CODE_NAME(msg_success) );
         fflush (stdout);
         fflush (stderr);
         abort ();
      }
   
   
   // wakeup message
#ifdef DEBUG_SIO
   printf ("SIO_THREAD %s (thread %d) initialized \n",
           JASONTALK_THREAD_NAME(sio.sio_thread_num),
           sio.sio_thread_num);
   printf ("SIO_THREAD File %s compiled at %s on %s\n", __FILE__, __TIME__, __DATE__);
#endif
   
   
   // ----------------------------------------------------------------------
   // open the serial port
   // ----------------------------------------------------------------------
   if (sio.my_sio_port_table_entry.sio_com_port_name == NULL)
      {
         printf (" sio device isnt available!\n");
      }
   else if(strcmp(sio.my_sio_port_table_entry.sio_com_port_name,"/dev/null"))
      {
         have_a_port = true;
         sio.sio_port_fid = open_serial (&sio);
#ifdef DEBUG_SIO
         printf ("got by open port\n");
#endif
         const char *parity_string;
         switch (sio.my_sio_port_table_entry.parity)
            {
            case SIO_PARITY_NONE:
               parity_string = "NONE";
               break;
            case SIO_PARITY_ODD:
               parity_string = "ODD";
               break;
            case SIO_PARITY_EVEN:
               parity_string = "EVEN";
               break;
            case SIO_PARITY_MARK:
               parity_string = "MARK";
               break;
            case SIO_PARITY_SPACE:
               parity_string = "SPACE";
               break;
            default:
               parity_string = "BAD PARITY CONFIGURATION!";
               break;
            }
         
         
         const char *flow_string;
         switch (sio.my_sio_port_table_entry.flow_control)
            {
            case SIO_FLOW_CONTROL_NONE:
               flow_string = "NONE";
               break;
            case SIO_FLOW_CONTROL_XONXOFF:
               flow_string = "XON/XOFF";
               break;
            case SIO_FLOW_CONTROL_RTSCTS:
               flow_string = "RTS/CTS";
               break;
            case SIO_FLOW_CONTROL_XONOFF_RTSCTS:
               flow_string = "XON/XOFF+RTS/CTS";
               break;
            default:
               flow_string = "BAD FLOW CONFIGURATION!";
               break;
            }
         
         
         
         // print newsy information
         if (sio.sio_port_fid != -1)
            {
               /*printf("%s: (thread %d) %s opened OK, fid %d, %d %s %d %d, flow=%s\n\r"
            "%s: echo=%s, mux=%d destinations, eol=0x%X, %s is connected to %s  \n",
                global_thread_table[sio.sio_thread_num].thread_name,
                sio.sio_thread_num,
                sio.my_sio_port_table_entry.sio_com_port_name,
       sio.sio_port_fid,
       sio.my_sio_port_table_entry.baud,
       parity_string,
       sio.my_sio_port_table_entry.data_bits,
       sio.my_sio_port_table_entry.stop_bits,
       flow_string,
       global_thread_table[sio.sio_thread_num].thread_name,
       sio.my_sio_port_table_entry.echo ? "ON" : "OFF",
       sio.my_sio_port_table_entry.n_of_destinations,
       //sio.my_sio_port_table_entry.auto_mux_address[0] < 0 ? "OFF" : global_thread_table[sio.my_sio_port_table_entry.auto_mux_address].thread_name,
       sio.my_sio_port_table_entry.auto_mux_term_char,
       sio.my_sio_port_table_entry.sio_com_port_name,
       sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
   */
            }
         else
            {
               printf ("%s ERROR (thread %d) device %s FAILED TO OPEN with fid %d connected to %s\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, sio.my_sio_port_table_entry.sio_com_port_name, sio.sio_port_fid, sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to);
               fflush (stdout);
               have_a_port = false;
               //abort ();
            }
         
         
         // launch the thread to process incoming characters from the serial port
#ifdef DEBUG_SIO
         printf ("SIO about to  launch rx thread.\n");
#endif
         if (have_a_port)
            {
               pthread_create (&sio.sio_rx_subthread, &DEFAULT_ROV_THREAD_ATTR, sio_rx_thread, (void *) (&sio));
               
#ifdef DEBUG_SIO
               printf ("SIO just launched rx thread.\n");
#endif
            }
      }				// end if there's actually a device
   // loop forever
   while (1)
      {
         
         // wait forever on the input channel
#ifdef DEBUG_SIO
         printf ("SIO calling get_message.\n");
#endif
         
         
         int msg_get_success = msg_get(sio.sio_thread_num,&hdr, data, MSG_DATA_LEN_MAX);
         if(msg_get_success != MSG_OK)
            {
               fprintf(stderr, "sio thread--error on msg_get:  %s\n",MSG_ERROR_CODE_NAME(msg_get_success));
            }
         else
            {
               // #define DEBUG_RECIEVED_MESSAGES
               
#ifdef DEBUG_RECIEVED_MESSAGES
               // print on stderr
               printf ("%s got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
                       JASONTALK_THREAD_NAME(sio.sio_thread_num),
                       JASONTALK_MESSAGE_NAME(hdr.type),hdr.type,
                       JASONTALK_THREAD_NAME(hdr.from),hdr.from,
                       JASONTALK_THREAD_NAME(hdr.to),hdr.to,
                       hdr.length );
               
               
#endif
#ifdef DEBUG_SIO
               printf ("%s got msg %s (%d) from %s (%d) TO %s (%d), %d bytes\n",
                       JASONTALK_THREAD_NAME(sio.sio_thread_num),
                       JASONTALK_MESSAGE_NAME(hdr.type),hdr.type,
                       JASONTALK_THREAD_NAME(hdr.from),hdr.from,
                       JASONTALK_THREAD_NAME(hdr.to),hdr.to,
                       hdr.length );
               
               
#endif
               // process new message
               switch (hdr.type)
                  {

                  case WAS:		// recieved a WAS (Write Serial String) Message
                     {
                        // send the characters to the port, keep stats
                        // note that WAS messages need NOT be null terminated
                        if (!have_a_port)
                           {
                              break;
                           }
                        if (hdr.length > 0)
                           {
                              // lock the mutex
                              pthread_mutex_lock (&sio.mutex);

                              // write the bytes
                              write (sio.sio_port_fid, data, hdr.length);

                              // keep stats
                              sio.total_tx_chars += hdr.length;

                              // unlock the mutex
                              pthread_mutex_unlock (&sio.mutex);

                           }
                        break;
                     }
                     
                     
                  case WSX:		// recieved a WSX (Begin Write Serial Xtra Transparent Mode)
                     {
                        if (!have_a_port)
                           {
                              break;
                           }
                        // if this port is already in WSX mode, ignore this command
                        if (sio.wsx_mode != 0)
                           break;

                        // lock the mutex
                        pthread_mutex_lock (&sio.mutex);

                        // set the WSX mode flag
                        sio.wsx_mode = 1;

                        // program the auto-mux to copy all characters to the calling process
                        sio.wsx_mode_mux_address = hdr.from;

                        // if the data is nonzero, use the first character as the WSX term character
                        if (hdr.length > 0)
                           {
                              sio.wsx_mode_term_char = data[0];
                           }
                        else
                           sio.wsx_mode_term_char = '~';

                        // unlock the mutex
                        pthread_mutex_unlock (&sio.mutex);


#ifdef DEBUG_SIO
                        printf ("%s (thread %d) device %s fid %d entering WSX mode " "term char is ascii %c, decimal %d, hex 0x%02X.\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, sio.my_sio_port_table_entry.sio_com_port_name, sio.sio_port_fid, sio.my_sio_port_table_entry.auto_mux_term_char, sio.my_sio_port_table_entry.auto_mux_term_char, sio.my_sio_port_table_entry.auto_mux_term_char);
                        fflush (stdout);
#endif
                        break;

                     }
                     
                  case WSY:		// recieved a WSY (Terminate Write Serial Xtra Transparent Mode)
                     {
                        if (!have_a_port)
                           {
                              break;
                           }			// if this port is not in WSX mode, ignore this command
                        if (sio.wsx_mode == 0)
                           break;

                        // lock the mutex
                        pthread_mutex_lock (&sio.mutex);

                        // set the WSX mode flag
                        sio.wsx_mode = 0;

                        // newsy message
                        printf ("%s (thread %d) device %s fid %d recieved WSY message, terminating WSX mode to %s (thread %d).\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, sio.my_sio_port_table_entry.sio_com_port_name, sio.sio_port_fid, sio.my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to, sio.wsx_mode_mux_address);

                        fflush (stdout);



                        // unlock the mutex
                        pthread_mutex_unlock (&sio.mutex);

                        break;
                     }
                     
                  case RAS:		// recieved a RAS (Read Serial String) Message
                     {
                        // lock the mutex
                        pthread_mutex_lock (&sio.mutex);

                        // send a SAS status message to the designated process
                        msg_send(hdr.from, sio.sio_thread_num, SAS, sio.inchar_index + 1, sio.inchars);

                        // zero the character array index
                        sio.inchar_index = 0;

                        // unlock the mutex
                        pthread_mutex_unlock (&sio.mutex);

                        break;
                     }
                     
                  case PNG:		// recieved a PNG (Ping) message
                     {
                        char msg[256];

                        snprintf (msg,256, "%s: (thread %d) is Alive!", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num);

                        // respond with a SPI (Status Ping) message
                        msg_send (hdr.from, hdr.to, SPI, strlen (msg), msg);

                        break;
                     }
                  case BYE:  // received a bye message--time to give up the ghost--or at least my socket
                     {

                        close(sio.sio_port_fid);
                        return(NULL);

                        break;
                     }
                  case SPI:		// recieved a SPI (Status Ping) message
                     break;
                     
                  default:		// recieved an unknown message type
                     {
                        printf ("%s: (thread %d) ERROR: RECIEVED UNKNOWN MESSAGE TYPE - " "got msg type %d from proc %d to proc %d, %d bytes data\n", global_thread_table[sio.sio_thread_num].thread_name, sio.sio_thread_num, hdr.type, hdr.from, hdr.to, hdr.length);
                        break;
                     }
                  } // switch
            } // else
         
      } //while
   
   return NULL;
   
}

/* ----------------------------------------------------------------------

   read_ini_sio_process
   
   Modification History:
   DATE               AUTHOR      COMMENT
   28 SEPTEMBER-2001  J. HOWLAND  Created and written.
   
---------------------------------------------------------------------- */

int read_ini_sio_process (FILE * ini_fd, char *my_name, sio_thread_t * my_sio)
{
   
   char *device;
   char port_name[MAX_PORT_NAME_LENGTH];
   char destination_string[MAX_DESTINATION_STRING_LENGTH];
   char *new_destination;
   
   
   // first read the device
   
   device = rov_read_string (ini_fd, my_name, "DEVICE", "null");
   
   
   snprintf (&(port_name[0]),MAX_PORT_NAME_LENGTH, "/dev/%s", device);
   
#ifdef DEBUG_CONFIG
   printf (" ini sio read device: %s\n", device);
#endif
   if (device)
      {
         // find the section with the device settings
         my_sio->my_sio_port_table_entry.baud = rov_read_int (ini_fd, my_name, "BAUD", 9600);
#ifdef DEBUG_CONFIG
         printf (" ini sio read baud: %d\n", my_sio->my_sio_port_table_entry.baud);
#endif
         my_sio->my_sio_port_table_entry.sio_com_port_name = strdup (&(port_name[0]));
         my_sio->my_sio_port_table_entry.parity = rov_read_int (ini_fd, my_name, "PARITY", 0);
         my_sio->my_sio_port_table_entry.data_bits = rov_read_int (ini_fd, my_name, "DATABITS", 8);
         my_sio->my_sio_port_table_entry.stop_bits = rov_read_int (ini_fd, my_name, "STOPBITS", 1);
         my_sio->my_sio_port_table_entry.echo = rov_read_int (ini_fd, my_name, "ECHO", FALSE);
         my_sio->my_sio_port_table_entry.flow_control = rov_read_int (ini_fd, my_name, "FLOWCONTROL", SIO_FLOW_CONTROL_NONE);
         my_sio->my_sio_port_table_entry.name_of_thing_this_com_port_is_connected_to = rov_read_string (ini_fd, my_name, "ATTACHED_TO", "unknown");
         char *new_string = rov_read_string (ini_fd, my_name, "TERM_CHAR", DEFAULT_TERMINATOR_STRING);
         my_sio->my_sio_port_table_entry.auto_mux_enabled = TRUE;
         
         if (!strcasecmp (new_string, "none"))
            {
               my_sio->my_sio_port_table_entry.auto_mux_enabled = FALSE;
            }
         else if (new_string[0] == '\\')
            {			// there are escape characters here
               if (new_string[1] == 'r')
                  my_sio->my_sio_port_table_entry.auto_mux_term_char = '\r';
               else if (new_string[1] == 'n')
                  my_sio->my_sio_port_table_entry.auto_mux_term_char = '\n';
            }
         
         else if (strlen (new_string) == 1)
            {
               my_sio->my_sio_port_table_entry.auto_mux_term_char = new_string[0];
            }
         
         else
            {
               unsigned int new_int;
               int items;
               //printf ("****** this should be a xx new string = %s\n", new_string);
               if (!strncmp (new_string, "0x", 2))
                  {			// this is the hex representation of a binary term character
                     items = sscanf (&(new_string[2]), "%x", &new_int);
                     //printf (" the new int is %d\n", new_int);
                     if (items == 1)
                        {
                           if (new_int <= 255)
                              {
                                 my_sio->my_sio_port_table_entry.auto_mux_term_char = (char) new_int;
                              }
                           else
                              {
                                 my_sio->my_sio_port_table_entry.auto_mux_term_char = DEFAULT_TERMINATOR_CHAR;
                              }
                        }
                  }
               else
                  {
                     my_sio->my_sio_port_table_entry.auto_mux_term_char = DEFAULT_TERMINATOR_CHAR;
                  }
            }
#ifdef DEBUG_SIO	
         printf ("term character is %x\n", my_sio->my_sio_port_table_entry.auto_mux_term_char);
         printf (" auto-enabled = %d\n", my_sio->my_sio_port_table_entry.auto_mux_enabled);
#endif
         free (new_string);
         
         // now read in the destinations for the sios
         for (int i = 0; i < MAX_SIO_DESTINATIONS; i++)
            {
               snprintf (destination_string,MAX_DESTINATION_STRING_LENGTH, "destination_%d", i);
               new_destination = rov_read_string (ini_fd, my_name, destination_string, "NONE");
#ifdef DEBUG_CONFIG
               printf (" ini sio destination read, destination = %s\n", new_destination);
#endif
               if (0 != strcasecmp(new_destination, "NONE"))
                  { // this means there isn't a destination thread for this string
                     // now search through the global process table and look for the process naem
                     int dest_thread_num;
                     
                     dest_thread_num = JASONTALK_THREAD_NUM( new_destination );
                     
                     if( dest_thread_num >= 0)
                        {
                           
                           /*printf ("SIO_THREAD %s: Adding destination %s (thread %d) \n",
             my_name,
             new_destination,
             dest_thread_num);*/
                           
                           my_sio->my_sio_port_table_entry.auto_mux_address[my_sio->my_sio_port_table_entry.n_of_destinations++] = dest_thread_num;
                           
                        }
                     else
                        {
                           for(int q = 1; q < 10; q++)
                              printf ("SIO_THREAD %s: ERROR: Destination %s (thread %d) NOT FOUND!!!!!!!!\a\n",
                                      my_name,
                                      new_destination,
                                      dest_thread_num);
                           
                        }
                     
                  }
            }
         return 1;
      }
   else
      return 0;
   
   
   
}
