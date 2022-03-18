#ifndef DSLOGTYPES_H
#define DSLOGTYPES_H


#include <arpa/inet.h>		/* for sockaddr_in and inet_addr() */



typedef enum
{
   UDP_ASCII,
   UDP_BINARY,
   SERIAL_ASCII,
   SERIAL_BINARY
} source_type_t;



typedef struct {
   char    *ipAddress;
   int     toSocketNumber;
   int     socket;
   struct  sockaddr_in Addr;
   bool    active;
} networkDestination_t;

typedef struct {
   char   *loggingDirectory;
   int    lastHour;
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

#endif // DSLOGTYPES_H
