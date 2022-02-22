/* ----------------------------------------------------------------------

   Time functions

   Modification History:
   DATE         AUTHOR  COMMENT
   23-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
#ifndef TIME_UTIL_INC
#define TIME_UTIL_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbosr debugging 
// ----------------------------------------------------------------------
// #define DEBUG_TIME
// ----------------------------------------------------------------------

typedef double rov_time_t;

extern rov_time_t rov_get_time (void);

extern double rov_diff_time (rov_time_t t1, rov_time_t t0);

int noaa_sprintf_date_string (char *str);
int noaa_sprintf_time_string (char *str);

extern int dslog_sprintf_date_string (char *str);
extern int dslog_sprintf_time_string (char *str);
extern int dslog_sprintf_date_time_string (char *str, char *delimiter);

extern int rov_sprintf_dsl_time_string (char *str);
extern int rov_convert_dsl_time_string (double total_secs, char *str);

#endif
