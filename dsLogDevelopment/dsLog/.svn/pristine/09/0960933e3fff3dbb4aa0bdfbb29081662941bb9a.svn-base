#include "LogStatusGui.h"


LoggerStatusWidget::LoggerStatusWidget(logging_descriptor_t *theLogger) : QWidget()
{
   staticDataGroupBox = new QGroupBox("input");
   dataThreadNumber = theLogger->threadNumber;
   sourceLabel = new QLabel((QString)theLogger->sourceName);
   timestampLabel = new QLabel("timestamp:");
   QVBoxLayout *infoLayout = new QVBoxLayout(staticDataGroupBox);
   infoLayout->addWidget(sourceLabel);
   infoLayout->addWidget(timestampLabel);


   QGridLayout *widgetLayout = new QGridLayout(this);
   widgetLayout->addWidget(staticDataGroupBox,0,0);

   incomingDataGroupBox = new QGroupBox("incoming data");
   timeSinceLastDataLabel = new QLabel("time since last Data:");
   bytesReceivedLabel = new QLabel("bytes recieved:");
   msgsReceivedLabel = new QLabel("msgs recieved:");
   readErrorsLabel = new QLabel("recieve error count:");
   LastStatusUpdateLabel = new QLabel("last status update");

   QGridLayout *incomingDataLayout = new QGridLayout(incomingDataGroupBox);
   incomingDataLayout->addWidget(timeSinceLastDataLabel,0,1);
   incomingDataLayout->addWidget(LastStatusUpdateLabel,0,0);
   incomingDataLayout->addWidget(msgsReceivedLabel,1,0);
   incomingDataLayout->addWidget(bytesReceivedLabel,1,1);
   incomingDataLayout->addWidget(readErrorsLabel,2,0);


   widgetLayout->addWidget(incomingDataGroupBox,1,0);

   loggingGroupBox = new QGroupBox("disk logging");
   QGridLayout *diskDataLayout = new QGridLayout(loggingGroupBox);
   for(int dN = 0; dN < theLogger->nOfDestinations; dN++)
       {
           percentageLabel[dN]= new QLabel("percentage:");
           loggingErrorsLabel[dN]= new QLabel("logging errors:");
           freeDiskSpaceLabel[dN]= new QLabel("free disk space:");



           diskDataLayout->addWidget(percentageLabel[dN],dN,0);
           diskDataLayout->addWidget(loggingErrorsLabel[dN],dN,1);
           diskDataLayout->addWidget(freeDiskSpaceLabel[dN],dN,2);
       }

   widgetLayout->addWidget(loggingGroupBox,0,1);

   sampleDataGroupBox = new QGroupBox("data samples");
   getDataPushButton = new QPushButton("get some data");
   connect(getDataPushButton,SIGNAL(clicked()), this, SLOT(getSampleData()));
   QGridLayout *sampleDataLayout = new QGridLayout(sampleDataGroupBox);
   sampleDataLayout->addWidget(getDataPushButton, 0,0);

   widgetLayout->addWidget(sampleDataGroupBox,1,1,1,1);

}

void LoggerStatusWidget::updateDiskStatus(logging_descriptor_t theLogger)
{
    for(int dN = 0; dN < theLogger.nOfDestinations; dN++)
        {
            if((DISK == theLogger.destinations[dN].destinationType) || (SINGLE_DISK == theLogger.destinations[dN].destinationType))
                {
                    percentageLabel[dN]->setText("percentage: " + QString::number(theLogger.destinations[dN].loggingDestination.percentage));
                    freeDiskSpaceLabel[dN]->setText("disk free: " + QString::number(theLogger.destinations[dN].loggingDestination.freeBytes/(1024*1024)) + " MB");
                    loggingErrorsLabel[dN]->setText("errors: " + QString::number(theLogger.destinations[dN].loggingDestination.errors));
                }
        }

}

void LoggerStatusWidget::getSampleData()
{

}

void LoggerStatusWidget::updateReceiveStatus(logging_status_t  theStatus, QString dtString)
{
   timeSinceLastDataLabel->setText("time since last data: " + QString::number(theStatus.timeSincedata,'g',2) + "s");
   bytesReceivedLabel->setText("bytes received: " + QString::number(theStatus.bytesRecieved));
   msgsReceivedLabel->setText("msgs received: " + QString::number(theStatus.msgsRecieved));
   readErrorsLabel->setText("receive errors: " + QString::number(theStatus.errorCount) );
   LastStatusUpdateLabel->setText("last status msg: " + dtString);
}

LogStatusGui::LogStatusGui(char	*startup_file_name)
   : QWidget()
{

   loggerCount = 0;
   int	startup_return_code = iniFile.open_ini(startup_file_name);
   if(startup_return_code != GOOD_INI_FILE_READ)
      {
         return ;
      }
   int   readFromIni = iniFile.read_int((char *)"GENERAL",(char *)"READ_FROM_INI", TRUE);
   iniRequestTimer = new QTimer(this);

   if(readFromIni)
      {
         connect(iniRequestTimer, SIGNAL(timeout()), this, SLOT(iniFileReadTime()));
         iniRequestTimer->start(1000);
      }
   else
      {


         // here is where I should set up the parameters for the tcp connection to navest
         iniServerAddressIsGood = false;
         connect(iniRequestTimer, SIGNAL(timeout()), this, SLOT(iniFileRequestTime()));
         char *scratchString = iniFile.read_string((char *)"GENERAL",(char *)"INI_SERVER_IP_ADDRESS",(char *)ERRONEOUS_IP_ADDRESS);
         if(!strcmp(scratchString,ERRONEOUS_IP_ADDRESS))
            {
               iniServerAddressIsGood = false;
            }
         else
            {
               iniServerAddress.setAddress(scratchString);
               iniServerAddressIsGood = true;
               iniServerSocketNumber = iniFile.read_int((char *)"GENERAL",(char *)"INI_SERVER_SOCKET",DEFAULT_CONTROLLER_SOCKET);
               iniClient = new QTcpSocket;
               connect(iniClient,SIGNAL(connected()),this,SLOT(tcpConnectionMade()));
               connect(iniClient, SIGNAL(readyRead()), this, SLOT(readIniFromServer()));

               iniClient->connectToHost( iniServerAddress,iniServerSocketNumber);
               iniClient->waitForConnected();
            }
         free(scratchString);


      }



}

void LogStatusGui::iniFileRequestTime()
{

   if(!gotIni){
         requestIni();
      }

}

void LogStatusGui::tcpConnectionMade()
{

   // a connection has been made.  request an ini file
   // actually, fire off a timer that requests an ini file

   requestIni();
   iniRequestTimer->start(3000);


}

void LogStatusGui::requestIni()
{
   char	requestData[32];
   int len = sprintf(requestData,"REQUEST_INI");
   iniClient->write(requestData,len);

}

void	LogStatusGui::readIniFromServer()
{


   QByteArray inArray(iniClient->bytesAvailable(),0);

   if(!iniStarted)
      {
         tempIniFile = new QTemporaryFile;
         if(tempIniFile->open())
            {
               iniStarted = true;
            }
         else
            {
               // error should be here!
            }

      }


   int bytesRead = iniClient->read(inArray.data(),inArray.size());
   int endLoc = inArray.lastIndexOf("END_OF_DSLOG_INI_FILE");
   if(-1 != endLoc)
      {
        // inArray.chop(endLoc);
         bytesRead = endLoc;
         iniFinished = true;
      }
   tempIniFile->write(inArray.data(),bytesRead);
   if(iniFinished)
      {
         tempIniFile->flush();
         tempIniFile->close();
         iniFile.close_ini();
         char *theTempFile = strdup((char *)tempIniFile->fileName().toAscii().data());
         iniFile.open_ini(theTempFile);
         gotIni = true;
         iniRequestTimer->stop();

         iniFileReadTime();
         free(theTempFile);
         iniClient->abort();
      }


}

void LogStatusGui::iniFileReadTime()
{
   iniRequestTimer->stop();

   loggerCount = readLogStatusGuiIniFile();
   iniFile.close_ini();

   if(loggerCount)
      {
         buildGUI();
      }
   listenSocket = new QUdpSocket(this);
   listenSocket->bind(loggerPort);
   connect(listenSocket, SIGNAL(readyRead()), this, SLOT(getLoggerInput()));

}

LogStatusGui::~LogStatusGui()
{

}
void LogStatusGui::buildGUI()
{
   loggerTabs = new QTabWidget;
   for(int loggerN = 0; loggerN < loggerCount; loggerN++)
      {
         statusWidgets[loggerN] = new LoggerStatusWidget(&(theLoggingList[loggerN]));
         loggerTabs->addTab(statusWidgets[loggerN], (QString)theLoggingList[loggerN].dataType);
      }
   QVBoxLayout *mainLayout = new QVBoxLayout(this);
   mainLayout->addWidget(loggerTabs);

}

void LogStatusGui::getLoggerInput()
{

   while(listenSocket->hasPendingDatagrams())
     {

       QByteArray	buffer(listenSocket->pendingDatagramSize(),0);
       listenSocket->readDatagram(buffer.data(),buffer.size());

       int retcode = parseLGS(buffer.data());
       printf(" the in bugger %s\n",buffer.data());
       printf("retcode from parseLGS %d\n",retcode);
       if(!retcode)
          {
               printf("updateing stats\n");
             updateInputStats();
          }
       else
           {
                retcode = parseDSR(buffer.data());
                if(!retcode)
                   {

                      updateDestinationStats();
                   }
           }


    }

}

void LogStatusGui::updateInputStats()
{
   QString dateTimeString = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz");
    for(int inputN = 0; inputN < loggingCount; inputN++)
      {
         statusWidgets[inputN]->updateReceiveStatus(theStatusList[inputN], dateTimeString);
      }

}
void LogStatusGui::updateDestinationStats()
{

    for(int inputN = 0; inputN < loggingCount; inputN++)
      {
            statusWidgets[inputN]->updateDiskStatus(theLoggingList[inputN]);
      }

}

int LogStatusGui::parseLGS(char *theIndata)
{
   printf(" the indata %s\n",theIndata);
   char  dateString[256];
   char  timeString[256];
   char *searchPlace = theIndata;
   int items = sscanf(theIndata, "LGS %s %s LGR", dateString,timeString);
   float timeSince;
   int   bytes;
   int   msgs;
   int   errors;
   if(2 != items)
      {
         return -1;
      }
   for(int inputN = 0; inputN < loggingCount; inputN++)
      {
         char *inputLoc = strstr(searchPlace,theLoggingList[inputN].identifier);
         if(inputLoc)
            {
                  int statusItems = sscanf(inputLoc, theLoggingList[inputN].formatString, &timeSince,
                                           &msgs, &bytes,&errors);
                  if(4 == statusItems)
                     {
                        theStatusList[inputN].errorCount = errors;
                        theStatusList[inputN].timeSincedata = timeSince;
                        theStatusList[inputN].bytesRecieved = bytes;
                        theStatusList[inputN].msgsRecieved = msgs;
                     }
                  else
                     {
                        return -3;
                     }

            }
         else
            {
               return -2;
            }

      }
   return 0;

}

int LogStatusGui::readLogStatusGuiIniFile()
{
   rootDirectory = iniFile.read_string("GENERAL","ROOT_DIR",".");
   char  *scratchString;

   loggerPort = iniFile.read_int("GUI_THREAD", "TO_PORT", DEFAULT_LOGGER_PORT);

   for(int inputNumber = 0; inputNumber < MAX_N_OF_INPUTS; inputNumber++)
      {
         logging_descriptor_t    thisDescriptor;
         thisDescriptor.nOfDestinations = 0;
         char inputField[256];
         snprintf(inputField,256,"INPUT_%d",inputNumber+1);
         scratchString = iniFile.read_string(inputField,"DATA_TYPE",DEFAULT_NO_ENTRY);
         if(!strcmp(DEFAULT_NO_ENTRY,scratchString))
            { // no entry
               continue;
            }
         else
            {
               thisDescriptor.dataType = strdup( scratchString);
            }
         free(scratchString);
         scratchString = iniFile.read_string(inputField,"DATA_SOURCE",DEFAULT_NO_ENTRY);
         if(!strcmp(DEFAULT_NO_ENTRY,scratchString))
            { // no entry
               continue;
            }
         else
            {
               thisDescriptor.sourceName = strdup( scratchString);
            }
         free(scratchString);
         int scratchInteger = iniFile.read_int(inputField,"SOURCE_TYPE",NO_SOURCE_TYPE);
         if(NO_SOURCE_TYPE == scratchInteger)
            {
               continue;
            }
         else
            {
               thisDescriptor.sourceType = (source_type_t)scratchInteger;
            }
         scratchInteger = iniFile.read_int(inputField,"TIMESTAMP",TRUE);
         thisDescriptor.timeStampFlag = scratchInteger;
         if(UDP_ASCII == thisDescriptor.sourceType)
            {
               thisDescriptor.inSocketNumber = iniFile.read_int(inputField,"IN_SOCKET",NO_IN_SOCKET);

            }
         sprintf(thisDescriptor.identifier, "S%d",loggingCount+1);
         sprintf(thisDescriptor.formatString, "S%d",loggingCount + 1);
         strcat(thisDescriptor.formatString," %f %d %d %d");
         thisDescriptor.threadNumber = LOG_THREAD_OFFSET + loggingCount;
         int temporaryDestinationCount = 0;

         for(int destinationCount = 0; destinationCount < MAX_N_OF_DESTINATIONS; destinationCount++)
            {
               char    inputField2[256];
               snprintf(inputField2,256,"DESTINATION_%d",destinationCount+1);
               scratchInteger = iniFile.read_int(inputField,inputField2,(int)NOT_A_DESTINATION);

               if(scratchInteger >=  ((int)NOT_A_DESTINATION) ||  (scratchInteger < (int)UDP_SOCKET))
                  {
                     continue;
                  }
               thisDescriptor.destinations[temporaryDestinationCount].destinationType = (destinationType_t)scratchInteger;

               if(DISK == thisDescriptor.destinations[temporaryDestinationCount].destinationType)
                  {
                       temporaryDestinationCount++;
                       thisDescriptor.nOfDestinations++;
                  }


            }
         theLoggingList[loggingCount] = thisDescriptor;
         loggingCount++;


      }
   return loggingCount;

}
int LogStatusGui::parseDSR(char *theIndata)
{
    if(strncmp(theIndata,"DSR",3))
        {
            return -1;
        }
    else
        {
            char *searchPlace = theIndata;

            for(int inputN = 0; inputN < loggingCount; inputN++)
               {
                  char *inputLoc = strstr(searchPlace,theLoggingList[inputN].identifier);
                  if(inputLoc)
                     {
                          int nOfDestinations;
                          int statusItems = sscanf(inputLoc, "%*s %d",&nOfDestinations);
                          if(!statusItems)
                              {
                                  return -1;
                              }
                          else
                              {
                                  for(int destN = 0; destN < nOfDestinations; destN++)
                                      {
                                          int errors = 0;
                                          double perc  = 0.0;
                                          unsigned long long bytes;
                                          int nDestStat = sscanf(inputLoc, "%*s %*s %d %lf %lld",&errors,&perc, &bytes);

                                          if(nDestStat == 3)
                                              {

                                                  theLoggingList[inputN].destinations[destN].loggingDestination.freeBytes = bytes;
                                                  theLoggingList[inputN].destinations[destN].loggingDestination.percentage = perc;
                                                  theLoggingList[inputN].destinations[destN].loggingDestination.errors = errors;


                                              }

                                      }
                              }
                      }
                }
            return 0;


        }

}

