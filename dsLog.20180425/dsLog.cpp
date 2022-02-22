
/* ----------------------------------------------------------------------
    Modification History:
    DATE        AUTHOR  COMMENT
    3 Nov 2009		jch		create from rov cod

  2012-12-20   tt       - mods to have prompt specific destinations
  2012-12-21   tt       - remove nOfPromptSequences from logging_descriptor_t
  2012-12-21   tt       - replace promptSequences[] array with promptSequence in logging_descriptor_t
  2012-12-22   tt       - changes to support prompt specific termination characters
  2012-12-22   tt       - changes to support prompt specific dataType and sourceName fields
  2012-12-22   tt       - changes to support prompt specific syncopation
  2012-12-26   tt       - changes to support prompt specific reply window
  2013-01-02   tt       - changes to support destination specific file prefix and extension
  2013-02-15   tt       - changes to support running this program as a daemon
  2013-04-19   tt       - changes to support disabling a specific input section in the ini_file
  2013-04-19   tt       - changes to support using free_running prompt sequences on 485 loops
  2013-04-24   tt       - changes to support specifying whether new log files are started each hour or daily
  2013-05-01   tt       - initialize new element 'inputNumber' in logging descriptor
  2013-05-30   tt       - changes to support servicing serial and udp sources with a single thread - eliminating nioLogThread.cpp

---------------------------------------------------------------------- */

/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//or other similar situations
#include <unistd.h>
#include <sys/time.h>

/* posix header files */

#include <pthread.h>
#include <limits.h>

//2013-02-15  tt - added following includes as part of effort to run as daemon
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <vector>
#include <algorithm>
//#include "stderr.h"
//2013-02-15  tt - end of additions ------

#include "dsLog.h"		/* main() header file */
#include "config_file.h"        /* ini file reading functions */


#include "cmd_thread.h"		/* cmd (backdoor) thread */
#include "sio_thread.h"		/* backdoor thread */
#include "nio_thread.h"		/* nio thread */
#include "time_util.h"		/* time utilities */
#include "msg_util.h"

#include "dsLogStatusThread.h"

#include "nioLogThread.h"
#include "sioLogThread.h"
#include "tcp_thread.h"

//2013-02-15  tt - added to prevent vector was not declared in this scope build error 
using namespace std; 

// some globals, moved from process.cpp

//----------------------------------------------------------------
//  this is the filename which is read as the argument to the main
//  program and stored as a double so everyvody can read it
//----------------------------------------------------------------

char        *dsLogIniFilename;
startup_t   startup;



// ----------------------------------------------------------------------
// This is a structure that is initialized in rov.cpp to contain
// default thread attributes for launching threads. 
// It is provided as global variable as a convenience to other threads.
// ----------------------------------------------------------------------
pthread_attr_t DEFAULT_ROV_THREAD_ATTR;

// ----------------------------------------------------------------------
// This is a table of channel pairs that contains all the communication
// channels used in the entire system.  
// Each channel pair connects a single thread to the router.
// the router itself is process 0.
// This array is initialized in rov.cpp
// ----------------------------------------------------------------------


// the global thread table

thread_table_entry_t    global_thread_table[MAX_NUMBER_OF_THREADS];
logging_descriptor_t    theLoggingList[MAX_N_OF_INPUTS];
distribution_t          theGlobalLogFile;
logging_descriptor_t    theGlobalLog;

#define ROV_THREAD_DEFAULT_STACKSIZE 2000000


//---------------------------------------------------------------------------i
int
      main (int argc, char *argv[])
{
   int i;
   int status;


   size_t size;

   FILE *inFD;
   startup.running = 0;
   startup.startup_time = rov_get_time();
   startup.nOfLogInputs  = 0;

   int   nOfLogInputs;

   //2013-02-15 - added variables for daemonizing
   bool daemonize = false;
   int devnull   = -1;       // Daemonize variable

   //2013-02-15 - added code to daemonize program
   vector<string> args(argv, argv + argc);
   if (find(args.begin(), args.end(), "-d") != args.end())   //GT
     daemonize = true;

   fprintf (stderr, "File %s compiled on %s at %s \n", __FILE__, __DATE__, __TIME__);

   // initialize the thread attributes
   pthread_attr_init (&DEFAULT_ROV_THREAD_ATTR);

   // set for detached threads
   pthread_attr_setdetachstate (&DEFAULT_ROV_THREAD_ATTR, PTHREAD_CREATE_DETACHED),
   // read and set the default stack size
   pthread_attr_getstacksize (&DEFAULT_ROV_THREAD_ATTR, &size), fprintf (stderr, "Default PTHREAD stack size was %d (0x%08X)\n", size, size);

   pthread_attr_setstacksize (&DEFAULT_ROV_THREAD_ATTR, ROV_THREAD_DEFAULT_STACKSIZE), pthread_attr_getstacksize (&DEFAULT_ROV_THREAD_ATTR, &size), fprintf (stderr, "New PTHREAD stack size was %d (0x%08X)\n", size, size);


   // time to read the startup file
   // first check for an argument

   if (argc > 1)
      {
         dsLogIniFilename = strdup (argv[1]);
         fprintf (stderr, "ini file argument is %s\n", dsLogIniFilename);
      }
   else
      {
         dsLogIniFilename = strdup (DEFAULT_INI_FILE);
         fprintf (stderr, "default ini file: %s\n", dsLogIniFilename);
      }

   // now try to open the file

   inFD = fopen (dsLogIniFilename, "r");
   if (!inFD)
      {
         fprintf (stderr, "%s ini file does not exist...exiting!\n", dsLogIniFilename);
         return -1;
      }


   if (inFD)
      {

         readIniDslogProcess(inFD,&(theLoggingList[0]),&nOfLogInputs,&theGlobalLogFile, &theGlobalLog);
         startup.running =  1;
         startup.nOfLogInputs = nOfLogInputs;
         fclose (inFD);
      }

   msg_queue_initialize();



   // fill the thread table.  first mark all entries so only full entries will be launched

   for (i = 0; i < MAX_NUMBER_OF_THREADS; i++)
      {
         global_thread_table[i].thread_function = NULL;
      }


   //make_thread_table_entry (LOGGING_THREAD, "LOGGING_THREAD", logging_thread, (void *)NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);

   make_thread_table_entry (TCP_SERVER_THREAD, "TCP_SERVER_THREAD", tcp_server_thread, (void *) TCP_SERVER_THREAD, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);
   make_thread_table_entry (CMD_0_SIO_THREAD, "CMD_SIO_THREAD", sio_thread, (void *) CMD_0_SIO_THREAD, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);
   make_thread_table_entry (GUI_THREAD, "GUI_THREAD", nio_thread, (void *) GUI_THREAD, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);
   make_thread_table_entry (CMD_THREAD_0, "CMD_THREAD_0", cmd_thread, (void *) CMD_THREAD_0, NULL_EXTRA_ARG_VALUE, CMD_0_SIO_THREAD, 1, NULL_EXTRA_ARG_VALUE);
   make_thread_table_entry (DSLOG_STATUS_THREAD, "DSLOG_STATUS_THREAD",dsLogStatusThread,(void *)DSLOG_STATUS_THREAD ,NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE );
   for(int nOfLoggers = 0; nOfLoggers < nOfLogInputs; nOfLoggers++)
      {
         char   threadName[256];
         if( UDP_ASCII == theLoggingList[nOfLoggers].sourceType)
            {

               snprintf(threadName,256, "UDP_LOG_%d",nOfLoggers);
               make_thread_table_entry (LOG_THREAD_OFFSET + nOfLoggers, threadName, sioLogThread, (void *) (LOG_THREAD_OFFSET + nOfLoggers),nOfLoggers, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE,NULL_EXTRA_ARG_VALUE);
               theLoggingList[nOfLoggers].threadNumber = LOG_THREAD_OFFSET + nOfLoggers;
            }
         else if(SERIAL_ASCII == theLoggingList[nOfLoggers].sourceType)
            {
               snprintf(threadName,256, "SERIAL_LOG_%d",nOfLoggers);
               make_thread_table_entry (LOG_THREAD_OFFSET + nOfLoggers, threadName, sioLogThread, (void *) (LOG_THREAD_OFFSET + nOfLoggers),nOfLoggers, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE,NULL_EXTRA_ARG_VALUE);
               theLoggingList[nOfLoggers].threadNumber = LOG_THREAD_OFFSET + nOfLoggers;

            }

      }

   /* ------------------------------------------------------------
      Standard DAEMONIZE function (GT)
      2010-04-21 JCK - ported to gravlog from nav r745
      2013-02-15 tt  - ported to dsLog from gravlog
      ------------------------------------------------------------ */
   
   if (daemonize) {
     fprintf(stderr, "DAEMONIZING DSLOG\n");
     
     // Daemon: Our process ID and Session ID
     pid_t pid, sid;
     
     // Daemon: Ignore sigchld, to prevent flesh-eating.
     struct sigaction action;
     action.sa_handler = SIG_IGN;
     sigaction(SIGCHLD, &action, NULL);
     
     // Daemon: Fork off the parent process
     pid = fork();
     if (pid < 0){
       exit(EXIT_FAILURE);
     }
     // Daemon: If we got a good PID, then we can exit the parent process.
     if (pid > 0) {
       exit(EXIT_SUCCESS);
     }
     
     // Daemon: Change the file mode mask
     umask(0);
     
     // Daemon: Create a new SID for the child process
     sid = setsid();
     if (sid < 0) {
       fprintf(stderr, "DSLOG.CPP: Not possible to get new SID.\n");
       exit(EXIT_FAILURE);
     }
     
     // Daemon: Close out the standard file descriptors
     devnull = open("/dev/null", O_RDWR);
     if (devnull == -1) {
       fprintf(stderr, "DSLOG.CPP: Unable to open /dev/null \n");
       exit(EXIT_FAILURE);
     }
     // unhook from stdio
     dup2(devnull, STDIN_FILENO);
     dup2(devnull, STDOUT_FILENO);
     dup2(devnull, STDERR_FILENO);
     // Closing all file descriptors
     close(devnull);
  }  // end of if(daemonize)

   
   // launch all the system threads

   for (i = 0; i < MAX_NUMBER_OF_THREADS; i++)
      {
         if (global_thread_table[i].thread_function != NULL)
            {
               status = pthread_create (&global_thread_table[i].thread, &DEFAULT_ROV_THREAD_ATTR, global_thread_table[i].thread_function, global_thread_table[i].thread_launch_arg);

            }
      }
   // sleep for a bit to give the threads time to start up
   usleep(500000);


   do{
         char c = getchar ();
         if('q' == c)
            {

               printf("\nBYE....\n");
               startup.running = 0;
               return 0;
            }
         if(EOF == c)
            {
               continue;
            }
         else
            {
               printf ("running...\n");
            }
      } while (startup.running);

}

int make_thread_table_entry (int thread_num, 
                             const char *thread_name,
                             void *(*thread_function) (void *),
                             void *thread_launch_arg,
                             int extra_arg_1,
                             int extra_arg_2,
                             int extra_arg_3,
                             int extra_arg_4)
{


   global_thread_table[thread_num].thread_num = thread_num;
   global_thread_table[thread_num].thread_name = strdup(thread_name);
   global_thread_table[thread_num].thread_function = thread_function;
   global_thread_table[thread_num].thread_launch_arg = thread_launch_arg;

   if (extra_arg_1 != NULL_EXTRA_ARG_VALUE)
      global_thread_table[thread_num].extra_arg_1 = extra_arg_1;
   if (extra_arg_2 != NULL_EXTRA_ARG_VALUE)
      global_thread_table[thread_num].extra_arg_2 = extra_arg_2;
   if (extra_arg_3 != NULL_EXTRA_ARG_VALUE)
      global_thread_table[thread_num].extra_arg_3 = extra_arg_3;
   if (extra_arg_4 != NULL_EXTRA_ARG_VALUE)
      global_thread_table[thread_num].extra_arg_4 = extra_arg_4;


   return 1;
}


int readIniDslogProcess(FILE * ini_fd, logging_descriptor_t *loggingList, int *nOfLoggers,distribution_t *singleLogFile,logging_descriptor_t *singleLog)
{
  *nOfLoggers = 0;
  int loggingCount = 0;
  int globalDailyLogfilesFlag = 0;
  
  globalDailyLogfilesFlag = rov_read_int(ini_fd, "GENERAL", "DAILY_LOGFILES_FLAG", 0);
  
  startup.rootDirectory = rov_read_string(ini_fd,"GENERAL","ROOT_DIR",DEFAULT_PATHNAME);
  singleLogFile->loggingDestination.loggingDirectory = startup.rootDirectory;
  
  char *scratchString = rov_read_string(ini_fd,"GENERAL","GENERIC_DATA_TYPE","DAT");
  singleLog->dataType = strdup( scratchString);
  free(scratchString);
  
  scratchString = rov_read_string(ini_fd,"GENERAL","FILENAME_PREFIX",DEFAULT_NO_ENTRY);
  if(!strcmp(DEFAULT_NO_ENTRY,scratchString)) { // no entry
    startup.defaultFilenamePrefix = strdup("");
  }
  else {
    startup.defaultFilenamePrefix = strdup( scratchString);
  }
  free(scratchString);
  
  for(int inputNumber = 0; inputNumber < MAX_N_OF_INPUTS; inputNumber++) {
    logging_descriptor_t    thisDescriptor;
    char inputField[256];
    snprintf(inputField,256,"INPUT_%d",inputNumber);
    
    // has the user disabled this input in the ini file??
    if(0 == rov_read_int(ini_fd, inputField, "INPUT_ENABLE", 1) ) { // user has disabled this input
      continue;
    }
    
    scratchString = rov_read_string(ini_fd,inputField,"DATA_TYPE",DEFAULT_NO_ENTRY);
    if(!strcmp(DEFAULT_NO_ENTRY,scratchString)) { // no entry
      continue;
    }
    else {
      thisDescriptor.dataType = strdup( scratchString);
    }
    free(scratchString);
    scratchString = rov_read_string(ini_fd,inputField,"DATA_SOURCE",DEFAULT_NO_ENTRY);
    if(!strcmp(DEFAULT_NO_ENTRY,scratchString)) { // no entry
      continue;
    }
    else {
      thisDescriptor.sourceName = strdup( scratchString);
    }
    free(scratchString);
    scratchString = rov_read_string(ini_fd,inputField,"FILENAME_PREFIX",DEFAULT_NO_ENTRY);
    if(!strcmp(DEFAULT_NO_ENTRY,scratchString)) { // no entry
      thisDescriptor.filenamePrefix = strdup("");
    }
    else {
      thisDescriptor.filenamePrefix = strdup( scratchString);
    }
    free(scratchString);
    int scratchInteger = rov_read_int(ini_fd,inputField,"SOURCE_TYPE",NO_SOURCE_TYPE);
    if(NO_SOURCE_TYPE == scratchInteger) {
      continue;
    }
    else {
      thisDescriptor.sourceType = (source_type_t)scratchInteger;
    }
    scratchInteger = rov_read_int(ini_fd,inputField,"TIMESTAMP",TRUE);
    thisDescriptor.timeStampFlag = scratchInteger;
    
    thisDescriptor.inputNumber = inputNumber;
    thisDescriptor.dailyLogfilesFlag = rov_read_int(ini_fd, inputField, "DAILY_LOGFILES_FLAG", globalDailyLogfilesFlag);
    
    if(UDP_ASCII == thisDescriptor.sourceType) {
      scratchInteger = rov_read_int(ini_fd,inputField,"IN_SOCKET",NO_IN_SOCKET);
      if(NO_IN_SOCKET == scratchInteger) {
	continue;
      }
      else {
	thisDescriptor.inSocketNumber = scratchInteger;
      }
      
      scratchInteger = rov_read_int(ini_fd,inputField,"OUT_SOCKET",NO_IN_SOCKET);
      if(NO_IN_SOCKET == scratchInteger) {
	thisDescriptor.outSocketNumber = 99999;
      }
      else {
	thisDescriptor.outSocketNumber = scratchInteger;
      }
      
      scratchString = rov_read_string(ini_fd,inputField, "OUT_IPADDRESS", NULL_IP_ADDRESS);
      thisDescriptor.outIpAddress = strdup(scratchString);
    }
    else if(SERIAL_ASCII == thisDescriptor.sourceType) {
      scratchString = rov_read_string(ini_fd,inputField,"SERIAL_PORT_NAME",DEFAULT_NO_SERIAL_PORT);
      if(!strcmp(DEFAULT_NO_SERIAL_PORT,scratchString)) {
	continue;
      }
      thisDescriptor.theSerialPort.sio_com_port_name = strdup(scratchString);
      free(scratchString);
      thisDescriptor.theSerialPort.parity = rov_read_int (ini_fd, inputField, "PARITY", 0);
      thisDescriptor.theSerialPort.baud = rov_read_int (ini_fd, inputField, "BAUD", 9600);
      thisDescriptor.theSerialPort.data_bits = rov_read_int (ini_fd, inputField, "DATABITS", 8);
      thisDescriptor.theSerialPort.stop_bits = rov_read_int (ini_fd, inputField, "STOPBITS", 1);
      thisDescriptor.theSerialPort.echo = rov_read_int (ini_fd, inputField, "ECHO", FALSE);
      thisDescriptor.theSerialPort.flow_control = rov_read_int (ini_fd, inputField, "FLOWCONTROL", SIO_FLOW_CONTROL_NONE);
      scratchString = rov_read_string(ini_fd,inputField,"TERM_CHAR", DEFAULT_TERMINATOR_STRING);
      thisDescriptor.theSerialPort.auto_mux_enabled = TRUE;
      
      if (!strcasecmp (scratchString, "none")) {
	thisDescriptor.theSerialPort.auto_mux_enabled = FALSE;
      }
      else if (scratchString[0] == '\\') {			// there are escape characters here
	if (scratchString[1] == 'r') {
	  thisDescriptor.theSerialPort.auto_mux_term_char = '\r';
	}
	else if (scratchString[1] == 'n') {
	  thisDescriptor.theSerialPort.auto_mux_term_char = '\n';
	}
      }
      else if (strlen (scratchString) == 1) {
	thisDescriptor.theSerialPort.auto_mux_term_char = scratchString[0];
      }
      else {
	unsigned int new_int;
	int items;
	
	if (!strncmp (scratchString, "0x", 2)) {
	  // this is the hex representation of a binary term character
	  items = sscanf (&(scratchString[2]), "%x", &new_int);
	  if (items == 1) {
	    if (new_int <= 255) {
	      thisDescriptor.theSerialPort.auto_mux_term_char = (char) new_int;
	    }
	    else {
	      thisDescriptor.theSerialPort.auto_mux_term_char = DEFAULT_TERMINATOR_CHAR;
	    }
	  }
	}
	else {
	  thisDescriptor.theSerialPort.auto_mux_term_char = DEFAULT_TERMINATOR_CHAR;
	}
      }      
    }
    thisDescriptor.freeRunPromptFlag = rov_read_int(ini_fd, inputField, "FREE_RUN_PROMPT_FLAG", 0);
    thisDescriptor.usePromptSpecificDestinationsFlag = rov_read_int(ini_fd, inputField, "USE_PROMPT_SPECIFIC_DESTINATIONS_FLAG", 0);
    thisDescriptor.usePromptSpecificTermChar = rov_read_int(ini_fd, inputField, "USE_PROMPT_SPECIFIC_TERM_CHAR", 0);
    thisDescriptor.usePromptSpecificDataType = rov_read_int(ini_fd, inputField, "USE_PROMPT_SPECIFIC_DATA_TYPE", 0);
    thisDescriptor.usePromptSpecificSourceName = rov_read_int(ini_fd, inputField, "USE_PROMPT_SPECIFIC_SOURCE_NAME", 0);
    thisDescriptor.usePromptSpecificSyncopation = rov_read_int(ini_fd, inputField, "USE_PROMPT_SPECIFIC_SYNCOPATION", 0);
    thisDescriptor.usePromptSpecificReplyWindows = rov_read_int(ini_fd, inputField, "USE_PROMPT_SPECIFIC_REPLY_WINDOWS", 0);
    thisDescriptor.useDestinationSpecificFileExtension = rov_read_int(ini_fd, inputField, "USE_DESTINATION_SPECIFIC_FILE_EXTENSION", 0);

    printf("***** inputNumber is %d *****\n", inputNumber);
    { //----- section to read ini file info for prompt sequences
      char fieldName[64];

      snprintf(fieldName,64,"PROMPT_SEQUENCE_INTERVAL");
      thisDescriptor.promptSequence.secondsBetweenSequences = rov_read_double(ini_fd,inputField,fieldName,DEFAULT_SEQUENCE_INTERVAL);
      snprintf(fieldName,64,"PROMPT_SEQUENCE_INTERVAL_BETWEEN_PROMPTS");
      thisDescriptor.promptSequence.secondsBetweenPrompts = rov_read_double(ini_fd,inputField,fieldName,DEFAULT_SEQUENCE_INTERVAL);
      
      snprintf(fieldName,64,"PROMPT_SEQUENCE_NUMBER_OF_PROMPTS");
      thisDescriptor.promptSequence.numberOfPrompts = rov_read_int(ini_fd, inputField,fieldName,0);
      for(int promptNumber = 0; promptNumber < thisDescriptor.promptSequence.numberOfPrompts; promptNumber++) {
	snprintf(fieldName,64,"PROMPT_%d_ENABLE", promptNumber+1);
	thisDescriptor.promptEnable[promptNumber] = rov_read_int(ini_fd, inputField, fieldName, 1);
	printf("***** promptEnable[%d] is %d *****\n", promptNumber, thisDescriptor.promptEnable[promptNumber]);
	
	if(thisDescriptor.usePromptSpecificDataType) {
	  char fieldName[64];
	  
	  snprintf(fieldName,64,"PROMPT_%d_DATA_TYPE", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, DEFAULT_NO_ENTRY);
	  thisDescriptor.promptDataType[promptNumber] = strdup(scratchString);
	  free(scratchString);
	} // end of if(thisDescriptor.usePromptSpecificDataType)
	
	if(thisDescriptor.usePromptSpecificSourceName) {
	  char fieldName[64];
	  
	  snprintf(fieldName,64,"PROMPT_%d_SOURCE_NAME", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, DEFAULT_NO_ENTRY);
	  thisDescriptor.promptSourceName[promptNumber] = strdup(scratchString);
	  free(scratchString);
	} // end of if(thisDescriptor.usePromptSpecificSourceName)
	
	if(thisDescriptor.usePromptSpecificTermChar) {
	  char fieldName[64];
	  
	  snprintf(fieldName,64,"PROMPT_%d_TERM_CHAR", promptNumber+1);
	  thisDescriptor.promptTermChar[promptNumber] = rov_read_hex(ini_fd,inputField,fieldName,'\n');
	} // end of if(thisDescriptor.usePromptSpecificTermChar)
	
	if(thisDescriptor.usePromptSpecificSyncopation) {
	  char fieldName[64];
	  
	  snprintf(fieldName,64,"PROMPT_%d_SYNCOPATION", promptNumber+1);
	  thisDescriptor.promptSyncopation[promptNumber] = rov_read_int(ini_fd,inputField,fieldName,0);
	  printf("promptNumber is %d, inputField is %s, fieldName is %s, thisDescriptor.promptSyncopation[promptNumber] is %d\n", promptNumber, inputField, fieldName, thisDescriptor.promptSyncopation[promptNumber]);
	} // end of if(thisDescriptor.usePromptSyncopation)
	
	if(thisDescriptor.usePromptSpecificReplyWindows) {		 
	  char fieldName[64];
	  
	  snprintf(fieldName,64,"PROMPT_%d_REPLY_WINDOW", promptNumber+1);
	  thisDescriptor.promptReplyWindows[promptNumber] = rov_read_int(ini_fd,inputField,fieldName,2);
	} // end of if(thisDescriptor.usePromptSpecificReplyWindows) {
	
	if(thisDescriptor.usePromptSpecificDestinationsFlag) {
	  for(int destinationCount = 0; destinationCount < MAX_N_OF_DESTINATIONS; destinationCount++) {
	    char fieldName[64];
	    
	    snprintf(fieldName,64,"PROMPT_%d_DESTINATION_ENABLE_%d", promptNumber+1, destinationCount+1);
	    thisDescriptor.promptDestinationEnable[promptNumber][destinationCount] = rov_read_int(ini_fd,inputField,fieldName,0);
	  } // end of for(int destinationCount = 0; destinationCount < MAX_N_OF_DESTINATIONS; destinationCount++)
	}  // end of if(thisDescriptor.usePromptSpecificDestinationsFlag)
	
	thisDescriptor.promptSequence.prompts[promptNumber].promptCharLength = 0;
	for(int promptChar = 0; promptChar < MAX_PROMPT_LENGTH; promptChar++) {
	  snprintf(fieldName,64,"PROMPT_%d_CHAR_%d",promptNumber+1,promptChar+1 );
	  thisDescriptor.promptSequence.prompts[promptNumber].promptChars[promptChar] = rov_read_hex(ini_fd,inputField,fieldName,'\0');
	  printf("--- %s --- %s ", inputField, fieldName);
	  printf("--- promptNumber is %d promptCharLength is %d character: 0x%02X ---\n", promptNumber, thisDescriptor.promptSequence.prompts[promptNumber].promptCharLength, thisDescriptor.promptSequence.prompts[promptNumber].promptChars[promptChar]);
	  if('\0' != thisDescriptor.promptSequence.prompts[promptNumber].promptChars[promptChar]) {
	    thisDescriptor.promptSequence.prompts[promptNumber].promptCharLength++;
	  } 
	} // end of for(int promptChar = 0; promptChar < MAX_PROMPT_LENGTH; promptChar++)
	printf("***** promptNumber is %d promptCharLength is %d *****\n", promptNumber, thisDescriptor.promptSequence.prompts[promptNumber].promptCharLength);
      } // end of for(int promptNumber = 0; promptNumber < thisDescriptor.promptSequence.numberOfPrompts; promptNumber++)
    } // end of section to read ini file info for prompt sequences
    
    //-------------------------------------------------
    thisDescriptor.nOfDestinations = 0;
    int temporaryDestinationCount = 0;
    for(int destinationCount = 0; destinationCount < MAX_N_OF_DESTINATIONS; destinationCount++) {
      char    inputField2[256];
      snprintf(inputField2,256,"DESTINATION_%d",destinationCount+1);
      scratchInteger = rov_read_int(ini_fd,inputField,inputField2,(int)NOT_A_DESTINATION);
      
      if(scratchInteger >=  ((int)NOT_A_DESTINATION) ||  (scratchInteger < (int)UDP_SOCKET)) {
	continue;
      }
      thisDescriptor.destinations[temporaryDestinationCount].destinationType = (destinationType_t)scratchInteger;
      
      snprintf(inputField2,256,"DESTINATION_%d_ENABLE",destinationCount+1);
      thisDescriptor.destinations[temporaryDestinationCount].destinationEnable = rov_read_int(ini_fd,inputField,inputField2,1);
      
      snprintf(inputField2,256,"DESTINATION_%d_RAWMODE",destinationCount+1);
      thisDescriptor.destinations[temporaryDestinationCount].rawMode = rov_read_int(ini_fd,inputField,inputField2,0);
      
      if((int)UDP_SOCKET == thisDescriptor.destinations[temporaryDestinationCount].destinationType) {
	snprintf(inputField2,256,"DESTINATION_%d_IPADDRESS",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_IP_ADDRESS);
	if(!strcmp(NULL_IP_ADDRESS,scratchString)) {
	  continue;
	}
	thisDescriptor.destinations[temporaryDestinationCount].networkDestination.ipAddress = strdup(scratchString);
	free(scratchString);
	snprintf(inputField2,256,"DESTINATION_%d_SOCKET",destinationCount+1);
	scratchInteger = rov_read_int(ini_fd,inputField,inputField2,NULL_SOCKET_NUMBER);
	if(NULL_SOCKET_NUMBER == scratchInteger) {
	  free(thisDescriptor.destinations[temporaryDestinationCount].networkDestination.ipAddress);
	  continue;
	}
	thisDescriptor.destinations[temporaryDestinationCount].networkDestination.toSocketNumber = scratchInteger;
	
	thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.implemented = false;
	snprintf(inputField2,256,"DESTINATION_%d_STARTS_WITH",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_GREP_FIELD);
	if(strcmp(NULL_GREP_FIELD,scratchString)) {
	  thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.startsWith = strdup( scratchString);
	  thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.implemented  = true;
	}
	free(scratchString);
	snprintf(inputField2,256,"DESTINATION_%d_STARTS_CONTAINS",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_GREP_FIELD);
	if(strcmp(NULL_GREP_FIELD,scratchString)) {
	  thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.greps = strdup( scratchString);
	  thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.implemented  = true;
	}
	free(scratchString);
	snprintf(inputField2,256,"DESTINATION_%d_STARTS_DOESNT_CONTAIN",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_GREP_FIELD);
	if(strcmp(NULL_GREP_FIELD,scratchString)) {
	  thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.grepMinusVs = strdup( scratchString);
	  thisDescriptor.destinations[temporaryDestinationCount].destinationFilter.implemented  = true;
	}
	free(scratchString);
      }
      else if(DISK == thisDescriptor.destinations[temporaryDestinationCount].destinationType) {
	// read the pathname
	snprintf(inputField2,256,"DESTINATION_%d_PATHNAME",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,startup.rootDirectory);
	thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.loggingDirectory = strdup(scratchString);
	free(scratchString);
	printf("\ninputField is %s; inputField2 is %s\n", inputField, inputField2);
	printf("\nDESTINATION_PATHNAME is %s\n\n", thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.loggingDirectory);
	
	// read the file name prefix
	snprintf(inputField2,256,"DESTINATION_%d_PREFIX",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,thisDescriptor.filenamePrefix);
	thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.filenamePrefix = strdup(scratchString);
	free(scratchString);
	printf("\ninputField is %s; inputField2 is %s\n", inputField, inputField2);
	printf("\nDESTINATION_PREFIX is %s\n\n", thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.filenamePrefix);
	
	if(thisDescriptor.useDestinationSpecificFileExtension) {
	  // read the file extension
	  snprintf(inputField2,256,"DESTINATION_%d_EXTENSION",destinationCount+1);
	  scratchString = rov_read_string(ini_fd,inputField,inputField2,"UNDEFINED");
	  thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.fileExtension = strdup(scratchString);
	  free(scratchString);
	  printf("\ninputField is %s; inputField2 is %s\n", inputField, inputField2);
	  printf("\nDESTINATION_EXTENSION is %s\n\n", thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.fileExtension);
	}
	else {
	  thisDescriptor.destinations[temporaryDestinationCount].loggingDestination.fileExtension = strdup(thisDescriptor.dataType);
	}
      }
      
      temporaryDestinationCount++;
      
    }  // end of for(int destinationCount = 0; destinationCount < MAX_N_OF_DESTINATIONS; destinationCount++) 
    thisDescriptor.nOfDestinations = temporaryDestinationCount;

    loggingList[loggingCount] = thisDescriptor;
    // memcpy((void *)(loggingList + loggingCount * sizeof(thisDescriptor)),&thisDescriptor, sizeof(thisDescriptor));
    
    loggingCount++;
    *nOfLoggers = loggingCount;
    
  }  // end of for(int inputNumber = 0; inputNumber < MAX_N_OF_INPUTS; inputNumber++)
  
  return 1;
}

int logOpenAsciiLogFile (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor)
{

   struct tm tm;
   time_t current_time;
   static char filename[512];
   int status = 0;

   current_time = time (NULL);
   // 18 July 2012 fix to use threadsafe localtime_r

   localtime_r (&current_time,&tm);

   //old - replaced on 2013-04-24 if (((thisDestination->loggingDestination.lastHour  != tm.tm_hour) || (thisDestination->loggingDestination.asciiLogFileD == NULL)))

   // open a new log file under ANY of the following conditions:
   //   - there is no logFile currently open
   //   - - configuration indicates hourly logfiles and it is a new hour
   //   - - configuration indicates daily logfiles and it is a new day
   if ( (thisDestination->loggingDestination.asciiLogFileD == NULL)  ||
   	( (1 != loggingDescriptor->dailyLogfilesFlag) && (thisDestination->loggingDestination.lastHour  != tm.tm_hour) ) ||
   	( (1 == loggingDescriptor->dailyLogfilesFlag) && (thisDestination->loggingDestination.lastDay  != tm.tm_mday) ) ) 
      {
#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to set last hour.\n");
#endif
         thisDestination->loggingDestination.lastHour = tm.tm_hour;
         thisDestination->loggingDestination.lastDay  = tm.tm_mday;

         /* close existing log file */
         if (thisDestination->loggingDestination.asciiLogFileD != NULL)
            {
#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to close existing file.\n");
#endif
               if (0 != fclose (thisDestination->loggingDestination.asciiLogFileD))
                  printf ("LOGGING: ERROR closing log file %s !!!!!!!!!!!!!!!!!!!\n", thisDestination->loggingDestination.asciiLogFileName);
            }

#ifdef DEBUG_LOGGING
   printf (" In logOpenAsciiLogFile - about to create new log file name.\n");
#endif
         /* create the new log file name */
	 snprintf (filename, 512,"%s/%s%04d%02d%02d_%02d%02d.%s",thisDestination->loggingDestination.loggingDirectory,thisDestination->loggingDestination.filenamePrefix,(int) tm.tm_year + 1900, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) tm.tm_min,thisDestination->loggingDestination.fileExtension);

         /* open the new file */
#ifdef DEBUG_LOGGING
         printf ("in open log file, opening a new file %s\n", filename);
#endif
         thisDestination->loggingDestination.asciiLogFileD = fopen (filename, "wa");

         /* check results of fopen operation */
         if (NULL == thisDestination->loggingDestination.asciiLogFileD)
            {
#ifdef DEBUG_LOGGING
               printf ("log.ascii_log_file_pointer was NULL\n");
#endif
               //strcpy(log.ascii_log_file_name, "ERROR OPENING: ");
               //strcat(log.ascii_log_file_name, filename);
               printf ("error opening %s\n", filename);
               status = 1;
            }
         else{
#ifdef DEBUG_LOGGING
               printf ("log->ascii_log_file_pointer was false\n");
               //printf ("ascii file pointer = %x\n", (int) log->ascii_log_file_pointer);
#endif
               strncpy(thisDestination->loggingDestination.asciiLogFileName, filename,256);
               //printf ("LOGGING: Opened log file %s OK\n", log->ascii_log_file_name);
            }
      }
   return (status);


}


int logThisNow(distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData)
      /*


     MODIFICATION HISTORY
     DATE         WHO             WHAT
     -----------  --------------  ----------------------------
     18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl
     15 OCT 2001  Jon Howland     hack to use with rov_54.1, remove timestamping
     05 Nov 2009  Jon Howland     hack again to work with dsLogger

     ---------------------------------------------------------------------- */
{


#ifdef DEBUG_LOGGING
   printf (" just entered log this now!\n");
   printf (" string to log is --- %s\n", recordData);
#endif
   // lock the data structure mutex so that we chan diddle with the file
   // commented out because in this architecture, this is the only access
   // to the file.  did the unlock too
   //  pthread_mutex_lock(&log.mutex);

   // open a log file if required
   logOpenAsciiLogFile (thisDestination,loggingDescriptor);

#ifdef DEBUG_LOGGING
   printf (" back from call to logOpenAsciiLogFile.\n");
#endif

   if (NULL != thisDestination->loggingDestination.asciiLogFileD)
      {
#ifdef DEBUG_LOGGING
   printf (" about to call fprintf from logThisNow.\n");
#endif
         fprintf (thisDestination->loggingDestination.asciiLogFileD, "%s", recordData);
         fflush (thisDestination->loggingDestination.asciiLogFileD);
      }

#ifdef DEBUG_LOGGING
   printf (" just left log this now!\n");
#endif

   return 0;

}

/* ---------------------------------------------------------------------- */
void flushAsciiLogAndClose (distribution_t *thisDestination,logging_descriptor_t *loggingDescriptor, char *recordData)
      /*


     MODIFICATION HISTORY
     DATE         WHO             WHAT
     -----------  --------------  ----------------------------
     18 Apr 1999  Louis Whitcomb  Created and Written based on Dana's original write_dvl
     15 Oct 2001  Jon Howland     modified to work with data structure, not globals
     ---------------------------------------------------------------------- */
{


   // open a log file if required
   logOpenAsciiLogFile (thisDestination,loggingDescriptor);

   if(NULL != thisDestination->loggingDestination.asciiLogFileD )
      {
         fflush (thisDestination->loggingDestination.asciiLogFileD);
         fclose (thisDestination->loggingDestination.asciiLogFileD);

      }



}

