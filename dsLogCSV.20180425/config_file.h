/* ----------------------------------------------------------------------

   Config file code for ROV system

   Modification History:
   DATE               AUTHOR      COMMENT
   22 September 2001  J. Howland  created and written
   25 OCT 2003  LLW  Increased length limitations on INI file port data
                     that was causing crashes

---------------------------------------------------------------------- */


#include "sio_thread.h"
#include "nio_thread.h"

#ifndef CONFIG_FILE_INC
#define CONFIG_FILE_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbosr debugging 
// ----------------------------------------------------------------------
//#define DEBUG_CONFIG

//#define DEBUG_ROV_INI
// #define DEBUG_READ_VECTOR
// ----------------------------------------------------------------------

extern int rov_read_int (FILE * config_fd, const char *section, const char *attribute, int default_value);
extern int rov_read_hex (FILE * config_fd, const char *section, const char *attribute, int default_value);
extern double rov_read_double (FILE * config_fd, const char *section, const char *attribute, double default_value);
extern int rov_read_double_vec (FILE * config_fd, const char *section, const char *attribute, double *vec, int vec_len,double default_value);
extern char *rov_read_string (FILE * config_fd, const char *section, const char *attribute, const char *default_value);
 
extern int read_ini_serial_ports (FILE * config_fd);
extern int read_ini_sio_process (FILE * config_fd, char *my_name, sio_thread_t * my_sio);
extern int read_ini_nio_process (FILE * config_fd, char *my_name, nio_thread_t * my_nio);
extern int read_ini_logging_process (FILE * ini_fd);




#define MAX_CHARACTER_COUNT 512
#define DEFAULT_TERMINATOR_STRING "\r"
#define MAX_PORT_NAME_LENGTH  256
#define DEFAULT_TERMINATOR_CHAR '\r'
#define MAX_DESTINATION_STRING_LENGTH 256
#define ERRONEOUS_NIO_PORT  9999
#define ERRONEOUS_IP_ADDRESS  "ERROR_ADDRESS"
#define DEFAULT_BAD_WECON_ID  "BAD_WECON_ID"
#define MAX_WECON_ID      0xFF
#define DEFAULT_POWER_DRAW  0.0
#define DEFAULT_A2D_BIAS  0.0
#define DEFAULT_A2D_GAIN  1.0
#define NOT_BINARY                    0
#endif
