/* ----------------------------------------------------------------------

   data thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   7/2002       jch     to talk to data acquisition system

---------------------------------------------------------------------- */
#ifndef STATUS_PROCESS_PROCESS_INC
#define STATUS_PROCESS_PROCESS_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbosr debugging 
// ----------------------------------------------------------------------
 //#define DEBUG_STATUS_THREAD
 //#define DEBUG_CSV
// ----------------------------------------------------------------------

extern void *dsLogStatusThread (void *);
int   makeIsLoggingStatusMessage(promptLoggerStatus_t   *statusData,rov_time_t statTime, char *msg);
int   makePsLoggingStatusMessage(promptLoggerStatus_t *statusData, int index, rov_time_t statusTime, char *msg);
int   makeSudsStatusMessage(promptLoggerStatus_t   *statusData, rov_time_t statusTime, char *msg);
int   makeLoggingStatusMessage(loggerStatus_t   *statusData,rov_time_t statTime, char *msg);
int   makeDestinationStatusMessage(DestinationStatus_t   *statusData,rov_time_t statTime, char *msg);

#define STATUS_PROMPT_INTERVAL 5.0
#define CONFIGURATION_PROMPT_INTERVAL  1800.0
#define  DISK_PROMPT_INTERVAL 30.0

#define STATUS_MSG_LENGTH        2048

#endif
