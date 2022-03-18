/* ----------------------------------------------------------------------

   Data Structures for Jason Interprocess Communication


   Modification History:
   DATE        AUTHOR   COMMENT
   25-SEP-1992 LLW      Created and written.
   25 JUN 95   LLW      Added WGY
   09 APR 96   RLE      Added PCG (Page Change Command).
	       RLE      Defines for Hydrolic Pump, Ram In and Ram Out,
	       RLE      and Solenoid Enable.
   16 AUG 96   LLW      Added Jasontalk messages:
                                WMV  - write mag variation
                                WLA  - wite latitide,
                                WLO  - wite longitude
                                WFC  - write film count

                        Removed unised obsolete jasontalk messages
                                WRITE_HEADING_VARIATION

   27-FEB-00 LLW Ported to Posix
   10-JUL-2000 LLW      Ported to Linux
                        uses __WIN32__ and __LINUX__ precompiler #defines

   23-JUL-00 LLW    New hotel thread
   24-JUL-00 LLW    New sensor thread
   25-JUL-00 LLW    New control thread
   26-NOV-01 JCH    many changes to numbers so as to use a new numbering scheme
   19 Feb 2002 LLW  moved header stuff from dsLogTalk.h and global.h
                    to the place that they belong
   2002-04-03 TET   added support for MTS thread
  4 MAR 2002 LLW Added dummy WMC Command to mts_thread 
 27 June 2002 LLW Thruster information is moved to control.fb.thalloc in control_thread.cpp
   07 JUL 02 LLW Added support for multiple depth sensors on vehicle 
   07 JUL 02 LLW increase to total of four paro sio threads  
   08 Jul 02 LLW  Added MTZ vectorized motor velocity command
   12 Oct 2003 LLW  Added SRC_SHARPS
   12 oct 2003 LLW  Removed #define  JHU_PILOT_JOY_BOX_SIO_THREAD  29  from service
   18 Oct 2003 LLW  Added KVH_ADGC_SIO_THREAD for JHU ROV
   25 Oct 2003 LLW  Cleaned up thread names
   27 OCT 2003  JCK&LLW XVISION  
   12 MAY 2004 LLW  Added WTT, WTT2, WGD2, WCG2, WSS2, WST2, WGI, WGI2, WRA, WRA2, WRI, WRI2
   14 OCT 2004 LLW  Added SRC_DVLNAV_ALTITUDE to state_source_t

   30 APR 2012 TET  Added promptLoggerStatus_t to support prompt specific logger messages
   01 MAY 2012 TET  Added ISLS message type
   ---------------------------------------------------------------------- */
#ifndef JASONLOGTALK_INC
#define JASONLOGTALK_INC

#define     TRUE                1
#define     FALSE               0

// declare this a posix source
#define _POSIX_SOURCE 1

// set for LINUX compiles.  Comment this our for windows.
#define __LINUX__
// note that windows compiler will automatically define __WIN32__

// pthread header files
#include <pthread.h>
#include <limits.h>

#include <sys/socket.h>		/* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>		/* for sockaddr_in and inet_addr() */

/* include jason system configuration file */
#include "time_util.h"
#include "launch_timer.h"

#define MAX_N_OF_PROMPTS_IN_SEQUENCE      32

#define  MAX_PROMPT_LENGTH       16

#define  DEFAULT_SEQUENCE_INTERVAL    1.0

#define MAX_N_OF_DESTINATIONS   64

typedef enum
{
   UDP_ASCII,
   UDP_BINARY,
   SERIAL_ASCII,
   SERIAL_BINARY
} source_type_t;

typedef enum
{
    UDP_SOCKET,
    DISK,
    SINGLE_DISK,
    NOT_A_DESTINATION
} destinationType_t;



typedef struct {
   char    *ipAddress;
   int     toSocketNumber;
   int     socket;
   struct  sockaddr_in Addr;
   bool    active;
} networkDestination_t;

/*-----------------------------------------------------------------------------------
  2013-01-02   tt       - changes to support destination specific file prefix and extension
*/
typedef struct {
   char   *loggingDirectory;
   char   *fileExtension;
   char   *filenamePrefix;
   int    lastHour;
   int    lastDay;
   char   asciiLogFileName[256];
   FILE   *asciiLogFileD;
   bool   active;
}  loggingDestination_t;

typedef struct {
   char    *startsWith;
   char    *greps;
   char    *grepMinusVs;
   int     implemented;
} destinationFilter_t;

typedef struct {
   int destinationEnable;
   int rawMode;
   destinationType_t       destinationType;
   destinationFilter_t     destinationFilter;
   networkDestination_t    networkDestination;
   loggingDestination_t    loggingDestination;

} distribution_t;



typedef struct {
   unsigned char     promptChars[MAX_PROMPT_LENGTH];
   int               promptCharLength;
} prompt_t;

typedef struct {
   prompt_t    prompts[MAX_N_OF_PROMPTS_IN_SEQUENCE];
   double      secondsBetweenPrompts;
   double      secondsBetweenSequences;
   int         numberOfPrompts;
   launched_timer_data_t   *betweenSequenceTimer;
   launched_timer_data_t   *betweenPromptTimer;


} prompt_sequence_t;


typedef struct 
{
  rov_time_t   startup_time;
  int          running;
  int          nOfLogInputs;
  char         *rootDirectory;
  char         *defaultFilenamePrefix;
} startup_t;

typedef struct
{
  rov_time_t   timeOfLastMsg;
  rov_time_t   statusTime;
  double       timeSinceLastData;
  unsigned int msgsRecieved;
  unsigned int bytesReceived;
  unsigned int readErrors;


} loggerStatus_t;

typedef struct
{
  int sourceNumber;
  char sourceName[32];
  char promptSourceName[MAX_N_OF_PROMPTS_IN_SEQUENCE][32];
  rov_time_t   timeOfLastMsg;
  rov_time_t   statusTime;
  double       timeSinceLastData;
  unsigned int msgsRecieved;
  unsigned int bytesReceived;
  unsigned int readErrors;

  int numberOfPrompts;
  loggerStatus_t promptStatus[MAX_N_OF_PROMPTS_IN_SEQUENCE];
} promptLoggerStatus_t;

typedef struct
{

   destinationType_t    destinationType;
   int                  bytesSent;
   int                  errorCount;
   unsigned long long             availableSpace;
   unsigned long long             freeSpace;
   double                percentageFree;

}  indDestinationStatus_t;

typedef struct
{
   indDestinationStatus_t  statuses[MAX_N_OF_DESTINATIONS];
   int                     nOfDestinations;
   rov_time_t              statusTime;
}  DestinationStatus_t;

#define DSLOG_PROGRAM_VERSION "dsLog V1.0"



/* ----------------------------------------------------------------------
   Define a canonical ID number for each thread
   ---------------------------------------------------------------------- */


#define ANY_THREAD                    0  /* used when thread doesnt matter--such as source in logging */
#define DSLOG_STATUS_THREAD           1
#define STDERR_THREAD                 2
#define CMD_0_SIO_THREAD              3
#define CMD_THREAD_0                  9
#define GUI_THREAD                    10
#define TCP_SERVER_THREAD             11

#define NULL_THREAD_ADDRESS               255




/* ----------------------------------------------------------------------
   Message types defined for use in the "type" field of message headers.
   See protocol documentation for the different variable length data
   which accompany each message type.


   ---------------------------------------------------------------------- */

/* ---------    SYSTEM STATUS MESSAGES TYPES ----------*/
#define HLP  0			/* help.  responds with a SSS */
#define PNG  1			/* ping message         */
#define SPI  2			/* ping message response */
#define RST  3			/* RESET message        */
#define VER  4			/* VERBOSE message      */


#define BYE  5		        /* flush logs and exit */


// look down to find the WMS, RMS, SMS messages!


/* ---------    SERIAL I/O MESSAGE TYPES ----------*/
#define WAS 113			/* write ascii serial string */
#define RAS 313			/* read ascii serial string */
#define SAS 513			/* ascii serial string status */

#define WHS 114			/* write hex serial string */
#define RHS 314			/* read hex serial string */
#define SHS 514			/* hex serial string status */

#define WSX 115			/* write:  terminal mode ON  */
#define WSY 116			/* write:  terminal mode OFF */

#define WSP 117			/* Configure Serial Port    */
#define RSP 317
#define SSP 517			/* Status Serial Port    */
#define RTD  231        /* request thread data */

#define  STPR  230   // status prompt
#define  ALD   231   // add logging destination
#define  RLD   232   // remove logging destination
#define  ALT   233   // add logging thread
#define  LSTM  234   // io thread status msg;
#define  CFPR  235   // log configuration prompt
#define RDSP   236   // check on disk space prompt
#define LSDM   237   // destination status report
#define INIR   238   // request for tcp delivery of ini file
#define TCPC   239   // timer request from tcp thread

#define  DSPR  240 // disk space prompt

#define ISLS 241   // input specific logging status - added by tet on 2013-05-01

#define CSVMSG 242  // aded by tet on 2014-01-01

// ----------------------------------------------------------------------
// END of Jasontalk Command message types
// ----------------------------------------------------------------------


/* ====================================================================== 
   functions
   =================================================================== */

extern const char * JASONTALK_MESSAGE_NAME (int num);
extern const char * JASONTALK_THREAD_NAME (int num);
extern int    JASONTALK_THREAD_NUM( char * name);
#endif

/* ----------------------------------------------------------------------
      END OF FILE JASONTALK.H
   ---------------------------------------------------------------------- */
