#include <windows.h> 
#include <stdio.h>
#include "consts.h"
#ifndef SIGPROC
#include "sigproc.h"
#endif
extern CAstSig main;
extern double block;

void closeXcom(SYSTEMTIME *lt, const char *AppPath);

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
	HMODULE h = HMODULE_THIS;
	char fullmoduleName[MAX_PATH], AppPath[MAX_PATH], drive[16], dir[256];
 	GetModuleFileName(h, fullmoduleName, MAX_PATH);
 	_splitpath(fullmoduleName, drive, dir, NULL, NULL);
 	sprintf (AppPath, "%s%s", drive, dir);

	string pathnotapppath;
  switch( fdwCtrlType ) 
  { 
    // This works properly only when ReadConsole or ReadConsoleInput or any other low level functions are used. Not for highlevel such as getchar
    case CTRL_C_EVENT: 
//      printf( "Ctrl-C pressed. Exiting...\n" );
// 	  LOGHISTORY("//\t<<CTRL-C Pressed>>")
      return( TRUE );
 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
		SYSTEMTIME lt;
		GetLocalTime(&lt);	
		closeXcom(&lt, AppPath);
		printf( "Ctrl-Close event. Exiting..\n" );
      return( TRUE ); 
 
    // Pass other signals to the next handler. 
    case CTRL_BREAK_EVENT: 
//	  LOGHISTORY("//\t<<CTRL-Break Pressed>>")
      Beep( 1900, 200 ); 
      printf( "Ctrl-Break event\n\n" );
      return FALSE; 
 
    case CTRL_LOGOFF_EVENT: 
      Beep( 1000, 200 ); 
      printf( "Ctrl-Logoff event\n\n" );
      return FALSE; 
 
    case CTRL_SHUTDOWN_EVENT: 
      Beep( 750, 500 ); 
      printf( "Ctrl-Shutdown event\n\n" );
      return FALSE; 
 
    default: 
      return FALSE; 
  } 
} 