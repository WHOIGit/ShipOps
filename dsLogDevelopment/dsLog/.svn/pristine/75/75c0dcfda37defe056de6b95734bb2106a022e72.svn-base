#ifndef DSLOGCSV_H
#define DSLOGCSV_H
/* ----------------------------------------------------------------------

  deep submergence logger  header

   Modification History:
   DATE 	AUTHOR 	COMMENT
  04 Nov 2009   jch     create from rov.h

  2012-12-20   tt       - mods to have prompt specific destinations
  2012-12-21   tt       - remove nOfPromptSequences from logging_descriptor_t
  2012-12-21   tt       - replace promptSequences[] array with promptSequence in logging_descriptor_t
  2012-12-22   tt       - changes to support prompt specific termination characters
  2012-12-22   tt       - changes to support prompt specific data types and source names
  2012-12-22   tt       - changes to support prompt specific syncopation
  2012-12-26   tt       - changes to support prompt specific reply window
  2013-01-02   tt       - changes to support destination specific file prefix and extension
  2013-05-01   tt       - added inputNumber to logging_descriptor_t
---------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "launch_timer.h"
#include "dsLogTalk.h"

#ifndef ROV_PROCESS_INC
#define ROV_PROCESS_INC

//#define  DEBUG_ROV_INI

#define DEFAULT_INI_FILE  "dslog.ini"

#define NULL_EXTRA_ARG_VALUE  -9999

#define LOGGING_STATUS_INTERVAL  5.0


#define MAX_NUMBER_OF_THREADS    1024

#define DEFAULT_NO_ENTRY        "NO_ENTRY"
#define NO_SOURCE_TYPE          99
#define NO_IN_SOCKET            99
#define NULL_GREP_FIELD         "NO_GREP_FIELD_ENTERED"
#define DEFAULT_NO_SERIAL_PORT  "NO_SERIAL_PORT"
#define DEFAULT_PATHNAME        "."
#define MAX_N_OF_INPUTS         256
#define LOG_THREAD_OFFSET       512
#define MAX_LOGGING_STRING_LENGTH   4096

#define MAX_N_OF_CSV_PARSE 16

typedef struct
{
   int thread_num;
   char *thread_name;
   void *(*thread_function) (void *);
   void *thread_launch_arg;
   int extra_arg_1;
   int extra_arg_2;
   int extra_arg_3;
   int extra_arg_4;
   pthread_t thread;
}
thread_table_entry_t;


typedef struct {
   char     *sio_com_port_name;
   int      baud;
   int      parity;
   int      data_bits;
   int      stop_bits;
   int      flow_control;
   int      echo;
   char     auto_mux_term_char;
   int      auto_mux_enabled;		// if this is set to 1 (true) then check the term character
   double   busTimeout;
   double   pollingInterval;

} serialPort_t;

//----------------------------------------------
#define PARSE_VALUE_TYPE_INT     0
#define PARSE_VALUE_TYPE_DOUBLE  1
#define PARSE_VALUE_TYPE_STRING  2

#define PARSE_INT_FORMAT_TYPE_DECIMAL     0
#define PARSE_INT_FORMAT_TYPE_HEX_NONE    1
#define PARSE_INT_FORMAT_TYPE_HEX_0X      2
#define PARSE_INT_FORMAT_TYPE_OCTAL_NONE  3


typedef struct
{
  int csvParseIndex;  // indicates what field this is
  int valueType;
  int intFormatType;
  int intValue;
  double doubleValue;
  char *prefixString;
  char *delimiterString;
  int parameterIndex;
  char *rawScanfString;

}   csv_parse_descriptor_t;

typedef struct
{
  char                 *sourceName;
  char                 *dataType;
  char                 *filenamePrefix;
  source_type_t        sourceType;
  int                  inSocketNumber;
  int                  outSocketNumber;
  char                 *outIpAddress;
  serialPort_t         theSerialPort;
  int                  timeStampFlag;
  int                  freeRunPromptFlag; 
  int                  dailyLogfilesFlag; 
  int                  numberOfCsvParse;
  csv_parse_descriptor_t csvParse[MAX_N_OF_CSV_PARSE];
  int                  useDestinationSpecificFileExtension; 
  int                  usePromptSpecificDestinationsFlag; 
  int                  usePromptSpecificTermChar; 
  int                  usePromptSpecificDataType; 
  int                  usePromptSpecificSourceName; 
  int                  usePromptSpecificSyncopation; 
  int                  usePromptSpecificReplyWindows; 
  int                  promptDestinationEnable[MAX_N_OF_PROMPTS_IN_SEQUENCE][MAX_N_OF_DESTINATIONS]; 
  int                  promptEnable[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  int                  promptSyncopation[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  int                  promptReplyWindows[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  char                 promptTermChar[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  char                 *promptDataType[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  char                 *promptSourceName[MAX_N_OF_PROMPTS_IN_SEQUENCE];
  prompt_sequence_t    promptSequence;
  distribution_t       destinations[MAX_N_OF_DESTINATIONS];
  int                  nOfDestinations;
  bool                 active;
  int                  inputNumber;
  int                  threadNumber;
  //int                  numberOfCsvParse;
  //csv_parse_descriptor_t csvParse[MAX_N_OF_CSV_PARSE];
}   logging_descriptor_t;

extern int make_thread_table_entry (int thread_num, const char *thread_name, void *(*thread_function) (void *), void *thread_launch_arg, int extra_arg_1, int extra_arg_2, int extra_arg_3, int extra_arg_4);

extern thread_table_entry_t global_thread_table[MAX_NUMBER_OF_THREADS];
extern int readIniDslogProcess(FILE * ini_fd, logging_descriptor_t *loggingList, int *nOfLoggers,distribution_t *singleLogFile, logging_descriptor_t *singleLog);
extern  int logOpenAsciiLogFile (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor);
extern  int logThisNow(distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData);
extern  void flushAsciiLogAndClose (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData);


extern char *dsLogIniFilename;
extern startup_t  startup;
extern  logging_descriptor_t     theLoggingList[MAX_N_OF_INPUTS];
extern  distribution_t           theGlobalLogFile;
extern  logging_descriptor_t     theGlobalLog;



#endif


#endif // DSLOGCSV_H
