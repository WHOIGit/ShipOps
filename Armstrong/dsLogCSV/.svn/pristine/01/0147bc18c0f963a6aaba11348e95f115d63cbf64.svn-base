
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
  2013-12-31   tt       - create from dsLog.cpp
  2014-01-25   tt       - test svn commit on virtual box (had trouble on shared folder)

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

#include "dsLogCsv.h"		/* main() header file */
#include "config_file.h"        /* ini file reading functions */


#include "cmd_thread.h"		/* cmd (backdoor) thread */
#include "sio_thread.h"		/* backdoor thread */
#include "nio_thread.h"		/* nio thread */
#include "time_util.h"		/* time utilities */
#include "msg_util.h"

#include "readIniDsLogCSV.h"
#include "csvLogThread.h"
#include "csvOutputThread.h"
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

csv_abstract_data_t csvAbstractData[MAX_N_OF_ABSTRACT_DATA];

// the global thread table

thread_table_entry_t    global_thread_table[MAX_NUMBER_OF_THREADS];
logging_descriptor_t    theLoggingList[MAX_N_OF_INPUTS];
csv_output_descriptor_t csvOutputList[MAX_N_OF_CSV_OUTPUTS];
distribution_t          theGlobalLogFile;
logging_descriptor_t    theGlobalLog;

calculated_values_config_t calcValuesConfig;

pthread_mutex_t dsLogCsvMutex;

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
   startup.nOfLogOutputs = 0;

   int   nOfLogInputs;
   int   nOfLogOutputs;

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
   pthread_attr_getstacksize (&DEFAULT_ROV_THREAD_ATTR, &size), fprintf (stderr, "Default PTHREAD stack size was %ld (0x%08lX)\n", size, size);

   pthread_attr_setstacksize (&DEFAULT_ROV_THREAD_ATTR, ROV_THREAD_DEFAULT_STACKSIZE), pthread_attr_getstacksize (&DEFAULT_ROV_THREAD_ATTR, &size), fprintf (stderr, "New PTHREAD stack size was %ld (0x%08lX)\n", size, size);

   // initialize the mutex
   pthread_mutex_init(&dsLogCsvMutex, NULL);

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

	readIniDslogCsvProcess(inFD,&(theLoggingList[0]),&nOfLogInputs,&(csvOutputList[0]),&nOfLogOutputs,&theGlobalLogFile, &theGlobalLog);
         startup.running =  1;
         startup.nOfLogInputs = nOfLogInputs;
         startup.nOfLogOutputs = nOfLogOutputs;
         fclose (inFD);
      }

   //*** initialize the csvAbstractData array
   for(int ii=0; ii < MAX_N_OF_ABSTRACT_DATA; ++ii) {
     csvAbstractData[ii].lastRxTime = rov_get_time();
     csvAbstractData[ii].dataString = strdup("NODATA");
   }


   msg_queue_initialize();



   // fill the thread table.  first mark all entries so only full entries will be launched

   for (i = 0; i < MAX_NUMBER_OF_THREADS; i++)
      {
         global_thread_table[i].thread_function = NULL;
      }


   //make_thread_table_entry (LOGGING_THREAD, "LOGGING_THREAD", logging_thread, (void *)NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);

   //make_thread_table_entry (TCP_SERVER_THREAD, "TCP_SERVER_THREAD", tcp_server_thread, (void *) TCP_SERVER_THREAD, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);
   make_thread_table_entry (CMD_0_SIO_THREAD, "CMD_SIO_THREAD", sio_thread, (void *) CMD_0_SIO_THREAD, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE);
   make_thread_table_entry (CMD_THREAD_0, "CMD_THREAD_0", cmd_thread, (void *) CMD_THREAD_0, NULL_EXTRA_ARG_VALUE, CMD_0_SIO_THREAD, 1, NULL_EXTRA_ARG_VALUE);

   for(int nOfLoggers = 0; nOfLoggers < nOfLogInputs; nOfLoggers++)
      {
         char   threadName[256];
         if( UDP_ASCII == theLoggingList[nOfLoggers].sourceType)
            {

               snprintf(threadName,256, "UDP_LOG_%d",nOfLoggers);
               make_thread_table_entry (LOG_THREAD_OFFSET + nOfLoggers, threadName, csvLogThread, (void *) (intptr_t) (LOG_THREAD_OFFSET + nOfLoggers),nOfLoggers, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE,NULL_EXTRA_ARG_VALUE);
               theLoggingList[nOfLoggers].threadNumber = LOG_THREAD_OFFSET + nOfLoggers;
            }
         else if(SERIAL_ASCII == theLoggingList[nOfLoggers].sourceType)
            {
               snprintf(threadName,256, "SERIAL_LOG_%d",nOfLoggers);
               make_thread_table_entry (LOG_THREAD_OFFSET + nOfLoggers, threadName, csvLogThread, (void *) (intptr_t) (LOG_THREAD_OFFSET + nOfLoggers),nOfLoggers, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE,NULL_EXTRA_ARG_VALUE);
               theLoggingList[nOfLoggers].threadNumber = LOG_THREAD_OFFSET + nOfLoggers;

            }

      }

#if 1
   for(int nOfOutputs = 0; nOfOutputs < nOfLogOutputs; nOfOutputs++) {
     char   threadName[256];
     snprintf(threadName,256, "OUTPUT_THREAD_%d",nOfOutputs);
     make_thread_table_entry (OUTPUT_THREAD_OFFSET + nOfOutputs, threadName, csvOutputThread, (void *) (intptr_t) (OUTPUT_THREAD_OFFSET + nOfOutputs),nOfOutputs, NULL_EXTRA_ARG_VALUE, NULL_EXTRA_ARG_VALUE,NULL_EXTRA_ARG_VALUE);
     theLoggingList[nOfOutputs].threadNumber = OUTPUT_THREAD_OFFSET + nOfOutputs;
     printf("threadName is %s\n", threadName);
   }
#endif

   /* ------------------------------------------------------------
      Standard DAEMONIZE function (GT)
      2010-04-21 JCK - ported to gravlog from nav r745
      2013-02-15 tt  - ported to dsLog from gravlog
      ------------------------------------------------------------ */
   
   if (daemonize) {
     fprintf(stderr, "DAEMONIZING DSLOGCSV\n");
     
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
       fprintf(stderr, "DSLOGCSV.CPP: Not possible to get new SID.\n");
       exit(EXIT_FAILURE);
     }
     
     // Daemon: Close out the standard file descriptors
     devnull = open("/dev/null", O_RDWR);
     if (devnull == -1) {
       fprintf(stderr, "DSLOGCSV.CPP: Unable to open /dev/null \n");
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

//-----------------------------------------------------------------------------------------------
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
   //   - - configuration is for NOAA format
   //   - - configuration indicates hourly logfiles and it is a new hour
   //   - - configuration indicates daily logfiles and it is a new day
   if ( ( thisDestination->loggingDestination.asciiLogFileD == NULL)  ||
   	( loggingDescriptor->outputFileFormat == OUTFILE_FORMAT_NOAA ) ||
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
	 switch(loggingDescriptor->outputFileFormat) {
	 case OUTFILE_FORMAT_NOAA:
	   snprintf (filename, 512,"%s/noaa.tmp",thisDestination->loggingDestination.loggingDirectory);
	   break;
	 default:
	   snprintf (filename, 512,"%s/%s%02d%02d%02d_%02d%02d.%s",thisDestination->loggingDestination.loggingDirectory,thisDestination->loggingDestination.filenamePrefix, ( (int) tm.tm_year + 1900 ) % 100, (int) tm.tm_mon + 1, (int) tm.tm_mday, (int) tm.tm_hour, (int) tm.tm_min,thisDestination->loggingDestination.fileExtension);
	   break;
	 }

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

	       // *** write the file header - added by tet - 2014-05-02  
	       // *** Updated for different outputFileFormat's by tet - 2014-11-09
	       // now write Header string for log files according to the outputFileFormat
	       switch(loggingDescriptor->outputFileFormat) {
	       case OUTFILE_FORMAT_SAMOS:
		 break;
	       case OUTFILE_FORMAT_NOAA:
		 {
		   char dateString[64];
		   char timeString[64];
		   noaa_sprintf_date_string(dateString);
		   noaa_sprintf_time_string(timeString);
		   fprintf(thisDestination->loggingDestination.asciiLogFileD, "<<< %s, %s,%s >>>\n", loggingDescriptor->shipName, dateString, timeString);
		   fprintf(thisDestination->loggingDestination.asciiLogFileD, "%s", loggingDescriptor->fileHeaderString);
		   fflush(thisDestination->loggingDestination.asciiLogFileD);
		 }
		 break;
	       case OUTFILE_FORMAT_DSLOGCSV:
	       default:
		 if(loggingDescriptor->useFileHeaderFlag) {
		   fprintf(thisDestination->loggingDestination.asciiLogFileD, "WHOI SSSG dsLogCsv\n");
		   fprintf(thisDestination->loggingDestination.asciiLogFileD, "%s", loggingDescriptor->fileHeaderString);
		   fflush(thisDestination->loggingDestination.asciiLogFileD);
		 }
		 break;
	       }  // end of switch over outputFileFormat
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
   // unknown author and date - was it me?  (t. thiel 2014-06-23)

   // was seeing instances where the files were getting stepped on by one another.  
   // reinstated calls to mutex lock/unlock
   // log.mutex was gone so created new mutex - dsLogCsvMutex (global)
   // t. thiel 2014-06-23
   pthread_mutex_lock(&dsLogCsvMutex);

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

   pthread_mutex_unlock(&dsLogCsvMutex);

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

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Post Mode:      DSLOGCSV_POST_MODE = 0
// Real-time Mode: DSLOGCSV_POST_MODE = 1
//#define DSLOGCSV_MODE 1
//#include "readIniDsLogCSV.cpp"
