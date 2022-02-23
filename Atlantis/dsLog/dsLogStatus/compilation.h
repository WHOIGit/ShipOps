/* this module is part of dsnav, a piece of software intended to communicate with n456, a linux based command processor
	that communicates with custom digital ranger boards

	the initial configuration of this software was developed on April 10, 2008, by Jonathan Howland 
	Woods Hole Oceanographic Institution
	Deep Submergence Laboratory

	508-289-2653
	jhowland@whoi.edu

  this module is intended to allow cross-platform compilation (Linux and win32)

*/
#ifndef COMPILATION_H
#define COMPILATION_H

#define _STDCALL __stdcall

#if defined(_WIN32) || defined(WIN32) 
#define WINDOWS
#include <windows.h>
#include <direct.h>
#include <process.h>
#else
/*  for Linux */
#include <arpa/inet.h>
#include <sys/vfs.h>
#define __stdcall
#define _stricmp strcmp
#define _strnicmp strncmp
#define _strdup   strdup
#define _timeb		timeb
#define _ftime	ftime
#endif

#endif
 
