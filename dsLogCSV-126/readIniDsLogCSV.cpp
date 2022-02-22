
/* ----------------------------------------------------------------------
    Modification History:
    DATE         AUTHOR   COMMENT

    2014-11-06   tt       - Created

---------------------------------------------------------------------- */

/* standard ansi C header files */
#include <stdio.h>
#include <string.h>

#include "dsLogCsv.h"
#include "config_file.h"        /* ini file reading functions */

#include "readIniDsLogCSV.h"

//---------------------------------------------------------------------------
// defines to control whether post processing mode is active
#define DSLOGCSV_POST_MODE 0
#define DSLOGCSV_REALTIME_MODE 1

#define DEBUG_READINIDSLOGCSV 0

#if DSLOGCSV_MODE == DSLOGCSV_POST_MODE
#include "dsLogPostCsv.h"
extern dslogPostCsv_t postCsv;
extern char *postGlobalInputDirectory;
#endif

//---------------------------------------------------------------------------
int readIniDslogCsvProcess(FILE * ini_fd, logging_descriptor_t *localLoggingList, int *nOfLoggers, csv_output_descriptor_t *localOutputList, int *nOfOutputs, distribution_t *singleLogFile,logging_descriptor_t *singleLog)
{
  *nOfLoggers = 0;
  int loggingCount = 0;
  *nOfOutputs = 0;
  int outputCount = 0;

  int globalDailyLogfilesFlag = 0;
  int globalSingleLogfileFlag = 0;
  int scratchInteger;
  
  globalDailyLogfilesFlag = rov_read_int(ini_fd, "GENERAL", "DAILY_LOGFILES_FLAG", 1);
  globalSingleLogfileFlag = rov_read_int(ini_fd, "GENERAL", "SINGLE_LOGFILE_FLAG", 1);
  
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

  //----------------------------------------------------------------------------------
#if DSLOGCSV_MODE == DSLOGCSV_POST_MODE
  //--- start read config info specific to post processing  
  postCsv.iniStartTime.year = rov_read_int(ini_fd, "POSTCSV", "START_YEAR", -99);
  postCsv.iniStartTime.month = rov_read_int(ini_fd, "POSTCSV", "START_MONTH", -99);
  postCsv.iniStartTime.day = rov_read_int(ini_fd, "POSTCSV", "START_DAY", -99);
  postCsv.iniStartTime.hour = rov_read_int(ini_fd, "POSTCSV", "START_HOUR", -99);
  postCsv.iniStartTime.minute = rov_read_int(ini_fd, "POSTCSV", "START_MINUTE", -99);
  postCsv.iniStartTime.second = rov_read_int(ini_fd, "POSTCSV", "START_SECOND", -99);

  postCsv.iniEndTime.year = rov_read_int(ini_fd, "POSTCSV", "END_YEAR", -99);
  postCsv.iniEndTime.month = rov_read_int(ini_fd, "POSTCSV", "END_MONTH", -99);
  postCsv.iniEndTime.day = rov_read_int(ini_fd, "POSTCSV", "END_DAY", -99);
  postCsv.iniEndTime.hour = rov_read_int(ini_fd, "POSTCSV", "END_HOUR", -99);
  postCsv.iniEndTime.minute = rov_read_int(ini_fd, "POSTCSV", "END_MINUTE", -99);
  postCsv.iniEndTime.second = rov_read_int(ini_fd, "POSTCSV", "END_SECOND", -99);

  if( (postCsv.iniStartTime.year < 1960) || (postCsv.iniStartTime.year > 2200) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTYEAR is out of bounds. \n");
    return(-1);
  }
  if( (postCsv.iniStartTime.month < 1) || (postCsv.iniStartTime.month > 12) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTMONTH is out of bounds. \n");
    return(-1);
  }
  if( postCsv.iniStartTime.day < 1 ) { 
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTDAY is out of bounds. \n"); 
    return(-1); 
  }
  if( postCsv.iniStartTime.day > lastDayOfMonth(postCsv.iniStartTime.month, postCsv.iniStartTime.year) ) { 
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTDAY is out of bounds. \n"); 
    return(-1); 
  }
  if( (postCsv.iniStartTime.hour < 0) || (postCsv.iniStartTime.hour > 23) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTHOUR is out of bounds. \n");
    return(-1);
  }
  if( (postCsv.iniStartTime.minute < 0) || (postCsv.iniStartTime.minute > 59) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTMINUTE is out of bounds. \n");
    return(-1);
  }
  if( (postCsv.iniStartTime.second < 0) || (postCsv.iniStartTime.second > 59) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-STARTSECOND is out of bounds. \n");
    return(-1);
  }

  if( (postCsv.iniEndTime.year < 1960) || (postCsv.iniEndTime.year > 2200) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDYEAR is out of bounds. \n");
    return(-1);
  }
  if( (postCsv.iniEndTime.month < 1) || (postCsv.iniEndTime.month > 12) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDMONTH is out of bounds. \n");
    return(-1);
  }
  if( postCsv.iniEndTime.day < 1 ) { 
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDDAY is out of bounds. \n"); 
    return(-1); 
  }
  if( postCsv.iniEndTime.day > lastDayOfMonth(postCsv.iniEndTime.month, postCsv.iniEndTime.year) ) { 
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDDAY is out of bounds. \n"); 
    return(-1); 
  }
  if( (postCsv.iniEndTime.hour < 0) || (postCsv.iniEndTime.hour > 23) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDHOUR is out of bounds. \n");
    return(-1);
  }
  if( (postCsv.iniEndTime.minute < 0) || (postCsv.iniEndTime.minute > 59) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDMINUTE is out of bounds. \n");
    return(-1);
  }
  if( (postCsv.iniEndTime.second < 0) || (postCsv.iniEndTime.second > 59) ) {
    fprintf(stderr, "**** ERROR **** POSTCSV-ENDSECOND is out of bounds. \n");
    return(-1);
  }

  postCsv.sampleSecond = rov_read_int(ini_fd, "POSTCSV", "SAMPLE_SECOND", 0);

  postGlobalInputDirectory = rov_read_string(ini_fd,"GENERAL","INPUT_DATA_DIR", "/tmp");

  //--- end read config info specific to post processing  
#endif 

  //----------------------------------------------------------------------------------
  // -- loop over input specifers  
  for(int inputNumber = 0; inputNumber < MAX_N_OF_INPUTS; inputNumber++) {
    logging_descriptor_t    thisInputDescriptor;
    char inputField[256];
    snprintf(inputField,256,"INPUT_%d",inputNumber);
    
    thisInputDescriptor.inputNumber = inputNumber;
    
    // has the user disabled this input in the ini file??
    if(0 == rov_read_int(ini_fd, inputField, "INPUT_ENABLE", 1) ) { // user has disabled this input
      continue;
    }
    
    //--- read sourceType
    // allow user to specify as either string or integer (to allow legacy ini files to work)
    scratchString = rov_read_string(ini_fd,inputField,"SOURCE_TYPE",DEFAULT_NO_ENTRY);
    if(!strcmp("UDP",scratchString)) { 
      thisInputDescriptor.sourceType = (source_type_t) 0;
    }
    else if(!strcmp("FILE",scratchString)) { 
      thisInputDescriptor.sourceType = (source_type_t) 1;
    }
    else {
      // no success reading the string - try to read as an integer
      scratchInteger = rov_read_int(ini_fd,inputField,"SOURCE_TYPE",NO_SOURCE_TYPE);
      if(NO_SOURCE_TYPE == scratchInteger) {
#if DSLOGCSV_MODE == DSLOGCSV_POST_MODE
	// just set to a useable value for postprocessing
      thisInputDescriptor.sourceType = (source_type_t) 0;
#else
	continue;
#endif
      }
      else {
	thisInputDescriptor.sourceType = (source_type_t)scratchInteger;
      }
    }
    free(scratchString);

    //--- 
    if(UDP_ASCII == thisInputDescriptor.sourceType) {
      scratchInteger = rov_read_int(ini_fd,inputField,"IN_SOCKET",NO_IN_SOCKET);
      if(NO_IN_SOCKET == scratchInteger) {
#if DSLOGCSV_MODE == DSLOGCSV_POST_MODE
	// just set to a useable value for postprocessing
	thisInputDescriptor.inSocketNumber = 55555;
#else
	continue;
#endif
      }
      else {
	thisInputDescriptor.inSocketNumber = scratchInteger;
      }
      
      scratchInteger = rov_read_int(ini_fd,inputField,"OUT_SOCKET",NO_IN_SOCKET);
      if(NO_IN_SOCKET == scratchInteger) {
	thisInputDescriptor.outSocketNumber = 99999;
      }
      else {
	thisInputDescriptor.outSocketNumber = scratchInteger;
      }
      
      scratchString = rov_read_string(ini_fd,inputField, "OUT_IPADDRESS", NULL_IP_ADDRESS);
      thisInputDescriptor.outIpAddress = strdup(scratchString);
    }
    else {
      // only UDP sources are valid
      continue;
    }
    thisInputDescriptor.useDestinationSpecificFileExtension = rov_read_int(ini_fd, inputField, "USE_DESTINATION_SPECIFIC_FILE_EXTENSION", 0);

#if DEBUG_READINIDSLOGCSV
    printf("***** inputNumber is %d *****\n", inputNumber);
#endif

  //----------------------------------------------------------------------------------
#if DSLOGCSV_MODE == DSLOGCSV_POST_MODE
    //--- start read config info specific to post processing  
    scratchString = rov_read_string(ini_fd, inputField, "INFILE_DIRECTORY", postGlobalInputDirectory);
    thisInputDescriptor.infileDirectory = strdup(scratchString);

    scratchString = rov_read_string(ini_fd, inputField, "INFILE_PREFIX", "INVALID_STRING");
    thisInputDescriptor.infilePrefix = strdup(scratchString);

    scratchString = rov_read_string(ini_fd, inputField, "INFILE_EXTENSION", "INVALID_STRING");
    thisInputDescriptor.infileExtension = strdup(scratchString);
    //--- end read config info specific to post processing  
#endif 


    //----- section to read ini file info for parsing csv data
    { 
      char fieldName[64];
      
      snprintf(fieldName,64,"NUMBER_OF_CSV_PARSE");
      thisInputDescriptor.numberOfCsvParse = rov_read_int(ini_fd, inputField,fieldName,0);
#if DEBUG_READINIDSLOGCSV
      printf("***** numberOfCsvParse is %d *****\n", thisInputDescriptor.numberOfCsvParse);
#endif

      if(thisInputDescriptor.numberOfCsvParse > 0) {
	for(int promptNumber = 0; promptNumber < thisInputDescriptor.numberOfCsvParse; promptNumber++) {
	  //  int parseAbstractIndex - controls where in the abstract array the parsed data is stored
	  snprintf(fieldName,64,"PARSE_ABSTRACT_INDEX_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].parseAbstractIndex = rov_read_int(ini_fd, inputField, fieldName, 1);
#if DEBUG_READINIDSLOGCSV
	  printf("***** parseAbstractIndex[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].parseAbstractIndex);
#endif

	  //  int parseDataType  - indicates how to parse for the input
	  snprintf(fieldName,64,"PARSE_DATATYPE_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "INVALID_DATATYPE");
	  if(0 == strcmp(scratchString, "GPGGA_UTC_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_UTC_TIME;}
	  else if(0 == strcmp(scratchString, "GPGGA_UTC_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_UTC_HOUR;}
	  else if(0 == strcmp(scratchString, "GPGGA_UTC_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_UTC_MINUTE;}
	  else if(0 == strcmp(scratchString, "GPGGA_UTC_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_UTC_SECOND;}
	  else if(0 == strcmp(scratchString, "GPGGA_UTC_FRACTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_UTC_FRACTION;}
	  else if(0 == strcmp(scratchString, "GPGGA_LAT_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LAT_DECDEG;}
	  else if(0 == strcmp(scratchString, "GPGGA_LAT_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LAT_DECMIN;}
	  else if(0 == strcmp(scratchString, "GPGGA_LAT_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LAT_DECSEC;}
	  else if(0 == strcmp(scratchString, "GPGGA_LAT_NOAA") )   { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LAT_NOAA;}
	  else if(0 == strcmp(scratchString, "GPGGA_LON_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LON_DECDEG;}
	  else if(0 == strcmp(scratchString, "GPGGA_LON_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LON_DECMIN;}
	  else if(0 == strcmp(scratchString, "GPGGA_LON_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LON_DECSEC;}
	  else if(0 == strcmp(scratchString, "GPGGA_LON_NOAA") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_LON_NOAA;}
	  else if(0 == strcmp(scratchString, "GPGGA_QUALITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_QUALITY;}
	  else if(0 == strcmp(scratchString, "GPGGA_NUM_SAT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_NUM_SAT;}
	  else if(0 == strcmp(scratchString, "GPGGA_DILUTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_DILUTION;}
	  else if(0 == strcmp(scratchString, "GPGGA_ALTITUDE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_ALTITUDE;}
	  else if(0 == strcmp(scratchString, "GPGGA_GEOID_HEIGHT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_GEOID_HEIGHT;}
	  else if(0 == strcmp(scratchString, "GPGGA_FIX_AGE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_FIX_AGE;}
	  else if(0 == strcmp(scratchString, "GPGGA_STATION_ID") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPGGA_STATION_ID;}
	  else if(0 == strcmp(scratchString, "GPRMB_STATUS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_STATUS;}
	  else if(0 == strcmp(scratchString, "GPRMB_CTE_MAG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_CTE_MAG;}
	  else if(0 == strcmp(scratchString, "GPRMB_CTE_DIR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_CTE_DIR;}
	  else if(0 == strcmp(scratchString, "GPRMB_ORIGIN_ID") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_ORIGIN_ID;}
	  else if(0 == strcmp(scratchString, "GPRMB_DEST_ID") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_DEST_ID;}
	  else if(0 == strcmp(scratchString, "GPRMB_DEST_LAT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_DEST_LAT;}
	  else if(0 == strcmp(scratchString, "GPRMB_DEST_LON") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_DEST_LON;}
	  else if(0 == strcmp(scratchString, "GPRMB_RANGE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_RANGE;}
	  else if(0 == strcmp(scratchString, "GPRMB_BEARING") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_BEARING;}
	  else if(0 == strcmp(scratchString, "GPRMB_DEST_SPEED") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_DEST_SPEED;}
	  else if(0 == strcmp(scratchString, "GPRMB_ARRIVAL_ALARM") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMB_ARRIVAL_ALARM;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_TIME;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_HOUR;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_MINUTE;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_SECOND;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_FRACTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_FRACTION;}
	  else if(0 == strcmp(scratchString, "GPRMC_RCVR_STATUS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_RCVR_STATUS;}
	  else if(0 == strcmp(scratchString, "GPRMC_LAT_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LAT_DECDEG;}
	  else if(0 == strcmp(scratchString, "GPRMC_LAT_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LAT_DECMIN;}
	  else if(0 == strcmp(scratchString, "GPRMC_LAT_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LAT_DECSEC;}
	  else if(0 == strcmp(scratchString, "GPRMC_LAT_NOAA") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LAT_NOAA;}
	  else if(0 == strcmp(scratchString, "GPRMC_LON_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LON_DECDEG;}
	  else if(0 == strcmp(scratchString, "GPRMC_LON_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LON_DECMIN;}
	  else if(0 == strcmp(scratchString, "GPRMC_LON_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LON_DECSEC;}
	  else if(0 == strcmp(scratchString, "GPRMC_LON_NOAA") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_LON_NOAA;}
	  else if(0 == strcmp(scratchString, "GPRMC_SOG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_SOG;}
	  else if(0 == strcmp(scratchString, "GPRMC_COG_SAMOS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_COG_SAMOS;}
	  else if(0 == strcmp(scratchString, "GPRMC_COG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_COG;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_DATE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_DATE;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_YEAR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_YEAR;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_MONTH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_MONTH;}
	  else if(0 == strcmp(scratchString, "GPRMC_UTC_DAY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_UTC_DAY;}
	  else if(0 == strcmp(scratchString, "GPRMC_MAGVAR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPRMC_MAGVAR;}
	  else if(0 == strcmp(scratchString, "GPVTG_TRUE_TRACK") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPVTG_TRUE_TRACK;}
	  else if(0 == strcmp(scratchString, "GPVTG_MAGNETIC_TRACK") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPVTG_MAGNETIC_TRACK;}
	  else if(0 == strcmp(scratchString, "GPVTG_SPEED_KNOTS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPVTG_SPEED_KNOTS;}
	  else if(0 == strcmp(scratchString, "GPVTG_SPEED_KPH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPVTG_SPEED_KPH;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_TIME;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_HOUR;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_MINUTE;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_SECOND;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_FRACTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_FRACTION;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_DATE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_DATE;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_YEAR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_YEAR;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_MONTH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_MONTH;}
	  else if(0 == strcmp(scratchString, "GPZDA_UTC_DAY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_UTC_DAY;}
	  else if(0 == strcmp(scratchString, "GPZDA_LOCALZONE_HOURS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_LOCALZONE_HOURS;}
	  else if(0 == strcmp(scratchString, "GPZDA_LOCALZONE_MINUTES") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPZDA_LOCALZONE_MINUTES;}
	  else if(0 == strcmp(scratchString, "GNGGA_UTC_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_UTC_TIME;}
	  else if(0 == strcmp(scratchString, "GNGGA_UTC_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_UTC_HOUR;}
	  else if(0 == strcmp(scratchString, "GNGGA_UTC_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_UTC_MINUTE;}
	  else if(0 == strcmp(scratchString, "GNGGA_UTC_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_UTC_SECOND;}
	  else if(0 == strcmp(scratchString, "GNGGA_UTC_FRACTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_UTC_FRACTION;}
	  else if(0 == strcmp(scratchString, "GNGGA_LAT_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LAT_DECDEG;}
	  else if(0 == strcmp(scratchString, "GNGGA_LAT_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LAT_DECMIN;}
	  else if(0 == strcmp(scratchString, "GNGGA_LAT_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LAT_DECSEC;}
	  else if(0 == strcmp(scratchString, "GNGGA_LAT_NOAA") )   { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LAT_NOAA;}
	  else if(0 == strcmp(scratchString, "GNGGA_LON_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LON_DECDEG;}
	  else if(0 == strcmp(scratchString, "GNGGA_LON_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LON_DECMIN;}
	  else if(0 == strcmp(scratchString, "GNGGA_LON_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LON_DECSEC;}
	  else if(0 == strcmp(scratchString, "GNGGA_LON_NOAA") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_LON_NOAA;}
	  else if(0 == strcmp(scratchString, "GNGGA_QUALITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_QUALITY;}
	  else if(0 == strcmp(scratchString, "GNGGA_NUM_SAT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_NUM_SAT;}
	  else if(0 == strcmp(scratchString, "GNGGA_DILUTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_DILUTION;}
	  else if(0 == strcmp(scratchString, "GNGGA_ALTITUDE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_ALTITUDE;}
	  else if(0 == strcmp(scratchString, "GNGGA_GEOID_HEIGHT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_GEOID_HEIGHT;}
	  else if(0 == strcmp(scratchString, "GNGGA_FIX_AGE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_FIX_AGE;}
	  else if(0 == strcmp(scratchString, "GNGGA_STATION_ID") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNGGA_STATION_ID;}
	  else if(0 == strcmp(scratchString, "GNRMB_STATUS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_STATUS;}
	  else if(0 == strcmp(scratchString, "GNRMB_CTE_MAG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_CTE_MAG;}
	  else if(0 == strcmp(scratchString, "GNRMB_CTE_DIR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_CTE_DIR;}
	  else if(0 == strcmp(scratchString, "GNRMB_ORIGIN_ID") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_ORIGIN_ID;}
	  else if(0 == strcmp(scratchString, "GNRMB_DEST_ID") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_DEST_ID;}
	  else if(0 == strcmp(scratchString, "GNRMB_DEST_LAT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_DEST_LAT;}
	  else if(0 == strcmp(scratchString, "GNRMB_DEST_LON") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_DEST_LON;}
	  else if(0 == strcmp(scratchString, "GNRMB_RANGE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_RANGE;}
	  else if(0 == strcmp(scratchString, "GNRMB_BEARING") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_BEARING;}
	  else if(0 == strcmp(scratchString, "GNRMB_DEST_SPEED") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_DEST_SPEED;}
	  else if(0 == strcmp(scratchString, "GNRMB_ARRIVAL_ALARM") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMB_ARRIVAL_ALARM;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_TIME;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_HOUR;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_MINUTE;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_SECOND;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_FRACTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_FRACTION;}
	  else if(0 == strcmp(scratchString, "GNRMC_RCVR_STATUS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_RCVR_STATUS;}
	  else if(0 == strcmp(scratchString, "GNRMC_LAT_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LAT_DECDEG;}
	  else if(0 == strcmp(scratchString, "GNRMC_LAT_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LAT_DECMIN;}
	  else if(0 == strcmp(scratchString, "GNRMC_LAT_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LAT_DECSEC;}
	  else if(0 == strcmp(scratchString, "GNRMC_LAT_NOAA") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LAT_NOAA;}
	  else if(0 == strcmp(scratchString, "GNRMC_LON_DECDEG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LON_DECDEG;}
	  else if(0 == strcmp(scratchString, "GNRMC_LON_DECMIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LON_DECMIN;}
	  else if(0 == strcmp(scratchString, "GNRMC_LON_DECSEC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LON_DECSEC;}
	  else if(0 == strcmp(scratchString, "GNRMC_LON_NOAA") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_LON_NOAA;}
	  else if(0 == strcmp(scratchString, "GNRMC_SOG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_SOG;}
	  else if(0 == strcmp(scratchString, "GNRMC_COG_SAMOS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_COG_SAMOS;}
	  else if(0 == strcmp(scratchString, "GNRMC_COG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_COG;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_DATE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_DATE;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_YEAR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_YEAR;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_MONTH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_MONTH;}
	  else if(0 == strcmp(scratchString, "GNRMC_UTC_DAY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_UTC_DAY;}
	  else if(0 == strcmp(scratchString, "GNRMC_MAGVAR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNRMC_MAGVAR;}
	  else if(0 == strcmp(scratchString, "GNVTG_TRUE_TRACK") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNVTG_TRUE_TRACK;}
	  else if(0 == strcmp(scratchString, "GNVTG_MAGNETIC_TRACK") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNVTG_MAGNETIC_TRACK;}
	  else if(0 == strcmp(scratchString, "GNVTG_SPEED_KNOTS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNVTG_SPEED_KNOTS;}
	  else if(0 == strcmp(scratchString, "GNVTG_SPEED_KPH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNVTG_SPEED_KPH;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_TIME;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_HOUR;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_MINUTE;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_SECOND;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_FRACTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_FRACTION;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_DATE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_DATE;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_YEAR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_YEAR;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_MONTH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_MONTH;}
	  else if(0 == strcmp(scratchString, "GNZDA_UTC_DAY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_UTC_DAY;}
	  else if(0 == strcmp(scratchString, "GNZDA_LOCALZONE_HOURS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_LOCALZONE_HOURS;}
	  else if(0 == strcmp(scratchString, "GNZDA_LOCALZONE_MINUTES") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GNZDA_LOCALZONE_MINUTES;}
	  else if(0 == strcmp(scratchString, "HEHDT_HDG_SAMOS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_HEHDT_HDG_SAMOS;}
	  else if(0 == strcmp(scratchString, "HEHDT_HDG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_HEHDT_HDG;}
	  else if(0 == strcmp(scratchString, "GPHDT_HDG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_GPHDT_HDG;}
	  else if(0 == strcmp(scratchString, "SBE45_TEMPERATURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SBE45_TEMPERATURE;}
	  else if(0 == strcmp(scratchString, "SBE45_CONDUCTIVITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SBE45_CONDUCTIVITY;}
	  else if(0 == strcmp(scratchString, "SBE45_SALINITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SBE45_SALINITY;}
	  else if(0 == strcmp(scratchString, "SBE45_SOUNDVELOCITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SBE45_SOUNDVELOCITY;}
	  else if(0 == strcmp(scratchString, "SBE48_TEMPERATURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SBE48_TEMPERATURE;}
	  else if(0 == strcmp(scratchString, "FLR_MILLIVOLTS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLR_MILLIVOLTS;}
	  else if(0 == strcmp(scratchString, "VAI_ADDRESS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_ADDRESS;}
	  else if(0 == strcmp(scratchString, "VAI_WIND_DIRECTION_AVG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_WIND_DIRECTION_AVG;}
	  else if(0 == strcmp(scratchString, "VAI_WIND_SPEED_MIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_WIND_SPEED_MIN;}
	  else if(0 == strcmp(scratchString, "VAI_WIND_SPEED_AVG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_WIND_SPEED_AVG;}
	  else if(0 == strcmp(scratchString, "VAI_WIND_SPEED_MAX") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_WIND_SPEED_MAX;}
	  else if(0 == strcmp(scratchString, "VAI_TEMPERATURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_TEMPERATURE;}
	  else if(0 == strcmp(scratchString, "VAI_HUMIDITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_HUMIDITY;}
	  else if(0 == strcmp(scratchString, "VAI_PRESSURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_PRESSURE;}
	  else if(0 == strcmp(scratchString, "VAI_RAIN_ACCUMULATION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_RAIN_ACCUMULATION;}
	  else if(0 == strcmp(scratchString, "VAI_RAIN_INTENSITY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VAI_RAIN_INTENSITY;}
	  else if(0 == strcmp(scratchString, "VDVBW_WATER_SPEED_AHEAD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VDVBW_WATER_SPEED_AHEAD;}
	  else if(0 == strcmp(scratchString, "VDVBW_WATER_SPEED_STBD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VDVBW_WATER_SPEED_STBD;}
	  else if(0 == strcmp(scratchString, "VDVBW_WATER_SPEED_STATUS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VDVBW_WATER_SPEED_STATUS;}
	  else if(0 == strcmp(scratchString, "VDVBW_GROUND_SPEED_AHEAD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VDVBW_GROUND_SPEED_AHEAD;}
	  else if(0 == strcmp(scratchString, "VDVBW_GROUND_SPEED_STBD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_VDVBW_GROUND_SPEED_STBD;}
	  else if(0 == strcmp(scratchString, "SWR_RADIATION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SWR_RADIATION;}
	  else if(0 == strcmp(scratchString, "LWR_DOME_TEMPERATURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_LWR_DOME_TEMPERATURE;}
	  else if(0 == strcmp(scratchString, "LWR_BODY_TEMPERATURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_LWR_BODY_TEMPERATURE;}
	  else if(0 == strcmp(scratchString, "LWR_THERMOPILE_VOLTAGE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_LWR_THERMOPILE_VOLTAGE;}
	  else if(0 == strcmp(scratchString, "LWR_RADIATION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_LWR_RADIATION;}
	  else if(0 == strcmp(scratchString, "PASHR_TIME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_TIME;}
	  else if(0 == strcmp(scratchString, "PASHR_HEADING") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_HEADING;}
	  else if(0 == strcmp(scratchString, "PASHR_ROLL") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ROLL;}
	  else if(0 == strcmp(scratchString, "PASHR_PITCH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_PITCH;}
	  else if(0 == strcmp(scratchString, "PASHR_HEAVE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_HEAVE;}
	  else if(0 == strcmp(scratchString, "PASHR_ROLL_ACCURACY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ROLL_ACCURACY;}
	  else if(0 == strcmp(scratchString, "PASHR_PITCH_ACCURACY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_PITCH_ACCURACY;}
	  else if(0 == strcmp(scratchString, "PASHR_HEADING_ACCURACY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_HEADING_ACCURACY;}
	  else if(0 == strcmp(scratchString, "PASHR_GPS_FLAG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_GPS_FLAG;}
	  else if(0 == strcmp(scratchString, "PASHR_IMU_FLAG") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_IMU_FLAG;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_SECONDS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_SECONDS;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_HEADING") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_HEADING;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_PITCH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_PITCH;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_ROLL") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_ROLL;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_MRMS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_MRMS;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_BRMS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_BRMS;}
	  else if(0 == strcmp(scratchString, "PASHR_ATT_REAQUISITION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PASHR_ATT_REAQUISITION;}
	  else if(0 == strcmp(scratchString, "TSS1_RAW_STRING") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_RAW_STRING;}
	  else if(0 == strcmp(scratchString, "TSS1_RAW_ACCEL_HOR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_RAW_ACCEL_HOR;}
	  else if(0 == strcmp(scratchString, "TSS1_RAW_ACCEL_VER") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_RAW_ACCEL_VER;}
	  else if(0 == strcmp(scratchString, "TSS1_RAW_HEAVE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_RAW_HEAVE;}
	  else if(0 == strcmp(scratchString, "TSS1_RAW_PITCH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_RAW_PITCH;}
	  else if(0 == strcmp(scratchString, "TSS1_RAW_ROLL") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_RAW_ROLL;}
	  else if(0 == strcmp(scratchString, "TSS1_ACCEL_HOR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_ACCEL_HOR;}
	  else if(0 == strcmp(scratchString, "TSS1_ACCEL_VER") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_ACCEL_VER;}
	  else if(0 == strcmp(scratchString, "TSS1_HEAVE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_HEAVE;}
	  else if(0 == strcmp(scratchString, "TSS1_PITCH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_PITCH;}
	  else if(0 == strcmp(scratchString, "TSS1_ROLL") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_ROLL;}
	  else if(0 == strcmp(scratchString, "TSS1_STATUS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_TSS1_STATUS;}
	  else if(0 == strcmp(scratchString, "PKEL_DEPTH_1") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PKEL_DEPTH_1;}
	  else if(0 == strcmp(scratchString, "PKEL_DUCER_DEPTH_1") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PKEL_DUCER_DEPTH_1;}
	  else if(0 == strcmp(scratchString, "PKEL_STATUS_1") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PKEL_STATUS_1;}
	  else if(0 == strcmp(scratchString, "PKEL_DEPTH_2") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PKEL_DEPTH_2;}
	  else if(0 == strcmp(scratchString, "PKEL_DUCER_DEPTH_2") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PKEL_DUCER_DEPTH_2;}
	  else if(0 == strcmp(scratchString, "PKEL_STATUS_2") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PKEL_STATUS_2;}
	  else if(0 == strcmp(scratchString, "PHINS_HEADING") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHINS_HEADING;}
	  else if(0 == strcmp(scratchString, "PHTRO_PITCH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHTRO_PITCH;}
	  else if(0 == strcmp(scratchString, "PHTRO_PITCH_FRAME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHTRO_PITCH_FRAME;}
	  else if(0 == strcmp(scratchString, "PHTRO_ROLL") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHTRO_ROLL;}
	  else if(0 == strcmp(scratchString, "PHTRO_ROLL_FRAME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHTRO_ROLL_FRAME;}
	  else if(0 == strcmp(scratchString, "PHLIN_SURGE_LIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHLIN_SURGE_LIN;}
	  else if(0 == strcmp(scratchString, "PHLIN_SWAY_LIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHLIN_SWAY_LIN;}
	  else if(0 == strcmp(scratchString, "PHLIN_HEAVE_LIN") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHLIN_HEAVE_LIN;}
	  else if(0 == strcmp(scratchString, "PHSPD_SURGE_SPD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHSPD_SURGE_SPD;}
	  else if(0 == strcmp(scratchString, "PHSPD_SWAY_SPD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHSPD_SWAY_SPD;}
	  else if(0 == strcmp(scratchString, "PHSPD_HEAVE_SPD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_PHSPD_HEAVE_SPD;}
	  else if(0 == strcmp(scratchString, "KIDPT_DEPTH") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_KIDPT_DEPTH;}
	  else if(0 == strcmp(scratchString, "KIDPT_BEAMS_GOOD") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_KIDPT_BEAMS_GOOD;}
	  else if(0 == strcmp(scratchString, "KIDPT_FREQUENCY") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_KIDPT_FREQUENCY;}
	  else if(0 == strcmp(scratchString, "XCALC_SSV") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_XCALC_SSV;}
	  else if(0 == strcmp(scratchString, "XCALC_TRUEWIND_SPEED") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_XCALC_TRUEWIND_SPEED;}
	  else if(0 == strcmp(scratchString, "XCALC_TRUEWIND_DIRECTION") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_XCALC_TRUEWIND_DIRECTION;}
	  else if(0 == strcmp(scratchString, "FLOW_RAW_COUNTS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_RAW_COUNTS;}
	  else if(0 == strcmp(scratchString, "FLOW_CUMULATIVE_COUNTS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_CUMULATIVE_COUNTS;}
	  else if(0 == strcmp(scratchString, "FLOW_LAST_SECOND") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_FLOW_LAST_SECOND;}
	  else if(0 == strcmp(scratchString, "FLOW_LAST_MINUTE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_FLOW_LAST_MINUTE;}
	  else if(0 == strcmp(scratchString, "FLOW_LAST_HOUR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_FLOW_LAST_HOUR;}
	  else if(0 == strcmp(scratchString, "FLOW_LAST_24HOURS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_FLOW_LAST_24HOURS;}
	  else if(0 == strcmp(scratchString, "FLOW_ELAPSED_SECONDS") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_FLOW_ELAPSED_SECONDS;}
	  else if(0 == strcmp(scratchString, "AML_SSV_TEMPERATURE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_AML_SSV_TEMPERATURE;}
	  else if(0 == strcmp(scratchString, "AML_SSV_SSV") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_AML_SSV_SSV;}
	  else if(0 == strcmp(scratchString, "RAD_SERIAL_NUMBER") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_SERIAL_NUMBER;}
	  else if(0 == strcmp(scratchString, "RAD_NUMBER") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_NUMBER;}
	  else if(0 == strcmp(scratchString, "RAD_PIR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_PIR;}
	  else if(0 == strcmp(scratchString, "RAD_LW") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_LW;}
	  else if(0 == strcmp(scratchString, "RAD_TCASE") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_TCASE;}
	  else if(0 == strcmp(scratchString, "RAD_TDOME") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_TDOME;}
	  else if(0 == strcmp(scratchString, "RAD_SW") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_SW;}
	  else if(0 == strcmp(scratchString, "RAD_T_AVR") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_T_AVR;}
	  else if(0 == strcmp(scratchString, "RAD_BATT") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_RAD_BATT;}
	  else if(0 == strcmp(scratchString, "SSSG_GENERIC") ) { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_SSSG_GENERIC;}
	  else { thisInputDescriptor.csvParse[promptNumber].parseDataType = CPD_INVALID_TYPE;}
#if DEBUG_READINIDSLOGCSV
	  printf("***** parseDataType[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].parseDataType);
#endif

	  //  int csvParseIndex;  // indicates what field this is
	  snprintf(fieldName,64,"PARSE_INDEX_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].csvParseIndex = rov_read_int(ini_fd, inputField, fieldName, 1);
#if DEBUG_READINIDSLOGCSV
	  printf("\n***** parseIndex[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].csvParseIndex);
#endif

	  //  char *mustContain;
	  snprintf(fieldName,64,"MUST_CONTAIN_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "INVALID_STRING");
	  thisInputDescriptor.csvParse[promptNumber].mustContain = strdup(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("***** mustContain[%d] is %s *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].mustContain);
#endif

	  //  char *mustContain2;
	  snprintf(fieldName,64,"MUST_CONTAIN2_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "INVALID_STRING");
	  thisInputDescriptor.csvParse[promptNumber].mustContain2 = strdup(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("***** mustContain2[%d] is %s *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].mustContain2);
#endif

	  //  char *mustNotContain;
	  snprintf(fieldName,64,"MUST_NOT_CONTAIN_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "INVALID_STRING");
	  thisInputDescriptor.csvParse[promptNumber].mustNotContain = strdup(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("***** mustNotContain[%d] is %s *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].mustNotContain);
#endif

	  //---------------------------------------------------------------------------------------------------------------
	  // These variables are intended to be used only when parsing SSSG_GENERIC. But they are read for every input field
	  //---------------------------------------------------------------------------------------------------------------

	  snprintf(fieldName,64,"GENERIC_HEADER_STRING_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "INVALID_STRING");
	  thisInputDescriptor.csvParse[promptNumber].genericHeaderString = strdup(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("***** genericHeaderString[%d] is %s *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].genericHeaderString);
#endif
	  thisInputDescriptor.csvParse[promptNumber].genericUseHeaderString = 0 != strcmp(thisInputDescriptor.csvParse[promptNumber].genericHeaderString, "INVALID_STRING");

	  snprintf(fieldName,64,"GENERIC_DELIMITER_CHAR_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "INVALID_STRING");
	  if(0 == strcmp(scratchString, "INVALID_STRING") ) {
	    thisInputDescriptor.csvParse[promptNumber].genericDelimiterChar = ',';
	  }
	  else {
	    thisInputDescriptor.csvParse[promptNumber].genericDelimiterChar = scratchString[0];
	  }
#if DEBUG_READINIDSLOGCSV
	  printf("***** genericDelimiterChar[%d] is %c *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].genericDelimiterChar);
#endif
	  thisInputDescriptor.csvParse[promptNumber].genericUseHeaderString = 0 != strcmp(thisInputDescriptor.csvParse[promptNumber].genericHeaderString, "INVALID_STRING");

	  snprintf(fieldName,64,"GENERIC_INDEX_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].genericIndex = rov_read_int(ini_fd, inputField, fieldName, 1);
#if DEBUG_READINIDSLOGCSV
	  printf("\n***** genericIndex[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].genericIndex);
#endif

	  snprintf(fieldName,64,"GENERIC_DATA_TYPE_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].genericDataType = rov_read_int(ini_fd, inputField, fieldName, 1);
#if DEBUG_READINIDSLOGCSV
	  printf("\n***** genericDataType[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].genericDataType);
#endif

	  snprintf(fieldName,64,"GENERIC_DATA_PRECISION_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].genericDataPrecision = rov_read_int(ini_fd, inputField, fieldName, 1);
#if DEBUG_READINIDSLOGCSV
	  printf("\n***** genericDataPrecision[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].genericDataPrecision);
#endif

	  snprintf(fieldName,64,"GENERIC_DELIMITER_SKIP_COUNT_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].genericDelimiterSkipCount = rov_read_int(ini_fd, inputField, fieldName, -1);
#if DEBUG_READINIDSLOGCSV
	  printf("\n***** genericDelimiterSkipCount[%d] is %d *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].genericDelimiterSkipCount);
#endif

	  thisInputDescriptor.csvParse[promptNumber].genericDelimiterAfterHeader = false;

#if 0
	  //---------------------------------------------------------------------------------------------------------------
	  //--- this section is for variables used to implement user defined parsing (not rule based)
	  //  int valueType;
	  snprintf(fieldName,64,"PARSE_VALUE_TYPE_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].valueType = rov_read_int(ini_fd, inputField, fieldName, 0);
	  // make sure there is a valid value
	  switch(thisInputDescriptor.csvParse[promptNumber].valueType) {
	  case PARSE_VALUE_TYPE_INT:
	  case PARSE_VALUE_TYPE_DOUBLE:
	  case PARSE_VALUE_TYPE_STRING:
	    break;
	  default:
	    thisInputDescriptor.csvParse[promptNumber].valueType = PARSE_VALUE_TYPE_INT;
	    break;
	  }
#if DEBUG_READINIDSLOGCSV
	  switch(thisInputDescriptor.csvParse[promptNumber].valueType) {
	  case PARSE_VALUE_TYPE_INT:
	    printf("***** valueType[%d] is INT *****\n", promptNumber);
	    break;
	  case PARSE_VALUE_TYPE_DOUBLE:
	    printf("***** valueType[%d] is DOUBLE *****\n", promptNumber);
	    break;
	  case PARSE_VALUE_TYPE_STRING:
	    printf("***** valueType[%d] is STRING *****\n", promptNumber);
	    break;
	  }
#endif

	  //  int intFormatType;
	  snprintf(fieldName,64,"PARSE_INT_FORMAT_TYPE_%d", promptNumber+1);
	  thisInputDescriptor.csvParse[promptNumber].intFormatType = rov_read_int(ini_fd, inputField, fieldName, 1);
	  // make sure there is a valid value
	  switch(thisInputDescriptor.csvParse[promptNumber].intFormatType) {
	  case PARSE_INT_FORMAT_TYPE_DECIMAL:
	  case PARSE_INT_FORMAT_TYPE_HEX_NONE:
	  case PARSE_INT_FORMAT_TYPE_HEX_0X:
	  case PARSE_INT_FORMAT_TYPE_OCTAL_NONE:
	    break;
	  default:
	    thisInputDescriptor.csvParse[promptNumber].intFormatType = PARSE_INT_FORMAT_TYPE_DECIMAL;
	    break;
	  }
#if DEBUG_READINIDSLOGCSV
	  switch(thisInputDescriptor.csvParse[promptNumber].intFormatType) {
	  case PARSE_INT_FORMAT_TYPE_DECIMAL:
	    printf("***** intFormatType[%d] is DECIMAL *****\n", promptNumber);
	    break;
	  case PARSE_INT_FORMAT_TYPE_HEX_NONE:
	    printf("***** intFormatType[%d] is HEX_NONE *****\n", promptNumber);
	    break;
	  case PARSE_INT_FORMAT_TYPE_HEX_0X:
	    printf("***** intFormatType[%d] is HEX_0X *****\n", promptNumber);
	    break;
	  case PARSE_INT_FORMAT_TYPE_OCTAL_NONE:
	    printf("***** intFormatType[%d] is OCTAL_NONE *****\n", promptNumber);
	    break;
	  }
#endif

	  //  char *delimiterString;
	  snprintf(fieldName,64,"DELIMITER_STRING_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "XXXX");
	  thisInputDescriptor.csvParse[promptNumber].delimiterString = strdup(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("***** delimiterString[%d] is %s *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].delimiterString);
#endif

	  //  char *rawScanfString;
	  snprintf(fieldName,64,"RAW_SCANF_STRING_%d", promptNumber+1);
	  scratchString = rov_read_string(ini_fd, inputField, fieldName, "XXXX");
	  thisInputDescriptor.csvParse[promptNumber].rawScanfString = strdup(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("***** rawScanfString[%d] is %s *****\n", promptNumber, thisInputDescriptor.csvParse[promptNumber].rawScanfString);
#endif
	  //---------------------------------------------------------------------------------------------------------------
	  //--- end of section is for variables used to implement user defined parsing (not rule based)
#endif
	}
      }  // end of if(thisInputDescriptor.numberOfCsvParse > 0)
    }  //----- end of section to read ini file info for csv parse values
    
    localLoggingList[loggingCount] = thisInputDescriptor;
    
    loggingCount++;
    *nOfLoggers = loggingCount;
  }  // end of loop over input number

  //----------------------------------------------------------------------------------
  // -- read in the arrays of values needed to control generating calculated values
  for(int calcIndex=0; calcIndex < MAX_CALCULATION_CONTROL_INDEX; ++calcIndex) {
    char calcSectionName[32];
    
    //--- read info for NORMDIFF calculation
    sprintf(calcSectionName, "NORMDIFF_%d", calcIndex+1);
    calcValuesConfig.normdiff[calcIndex].minuendAbstractIndex = rov_read_int(ini_fd, calcSectionName, "MINUEND_ABSTRACT_INDEX", -1);
    calcValuesConfig.normdiff[calcIndex].subtrahendAbstractIndex = rov_read_int(ini_fd, calcSectionName, "SUBTRAHEND_ABSTRACT_INDEX", -1);
    calcValuesConfig.normdiff[calcIndex].normThreshold = rov_read_double(ini_fd, calcSectionName, "NORM_THRESHOLD", 180.0);
    calcValuesConfig.normdiff[calcIndex].normOffset = rov_read_double(ini_fd, calcSectionName, "NORM_OFFSET", 360.0);
    calcValuesConfig.normdiff[calcIndex].precision = rov_read_int(ini_fd, calcSectionName, "OUTPUT_PRECISION", 1);
    
    //--- read info for DIFF calculation
    sprintf(calcSectionName, "DIFF_%d", calcIndex+1);
    calcValuesConfig.diff[calcIndex].minuendAbstractIndex = rov_read_int(ini_fd, calcSectionName, "MINUEND_ABSTRACT_INDEX", -1);
    calcValuesConfig.diff[calcIndex].subtrahendAbstractIndex = rov_read_int(ini_fd, calcSectionName, "SUBTRAHEND_ABSTRACT_INDEX", -1);
    calcValuesConfig.diff[calcIndex].precision = rov_read_int(ini_fd, calcSectionName, "OUTPUT_PRECISION", 1);
    
    //--- read info for SCALE_OFFSET calculation
    sprintf(calcSectionName, "SCALE_OFFSET_%d", calcIndex+1);
    calcValuesConfig.scale_offset[calcIndex].inputAbstractIndex = rov_read_int(ini_fd, calcSectionName, "INPUT_ABSTRACT_INDEX", -1);
    calcValuesConfig.scale_offset[calcIndex].scale = rov_read_double(ini_fd, calcSectionName, "SCALE", 1.0);
    calcValuesConfig.scale_offset[calcIndex].offset = rov_read_double(ini_fd, calcSectionName, "OFFSET", 0.0);
    calcValuesConfig.scale_offset[calcIndex].precision = rov_read_int(ini_fd, calcSectionName, "OUTPUT_PRECISION", 3);
    
    //--- read info for SSV calculation
    sprintf(calcSectionName, "SSV_%d", calcIndex+1);
    calcValuesConfig.ssv[calcIndex].temperatureAbstractIndex = rov_read_int(ini_fd, calcSectionName, "TEMPERATURE_ABSTRACT_INDEX", -1);
    calcValuesConfig.ssv[calcIndex].salinityAbstractIndex = rov_read_int(ini_fd, calcSectionName, "SALINITY_ABSTRACT_INDEX", -1);
    calcValuesConfig.ssv[calcIndex].pressure = rov_read_double(ini_fd, calcSectionName, "PRESSURE", 0.5);  // default pressure is 0.5 Bars for 5 m depth
    calcValuesConfig.ssv[calcIndex].precision = rov_read_int(ini_fd, calcSectionName, "OUTPUT_PRECISION", 3);

    //--- read info for BAROMETER calculation
    sprintf(calcSectionName, "BAROMETER_%d", calcIndex+1);
    calcValuesConfig.barometer[calcIndex].rawPressureAbstractIndex = rov_read_int(ini_fd, calcSectionName, "RAW_PRESSURE_ABSTRACT_INDEX", -1);
    calcValuesConfig.barometer[calcIndex].sensorHeight = rov_read_double(ini_fd, calcSectionName, "SENSOR_HEIGHT", 15.5);  // default sensorHeight is 15.5 meters
    calcValuesConfig.barometer[calcIndex].precision = rov_read_int(ini_fd, calcSectionName, "OUTPUT_PRECISION", 3);

    //--- read info for truewind calculation
    sprintf(calcSectionName, "TRUEWIND_%d", calcIndex+1);
    calcValuesConfig.truewind[calcIndex].cogAbstractIndex = rov_read_int(ini_fd, calcSectionName, "COG_ABSTRACT_INDEX", -1);
    calcValuesConfig.truewind[calcIndex].sogAbstractIndex = rov_read_int(ini_fd, calcSectionName, "SOG_ABSTRACT_INDEX", -1);
    calcValuesConfig.truewind[calcIndex].hdgAbstractIndex = rov_read_int(ini_fd, calcSectionName, "HDG_ABSTRACT_INDEX", -1);
    calcValuesConfig.truewind[calcIndex].windSpeedAbstractIndex = rov_read_int(ini_fd, calcSectionName, "WIND_SPEED_ABSTRACT_INDEX", -1);
    calcValuesConfig.truewind[calcIndex].windDirectionAbstractIndex = rov_read_int(ini_fd, calcSectionName, "WIND_DIR_ABSTRACT_INDEX", -1);
    calcValuesConfig.truewind[calcIndex].zlr = rov_read_double(ini_fd, calcSectionName, "ZLR", 0.0);  // angle between bow and zero line on anemometer (deirection is clockwise from bow)
    calcValuesConfig.truewind[calcIndex].precision = rov_read_int(ini_fd, calcSectionName, "OUTPUT_PRECISION", 3);
  }  // end of for loop over calcIndex

  //----------------------------------------------------------------------------------
  // -- loop over output specifers  
#if 1
  for(int outputNumber = 0; outputNumber < MAX_N_OF_CSV_OUTPUTS; outputNumber++) {
    csv_output_descriptor_t    thisOutputDescriptor;
    char inputField[256];
    int scratchInteger;

    snprintf(inputField,256,"OUTPUT_%d", outputNumber+1);

    thisOutputDescriptor.outputEnable = rov_read_int(ini_fd, inputField, "OUTPUT_ENABLE", 1);
    if(1 != thisOutputDescriptor.outputEnable) {
      continue;
    }

    thisOutputDescriptor.outputInterval = rov_read_int(ini_fd, inputField, "OUTPUT_INTERVAL", 60);

    scratchString = rov_read_string(ini_fd,inputField,"OUTPUT_FILE_FORMAT", "DSLOGCSV");
    if ( 0 == strcmp(scratchString, "SAMOS") ) { thisOutputDescriptor.outputFileFormat = OUTFILE_FORMAT_SAMOS; }
    else if ( 0 == strcmp(scratchString, "NOAA") ) { thisOutputDescriptor.outputFileFormat = OUTFILE_FORMAT_NOAA; }
    else { thisOutputDescriptor.outputFileFormat = OUTFILE_FORMAT_DSLOGCSV; }

    scratchString = rov_read_string(ini_fd,inputField,"TERM_CHAR_1","DEFAULT_VALUE");
    if( 0 == strcmp(scratchString, "DEFAULT_VALUE") ) {
      thisOutputDescriptor.termChar1 = '\n';
    }
    else if( 0 == strncmp(scratchString, "\\n", 2) ) {
      thisOutputDescriptor.termChar1 = '\n';
    }
    else if( 0 == strncmp(scratchString, "\\r", 2) ) {
      thisOutputDescriptor.termChar1 = '\r';
    }
    else {
      thisOutputDescriptor.termChar1 = '\n';
    }
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"TERM_CHAR_2","DEFAULT_VALUE");
    if( 0 == strcmp(scratchString, "DEFAULT_VALUE") ) {
      thisOutputDescriptor.termChar2 = -1;
    }
    else if( 0 == strncmp(scratchString, "\\n", 2) ) {
      thisOutputDescriptor.termChar2 = '\n';
    }
    else if( 0 == strncmp(scratchString, "\\r", 2) ) {
      thisOutputDescriptor.termChar2 = '\r';
    }
    else {
      thisOutputDescriptor.termChar2 = -1;
    }
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"SHIP_NAME","BADINI_SHIP_NAME");
    thisOutputDescriptor.shipName = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"SHIP_CALL_SIGN","BADINI_SHIP_CALL_SIGN");
    thisOutputDescriptor.shipCallSign = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"NOAA_VERSION","BADINI_NOAA_VERSION");
    thisOutputDescriptor.noaaVersionString = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"DATE_HEADER_STRING","DATE_GMT");
    thisOutputDescriptor.dateHeaderString = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"TIME_HEADER_STRING","TIME_GMT");
    thisOutputDescriptor.timeHeaderString = strdup(scratchString);
    free(scratchString);

    scratchInteger = rov_read_int(ini_fd,inputField,"TIMESTAMP_ENABLE", 1);
    thisOutputDescriptor.timestampEnable = ( 1 == scratchInteger ) ;

    scratchString = rov_read_string(ini_fd,inputField,"OUTPUT_PREFIX_1","");
    thisOutputDescriptor.outputPrefix1 = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"OUTPUT_PREFIX_2","");
    thisOutputDescriptor.outputPrefix2 = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"OUTPUT_DELIMITER",", ");
    thisOutputDescriptor.outputDelimiter = strdup(scratchString);
    free(scratchString);

    scratchString = rov_read_string(ini_fd,inputField,"BAD_DATA_STRING","NAN");
    thisOutputDescriptor.badDataString = strdup(scratchString);
    free(scratchString);

    thisOutputDescriptor.numberOfOutputFields = rov_read_int(ini_fd, inputField, "NUMBER_OF_OUTPUT_FIELDS", 0);
    if(thisOutputDescriptor.numberOfOutputFields < 0) {
      thisOutputDescriptor.numberOfOutputFields = 0;
    }
    if(thisOutputDescriptor.numberOfOutputFields > MAX_N_OF_OUTFIELD) {
      thisOutputDescriptor.numberOfOutputFields = MAX_N_OF_OUTFIELD;
    }

    if(thisOutputDescriptor.numberOfOutputFields == 0) {
      continue;
    }

    if(thisOutputDescriptor.numberOfOutputFields > 0) {
      for(int outputFieldIndex=0; outputFieldIndex < thisOutputDescriptor.numberOfOutputFields; ++outputFieldIndex) {
	char    inputField2[256];
	int scratchInt;

        // set lastNoaaHour to 25 - so we always get an output on first time through
	thisOutputDescriptor.lastNoaaHour = 25;

	// outfieldEnable - set to false to disable the inclusion of this field in outputs
	snprintf(inputField2,256,"OUTFIELD_ENABLE_%d",outputFieldIndex+1);
	scratchInt = rov_read_int(ini_fd, inputField, inputField2, 1);
	thisOutputDescriptor.outfieldEnable[outputFieldIndex] = (0 != scratchInt);

	// outfieldAgeLimit - set to any negative value to require data received since last output.  Default is -999.9
	snprintf(inputField2,256,"OUTFIELD_AGE_LIMIT_%d",outputFieldIndex+1);
	thisOutputDescriptor.outfieldAgeLimit[outputFieldIndex] = rov_read_double(ini_fd, inputField, inputField2, -999.9);

	//  outfieldType - default case is just a parsed value, others include trueWind and SSV
        thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_DEFAULT_TYPE;  // setup default value
	thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = 0;           // setup default value

	snprintf(inputField2,256,"OUTFIELD_TYPE_%d",outputFieldIndex+1);
	scratchString = rov_read_string(ini_fd, inputField, inputField2, "INVALID_OUTFIELD_TYPE");
	for(int calcIndex=0; calcIndex < MAX_CALCULATION_CONTROL_INDEX; ++calcIndex) {
	  char    outTypeString[256];

	  snprintf(outTypeString, 256, "NORMDIFF_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_NORMDIFF;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
#if DEBUG_READINIDSLOGCSV
	    printf("********************************************** normdiff - calcIndex=%d=%d\n", calcIndex, thisOutputDescriptor.calculationControlIndex[outputFieldIndex]);
#endif
	  }

	  snprintf(outTypeString, 256, "DIFF_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_DIFF;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
#if DEBUG_READINIDSLOGCSV
	    printf("********************************************** diff - calcIndex=%d=%d\n", calcIndex, thisOutputDescriptor.calculationControlIndex[outputFieldIndex]);
#endif
	  }

	  snprintf(outTypeString, 256, "SCALE_OFFSET_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_SCALE_OFFSET;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
#if DEBUG_READINIDSLOGCSV
	    printf("********************************************** scale_offset - calcIndex=%d=%d\n", calcIndex, thisOutputDescriptor.calculationControlIndex[outputFieldIndex]);
#endif
	  }

	  snprintf(outTypeString, 256, "SSV_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_SSV;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
#if DEBUG_READINIDSLOGCSV
	    printf("********************************************** ssv - calcIndex=%d=%d\n", calcIndex, thisOutputDescriptor.calculationControlIndex[outputFieldIndex]);
#endif
	  }

	  snprintf(outTypeString, 256, "BAROMETER_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_BAROMETER;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
	  }

	  snprintf(outTypeString, 256, "TRUEWIND_SPEED_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_TRUEWIND_SPEED;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
	  }

	  snprintf(outTypeString, 256, "TRUEWIND_DIRECTION_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_TRUEWIND_DIRECTION;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
	  }

	  snprintf(outTypeString, 256, "TRUEWIND_BOTH_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_TRUEWIND_BOTH;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
	  }

	  snprintf(outTypeString, 256, "TRUEWIND_DEBUG_%d", calcIndex+1);
	  if(0 == strcmp(scratchString, outTypeString) ) {
	    thisOutputDescriptor.outfieldType[outputFieldIndex] = OUT_TYPE_TRUEWIND_DEBUG;
	    thisOutputDescriptor.calculationControlIndex[outputFieldIndex] = calcIndex;
	  }
	}  // end of for loop over calcIndex

	snprintf(inputField2,256,"OUTFIELD_ABSTRACT_INDEX_%d",outputFieldIndex+1);
	thisOutputDescriptor.outfieldAbstractIndex[outputFieldIndex] = rov_read_int(ini_fd, inputField, inputField2, 0);

	snprintf(inputField2,256,"OUTFIELD_HEADER_STRING_%d",outputFieldIndex+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,"");
	thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(scratchString);
	free(scratchString);

	//--- fill in default header strings for calculated values if the user did not specify a custom header
	if(0 == strlen(thisOutputDescriptor.outfieldHeaderString[outputFieldIndex]) ) {
	  char tmpString[32];
	  switch(thisOutputDescriptor.outfieldType[outputFieldIndex]) {
	  case OUT_TYPE_NORMDIFF:
	    sprintf(tmpString, "NORMDIFF_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_DIFF:
	    sprintf(tmpString, "DIFF_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_SCALE_OFFSET:
	    sprintf(tmpString, "SCALE_OFFSET_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_SSV:
	    sprintf(tmpString, "SSV_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_BAROMETER:          
	    sprintf(tmpString, "BAROMETER_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_TRUEWIND_SPEED:     
	    sprintf(tmpString, "TRUEWIND_SPEED_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_TRUEWIND_DIRECTION: 
	    sprintf(tmpString, "TRUEWIND_DIRECTION_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_TRUEWIND_BOTH:      
	    sprintf(tmpString, "TRUEWIND_SPEED_%d, TRUEWIND_DIRECTION_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1, thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  case OUT_TYPE_TRUEWIND_DEBUG:     
	    sprintf(tmpString, "TRUEWIND DEBUG_%d", thisOutputDescriptor.calculationControlIndex[outputFieldIndex]+1);               
	    thisOutputDescriptor.outfieldHeaderString[outputFieldIndex] = strdup(tmpString); 
	    break;
	  };
	}

	snprintf(inputField2,256,"OUTFIELD_WIDTH_%d",outputFieldIndex+1);
	thisOutputDescriptor.outfieldWidth[outputFieldIndex] = rov_read_int(ini_fd, inputField, inputField2, 1);
      } // end of loop over outputFieldIndex
    }

    // read the file name prefix
    scratchString = rov_read_string(ini_fd,inputField,"DESTINATION_PREFIX","CSV");
    thisOutputDescriptor.logging.filenamePrefix = strdup(scratchString);
    free(scratchString);
#if DEBUG_READINIDSLOGCSV
    printf("\ninputField is %s\n", inputField);
    printf("\nDESTINATION_PREFIX is %s\n\n", thisOutputDescriptor.logging.filenamePrefix);
#endif
	
    thisOutputDescriptor.logging.dailyLogfilesFlag = rov_read_int(ini_fd, inputField, "DAILY_LOGFILES_FLAG", globalDailyLogfilesFlag);
    thisOutputDescriptor.logging.singleLogfileFlag = rov_read_int(ini_fd, inputField, "SINGLE_LOGFILE_FLAG", globalSingleLogfileFlag);
    
    // read the useFileHeaderFlag
    scratchInteger = rov_read_int(ini_fd,inputField,"USE_FILE_HEADER",1);
    if(0 == scratchInteger) {
      thisOutputDescriptor.logging.useFileHeaderFlag = false;
      thisOutputDescriptor.logging.fileHeaderString = strdup("No file header defined");
    }
    else {
      thisOutputDescriptor.logging.useFileHeaderFlag = true;
      thisOutputDescriptor.logging.fileHeaderString = strdup("this is the temporary file header\n");
    }
    

    thisOutputDescriptor.logging.nOfDestinations = 0;
    int temporaryDestinationCount = 0;
    for(int destinationCount = 0; destinationCount < MAX_N_OF_DESTINATIONS; destinationCount++) {
      char    inputField2[256];

      //---- read destinationType
      // allow user to specify as either string or integer (to allow legacy ini files to work)
      snprintf(inputField2,256,"DESTINATION_%d_TYPE",destinationCount+1);
      scratchString = rov_read_string(ini_fd, inputField, inputField2, DEFAULT_NO_ENTRY);
      if(!strcmp("UDP",scratchString)) { 
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationType = (destinationType_t) UDP_SOCKET;
      }
      else if(!strcmp("FILE",scratchString)) { 
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationType = (destinationType_t) DISK;
      }
      else {
	// no success reading the string - try to read as an integer
	scratchInteger = rov_read_int(ini_fd,inputField,inputField2,(int)NOT_A_DESTINATION);
	if(scratchInteger >=  ((int)NOT_A_DESTINATION) ||  (scratchInteger < (int)UDP_SOCKET)) {
	  continue;
	}
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationType = (destinationType_t)scratchInteger;
      }
      free(scratchString);
      
      //----      
      snprintf(inputField2,256,"DESTINATION_%d_ENABLE",destinationCount+1);
      thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationEnable = rov_read_int(ini_fd,inputField,inputField2,1);

      // --- Should this be removed for post processing??      
      if((int)UDP_SOCKET == thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationType) {
#if 1
	snprintf(inputField2,256,"DESTINATION_%d_IPADDRESS",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_IP_ADDRESS);
	if(!strcmp(NULL_IP_ADDRESS,scratchString)) {
	  continue;
	}
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].networkDestination.ipAddress = strdup(scratchString);
	free(scratchString);
	snprintf(inputField2,256,"DESTINATION_%d_SOCKET",destinationCount+1);
	scratchInteger = rov_read_int(ini_fd,inputField,inputField2,NULL_SOCKET_NUMBER);
	if(NULL_SOCKET_NUMBER == scratchInteger) {
	  free(thisOutputDescriptor.logging.destinations[temporaryDestinationCount].networkDestination.ipAddress);
	  continue;
	}
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].networkDestination.toSocketNumber = scratchInteger;
	
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.implemented = false;
	snprintf(inputField2,256,"DESTINATION_%d_STARTS_WITH",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_GREP_FIELD);
	if(strcmp(NULL_GREP_FIELD,scratchString)) {
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.startsWith = strdup( scratchString);
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.implemented  = true;
	}
	free(scratchString);
	snprintf(inputField2,256,"DESTINATION_%d_STARTS_CONTAINS",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_GREP_FIELD);
	if(strcmp(NULL_GREP_FIELD,scratchString)) {
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.greps = strdup( scratchString);
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.implemented  = true;
	}
	free(scratchString);
	snprintf(inputField2,256,"DESTINATION_%d_STARTS_DOESNT_CONTAIN",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,NULL_GREP_FIELD);
	if(strcmp(NULL_GREP_FIELD,scratchString)) {
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.grepMinusVs = strdup( scratchString);
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationFilter.implemented  = true;
	}
	free(scratchString);
#endif
      } // *** end of if destinationType == UDP_SOCKET
      else if(DISK == thisOutputDescriptor.logging.destinations[temporaryDestinationCount].destinationType) {
	// read the pathname
	snprintf(inputField2,256,"DESTINATION_%d_PATHNAME",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,startup.rootDirectory);
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.loggingDirectory = strdup(scratchString);
	free(scratchString);
#if DEBUG_READINIDSLOGCSV
	printf("\ninputField is %s; inputField2 is %s\n", inputField, inputField2);
	printf("\nDESTINATION_PATHNAME is %s\n\n", thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.loggingDirectory);
#endif
	
	// read the file name prefix
	snprintf(inputField2,256,"DESTINATION_%d_PREFIX",destinationCount+1);
	scratchString = rov_read_string(ini_fd,inputField,inputField2,thisOutputDescriptor.logging.filenamePrefix);
	thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.filenamePrefix = strdup(scratchString);
	free(scratchString);
#if DEBUG_READINIDSLOGCSV
	printf("\ninputField is %s; inputField2 is %s\n", inputField, inputField2);
	printf("\nDESTINATION_PREFIX is %s\n\n", thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.filenamePrefix);
#endif
	
#if 1
	if(thisOutputDescriptor.logging.useDestinationSpecificFileExtension) {
	  // read the file extension
	  snprintf(inputField2,256,"DESTINATION_%d_EXTENSION",destinationCount+1);
	  scratchString = rov_read_string(ini_fd,inputField,inputField2,"UNDEFINED");
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.fileExtension = strdup(scratchString);
	  free(scratchString);
#if DEBUG_READINIDSLOGCSV
	  printf("\ninputField is %s; inputField2 is %s\n", inputField, inputField2);
	  printf("\nDESTINATION_EXTENSION is %s\n\n", thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.fileExtension);
#endif
	}
	else {
	  thisOutputDescriptor.logging.destinations[temporaryDestinationCount].loggingDestination.fileExtension = strdup("csv");
	}
#endif
      }  // *** end of if destinationType == DISK
      
      temporaryDestinationCount++;
#endif      
    }  // *** end of loop over destinationCount
    thisOutputDescriptor.logging.nOfDestinations = temporaryDestinationCount;
#if DEBUG_READINIDSLOGCSV
    printf("**** **** thisOutputDescriptor.nOfDestinations is %d **** ****\n", thisOutputDescriptor.logging.nOfDestinations);
#endif

    localOutputList[outputCount] = thisOutputDescriptor;
    
    outputCount++;
    *nOfOutputs = outputCount;
  }  // *** end of loop over csv outputs
#if DEBUG_READINIDSLOGCSV
  printf("outputCount is %d \n", outputCount);
#endif
  
  return 1;
}


