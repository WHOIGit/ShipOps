@echo off

color b0
cls

echo This program will:
echo -Make sure all of the current cruise files have been backed up to D:\Data\winch
echo -Create a new directory in C:\\\\\\LogFiles\PreviousCruises\
echo -Clean out C:\\\\\\Logfiles\ and move the files to C:\\\\\\\PreviousCruises\(previouscruise)\
echo -Create a new directory in D:\Data\winch\archive\
echo -Clean out D:\\winch\ and move the files to permanent archive in D:\\winch\archive\(previouscruise)\
echo -
echo Don't forget to delete really old cruises from C:\\\\\\\PreviousCruises\ before it gets full
echo - 
echo We are about to archive the winch files from cruise %1 !!!
echo If this is not correct, hit ctrl-c now.  Otherwise
pause
cls

echo Here I go, archiving...
robocopy "C:\Documents and Settings\science\My Documents\3PS\WinchMonitor\LogFiles" "D:\Data\winch"
mkdir "C:\Documents and Settings\science\My Documents\3PS\WinchMonitor\LogFiles\PreviousCruises\%1"
move /y "C:\Documents and Settings\science\My Documents\3PS\WinchMonitor\LogFiles\*" "C:\Documents and Settings\science\My Documents\3PS\WinchMonitor\LogFiles\PreviousCruises\%1"
mkdir "D:\Data\winch\archive\%1"
move /y "D:\Data\winch\*" "D:\Data\winch\archive\%1"

echo I'm done!
