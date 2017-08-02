#include "graffy.h" // this should come before the rest because of wxx820
#include <process.h>
#include "showvar.h"
#include "resource1.h"
#include "audfret.h"
#include "xcom.h"
#include "histDlg.h"
#include "TabCtrl.h"

extern xcom mainSpace;

extern CDebugDlg mDebug; // delete?
extern map<string, CDebugDlg*> dbmap;

extern CWndDlg wnd;
extern CShowvarDlg mShowDlg;
extern char udfpath[4096];
extern void* soundplayPt;
extern double playbackblock;

extern HANDLE hEvent;
extern CHistDlg mHistDlg;
extern CDebugBaseDlg debugBase;
extern CTabControl mTab;

#define CELLVIEW 87893
#define ID_HELP_SYSMENU		33758

#define WM__NEWDEBUGDLG	WM_APP+0x2000

extern vector<UINT> exc; //temp

FILE *fp;

vector<CWndDlg*> cellviewdlg;
CFileDlg fileDlg;

void nonnulintervals(CSignal *psig, string &out, bool unit, bool clearout=false);
HWND CreateTT(HINSTANCE hInst, HWND hParent, RECT rt, char *string, int maxwidth=400);
BOOL CtrlHandler( DWORD fdwCtrlType );

BOOL CALLBACK vectorsheetDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL AboutDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK debugDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DebugBaseProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


#define RMSDB(BUF,FORMAT1,FORMAT2,X) { double rms;	if ((rms=X)==-1.*std::numeric_limits<double>::infinity()) strcpy(BUF, FORMAT1); else sprintf(BUF, FORMAT2, rms); }

AUDFRET_EXP void makewmstr(map<unsigned int, string> &wmstr);

uintptr_t hDebugThread(NULL);
uintptr_t hDebugThread2(NULL);

unsigned int WINAPI debugThread2 (PVOID var) ;

unsigned int WINAPI debugThread (PVOID var) 
{
	char fname[256];
	CDebugDlg *newdbdlg;
	vector<CDebugDlg *> dlgVec;
	_splitpath((char*)var, NULL, NULL, fname, NULL);
	// if the UDF name is not on the map, create a new map element
	if (dbmap.find(fname)==dbmap.end())
	{
		newdbdlg = new CDebugDlg;
		CREATE_CDebugDlg transfer;
		transfer.dbDlg = newdbdlg;
		strcpy(transfer.fullfilename, (char*)var);
		if (newdbdlg->hDlg = CreateDialogParam (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DEBUG), mShowDlg.hDlg, (DLGPROC)debugDlgProc, (LPARAM)&transfer))
		{
			dlgVec.push_back(newdbdlg);
		}
	}
	//FILE *fp = fopen("c:\\temp\\rec", "at");
	//fprintf(fp, "hList = %4x\n",hList);
	//fclose(fp);

	MSG         msg ;
	HACCEL hAccDebug = LoadAccelerators (GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ACCEL_DEBUG));
	HWND foreWnd;
	while (GetMessage (&msg, NULL, 0, 0))
	{ 
		spyWM(msg.hwnd, msg.message, msg.wParam, msg.lParam, "c:\\temp\\rec", exc, "[debugThread-msg loop]");
		if (msg.message==WM__ENDTHREAD) 
			break;
		else if (msg.message==WM__NEWDEBUGDLG)
		{
			_splitpath((char*)msg.wParam, NULL, NULL, fname, NULL);
			if (dbmap.find(fname)==dbmap.end())
			{
				newdbdlg = new CDebugDlg;
				CREATE_CDebugDlg transfer;
				transfer.dbDlg = newdbdlg;
				strcpy(transfer.fullfilename, (char*)msg.wParam);
				if (newdbdlg->hDlg = CreateDialogParam (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DEBUG), mShowDlg.hDlg, (DLGPROC)debugDlgProc, (LPARAM)&transfer))
					dlgVec.push_back(newdbdlg);
			}
			else
				dbmap[fname]->ShowWindow(SW_SHOW);
		}
		else
		{
			int res(0);
			foreWnd = GetForegroundWindow ();
			for (map<string, CDebugDlg*>::iterator it=dbmap.begin(); it!=dbmap.end(); it++)
			{
				if (it->second->hDlg == foreWnd)
				{	res = TranslateAccelerator (foreWnd, hAccDebug, &msg); break; }
			}
			if (!res && !IsDialogMessage(msg.hwnd, &msg))
			{
				TranslateMessage (&msg) ;
				DispatchMessage (&msg) ;
			}
		}
	}
	//cleanup
	for (vector<CDebugDlg *>::iterator it=dlgVec.begin(); it!=dlgVec.end(); it++)
	{
		DeleteObject((*it)->eFont);
		(*it)->DestroyWindow();
		delete *it;
	}
	return 0;
}

bool isWin7()
{
	char block[4096];
	float val;
	getVersionString("cmd.exe", block, sizeof(block));
	block[4]=0;
	sscanf(block+1, "%f", &val);
	if (val<6.2) return true;
	else return false;
}

LRESULT CALLBACK HookProc2(int code, WPARAM wParam, LPARAM lParam)
{
	static	char varname[256];
	switch(code)
	{
	case HC_ACTION:
		SetEvent(hEvent);
	break;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	static	char varname[256];
	CSignals *tp;
	switch(code)
	{
	case HC_ACTION:
		{
			CSignals gcf;
			MSG *pmsg = (MSG*)lParam;
			if ((pmsg->message>=WM_NCLBUTTONDOWN && pmsg->message<=WM_NCMBUTTONDBLCLK) || ((pmsg->message>=WM_LBUTTONDOWN && pmsg->message<=WM_MBUTTONDBLCLK)) )
			{
				if (GetFigID(pmsg->hwnd, gcf) || GetFigID(GetParent(pmsg->hwnd), gcf))
				{
					mShowDlg.Fillup();
				}
			}
			else if (pmsg->message==WM__VAR_CHANGED)
			{
				if (!(tp = (CSignals *)pmsg->lParam))
				{
					CAstSig *pabteg = mainSpace.vecast.back();
					tp = pabteg->GetSig((char*)pmsg->wParam);
				}
				if (tp && (tp->GetType()==CSIG_AUDIO || tp->GetType()==CSIG_VECTOR))
				{
					HANDLE fig = FindFigure((char*)pmsg->wParam);
					CFigure *cfig = static_cast<CFigure *>(fig);
					HANDLE ax = cfig->ax.front();
					CAxis *cax = static_cast<CAxis *>(ax);
					SetRange(ax, 'x', 0., -1.);  // $$2816xlim 4/25/2017
					double xlim[2];
					memcpy(xlim,cax->xlim, 2*sizeof(cax->xlim[0]));
					while(cfig->ax.front()->m_ln.size()>0)	
						deleteObj(cfig->ax.front()->m_ln.front());
					if (tp->GetType()==CSIG_VECTOR)
						PlotCSignals(ax, *tp, 0xff0000, '*', LineStyle_noline); // plot for VECTOR, no line and marker is * --> change if you'd like
					else
						PlotCSignals(ax, *tp, 0xff0000);
					if (tp->next)
						PlotCSignals(ax, *tp->next, RGB(200,0,50));
					mShowDlg.Fillup();
				}
				else // if the variable is no longer unavable, audio or vector, exit the thread (and delete the fig window)
					PostThreadMessage (GetCurrentThreadId(), WM_QUIT, 0, 0);
				break;
			}
			else if (pmsg->message==WM_QUIT)
			{
				CSignals var;
				int res  = GetFigID(pmsg->hwnd, var);
				HWND hp=GetParent(pmsg->hwnd);
				strcpy(varname, var.string().c_str());
				PostMessage(hp, WM__PLOTDLG_DESTROYED, (WPARAM)varname, 0);
			}
		}
		break;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

BOOL CALLBACK showvarDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
//	spyWM(hDlg, umsg, wParam, lParam, "c:\\temp\\rec", exc, "[showvarDlg]");

	CShowvarDlg *cvDlg;
	static map<unsigned int, string> wmstr;

	bool alreadymade(false);
	for (vector<CWndDlg*>::iterator it=cellviewdlg.begin(); it!=cellviewdlg.end(); it++)
	{
		if ( (*it)->hDlg == hDlg) {	cvDlg = (CShowvarDlg *)*it; alreadymade=true; 	break;}
	}
	if (!alreadymade) cvDlg = &mShowDlg;

	switch (umsg)
	{
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_ACTIVATE:
		SetForegroundWindow (hDlg);
		if (cvDlg->changed) 
			cvDlg->Fillup();
		break;
	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, cvDlg->OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, cvDlg->OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, cvDlg->OnClose);
	chHANDLE_DLGMSG (hDlg, WM_SHOWWINDOW, cvDlg->OnShowWindow);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, cvDlg->OnCommand);
	chHANDLE_DLGMSG (hDlg, WM_SYSCOMMAND, cvDlg->OnSysCommand);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE_FIG, cvDlg->OnCloseFig);
	chHANDLE_DLGMSG (hDlg, WM__SOUND_EVENT, cvDlg->OnSoundEvent);

	case WM__PLOTDLG_CREATED: // Posted by plotThread when the plot dlg is displayed.
		//wParam: var name. If NULL, that means generic figure window, not attached to a variable
		//lParam: pointer to the GRAFWNDDLGSTRUCT structure
		cvDlg->OnPlotDlgCreated((char*)wParam, (GRAFWNDDLGSTRUCT*)lParam);
		break;

	case WM__PLOTDLG_DESTROYED: // Posted by plotThread when the plot dlg is displayed.
		cvDlg->OnPlotDlgDestroyed((char*)wParam);
		break;

	case WM__VAR_CHANGED: // Posted by plotThread when the plot dlg is displayed.
		cvDlg->OnVarChanged((char*)wParam, (CSignals*)lParam);
		break;

	case WM_APP+WOM_OPEN: // from waves.cpp in sigproc
	case WM_APP+WOM_CLOSE: // from waves.cpp in sigproc
		EnableDlgItem(hDlg, IDC_STOP, umsg==WM_APP+WOM_OPEN ? 1:0);
		break;

	//	chHANDLE_DLGMSG (hDlg, WM_NOTIFY, cvDlg->OnNotify);
	case WM_NOTIFY: //This cannot be through msg cracker... why? NM_CUSTOMDRAW inside WM_NOTIFY needs to call SetWindowLongPtr to have an effect. Msg cracker uses a vanila SetWindowLongPtr for everything without special needs. 5/1/2016 bjkwon
		cvDlg->OnNotify(hDlg, (int)(wParam), lParam);
		break;

	case WM_GCF_UPDATED:
	{
		CSignals gcf;
		GetFigID((HANDLE)wParam, gcf);
		cvDlg->pcast->SetVar("gcf", gcf);
		cvDlg->OnCommand(IDC_REFRESH, NULL, 0);
		break;
	}
	//case WM_SYSCOMMAND:

	case WM__DEBUG:
		cvDlg->OnDebug(*(DEBUG_STATUS*)wParam, (CAstSig *)lParam, umsg-(WM_APP+4000));
		break;

	case WM_DEBUG_CLOSE:
		if (dbmap.find((char*)wParam)!=dbmap.end())
		{
			map<string, CDebugDlg*>::iterator it = dbmap.find((char*)wParam);
			delete it->second;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

CShowvarDlg::CShowvarDlg(void)
:changed(false), win7(false)
{

}

CShowvarDlg::~CShowvarDlg(void)
{

}

void CShowvarDlg::OnVarChanged(char *varname, CSignals *sig)
{
	if (strlen(varname) && plotDlgThread.find(varname) != plotDlgThread.end())
		PostThreadMessage(plotDlgThread[varname], WM__VAR_CHANGED, (WPARAM)varname, (LPARAM)sig); // this is processed in HookProc
}

void CShowvarDlg::OnDebug(DEBUG_STATUS status, CAstSig *debugAstSig, int entry)
{
	map<string, CDebugDlg*>::iterator it;
	int res, line;
	CAstSig *lp;
	static	string fullname;

	switch(status)
	{
	case entering: 
		//inspect how many layers of ast to add to vecast
		res=0;
		for (CAstSig *tp=debugAstSig; tp; tp=tp->dad)
			res++;
		for (CAstSig *tp=mainSpace.vecast.front(); res>1; res--)
		{
			tp = tp->son;
			if (mainSpace.vecast.back()!=tp) 
			{
				mainSpace.vecast.push_back(tp);
				SendDlgItemMessage(IDC_DEBUGSCOPE, CB_ADDSTRING, 0, (LPARAM)tp->GetScript().c_str());
			}
		}
	case progress:
		SendDlgItemMessage(IDC_DEBUGSCOPE, CB_SETCURSEL, SendDlgItemMessage(IDC_DEBUGSCOPE, CB_GETCOUNT)-1);
		if ((it = dbmap.find(debugAstSig->GetScript()))==dbmap.end()) 
		{ // only when debug Dlg is opened by a function with a lower or higher scope
		  // otherwise debug Dlg's are opened by a button click in the show Dlg, so this can't happen.
			debugAstSig->OpenFileInPath (debugAstSig->GetScript().c_str(), "aux", fullname);

			res = ::SendMessage(mTab.hTab, TCM_GETITEMCOUNT, 0, 0);
			//Old way---through "Debug" button
			DWORD id = GetThreadId ((HANDLE) hDebugThread2);
			PostThreadMessage(id, WM__NEWDEBUGDLG, (WPARAM)fullname.c_str(), 0);

			//New way--using tab control
			res = ::SendMessage(mTab.hTab, TCM_GETITEMCOUNT, 0, 0);
//			debugBase.open_add_UDF(fullname.c_str(), 1);
			Sleep(200);
			TabCtrl_SetCurSel (mTab.hTab, res);
		}
		while ((it = dbmap.find(debugAstSig->GetScript()))==dbmap.end()) {Sleep(50);};
		lastDebug = it->second;
		lastDebug->pAst = debugAstSig;
		lastDebug->Debug(status);
		break;
	case stepping:
		lastDebug->Debug(status, (int)debugAstSig);
		break;
	case exiting:
		res = SendDlgItemMessage(IDC_DEBUGSCOPE, CB_GETCOUNT)-1;
		SendDlgItemMessage(IDC_DEBUGSCOPE, CB_DELETESTRING, res);
		SendDlgItemMessage(IDC_DEBUGSCOPE, CB_SETCURSEL, res);
		lastDebug->Debug(status);
		break;
	case cleanup: // no longer needed 8/2/2017
		res=mainSpace.vecast.size()-1;
		for (int k=0; k<res; k++)
			mainSpace.vecast.pop_back();
	// Cleans up all orphaned son's
	// cleaning up son--need this if exception was thrown without cleaning it in CallUDF
		for (CAstSig *p = mainSpace.vecast.front()->son; p; p = p->son) 
			if (!p->son) lp = p;
		for (CAstSig *next=NULL, *p = lp; p; )
		{
			if (!p->dad) 
				break;
			else
			{
				next = p->dad;
				delete p; 
				p=next;
			}
		}
		mainSpace.vecast.front()->son=NULL;
		break;
	}
}
void CShowvarDlg::OnPlotDlgCreated(char *varname, GRAFWNDDLGSTRUCT *pin)
{
	//It's better to update gcf before WM__PLOTDLG_CREATED is posted
	//that way, environment context can be set properly (e.g., main or inside CallSub)
	//4/24/2017 bjkwon
	if (strlen(varname)>0)
		Fillup();
	HHOOK hh = SetWindowsHookEx (WH_GETMESSAGE, HookProc, NULL, pin->threadPlot);
	HHOOK hh2 = SetWindowsHookEx (WH_KEYBOARD, HookProc2, NULL, pin->threadPlot);
	
	// this line is problematic... If varname is empty, new plotDlgThread replaces the old one.
	// maybe this is OK if the use of plotDlgThread is limited to cases of non-empty varname 
	// 02/20/2017 bjk
	plotDlgThread[varname] = pin->threadPlot; 
}

void CShowvarDlg::OnPlotDlgDestroyed(char *varname)
{
	plotDlgThread.erase(varname);
}

BOOL CShowvarDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	CWndDlg::OnInitDialog(hwndFocus, lParam);

	if (isWin7()) win7=true;

	RECT rtDlg;
	char buf[64];
	mShowDlg.GetWindowRect(&rtDlg);
	if ((int)hwndFocus==CELLVIEW)
	{
		::DestroyWindow(GetDlgItem(IDC_REFRESH));
		::DestroyWindow(GetDlgItem(IDC_SETPATH));
		::DestroyWindow(GetDlgItem(IDC_STATIC_FS));
		::DestroyWindow(GetDlgItem(IDC_FS));

		int cx = (rtDlg.right-rtDlg.left)*3/5;
		int cy = (rtDlg.bottom-rtDlg.top)*2/3;
		MoveWindow((rtDlg.left+rtDlg.right)/2, (rtDlg.bottom+rtDlg.top*2)/3, cx, cy, TRUE);
		sprintf(buf,"%s (cell)", (char*)lParam);
		SetWindowText(buf);
		::MoveWindow(GetDlgItem(IDC_STATIC_AUDIO), 7, 0, 200, 18, NULL);
		::MoveWindow(GetDlgItem(IDC_LIST1), 5, 18, cx, cy*9/22, NULL);
		::MoveWindow(GetDlgItem(IDC_STATIC_NONAUDIO), 7, cy/2, 210, 18, NULL);
		::MoveWindow(GetDlgItem(IDC_LIST2), 5, cy/2+16, cx, cy*5/11, NULL);
	}
	else
		fileDlg.InitFileDlg(hDlg, hInst, "");
	SendDlgItemMessage(IDC_REFRESH,BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
		(LPARAM)LoadImage(hInst,MAKEINTRESOURCE(IDB_BTM_REFRESH),IMAGE_BITMAP,30,30,LR_DEFAULTSIZE));

	lvInit();
//	FILE *fpp = fopen("track.txt","at"); fprintf(fpp,"tshowvar = %x\n", hDlg); fclose(fpp);

	SendDlgItemMessage(IDC_DEBUGSCOPE, CB_ADDSTRING, 0, (LPARAM)"base workspace");
	SendDlgItemMessage(IDC_DEBUGSCOPE, CB_SETCURSEL, 0);

	return TRUE;
}

void CShowvarDlg::OnShowWindow(BOOL fShow, UINT status)
{
	HMENU hMenu = ::GetSystemMenu(hDlg, FALSE);
	AppendMenu(hMenu, MF_SEPARATOR, 0, "");
	int res = AppendMenu(hMenu, MF_STRING, ID_HELP_SYSMENU, "&About AUX Lab");
}

void CShowvarDlg::OnSize(UINT state, int cx, int cy)
{
	if (hDlg==mShowDlg.hDlg)
	{
		::MoveWindow(GetDlgItem(IDC_LIST1), 5, 80, cx, cy*4/11, 1);
		::MoveWindow(GetDlgItem(IDC_LIST2), 5, cy/2+50, cx, cy*4/11, 1);
		::MoveWindow(GetDlgItem(IDC_STATIC_AUDIO), 7, 65, cx/2, 18, 1);
		::MoveWindow(GetDlgItem(IDC_STATIC_NONAUDIO), 7, cy/2+30, cx/2, 18, 1);
	}
	else
	{
		::MoveWindow(GetDlgItem(IDC_STATIC_AUDIO), 7, 0, 200, 18, 1);
		::MoveWindow(GetDlgItem(IDC_LIST1), 5, 18, cx, cy*9/22, 1);
		::MoveWindow(GetDlgItem(IDC_STATIC_NONAUDIO), 7, cy/2, 210, 18, 1);
		::MoveWindow(GetDlgItem(IDC_LIST2), 5, cy/2+16, cx, cy*5/11, 1);
	}

	int width[4];
	int ewas(0);
	for (int k=0; k<4; k++) width[k] = ListView_GetColumnWidth(hList1, k);
	for (int k=0; k<3; k++)  ewas += width[k];
	int newwidth = cx - ewas-20;
	int res = ListView_SetColumnWidth(hList1, 3, newwidth);
}

void CShowvarDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}

void CShowvarDlg::OnDestroy()
{

}

void CShowvarDlg::OnSysCommand(UINT cmd, int x, int y)
{
	// processing WM_SYSCOMMAND should return 0. How is this done?
	// just add (msg) == WM_SYSCOMMAND	in SetDlgMsgResult in message cracker
	HINSTANCE hInst;
	switch(cmd)
	{
	case ID_HELP_SYSMENU:
		hInst=GetModuleHandle(NULL);
		DialogBoxParam (hInst, MAKEINTRESOURCE(IDD_ABOUT), GetConsoleWindow(), (DLGPROC)AboutDlg, (LPARAM)hInst);
		break;
	}
}

void CShowvarDlg::OnCommand(int idc, HWND hwndCtl, UINT event)
{
	string addp, str;
	int res;
	size_t id;
	vector<HANDLE> figs;
	char *longbuffer, errstr[256];
	static char fullfname[256], fname[256];
//	if (changed) 
//		Fillup(); 
	
	switch(idc)
	{
	case ID_HELP1: 
		DialogBoxParam (hInst, MAKEINTRESOURCE(IDD_ABOUT), GetConsoleWindow(), (DLGPROC)AboutDlg, (LPARAM)hInst);
		break;
	case IDC_FS: 
		if (event==EN_KILLFOCUS)
		{
			res = GetDlgItemInt(idc);
			if (res!=pcast->GetFs())
			{
				if (res<500) MessageBox("Sampling rate must be greater than 500 Hz.");
				else
				{
					pcast->pEnv->Fs = res; //Sample rate adjusted
					for (map<string, CSignals>::iterator what=pcast->Vars.begin(); what!=pcast->Vars.end(); what++)
					{
						CSignals tpp = what->second;
						if(tpp.GetType()==CSIG_AUDIO)
						{
							tpp.Resample(res,errstr); // Sometimes resample generates an array with slightly different length than requested and zeros are padded. If you want to track it down, check errstr here
							what->second = tpp;
						}
					}
				}
			}
		}
		break;
	case IDC_SETPATH: 
		str = udfpath;
		id = str.find_first_not_of(';');
		longbuffer = new char[sizeof(udfpath)*8];
		strcpy(longbuffer,udfpath+id);
		if (InputBox("AUX UDF PATH", "Put path here (Each path is separated by a semicolon) (NOTE: The current application folder is the first priority path by default.)", longbuffer, sizeof(udfpath)*8, false, hDlg)==1)
		{
			strcpy(udfpath,longbuffer);
			addp = AppPath;
			addp += ';';
			addp += longbuffer;
			pcast->pEnv->SetPath(addp.c_str());
		}
		delete[] longbuffer;
		break;

	case IDC_STOP:
		// TEMPORARY HACK... 32799 is IDM_STOP in PlotDlg.cpp in graffy
		// Not a good way, but other than this, there' no way to make the playback stop
		// that was initiated by a different thread.... That is, call to TerminatePlay() won't work 
		// because threadIDs and len_threadIDs are thread specific 
		// (sort of... these variables are shared by some threads. But not by some other threads.)
		// One bonus feature in going this way is, this would stop ALL playbacks easily that were 
		// going on; otherwise it would have been quite complicated if done individually thread by thread.
		// 7/15/2016 bjk
		figs = graffy_Figures();
		for (vector<HANDLE>::iterator it=figs.begin(); it!=figs.end(); it++)
			::SendMessage(GetHWND_PlotDlg2(*it), WM_COMMAND, 32799, 0); 
		TerminatePlay();
		break;

	case IDC_REFRESH: //let's get rid of this.... 5/1/2017 bjk
		Fillup();
		UpdateSheets();
		changed = false;
		break;

	case IDC_DEBUG2:
		id = GetThreadId ((HANDLE) hDebugThread2);
		if (hDebugThread2)
			PostThreadMessage(id, WM__NEWDEBUGDLG, 0, 0);
		else
		{
			if ((hDebugThread2 = _beginthreadex (NULL, 0, debugThread2, (void*)fullfname, 0, NULL))==-1)
			{MessageBox ("Debug Thread Creation Failed."); break;}
		}
		break;

	case IDC_DEBUG:
		fullfname[0]=fname[0]=0;
		if (fileDlg.FileOpenDlg(fullfname, fname, "AUX UDF file (*.AUX)\0*.aux\0", "aux"))
		{
			DWORD id = GetThreadId ((HANDLE) hDebugThread);
			if (hDebugThread)
				PostThreadMessage(id, WM__NEWDEBUGDLG, (WPARAM)fullfname, 0);
			else
				if ((hDebugThread = _beginthreadex (NULL, 0, debugThread, (void*)fullfname, 0, NULL))==-1)
				{MessageBox ("Debug Thread Creation Failed."); break;}
//			LvItem.pszText=fullfname;
//			LvItem.iItem=ListView_GetItemCount(hList)+1;
//			SendDlgItemMessage(IDC_LIST,LVM_INSERTITEM,0,(LPARAM)&LvItem);
		}
		else
		{
			if (GetLastError()!=0) GetLastErrorStr(errstr), MessageBox (errstr, "File Open dialog box error");
		}
		break;
	case IDCANCEL:
		OnClose();
		break;
	}
}

void CShowvarDlg::OnCloseFig(int figID)
{

}

bool IsMarkedToDelete(CWndDlg* x)
{
	//char buf[256]; 
	//x->GetWindowTextA(buf,256);
	//if (GetSig(buf)==NULL)
	//{
	//	x->DestroyWindow(); 
	//	return true;
	//}
	return false;
}

void CShowvarDlg::UpdateSheets()
{
	char buf[256];
	bool var_available(false);

	m_sheets.erase(remove_if(m_sheets.begin(), m_sheets.end(), IsMarkedToDelete), m_sheets.end());

	for (vector<CWndDlg*>::iterator it=m_sheets.begin(); it<m_sheets.end(); ++it)
	{
		if (!(*it)->GetWindowTextA(buf, 256)) MessageBox("GetWindowTextA error");
		((CVectorsheetDlg*)(*it))->FillupVector(*pcast->GetSig(buf)); // now, GetSig(buf) is never NULL because of remove_if above
	}
}

void CShowvarDlg::lvInit()
{
	LvItem.mask=LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 256; // Max size of test
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	hList1 = GetDlgItem(IDC_LIST1);
	hList2 = GetDlgItem(IDC_LIST2);
	if (!hList1) MessageBox("lvInit");
	::SendMessage(hList1,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);
	::SendMessage(hList2,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);

	//These two functions are typically called in pair--so sigproc and graffy can communicate with each other for GUI updates, etc.
	SetHWND_SIGPROC(hDlg);
	SetHWND_GRAFFY(hDlg);
	SendDlgItemMessage(IDC_REFRESH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
		(LPARAM)LoadImage(hInst,MAKEINTRESOURCE(IDB_BTM_REFRESH),IMAGE_BITMAP,30,30,LR_DEFAULTSIZE));
	SendDlgItemMessage(IDC_STOP, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
		(LPARAM)LoadImage(hInst,MAKEINTRESOURCE(IDB_BTM_STOP),IMAGE_BITMAP,30,30,LR_DEFAULTSIZE));

	RECT rt;
	char buf[256];
	::GetClientRect (hList1, &rt);
	LoadString (hInst, IDS_HELP_SHOWVARLIST1, buf, sizeof(buf));
	CreateTT(hInst, hList1, rt, buf);
	::GetClientRect (hList2, &rt);
	LoadString (hInst, IDS_HELP_SHOWVARLIST2, buf, sizeof(buf));
	CreateTT(hInst, hList2, rt, buf, 300);
}



void CShowvarDlg::OnNotify(HWND hwnd, int idcc, LPARAM lParam)
{
	int res(0);
	static char buf[256]; // keep this in the heap (need to survive during _beginthread)
	char title[256];
	char errstr[256];
	static VARPLOT messenger;

	LPNMHDR pnm = (LPNMHDR)lParam;
	LPNMLISTVIEW pview = (LPNMLISTVIEW)lParam;
	LPNMLVKEYDOWN lvnkeydown;
	UINT code=pnm->code;
	LPNMITEMACTIVATE lpnmitem;
	static int clickedRow;
	bool alreadymade(false);
	CSignals sig;
	CWndDlg *cvdlg(NULL);
	if (changed) Fillup(); 
	int type, id;
	vector<string> player;
	vector<int> selectState;// This is the placeholder for select state used exclusively for mShowDlg, to use to get non-consecutively selected rows. Probably there are easier features already available if I was using .NET or C# 7/11/2016

	static GRAFWNDDLGSTRUCT in;

	switch(code)
	{
	case NM_CLICK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		break;
	case NM_DBLCLK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		ListView_GetItemText(lpnmitem->hdr.hwndFrom, ListView_GetSelectionMark(lpnmitem->hdr.hwndFrom), 0, buf, 256);
		if (Sig.GetType()!=CSIG_CELL)
			type=(sig=*pcast->GetSig(buf)).GetType();
		else
		{
			res = sscanf(buf, "{%d}", &id); // AUX is one-based. C++ is zero-based.
			type=(sig=Sig.cell[id-1]).GetType(); // AUX is one-based. C++ is zero-based.
		}
		switch(type)
		{
		for (vector<CWndDlg*>::iterator it=m_sheets.begin(); it!=m_sheets.end(); it++)
		{
			if (!(*it)->GetWindowText(title, 256)) MessageBox("GetWindowText error");
			if (!strcmp(title,buf)) { 
				alreadymade=true;
				if (type==CSIG_CELL)								cvdlg = (CShowvarDlg*)*it;	
				else if (type==CSIG_VECTOR	|| type==CSIG_STRING)	cvdlg = (CVectorsheetDlg*)*it;	
				break;}
		}
		case CSIG_VECTOR:
		case CSIG_STRING:
		case CSIG_COMPLEX:
			if (!alreadymade)
			{
				cvdlg = new CVectorsheetDlg;
				cvdlg->hParent = this;
				m_sheets.push_back(cvdlg); // is this the right object to put in cvdlg?? check it.... 6/4
				cvdlg->hDlg = CreateDialog (hInst, MAKEINTRESOURCE(IDD_DISPVECTOR), hDlg, (DLGPROC)vectorsheetDlg);
				cvdlg->OnInitDialog(NULL, (LPARAM)buf);
			}
			((CVectorsheetDlg*)cvdlg)->FillupVector(sig);
			break;
		case CSIG_CELL:
			if (!alreadymade)
			{
				cvdlg = new CShowvarDlg;
				cvdlg->hParent = this;
				cvdlg->hDlg = CreateDialog (hInst, MAKEINTRESOURCE(IDD_SHOWVAR), hDlg, (DLGPROC)showvarDlg);
//				((CShowvarDlg*)cvdlg)->lvInit();
				((CShowvarDlg*)cvdlg)->Sig = sig; // This is necessary so that when a cell element is clicked recursively, only the proper sig within the cell is retrived.
				if (cvdlg->hDlg!=NULL)
				{
					cellviewdlg.push_back(cvdlg);
					cvdlg->OnInitDialog((HWND)CELLVIEW, (LPARAM)buf);  // This go into CShowvarDlg::OnInitDialog
				}
			}
			((CShowvarDlg*)cvdlg)->Fillup(NULL, &sig);
			break;
		case CSIG_SCALAR:
		case CSIG_AUDIO:
			return;
		}
		cvdlg->ShowWindow(SW_SHOW);
		break;
	case LVN_KEYDOWN:
		lvnkeydown = (LPNMLVKEYDOWN)lParam;
		ListView_GetItemText(lvnkeydown->hdr.hwndFrom, ListView_GetSelectionMark(lvnkeydown->hdr.hwndFrom), 0, buf, 256);
		if (strlen(buf)>0) 
		{	
			CGobj * hobj;
			switch(lvnkeydown->wVKey)
			{
			case VK_DELETE: // $$231109 4/25/2017
				pcast->ClearVar(buf);
				Fillup();
				break;
			case VK_SPACE:
				selectState.resize(ListView_GetItemCount(lvnkeydown->hdr.hwndFrom));
				for (int k(0); k<ListView_GetItemCount(lvnkeydown->hdr.hwndFrom); k++)
				{
					if (ListView_GetItemState(lvnkeydown->hdr.hwndFrom, k, LVIS_SELECTED))
					{
						ListView_GetItemText(lvnkeydown->hdr.hwndFrom, k, 0, buf, 256);
						player.push_back(buf);
						pcast->GetSig(buf)->PlayArray(0, WM_APP+100, hDlg, &playbackblock, errstr);
					}
				}
				break;
			case VK_RETURN:
				sig.SetString(buf);
				hobj = (CGobj *)GCF(&sig);
				if (!hobj)
				{
					pcast->Sig = *pcast->GetSig(buf);
					if ( (pcast->Sig.GetType()==CSIG_AUDIO || pcast->Sig.GetType()==CSIG_VECTOR) && pcast->Sig.nSamples>0)
					{
						CSignals gcf;
						CRect rt(0, 0, 500, 310);
						HANDLE fig = OpenGraffy(rt, buf, GetCurrentThreadId(), win7 ? NULL:hDlg, in); // due to z-order problem in Windows 7
						HANDLE ax = AddAxis (fig, .08, .18, .86, .72);
						CAxis *cax = static_cast<CAxis *>(ax);
						if (pcast->Sig.GetType()==CSIG_VECTOR)
							PlotCSignals(ax, pcast->Sig, 0xff0000, '*', LineStyle_noline); // plot for VECTOR, no line and marker is * --> change if you'd like
						else
							PlotCSignals(ax, pcast->Sig, 0xff0000);
						if (pcast->Sig.next)
							PlotCSignals(ax, *pcast->Sig.next, RGB(200,0,50));
						//update gcf here
						GetFigID(in.fig, gcf);
						pcast->SetVar("gcf", gcf);
						PostMessage(WM__PLOTDLG_CREATED, (WPARAM)buf, (LPARAM)&in);
					}
				}
				else
				{ //essentially the same as CPlotDlg::SetGCF()
					if (hobj->m_dlg->hDlg!=GetForegroundWindow())
						SetForegroundWindow(hobj->m_dlg->hDlg);
					PostMessage(WM_GCF_UPDATED, (WPARAM)hobj);
				}
				break;
			}
		}
		break;

	case NM_CHAR:
		break;
	case NM_CUSTOMDRAW:
        ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)ProcessCustomDraw(pnm));
        return;
	default:
		break;
	}
}

void CShowvarDlg::OnSoundEvent(int index, int code)
{
	soundID = index;
	if (code>1) playing = false; // when done, code is the total number of samples played.
}

LRESULT CShowvarDlg::ProcessCustomDraw (NMHDR *lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	switch(lplvcd->nmcd.dwDrawStage) 
	{
	case CDDS_PREPAINT : //Before the paint cycle begins
	//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;
            
	case CDDS_ITEMPREPAINT: //Before an item is drawn
		//if (ListView_GetItemState (lplvcd->nmcd.hdr.hwndFrom,lplvcd->nmcd.dwItemSpec,LVIS_FOCUSED|LVIS_SELECTED) == (LVIS_FOCUSED|LVIS_SELECTED))
		//{
		//	RECT ItemRect;
		//	
		//	ListView_GetItemRect(lplvcd->nmcd.hdr.hwndFrom,lplvcd->nmcd.dwItemSpec,&ItemRect,LVIR_BOUNDS);
		//	HRGN bgRgn = CreateRectRgnIndirect(&ItemRect);
		//	HBRUSH MyBrush = CreateSolidBrush(RGB(150,150,150));
		//	FillRgn(lplvcd->nmcd.hdc, bgRgn, MyBrush);
		//	DeleteObject(MyBrush);
		//	return CDRF_SKIPDEFAULT;
		//}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
		switch(lplvcd->iSubItem)
		{
		case 10:
			lplvcd->clrText   = RGB(255,255,255);
			lplvcd->clrTextBk = RGB(240,55,23);
			return CDRF_NEWFONT;
		case 0:
			lplvcd->clrText   = RGB(255,255,0);
			lplvcd->clrTextBk = RGB(0,0,0);
			return CDRF_NEWFONT;
		case 1:
			lplvcd->clrText   = RGB(20,26,158);
			lplvcd->clrTextBk = RGB(200,200,10);
			return CDRF_NEWFONT;
		case 2:
		case 3:
			lplvcd->clrText   = RGB(12,15,46);
			lplvcd->clrTextBk = RGB(200,200,200);
			return CDRF_NEWFONT;
		case 11:
			lplvcd->clrText   = RGB(120,0,128);
			lplvcd->clrTextBk = RGB(20,200,200);
			return CDRF_NEWFONT;
		case 4:
		case 5:
			lplvcd->clrText   = RGB(255,255,255);
			lplvcd->clrTextBk = RGB(0,0,150);
			return CDRF_NEWFONT;
		case 6:
		case 7:
			lplvcd->clrText   = RGB(42,45,46);
			lplvcd->clrTextBk = RGB(200,200,200);
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

void SetInsertLV(int type, HWND h1, HWND h2, UINT msg, LPARAM lParam)
{
	if (type==CSIG_AUDIO)
		SendMessage(h1, msg, 0, lParam);
	else
		SendMessage(h2, msg, 0, lParam);
}


void CShowvarDlg::Fillup(map<string,CSignals> *Vars, CSignals *cell)
{
	int res;
	changed=false;
	HWND h, hList;
	HINSTANCE hModule = GetModuleHandle(NULL);
	char buf[256], buf1[256], buf2[256];
	if (Vars==NULL)
		Vars = &pcast->Vars;
	else
		pcast->Vars = *Vars;

	if ( (h = GetDlgItem(IDC_FS))!=NULL)
		::SetWindowText(h, itoa(pcast->pEnv->Fs, buf, 10));
	if (cell!=NULL) 
	{
		::SetWindowText(GetDlgItem(IDC_STATIC_AUDIO), "Audio Signal cell elements");
		::SetWindowText(GetDlgItem(IDC_STATIC_NONAUDIO), "Non-Audio Signal elements");
	}
	SendDlgItemMessage(IDC_LIST1,LVM_DELETEALLITEMS,0,0);
	while (SendDlgItemMessage(IDC_LIST1,LVM_DELETECOLUMN,0,0)) {};
	SendDlgItemMessage(IDC_LIST2,LVM_DELETEALLITEMS,0,0);
	while (SendDlgItemMessage(IDC_LIST2,LVM_DELETECOLUMN,0,0)) {};

	int width[]={60, 45, 70, 300, };
	if (cell!=NULL) {width[0] = 30; width[2] = 45;} // narrower width for index and length with cell view
	for(int k = 0; k<4; k++)
	{
		LvCol.cx=width[k];
		LvCol.pszText=buf;
		if (k==0 && cell!=NULL) strcpy(buf,"id");
		else		LoadString(hModule, IDS_STRING105+k, buf, sizeof(buf));
		res = (int)SendDlgItemMessage(IDC_LIST1, LVM_INSERTCOLUMN,k,(LPARAM)&LvCol); 
	}
	int width2[]={60, 45, 50, 300, 40, 60,200,};
	if (cell!=NULL) {width2[0] = 30; width2[2] = 45;} // narrower width for index and length with cell view
	for(int k = 0; k<4; k++)
	{
		LvCol.cx=width2[k];
		LvCol.pszText=buf;
		if (k==0 && cell!=NULL) strcpy(buf,"id");
		else		LoadString(hModule, IDS_STRING101+k, buf, sizeof(buf));
		res = (int)SendDlgItemMessage(IDC_LIST2, LVM_INSERTCOLUMN,k,(LPARAM)&LvCol); 
	}
	CAstSigEnv *pe;
	if (cell==NULL) // if it is main work space, grab it from global main
		pe = pcast->pEnv;
	else // else just make a temporary workspace for the input cell variable
	{
		char buff[256];
//		pe = new CAstSigEnv(3); // just to circumvent Fs > 1 requirement in CAstSigEnv::CAstSigEnv()
		for (size_t u=cell->cell.size(); u!=0; u--)
		{
			sprintf(buff, "{%d}", u);
			(*Vars)[buff] = cell->cell[u-1];
		}
	}
	int xc(0);
	int k(0); // k is for column
	string out, arrout, lenout;
	for (map<string, CSignals>::iterator what=Vars->begin(); what!=Vars->end(); what++)
	{
		CSignals tpp = what->second;
		LvItem.iSubItem=0; //First item (InsertITEM)
		strcpy(buf, what->first.c_str()); 
		LvItem.pszText=buf;
		int type = tpp.GetType();
		(type==CSIG_AUDIO || type==CSIG_NULL) ? hList = GetDlgItem(IDC_LIST1) : hList = GetDlgItem(IDC_LIST2);
		LvItem.iItem=ListView_GetItemCount(hList);
		::SendMessage (hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
		LvItem.iSubItem=1; //second item
		switch(type)
		{
		case CSIG_AUDIO:
			if (tpp.IsLogical()) strcpy(buf, "logical");
			else if (!tpp.next)		RMSDB(buf,"-Inf","%5.1f", tpp.RMS())
			else											{	
				RMSDB(buf1,"-Inf","%5.1f", tpp.RMS()) 
				RMSDB(buf2,"-Inf","%5.1f", tpp.next->RMS()) 
				sprintf(buf,"%s,%s",buf1,buf2);					}
			LvItem.pszText=buf;
			break;
		case CSIG_NULL:
			sprintf(buf, "???");
			LvItem.pszText=buf;
			break;
		case CSIG_STRING:
			LvItem.pszText="string";
			break;
		case CSIG_EMPTY:
			LvItem.pszText="empty";
			break;
		case CSIG_SCALAR:
			LvItem.pszText="scalar";
			break;
		case CSIG_VECTOR:
			if (!tpp.next)	LvItem.pszText="vector";
			else			LvItem.pszText="2D vector";
			break;
		case CSIG_CELL:
			LvItem.pszText="cell";
			break;
		case CSIG_COMPLEX:
			LvItem.pszText="complex";
			break;
		}
		if (type!=CSIG_AUDIO && tpp.IsLogical()) LvItem.pszText="logical";
		::SendMessage (hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
		LvItem.iSubItem=2; 
		switch(type)
		{
		case CSIG_AUDIO:
			lenout.clear();
			// next is CSignal, not CSignals (next does not have next)
			xc=0;
			for (CSignals* pchan=&tpp; xc<2 && pchan; xc++, pchan = (CSignals*)pchan->next)
			{
				for (CSignal* p=pchan; p; p=p->chain)
				{
					sprintf(buf, "%d", p->nSamples);	
					lenout += buf;
					if (p->chain) lenout += ", ";
				}
				if (xc==0 && pchan->next) lenout += " ; ";
			}
			if (lenout.size()>256) lenout[255]=0;
			strcpy_s(buf, 256, lenout.c_str());
			break;
		case CSIG_SCALAR:
			strcpy(buf,"1");
			break;
		case CSIG_NULL:
			strcpy(buf,"0");
			break;
		case CSIG_STRING:
		case CSIG_VECTOR:
		case CSIG_COMPLEX:
			sprintf(buf, "%d", tpp.length());
			break;
		case CSIG_CELL:
			sprintf(buf, "%d", tpp.cell.size());
			break;
		case CSIG_EMPTY:
			sprintf(buf, "0");
			break;
		}
		LvItem.pszText=buf;
		::SendMessage (hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
		LvItem.iSubItem=3; 
		switch(type)
		{
		case CSIG_STRING:
			LvItem.pszText=tpp.getString(buf,sizeof(buf));
			break;
		case CSIG_EMPTY:
			LvItem.pszText="----";
			break;
		case CSIG_NULL:
			sprintf(buf, "NULL(0~%g)", tpp.tmark);
			LvItem.pszText=buf;
			break;
		case CSIG_AUDIO:
			nonnulintervals(&tpp, out, false, true);
			if (tpp.next) { out += " ; "; nonnulintervals(tpp.next, out, false); }
			if (out.size()>256) out[255]=0;
			strcpy_s(buf, 256, out.c_str());
			LvItem.pszText=buf;
			break;
		case CSIG_VECTOR:
			arrout="[";
			if (tpp.IsLogical())
			{
				for (int k=0; k<min(tpp.nSamples,10); k++)
				{	sprintf(buf,"%d ", tpp.logbuf[k]); arrout += buf;		}
				if (tpp.nSamples>10) arrout += "...";
			}
			else
			{
				for (int k=0; k<min(tpp.nSamples,10); k++)
				{	sprintf(buf,"%g ", tpp.buf[k]); arrout += buf;		}
				if (tpp.nSamples>10) arrout += "...";
				if (tpp.next)
				{
					arrout +=" ; ";
					for (int k=0; k<min(tpp.next->nSamples,10); k++)
					{	sprintf(buf,"%g ", tpp.next->buf[k]); arrout += buf;		}
					if (tpp.next->nSamples>10) arrout += "...";
				}
			}
			strcpy(buf, arrout.c_str());
			buf[strlen(buf-1)-2] = ']';
			LvItem.pszText=buf;
			break;
		case CSIG_COMPLEX:
			if (tpp.nSamples==1)
				if (tpp.buf[1]==1.)
					sprintf(buf,"%g+i ", tpp.buf[0]);
				else
					sprintf(buf,"%g%+gi ", tpp.buf[0], tpp.buf[1]);
			else
			{
				arrout="[";
				for (int k=0; k<min(tpp.nSamples,10); k++)
				{	
					if (tpp.buf[1]==1.)
						sprintf(buf,"%g+i ", tpp.buf[2*k]);
					else
						sprintf(buf,"%g%+gi ", tpp.buf[2*k], tpp.buf[2*k+1]);
					arrout += buf;		
				}
				if (tpp.nSamples>10) arrout += "...";
				strcpy(buf, arrout.c_str());
				buf[strlen(buf-1)-2] = ']';
			}
			LvItem.pszText=buf;
			break;
		case CSIG_SCALAR:
			if (tpp.IsLogical())
				sprintf(buf, "%d", tpp.logbuf[0]);
			else
				sprintf(buf, "%g", tpp.value());
			LvItem.pszText=buf;
			break;
		case CSIG_CELL:
			LvItem.pszText="----";
			break;
		}
		::SendMessage (hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
}

