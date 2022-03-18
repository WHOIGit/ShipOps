/* ----------------------------------------------------------------------

   Time functions

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.
   01-JAN-2002  LLW     Ported to RedHat Linux 7.2
---------------------------------------------------------------------- */
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "time_util.h"		/* time utilities */


/* ----------------------------------------------------------------------

   get time (since 1970?) in seconds, LSD is microseconds, granularity is 
   not known

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
rov_time_t
rov_get_time (void)
{

  struct timeval tv;
  struct timezone tz;
  double t;

  time_t current_time;
  struct tm tm;

  current_time = time (NULL);
  // 18 July 2012 fix to use threadsafe localtime_r

  localtime_r (&current_time,&tm);


  gettimeofday (&tv, &tz);

  t = (((double) tv.tv_sec) + (((double) tv.tv_usec) * 0.000001));

  // printf("TIME:  time=%ld sec, %ld usec, %f\n",tv.tv_sec, tv.tv_usec, t);

  return t;

}


/* ----------------------------------------------------------------------

   difference between two time values

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
double
rov_diff_time (rov_time_t t1, rov_time_t t0)
{
  return (t1 - t0);
}


/* ----------------------------------------------------------------------

   sprintfs dsl data time string based on computer system clock

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   14 APR 1999  Louis Whitcomb  Created and Written

   ---------------------------------------------------------------------- */
int
rov_sprintf_dsl_time_string (char *str)
{
  int num_chars;

  // for min, sec
  double total_secs;
  double secs_in_today;
  //   double day;
  double hour;
  double min;
  double sec;

  // for date and hours
  struct tm tm;
  time_t current_time;


  // read gettimeofday() clock and compute min, and
  // sec with microsecond precision
  total_secs = rov_get_time ();
  secs_in_today = fmod (total_secs, 24.0 * 60.0 * 60.0);
  hour = secs_in_today / 3600.0;
  min = fmod (secs_in_today / 60.0, 60.0);
  sec = fmod (secs_in_today, 60.0);

  // call time() and localtime for hour and date 

  // 18 July 2012 fix to use threadsafe localtime_r
  current_time = time (NULL);
  localtime_r (&current_time,&tm);

  num_chars = snprintf (str,32,"%04d/%02d/%02d %02d:%02d:%06.3f", (int) tm.tm_year +1900, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) min, sec);


  return num_chars;

}

/* ----------------------------------------------------------------------

   sprintfs dsl data time string based on an input time in seconds

   MODIFICATION HISTORY
   DATE         WHO             WHAT
   -----------  --------------  ----------------------------
   11 Mar 2002  J Howland       created from the above function

   ---------------------------------------------------------------------- */
int
rov_convert_dsl_time_string (double total_secs, char *str)
{
  int num_chars;

  // for min, sec
  
  double secs_in_today;
  //   double day;
  double hour;
  double min;
  double sec;

  // for date and hours
  struct tm tm;
  time_t current_time;


  // read gettimeofday() clock and compute min, and
  // sec with microsecond precision
  
  secs_in_today = fmod (total_secs, 24.0 * 60.0 * 60.0);
  hour = secs_in_today / 3600.0;
  min = fmod (secs_in_today / 60.0, 60.0);
  sec = fmod (secs_in_today, 60.0);

  // call time() and localtime for hour and date 
  // 18 July 2012 fix to use threadsafe localtime_r

  current_time = time (NULL);
  localtime_r (&current_time,&tm);

  num_chars = snprintf (str, 32,"%04d/%02d/%02d %02d:%02d:%06.3f", (int) tm.tm_year +1900, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) min, sec);


  return num_chars;

}

