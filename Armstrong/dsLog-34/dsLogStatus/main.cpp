#include <QtGui>
#include "LogStatusGui.h"


LogStatusGui *statusGui;

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);

   char	*enteredIniFileName;
   if(argc > 1)
   {
       enteredIniFileName = strdup((char *)argv[1]);
   }
   else
   {

       enteredIniFileName = strdup("./logStatus.ini");

   }
    statusGui = new LogStatusGui(enteredIniFileName);
;
    char	windowTitle[256];
    sprintf(windowTitle,"dsLogStatus V %s compiled %s",PROGRAM_VERSION, __DATE__);
    statusGui->show();

    return app.exec();
}
