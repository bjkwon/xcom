#include "graffy.h" // this should come before the rest because of wxx820
#include <windows.h> 
#include <stdio.h>
#include "consts.h"
#ifndef SIGPROC
#include "sigproc.h"
#endif
#include "audfret.h"
#include "audstr.h"
#include "showvar.h"
#include "histDlg.h"
#include "xcom.h"

extern double block;
extern HANDLE hStdin, hStdout; 
extern CShowvarDlg mShowDlg;
extern CHistDlg mHistDlg;
extern xcom mainSpace;

void closeXcom(const char *AppPath);

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
	HMODULE h = HMODULE_THIS;
	char fullmoduleName[MAX_PATH], AppPath[MAX_PATH], drive[16], dir[256];
 	GetModuleFileName(h, fullmoduleName, MAX_PATH);
 	_splitpath(fullmoduleName, drive, dir, NULL, NULL);
 	sprintf (AppPath, "%s%s", drive, dir);

  switch( fdwCtrlType ) 
  { 
    // This works properly only when ReadConsole or ReadConsoleInput or any other low level functions are used. Not for highlevel such as getchar
    case CTRL_C_EVENT:
//      printf( "Ctrl-C pressed. Exiting...\n" );
// 	  LOGHISTORY("//\t<<CTRL-C Pressed>>")
      return( TRUE );
 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
		closeXcom(AppPath);
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

#define INRECORD_SIZE 16
#define CONTROLKEY  (in[k].Event.KeyEvent.wVirtualKeyCode==VK_CONTROL)

int paste(char *buf)
{ // buf is the same buf in getinput
	if (!IsClipboardFormatAvailable(CF_TEXT)) 
	{
		MessageBox(NULL, "Clipboard not available", 0, 0);
		return 0;
	}
	int res = OpenClipboard(NULL);
	HANDLE hglb = GetClipboardData(CF_TEXT); 
	char *buff = (char*)GlobalLock(hglb);
	DWORD len = (DWORD)strlen(buff);
	DWORD dw;
	res = WriteConsole(hStdout, buff, len, &dw, NULL);
	strcpy(buf, buff);
	GlobalUnlock((HGLOBAL)buff);
	res = CloseClipboard();
	return (int)len;
}

bool check_CHAR_INFO_attri(CHAR_INFO c)
{
	if (c.Attributes==0xcdcd || c.Attributes==0)  return false;
	else	return true;
}

size_t CHAR_INFO2char(CHAR_INFO *chbuf, COORD sz, char* out)
{ // Assume: out was alloc'ed prior to this call
	size_t count(0);
	for (int m=0; m<sz.Y; m++)
	{
		for (int n=0; n<sz.X; n++,count++)
		{
			if (!check_CHAR_INFO_attri(chbuf[n+sz.X*m])) continue;
			out[count] = chbuf[n+sz.X*m].Char.AsciiChar;
			if (n==sz.X-1 && out[count]==' ') // if the last element is blank, put a CR
				out[count] = '\n';
		}
		char *pt = trimRight(out, " \r\n\t");
		count = strlen(pt);
	}
	return count;
}


size_t ReadThisLine(string &linebuf, HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO coninfo0, SHORT thisline, size_t promptoffset)
{ 
	bool loop(true);
	int out;
	size_t res1;
	CHAR_INFO *chs;
	SMALL_RECT srct;
	COORD      now, sz;
	srct.Top = thisline; // coninfo0.srWindow.Top;
	srct.Bottom = thisline;// coninfo0.srWindow.Bottom;
	srct.Left = coninfo0.srWindow.Left + (SHORT)promptoffset;
	srct.Right = coninfo0.srWindow.Right;
	now.X = 0; now.Y = 0;//coninfo0.dwCursorPosition.Y;
	sz.X = coninfo0.dwSize.X-(SHORT)promptoffset;  sz.Y=1;

	chs = new CHAR_INFO[sz.X*sz.Y];
	int res = ReadConsoleOutput (hStdout, chs, sz, now, &srct);
	char *buf = new char[sz.X*sz.Y+1];
	memset(buf,0,sz.X*sz.Y+1);
	res1 = CHAR_INFO2char(chs, sz, buf); 
	delete[] chs;
	linebuf = buf;
	out = (int)strlen(buf); 
	delete[] buf;
	return out;
}


size_t ReadTheseLines(char *readbuffer, DWORD &num, HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO coninfo0, CONSOLE_SCREEN_BUFFER_INFO coninfo)
{
	string linebuf;
	size_t res;
	res = ReadThisLine(linebuf, hStdout, coninfo0, coninfo0.dwCursorPosition.Y, mainSpace.comPrompt.length());
	strcpy(readbuffer, linebuf.c_str());
	num = (DWORD)res;
	//check when the line is continuing down to the next line.
	for (int k=coninfo0.dwCursorPosition.Y+1; k<coninfo.dwCursorPosition.Y+1; k++)
	{
		res = ReadThisLine(linebuf, hStdout, coninfo0, k, 0);
		if (res>0)
		{
			strcat(readbuffer, "\n");
			strcat(readbuffer, linebuf.c_str());
			num += (DWORD)res+1;
		}
	}
	return num;
}

void xcom::getinput(char* readbuffer)
{
	string linebuf;
	readbuffer[0]=0;
	char buf1[4096]={0}, buf[4096]={0};
	DWORD nRemove(0), dw, dw2;
	size_t cumError(0), ures;
	vector<string> tar;
	int res, res1;
	INPUT_RECORD in[INRECORD_SIZE];
	CHAR read;
	SHORT delta;
	bool showscreen(true);
	size_t histOffset;
	DWORD num(0); // total count of chracters typed in
	size_t offset; // how many characters shifted left from the last char
	size_t off; // how much shift of the current cursor is from the command prompt
	int line, code;
	CONSOLE_SCREEN_BUFFER_INFO coninfo0, coninfo;
	GetConsoleScreenBufferInfo(hStdout, &coninfo0);
	bool loop(true), returndown(false);
	bool replacemode(false);
	CONSOLE_CURSOR_INFO concurinf;
	offset = histOffset = buf[0] = 0;
	bool controlkeydown(false);
	while (loop)
	{		
		if (ReadConsoleInput(hStdin, in, INRECORD_SIZE, &dw)==0)	{ 
			dw = GetLastError(); 	GetLastErrorStr(buf);
			sprintf(buf1, "code=%d", dw);	MessageBox(GetConsoleWindow(), buf, buf1, 0); 
			if (cumError++==2) 	loop=false;
			else	continue; 
		}
		//dw can be greater than one when 1) control-v is pressed, or 2) debug command string is dispatched from OnNotify of debugDlg, or 3) maybe in other occassions
		if (dw>1) 
			showscreen = !isdebugcommand(in, dw);
		for (UINT k=0; k<dw; k++)
		{
			if (CONTROLKEY) 
				controlkeydown = !(in[k].Event.KeyEvent.bKeyDown==0);
			code = in[k].Event.KeyEvent.uChar.AsciiChar;
			if (in[k].Event.KeyEvent.wVirtualKeyCode==0x56 && (in[k].Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED+RIGHT_CTRL_PRESSED))
				 && in[k].Event.KeyEvent.bKeyDown ) //control-V 
			{
				//This works only for a Debug build. Release build will not be caught by this if. 9/21/2017 bjk
				num += paste(buf+num);
				//Right now, paste generates a string with an unnecesary trailing character \x16 --- don't know why 9/21/2017
			}
			//Pasting by right mouse click (or right mouse click to invoke the Menu and choose "Paste") will only paste the first line, even if multiple lines were copied... fix it 9/21/2017 bjk
			//Control-V works (Debug and Release mode work differently but anyhow works---I still need to check if it works in regular console (I am running on Clink console) 9/21/2017 
			if ( in[k].Event.KeyEvent.bKeyDown)
			{
				read = in[k].Event.KeyEvent.uChar.AsciiChar;
				GetConsoleScreenBufferInfo(hStdout, &coninfo);
				off = (coninfo.dwCursorPosition.Y-coninfo0.dwCursorPosition.Y)*coninfo0.dwMaximumWindowSize.Y + coninfo.dwCursorPosition.X-coninfo0.dwCursorPosition.X;
				COORD      now;
				if (num==0) 
				{
					switch(in[k].Event.KeyEvent.wVirtualKeyCode)
					{
					case VK_BACK:
					case VK_RETURN:
					case VK_LEFT:
					case VK_RIGHT:
					case VK_UP:
					case VK_DOWN:
					case VK_HOME:
					case VK_END:
					case VK_INSERT:
					case VK_DELETE:
						// pressing enter from history window... no characters registered yet
						num = (DWORD)ReadTheseLines(buf, num, hStdout, coninfo0, coninfo);
					}
				}
				switch(in[k].Event.KeyEvent.wVirtualKeyCode)
				{ 
				case VK_RETURN:
					// Is this actual return or enter key pressing, or transport of a block?
					// For pasting, i.e., control-v---> keep in the loop; otherwise, get out of the loop
					if (!controlkeydown) 
						loop=false;
					// if characters were already typed in or pasted on the console lines, they were registered in buf and num represents the count of them
					// if characters were lightly copied (pressing enter from the history window), they were not registered in num, so num is zero
//					buf[num++] = '\n'; //Without this line, when multiple lines are pasted with Control-V, Debug version will work, but Release will not (no line separation..just back to back and error will occur)
//					strcpy(readbuffer, buf);
					if (showscreen)
						num = (DWORD)ReadTheseLines(buf, num, hStdout, coninfo0, coninfo);
					else // for now (9/21/2017) this is only for debug messages (#step #cont ....)
						buf[num++] = '\n'; 
					strcpy(readbuffer, buf);
					buf1[0] = 0, histOffset = 0; 
					WriteConsole (hStdout, "\r\n", 2, &dw2, NULL); // this is the real "action" of pressing the return/enter key, exiting from the loop and return out of getinput()
					break;
				case VK_CONTROL:
					break;
				case VK_BACK:
					//don't assume that num==0 means nothing--it could be a light copy (entering from history dlg)
					if (!(num-offset))  break;
					buf1[0] = '\b';
					memcpy(buf1+1, buf+num-offset, offset+1);
					memcpy(buf+num---offset-1, buf1+1, offset+1);
					if (showscreen) WriteConsole (hStdout, buf1, (DWORD)strlen(buf1+1)+3, &dw2, NULL); // normally +2 is OK for backspace, but putting more char to cover the case where the line number is reduced
				case VK_LEFT:
					if (in[k].Event.KeyEvent.wVirtualKeyCode==VK_LEFT && !(num-offset)) break;
					if (in[k].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || in[k].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
						delta = (SHORT)( strlen(buf) - ctrlshiftleft(buf, (DWORD)offset) - offset );
					else
						delta = 1;
					for (int pp=0; pp<delta; pp++)
					{
						if ( (coninfo.dwCursorPosition.Y-coninfo0.dwCursorPosition.Y) && !coninfo.dwCursorPosition.X)
						{
							coninfo.dwCursorPosition.X = coninfo0.dwMaximumWindowSize.X-1; 
							coninfo.dwCursorPosition.Y--;
						}
						else
							coninfo.dwCursorPosition.X--;
					}
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					if (in[k].Event.KeyEvent.wVirtualKeyCode==VK_LEFT) offset += delta;
					break;
				case VK_RIGHT:
					if (in[k].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || in[k].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
						delta = (SHORT)(ctrlshiftright(buf, (DWORD)offset) - off);
					else
						delta = 1;
					//first determine if current location is inside the range of num, if not break
					for (int pp=0; pp<delta; pp++)
						if (off<num)
						{
							coninfo.dwCursorPosition.X++;
							if (coninfo.dwCursorPosition.X>coninfo.dwMaximumWindowSize.X)  coninfo.dwMaximumWindowSize.Y++, coninfo.dwCursorPosition.X++;
							// if the current cursor is on the bottom, the whole screen should be scrolled---do this later.
							SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
							offset--;
						}
					break;
				case VK_SHIFT:
				case VK_MENU:
				case VK_PAUSE:
				case VK_CAPITAL:
				case VK_HANGUL:
				case 0x16:
				case 0x17:
				case 0x18:
				case 0x19:
				case 0x1c:
				case VK_SNAPSHOT:
					break;
				case VK_ESCAPE:
					SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
					if (num>0)
					{
						memset(buf, ' ', num);
						memset(buf+num, 0, 1);
					}
					else
					{
						memset(buf, ' ', coninfo0.dwMaximumWindowSize.X);
						memset(buf+coninfo0.dwMaximumWindowSize.X, 0, 1);
					}
					res1 = coninfo.dwCursorPosition.Y-coninfo0.dwCursorPosition.Y;
					for (res=0; res<=res1; res++)
					{
						DWORD res2;
						if (num==0)
							res2 = (DWORD)ReadThisLine(linebuf, hStdout, coninfo0, res, res==0? mainSpace.comPrompt.size(): 0 );
						else
							res2 = num;
						WriteConsole (hStdout, buf, min(res2,(DWORD)coninfo0.dwMaximumWindowSize.X), &dw2, NULL);
						if (res<res1) 
						{
							now.X=0; now.Y=coninfo0.dwCursorPosition.Y+res+1;
							SetConsoleCursorPosition (hStdout, now);
						}
					}
					SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
					histOffset=offset=0;
					num=0;
					break;

				case VK_UP:
				case VK_DOWN:
					if (in[k].Event.KeyEvent.wVirtualKeyCode==VK_UP)
					{
						if (comid==histOffset) break;
						histOffset++;
						if (history.size()>comid-histOffset+1)
							nRemove=(DWORD)history[comid-histOffset+1].size();
						else
							nRemove = num;
					}
					else
					{
						if (histOffset==0) break;
						histOffset--;
						nRemove=(DWORD)history[comid-histOffset-1].size();
					}
					memset(buf, 0, nRemove);
					if (histOffset==0) buf[0]=0;
					else strcpy(buf, history[comid-histOffset].c_str());
					SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
					num = (DWORD)strlen(buf);
					if (showscreen) WriteConsole (hStdout, buf, max(nRemove, num), &dw2, NULL);
					coninfo.dwCursorPosition.X = coninfo0.dwCursorPosition.X + (SHORT)num;
					coninfo.dwCursorPosition.Y = coninfo0.dwCursorPosition.Y;
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					break;
				case VK_HOME:
					coninfo = coninfo0;
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					offset = num;
					break;
				case VK_END:
					line=0;
					ures = ReadThisLine(linebuf, hStdout, coninfo0, coninfo.dwCursorPosition.Y, mainSpace.comPrompt.size()) + mainSpace.comPrompt.size();
					while (ures==coninfo.dwMaximumWindowSize.X)
					{
						line++;
						ures = ReadThisLine(linebuf, hStdout, coninfo0, coninfo.dwCursorPosition.Y+line, 0);
					}
					coninfo.dwCursorPosition.X = mod((SHORT)ures, coninfo.dwMaximumWindowSize.X);
					coninfo.dwCursorPosition.Y += line;
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					offset = 0;
					break;
				case VK_INSERT:
					replacemode = !replacemode;
					concurinf.bVisible = 1;
					if (!replacemode)	concurinf.dwSize = 70;
					else concurinf.dwSize = 25;
					SetConsoleCursorInfo (hStdout, &concurinf);
					break;
				case VK_DELETE:
					if (!offset) break;
					memcpy(buf1, buf+num-offset+1, offset);
					memcpy(buf+num---offset--, buf1, offset+1);
					if (showscreen) WriteConsole (hStdout, buf1, (DWORD)strlen(buf1)+1, &dw2, NULL);
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					break;
				default:
					if (!read || (in[k].Event.KeyEvent.wVirtualKeyCode>=VK_F1 && in[k].Event.KeyEvent.wVirtualKeyCode<=VK_F24) ) break;
					//default is replace mode (not insert mode)
					// if cursor is in the middle
					if (showscreen) res = WriteConsole (hStdout, &read, 1, &dw2, NULL);
					if (replacemode && offset>0)
						buf[num-offset--] = read;
					else 
					{
						if (offset)
						{
							GetConsoleScreenBufferInfo(hStdout, &coninfo);
							strcpy(buf1, &buf[num-offset]);
							buf[num++-offset] = read;
							strcpy(buf+num-offset, buf1);
							if (showscreen) res = WriteConsole (hStdout, &buf1, (DWORD)strlen(buf1), &dw2, NULL);
							GetConsoleScreenBufferInfo(hStdout, &coninfo);
							coninfo.dwCursorPosition.X -= (SHORT)dw2;
							if (coninfo.dwCursorPosition.X<0) //remainder string continues across line break
							{
								coninfo.dwCursorPosition.Y--;
								coninfo.dwCursorPosition.X += coninfo.dwMaximumWindowSize.X;
							}
							if (showscreen) SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
						}
						else
						{
							buf[num++-offset] = read;
						}
					}
					break;

				}
				if (!loop) k=dw+1, strcat(buf, "\n");
			}
		}
	}
	if (!debugcommand(readbuffer))
	{
		// if a block input is given (i.e., control-V), each line is separately saved/logged.
		size_t count = str2vect(tar, readbuffer, "\r\n");
		LogHistory(tar);
		mHistDlg.AppendHist(tar);
		for (size_t k=0; k<tar.size(); k++)
		{
			history.push_back(tar[k].c_str());
			comid++;
		}
	}
	else
		SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
}
	
