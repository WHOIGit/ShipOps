#ifndef LOGSTATUSGUI_H
#define LOGSTATUSGUI_H

#include <QtGui>
#include <QtNetwork>


#include "ini_file.h"

#define PROGRAM_VERSION       "V1.0"
#define MAX_N_OF_INPUTS         256
#define DEFAULT_NO_ENTRY        "NO_ENTRY"

#define  MAX_N_OF_PROMPTSEQUENCES        16
#define MAX_N_OF_PROMPTS_IN_SEQUENCE      16

#define MAX_N_OF_DESTINATIONS   16


#define NO_SOURCE_TYPE          99
#define  NO_IN_SOCKET      99

#define  MAX_PROMPT_LENGTH       8

#define	ERRONEOUS_IP_ADDRESS          "987.654.321.012"
#define	DEFAULT_CONTROLLER_SOCKET     60001

#define  DEFAULT_LOGGER_PORT     10505
#define  LOG_THREAD_OFFSET    512


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
   unsigned char     promptChars[MAX_PROMPT_LENGTH];
   int               promptCharLength;
} prompt_t;

typedef struct {
   prompt_t    prompts[MAX_N_OF_PROMPTS_IN_SEQUENCE];
   double      secondsBetweenPrompts;
   double      secondsBetweenSequences;
   int         numberOfPrompts;

} prompt_sequence_t;

typedef struct {
   char   *loggingDirectory;
   int    lastHour;
   char   asciiLogFileName[256];
   FILE   *asciiLogFileD;
   bool   active;
   int    errors;
   long long unsigned freeBytes;
   double   percentage;
}  loggingDestination_t;

typedef struct {
   destinationType_t       destinationType;
   loggingDestination_t    loggingDestination;

} distribution_t;

typedef struct
{
   char                 *sourceName;
   char                 *dataType;
   source_type_t        sourceType;
   int                  threadNumber;
   int                  inSocketNumber;
   int                  timeStampFlag;
   int                  nOfPromptSequences;
   distribution_t       destinations[MAX_N_OF_DESTINATIONS];

   int                  nOfDestinations;
   bool                 active;
   char                 formatString[512];
   char                identifier[12];
}   logging_descriptor_t;

typedef struct
{
   float    timeSincedata;
   int      bytesRecieved;
   int      msgsRecieved;
   int      errorCount;
} logging_status_t;

class LoggerStatusWidget : public QWidget
{

   Q_OBJECT

public:
   LoggerStatusWidget(logging_descriptor_t   *theLogger);
   void           updateReceiveStatus(logging_status_t  theStatus, QString dtStr);
   void           updateDiskStatus(logging_descriptor_t theLogger);
private:
   QGroupBox      *staticDataGroupBox;
   QLabel         *sourceLabel;
   QLabel         *timestampLabel;

   QGroupBox      *incomingDataGroupBox;
   QLabel         *timeSinceLastDataLabel;
   QLabel         *bytesReceivedLabel;
   QLabel         *msgsReceivedLabel;
   QLabel         *readErrorsLabel;
   QLabel         *LastStatusUpdateLabel;

   QGroupBox      *loggingGroupBox;
   QLabel         *percentageLabel[MAX_N_OF_DESTINATIONS];
   QLabel         *loggingErrorsLabel[MAX_N_OF_DESTINATIONS];
   QLabel         *freeDiskSpaceLabel[MAX_N_OF_DESTINATIONS];

   QGroupBox      *sampleDataGroupBox;
   QPushButton    *getDataPushButton;

   int            dataThreadNumber;

private slots:
   void           getSampleData();




};





class LogStatusGui : public QWidget
{
    Q_OBJECT

public:
    LogStatusGui(char	*startup_file_name);
    ~LogStatusGui();

 private slots:
   void                       getLoggerInput();
   void                       tcpConnectionMade();
   void                       requestIni();
   void                       readIniFromServer();
   void                       iniFileReadTime();
   void                       iniFileRequestTime();

private:
   QUdpSocket              *listenSocket;
   unsigned short          loggerPort;
   IniFile                 iniFile;
   int                     readLogStatusGuiIniFile();
   int                     loggingCount;
   logging_descriptor_t    theLoggingList[MAX_N_OF_INPUTS];
   logging_status_t        theStatusList[MAX_N_OF_INPUTS];


   char                    *rootDirectory;

   void                    buildGUI();
   int                     loggerCount;
   LoggerStatusWidget      *statusWidgets[MAX_N_OF_INPUTS];
   QTabWidget              *loggerTabs;

   int                     parseLGS(char *theIndata);
   int                     parseDSR(char *theIndata);
   void                    updateInputStats();
   void                    updateDestinationStats();

   QTcpSocket                  *iniClient;
   QHostAddress                iniServerAddress;
   int                         iniServerAddressIsGood;
   unsigned short              iniServerSocketNumber;
   bool                        iniStarted;
   bool                        iniFinished;

   QTimer                     *iniRequestTimer;
   QTemporaryFile             *tempIniFile;
   bool                       gotIni;



};

#endif // LOGSTATUSGUI_H
