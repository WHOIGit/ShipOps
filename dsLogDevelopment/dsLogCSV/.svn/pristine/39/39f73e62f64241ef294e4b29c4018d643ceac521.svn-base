/* ----------------------------------------------------------------------

   Logging thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   07-03-08     SCM     CRExATED FROM ADLIENT STUFF
   2007-08-29    mvj    Kludged maximum message length to avoid segfault
                        on my Ubuntu 7.04 machine.

   2007-09-23   LLW     print timestamp string first.
   2008-04-21    mvj    Added abort_printf for terminal error messages.

   2008-07-20   LLW     changed char * to const char *
*/

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msg_util.h"
#include "dsLogTalk.h"

int stderr_printf(const char * fmt, ...)
{

  char   err_printf_str[10240];
  char   full_str[10240];
  int    err_printf_strlen = 0;
  int    full_strlen;
  char   tmp_str[4097];

  //Define the random number of variables above 1
  va_list argptr;
  

  // prefix the string with date and time stamp, followed by a space
  full_strlen  = rov_sprintf_dsl_time_string(full_str);
  full_strlen += sprintf(&full_str[full_strlen]," ");

  //Create a char string of the variables defined above
  va_start(argptr, fmt);
  err_printf_strlen += vsprintf(&err_printf_str[err_printf_strlen], fmt, argptr);
  va_end(argptr);

  // log it to the system log file
  //SEND_TO_LOGGER(SDE, ANY_THREAD, err_printf_strlen, err_printf_str);  @@@ mvj This line eventually causes a segfault for strings longer than approximately 4103.
  strncpy(tmp_str,err_printf_str,4096);
  SEND_TO_LOGGER(SDE, ANY_THREAD, strlen(tmp_str), tmp_str);


  //Combine the TimeStamp with the char string
  strcat(full_str, err_printf_str);
  full_strlen = full_strlen + err_printf_strlen;

  // write to stderr memo
  printf("%s",full_str);

  return(full_strlen);

}

void stderr_abort( char * fmt, ...)
{
  char   err_printf_str[10240];
  int    err_printf_strlen = 0;

  //Define the random number of variables above 1
  va_list argptr;
  

  // Prepend some indicator that a thread has aborted.
  strcat(err_printf_str,"!! ABORT !! ");
  err_printf_strlen = strlen(err_printf_str);

  //Create a char string of the variables defined above
  va_start(argptr, fmt);
  err_printf_strlen += vsprintf(&err_printf_str[err_printf_strlen], fmt, argptr);
  va_end(argptr);

  stderr_printf(err_printf_str);
  fflush(stdout);
  fflush(stderr);
  abort();
		
}
