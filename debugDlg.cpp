#include "wxx_wincore.h" // Win32++ 8.2. This must be placed prior to <windows.h> to avoid including winsock.h
#include "debugDlg.h"
#include "resource1.h"
#include "audfret.h"
#include "FileDlg.h" // from common/include
#include "showvar.h"
#include "Objbase.h"
#include "Shobjidl.h"

#ifndef SIGPROC
#include "sigproc.h"
#endif

#include "xcom.h"
#include "consts.h"
#include "TabCtrl.h"

extern xcom mainSpace;
extern HANDLE hStdin, hStdout; 
CFileDlg fileDlg2;
extern CShowvarDlg mShowDlg;
extern uintptr_t hDebugThread2;
extern char iniFile[256];

CTabControl mTab;

CDebugBaseDlg debugBase;
vector<CDebugDlg *> debugVct; // Debug dialogboxes, which appear on the tab control

map<string, CDebugDlg*> dbmap;
char fullfname[_MAX_PATH], fname[_MAX_FNAME + _MAX_EXT];

int spyWM(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam, char* const fname, vector<UINT> msg2excl, char* const tagstr);

BOOL CALLBACK DebugBaseProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabPage_DlgProc(HWND hwndDlg, UINT umsg, WPARAM wParam, LPARAM lParam);

#define WM__NEWDEBUGDLG	WM_APP+0x2000
#define WM__UPDATE_UDF_CONTENT	WM_APP+0x2001

extern vector<UINT> exc; //temp

int getID4hDlg(HWND hDlg)
{
	size_t k(0);
	for (vector<CDebugDlg*>::iterator it=debugVct.begin(); it!=debugVct.end(); it++, k++) 
	{ if (hDlg==(*it)->hDlg)   return (int)k; }
	return -1;
}

#define m_THIS  debugVct[id]

extern vector<UINT> exc;

BOOL CALLBACK debugDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	if (exc.size()==0)
	{
		exc.push_back(WM_NCHITTEST);
		exc.push_back(WM_SETCURSOR);
		exc.push_back(WM_MOUSEMOVE);
		exc.push_back(WM_NCMOUSEMOVE);
		exc.push_back(WM_WINDOWPOSCHANGING);
		exc.push_back(WM_WINDOWPOSCHANGED);
		exc.push_back(WM_CTLCOLORDLG);
		exc.push_back(WM_NCPAINT);
		exc.push_back(WM_GETMINMAXINFO);
		exc.push_back(WM_MOVE);
		exc.push_back(WM_MOVING);
		exc.push_back(WM_NCMOUSEMOVE);
		exc.push_back(WM_ERASEBKGND);
	}
	
//	spyWM(hDlg, umsg, wParam, lParam, "c:\\temp\\rec", exc, "[debugDlgProc]");

	CDebugDlg *p_db(NULL); 
	int line(-1);
	for (map<string, CDebugDlg*>::iterator it = dbmap.begin(); it!=dbmap.end(); it++)
		if (hDlg == it->second->hDlg) p_db = it->second;
	if (!p_db && umsg!=WM_INITDIALOG) return FALSE;

	if (umsg==WM_INITDIALOG)
	{
		//FILE *fp = fopen("c:\\temp\\rec","wt");
		//fprintf(fp, "NM_CLICK=%4x\n", NM_CLICK);
		//fprintf(fp, "NM_DBLCLK=%4x\n", NM_DBLCLK);
		//fprintf(fp, "NM_SETFOCUS=%4x\n", NM_SETFOCUS);
		//fprintf(fp, "NM_KILLFOCUS=%4x\n", NM_KILLFOCUS);
		//fprintf(fp, "NM_NCHITTEST=%4x\n", NM_NCHITTEST);
		//fprintf(fp, "NM_KEYDOWN=%4x\n", NM_KEYDOWN);
		//fprintf(fp, "LVN_ITEMCHANGED=%4x\n", LVN_ITEMCHANGED);
		//fprintf(fp, "NM_CUSTOMDRAW=%4x\n", NM_CUSTOMDRAW);
		//fclose(fp);
		((CREATE_CDebugDlg*)lParam)->dbDlg->hDlg = hDlg;
		return ((CREATE_CDebugDlg*)lParam)->dbDlg->OnInitDialog((HWND)wParam, lParam);
	}
	switch (umsg)
	{
	chHANDLE_DLGMSG (hDlg, WM_SIZE, p_db->OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, p_db->OnClose);
	chHANDLE_DLGMSG (hDlg, WM_DESTROY, p_db->OnDestroy);
	chHANDLE_DLGMSG (hDlg, WM_SHOWWINDOW, p_db->OnShowWindow);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, p_db->OnCommand);

	case WM_NOTIFY:
		p_db->OnNotify(hDlg, (int)(wParam), lParam);
		//{
		//	FILE *fp = fopen("c:\\temp\\rec","at");
		//	fprintf(fp, "\t\t\t\tcode=%4x\n", ((LPNMHDR)lParam)->code);
		//	fclose(fp);
		//}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}


unsigned int WINAPI debugThread2 (PVOID var) 
{
	CRect *prt = (CRect *)var;
	HWND hBase = CreateDialogParam (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DEBUG2), mShowDlg.hDlg, (DLGPROC)DebugBaseProc, (LPARAM)&debugBase);
	int res=debugBase.MoveWindow(*prt);
	MSG         msg ;
	HACCEL hAccDebug = LoadAccelerators (GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ACCEL_DEBUG));
	while (GetMessage (&msg, NULL, 0, 0))
	{ 
//		spyWM(msg.hwnd, msg.message, msg.wParam, msg.lParam, "c:\\temp\\rec", exc, "[debugThread2-msg loop]");
		if (msg.message==WM__ENDTHREAD) 
			break;
		else if (msg.message==WM__NEWDEBUGDLG)
		{
			if (!msg.wParam)
			{
		//		fileDlg2.FileOpenDlg(fullfname, fname, "AUX UDF file (*.AUX)\0*.aux\0", "aux");
				debugBase.FileOpen(fullfname);
			}
			else
				strcpy(fullfname, (char*)msg.wParam);
			if (strlen(fullfname)>0)
				debugBase.open_tab_with_UDF(fullfname, -1);
		}
		else
		{
			int res(0);
					if (msg.message==0x100)
					{res++; res--;}
			int iSel = TabCtrl_GetCurSel(debugBase.hTab);
			for (map<string, CDebugDlg*>::iterator it=dbmap.begin(); it!=dbmap.end(); it++)
			{
				if (it->second->hDlg == mTab.hCur)
				{	
					res = TranslateAccelerator (mTab.hCur, hAccDebug, &msg); 
					if (res)						
					{res++; res--;}
				
					break; }
			}
			if (!res && !IsDialogMessage(msg.hwnd, &msg))
			{
				TranslateMessage (&msg) ;
				DispatchMessage (&msg) ;
			}
		}
	}
	//cleanup -- this may not have enough time during exiting from closeXcom
	for (vector<CDebugDlg *>::iterator it=debugVct.begin(); it!=debugVct.end(); it++)
	{
		DeleteObject((*it)->eFont);
		DeleteObject((*it)->eFont2);
		(*it)->DestroyWindow();
		delete *it;
	}
	return 0;
}


CDebugDlg::CDebugDlg(void)
:pAstSig(NULL)
{

}

CDebugDlg::~CDebugDlg(void)
{
//	dbmap.erase(udfname);
}


/* For now (9/12/2017), debug window showing udf files is updated only IF
 the udf is displayed (either click the tab or keyboard input)
 OR
 the udf is called in the command window or called by another function during a udf call

 in CAstSig::SetNewScriptFromFile(), if the file content has been changed, 
 sigproc sendmessage's to showvarDlg with an lParam of fullfilame -->so ShowFileContent is called inside CShowvarDlg::OnDebug()
*/

int CDebugDlg::ShowFileContent(const char* fullfilename)
{
	vector<string> lines;
	size_t count(0);
	FILE *fp = fopen(fullfilename, "rt");
	if (fp!=NULL)
	{
		fseek (fp, 0, SEEK_END);
		int filesize=ftell(fp);
		fseek (fp, 0, SEEK_SET);
		char *buf = new char[filesize+1];
		size_t res = fread(buf, 1, (size_t)filesize, fp);
		buf[res]=0;
		res = fclose(fp);
		// str2vect is not used, because we need to take blank lines as they come
		char *lastpt=buf;
		char *pch=strchr(buf,'\n');
		while (pch)
		{
			*pch = '\0';
			lines.push_back(lastpt);
			lastpt = pch+1;
			pch=strchr(lastpt,'\n');
		}
		// if there's a remainder, take it.
		if (strlen(lastpt)>0) 
			lines.push_back(lastpt);
		delete[] buf;
		FillupContent(lines);
		return 1;
	}
	else
	{
		//Error handle
		return 0;
	}
}

BOOL CDebugDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	CWndDlg::OnInitDialog(hwndFocus, lParam);

	char buf0[256], fname[256];
	CREATE_CDebugDlg *ptransfer = (CREATE_CDebugDlg *)lParam;
	strcpy(buf0, "[Debug] ");
	strcat(buf0, ptransfer->fullfilename);
	::SetWindowText(GetParent(hwndFocus), buf0);
	_splitpath(ptransfer->fullfilename, NULL, NULL, fname, buf0);
	dbmap[fname] = this;
	strcpy(fullUDFpath, ptransfer->fullfilename);
	string temp = fullUDFpath;
	transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
	strcpy(fullUDFpath, temp.c_str());

	udfname = fname;

	CDebugBaseDlg *mPar = (CDebugBaseDlg*)hParent;
	mPar->udfname[hDlg] = udfname;
	mPar->udf_fullpath[udfname] = fullUDFpath;
//	ptransfer->dbDlg->hDlg = GetParent(hwndFocus); // not needed since tabcontrol is used.
	lvInit();

	LvItem.mask=LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 256; // Max size of text
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	hList = GetDlgItem(IDC_LISTDEBUG);
	LRESULT res = ::SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0, /*LVS_SHOWSELALWAYS | */ LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);
	
	char buf[256];
	int width[]={50, -1, };
	for(int k = 0; k<2; k++)
	{
		LvCol.cx=width[k];
		LoadString(hInst, IDS_DEBUGBOX1+k, buf, sizeof(buf));
		LvCol.pszText=buf;
		res = SendDlgItemMessage(IDC_LISTDEBUG, LVM_INSERTCOLUMN,k,(LPARAM)&LvCol); 
	}
	ShowFileContent(fullUDFpath);

	fontsize = 10;
	HDC hdc = GetDC(NULL);
	lf.lfHeight = -MulDiv(fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(NULL, hdc);
	strcpy(lf.lfFaceName, "Courier New");
	eFont = CreateFont(lf.lfHeight,0,0,0, FW_MEDIUM, FALSE, FALSE, 0,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
		FIXED_PITCH | FF_MODERN, lf.lfFaceName);
	strcpy(lf.lfFaceName, "Arial Black");
	eFont2 = CreateFont(lf.lfHeight,0,0,0, FW_BOLD, FALSE, FALSE, 0,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
		FIXED_PITCH | FF_MODERN, lf.lfFaceName);

	return TRUE;
}

void CDebugDlg::Debug(CAstSig *pastsig, DEBUG_STATUS type, int line)
{
	char buf[256];
	int nLines;
	switch(type)
	{
	case entering:
	case progress:
		SetFocus(hList);
		break;
	case stepping:
		pAstSig = pastsig;
		::RedrawWindow(hDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
		nLines = (int)::SendMessage(hList, LVM_GETITEMCOUNT, 0,0); 
		if (nLines<line)
		{
			LvItem.iSubItem=0; //First item (InsertITEM)
			LvItem.pszText="";
			::SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
			strcpy(buf, "Going back to the last scope");
			LvItem.iSubItem++; //Second column (SetITEM)
			::SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
		}
		SetFocus(hList);
		break;
	case exiting:
		pAstSig->currentLine = -1;
		::RedrawWindow(hDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
		break;
	}
}

void CDebugDlg::FillupContent(vector<string> in)
{
	SendDlgItemMessage(IDC_LISTDEBUG,LVM_DELETEALLITEMS,0,0);
	char buf[256];
	LRESULT res;
	size_t ss;
	for (LvItem.iItem=0; LvItem.iItem<(int)in.size(); LvItem.iItem++)
	{
		itoa(LvItem.iItem+1, buf, 10);
		LvItem.iSubItem=0; //First item (InsertITEM)
		LvItem.pszText=buf;
		::SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

		LvItem.iSubItem++; //Second column (SetITEM)
		while ( (ss = in[LvItem.iItem].find('\t'))!=string::npos)
			in[LvItem.iItem].replace(ss,1,"  ");
		strcpy(buf,in[LvItem.iItem].c_str());
		res = ::SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
//	res = ListView_Scroll(hList,0,ListView_GetItemCount(hList)*14);
	for (vector<int>::iterator it=breakpoint.begin(); it!=breakpoint.end(); it++)
		ListView_SetCheckState(hList, *it-1, 1);

}

void CDebugDlg::lvInit()
{
	LvItem.mask=LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 256; // Max size of text
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	hList = GetDlgItem(IDC_LISTDEBUG);
	LRESULT res = ::SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0, /*LVS_SHOWSELALWAYS | */ LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);
	RECT rt;
//	char buf[256];
	::GetClientRect (hList, &rt);
//	LoadString (hInst, IDS_HELP_HISTORY, buf, sizeof(buf));
//	CreateTT(hInst, hList, rt, buf, 270);
}

void CDebugDlg::OnShowWindow(BOOL fShow, UINT status)
{
	int res(0);
	::ShowWindow(hList, fShow);
#ifdef _DEBUG
	RECT rt;
	char __buf[256], __buf2[256];
	sprintf(__buf,"CDebugDlg=0x%x\t%s fShow=%d\n", hDlg, udfname.c_str(), fShow);
	sprintf(__buf2,"hList=0x%x, fShow=%d\n", hList, fShow);
	if (fShow)
	{
		res = ::GetClientRect (mTab.hTab, &rt);
	}
	strcat(__buf,__buf2);
	PRINTLOG("c:\\temp\\windows_log.txt", __buf)
#endif //_DEBUG
}

int CDebugDlg::SetSize()
{ // evoked by CDebugBaseDlg::OnSize
 	CRect rt;
	int res = ::GetClientRect (hParent->hDlg, &rt);
	int offset = 25;
	res = MoveWindow(0, offset, rt.right, rt.bottom-offset, 1); // Causing CDebugDlg::OnSize() to run
	return res;
}

void CDebugDlg::OnSize(UINT state, int cx, int cy)
{
	::MoveWindow(hList, 0, 0, cx, cy, 1);
	int res = ListView_SetColumnWidth(hList, 1, cx-53);
}

void CDebugDlg::OnClose()
{
	// Then bring it back ---ShowWindow(SW_SHOW); if it hits the breakpoint.... do this tomorrow 7/3/2017 bjk
}

void CDebugDlg::OnDestroy()
{
//	::SendMessage(GetParent(hDlg), WM_DEBUG_CLOSE, (WPARAM)udfname.c_str(), 0);
}

int CDebugDlg::GetCurrentCursor()
{
	int marked = ListView_GetSelectionMark(hList);
	return marked;
}

void CDebugDlg::OnCommand(int idc, HWND hwndCtl, UINT event)
{
#ifdef _DEBUG
	bool play=true;
#else
	bool play=true;
#endif
	bool debugging = mainSpace.vecast.size()>1;
	char buf[256];
	DWORD dw;
	WORD vcode;
	int res, marked, lineChecked;
	switch(idc)
	{
	case ID_F11: //from accelerator, event should be 1
		strcpy(buf, "#stin ");
		vcode = VK_F11;
		break;
	case ID_F10: //from accelerator, event should be 1
		strcpy(buf, "#step ");
		vcode = VK_F10;
		break;
	case ID_F5: //from accelerator, event should be 1
		strcpy(buf, "#cont ");
		vcode = VK_F5;
//		if (play) Beep(150,400);
		break; 
	case ID_DB_EXIT: //from accelerator, event should be 1
		strcpy(buf, "#abor ");
		vcode = VK_F5;
//		if (play) Beep(2500,400);
		break;

	case ID_RUN2CUR:
		strcpy(buf, "#r2cu ");
		vcode = VK_F10;
		for (size_t k=0; k<mainSpace.vecast.size(); k++)
			mainSpace.vecast[k]->pEnv->curLine = ListView_GetSelectionMark(hList)+1;
		break;

	case ID_F9: //from accelerator, event should be 1
		marked = ListView_GetSelectionMark(hList);
		lineChecked = ListView_GetCheckState(hList, marked);
		ListView_SetCheckState(hList, marked, lineChecked ? 0:1);
		break;

	case ID_OPEN:
		fullfname[0]=fname[0]=0;
		if (fileDlg2.FileOpenDlg(fullfname, fname, "AUX UDF file (*.AUX)\0*.aux\0", "aux"))
//		if (((CDebugBaseDlg*)hParent)->FileOpen(fullfname))
			((CDebugBaseDlg*)hParent)->open_tab_with_UDF(fullfname, -1);
		break;
	case IDCANCEL:
	//	OnClose();
		break;
	}
	if (debugging && (idc==ID_F10 || idc==ID_F11 || idc==ID_F5 || idc==ID_DB_EXIT || idc==ID_RUN2CUR) )
	{
		for (int k=0; k<2;k++)
		{
			mainSpace.debug_command_frame[k].EventType = KEY_EVENT;
			mainSpace.debug_command_frame[k].Event.KeyEvent.uChar.AsciiChar = 0;
			mainSpace.debug_command_frame[k].Event.KeyEvent.wVirtualKeyCode = vcode;
			if (vcode==VK_F5 && idc==ID_DB_EXIT)
				mainSpace.debug_command_frame[k].Event.KeyEvent.dwControlKeyState = SHIFT_PRESSED;
			else if (vcode==VK_F10 && idc==ID_RUN2CUR) 
				mainSpace.debug_command_frame[k].Event.KeyEvent.dwControlKeyState = RIGHT_CTRL_PRESSED;
		}
		mainSpace.debug_command_frame[0].Event.KeyEvent.bKeyDown = 1;
		mainSpace.debug_command_frame[1].Event.KeyEvent.bKeyDown = 0;
		res = WriteConsoleInput(hStdin, mainSpace.debug_command_frame, 2, &dw);
	}
}

bool CDebugDlg::cannotbeBP(int line)
{
	if (line==0) return true;
	char buffer[512];
	ListView_GetItemText(hList, line, 1, buffer, sizeof(buffer));
	if (!strncmp(buffer, "//", 2)) return true;
	if (strlen(buffer)==0) return true;
	return false;
}

void CDebugDlg::OnNotify(HWND hwnd, int idcc, LPARAM lParam)
{
	static char buf[256];
	string name;
	LPNMHDR pnm = (LPNMHDR)lParam;
	LPNMLISTVIEW pview = (LPNMLISTVIEW)lParam;
	UINT code=pnm->code;
	LPNMITEMACTIVATE lpnmitem;
	vector<int>::iterator it;
	int marked, current, clickedRow;
	int res(0), lineChecked;
	CDebugBaseDlg *mPar = (CDebugBaseDlg*)hParent;
	switch(code)
	{
	case NM_CLICK:
		break;
	case NM_DBLCLK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		marked = ListView_GetSelectionMark(lpnmitem->hdr.hwndFrom);
		ListView_GetItemText(lpnmitem->hdr.hwndFrom, clickedRow, 0, buf, 256);
		break;

	case LVN_ITEMCHANGED:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		if (cannotbeBP(clickedRow)) {ListView_SetCheckState(hList, clickedRow, 0); break;}
		lineChecked = ListView_GetCheckState(hList, clickedRow);
		if (lpnmitem->uNewState>=LVIF_DI_SETITEM && !lpnmitem->uOldState) break;
		if ( (lpnmitem->uNewState & LVIS_SELECTED) && !lineChecked) 	break;
		it = find(breakpoint.begin(), breakpoint.end(), clickedRow+1);
		current = mTab.GetCurrentPageID();
		udfname = mPar->udfname[mTab.hCur];
//		mPar->udfname[this] = udfname;
		if (lineChecked)
		{
			if (it==breakpoint.end())
			{
				breakpoint.push_back(clickedRow+1);
				sort(breakpoint.begin(), breakpoint.end());
			}
		}
		else
		{
			if (it!=breakpoint.end())
				breakpoint.erase(it);
		}
		if (breakpoint.empty())
			mainSpace.vecast.front()->pEnv->DebugBreaks.erase(udfname);
		else 
			mainSpace.vecast.front()->pEnv->DebugBreaks[fullUDFpath] = breakpoint;
		break;

	//case LVN_KEYDOWN:
	//	lvnkeydown = (LPNMLVKEYDOWN)lParam;
	//	switch(lvnkeydown->wVKey)
	//	{
	//	case VK_RETURN:
	//		lpnmitem = (LPNMITEMACTIVATE) lParam;
	//		marked = ListView_GetSelectionMark(lpnmitem->hdr.hwndFrom);
	//		nItems = ListView_GetSelectedCount(lpnmitem->hdr.hwndFrom);
	//		for (int k=marked; k<marked+nItems; k++)
	//			ListView_GetItemText(lpnmitem->hdr.hwndFrom, k, 0, buf, 256);
	//		SetFocus(GetConsoleWindow());
	//		break;
	//	}
	case NM_CUSTOMDRAW:
  		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
      ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)ProcessCustomDraw(pnm));
        return;
	default:
		break;
	}
}

LRESULT CDebugDlg::ProcessCustomDraw (NMHDR *lParam, bool tick)
{
	int iRow;
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	switch(lplvcd->nmcd.dwDrawStage) 
	{
	case CDDS_PREPAINT : //Before the paint cycle begins
	//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;
            
	case CDDS_ITEMPREPAINT: //Before an item is drawn
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
		iRow = lplvcd->nmcd.dwItemSpec;	
		switch(lplvcd->iSubItem)
		{
		case 0:
			lplvcd->clrText   = RGB(50,50,50);
			lplvcd->clrTextBk = RGB(150,150,150);
			SelectObject(lplvcd->nmcd.hdc, eFont);
			if (pAstSig)
			{
				if (iRow==pAstSig->currentLine-1) 
				{
					lplvcd->clrTextBk = RGB(100,200,40);
					SelectObject(lplvcd->nmcd.hdc, eFont2);
				}
			}
			return CDRF_NEWFONT;
		case 1:
		case 2:
			lplvcd->clrText   = RGB(0,0,0);
			lplvcd->clrTextBk = RGB(243,255,193);
			SelectObject(lplvcd->nmcd.hdc, eFont);
			if (pAstSig)
			{
				if (iRow==pAstSig->currentLine-1) 
				{
					lplvcd->clrTextBk = RGB(100,200,40);
					SelectObject(lplvcd->nmcd.hdc, eFont2);
				}
			}
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

int CDebugDlg::GetBPandUpdate()
{
	set<int> out;
	int nLines = ListView_GetItemCount(hList);
	for (int k=0; k<nLines; k++)
	{
		if (ListView_GetCheckState(hList, k))
			out.insert(k);
	}
//	breakpoint = out;

	return nLines;
}


BOOL CALLBACK DebugBaseProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	if (umsg==WM_INITDIALOG)
	{
		((CDebugBaseDlg*)lParam)->hDlg = hDlg;
		return ((CDebugBaseDlg*)lParam)->OnInitDialog((HWND)wParam, lParam);
	}
	switch (umsg)
	{
//	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, debugBase.OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, debugBase.OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, debugBase.OnClose);
	chHANDLE_DLGMSG (hDlg, WM_DESTROY, debugBase.OnDestroy);
	chHANDLE_DLGMSG (hDlg, WM_SHOWWINDOW, debugBase.OnShowWindow);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, debugBase.OnCommand);

	case WM_NOTIFY:
		debugBase.OnNotify(hDlg, (int)(wParam), (NMHDR *)lParam);
		return 1; // don't I need this???? Check. 8/4/2017
	default:
		return FALSE;
	}
	return TRUE;
}

CDebugBaseDlg::CDebugBaseDlg(void)
{
	//udfname[IDD_PAGE1] = "FILE 1";
	//udfname[IDD_PAGE1] = "ANOTHER";
}

CDebugBaseDlg::~CDebugBaseDlg(void)
{ // This does not get called... find out how to clean debugVct 7/31/2017
	CoUninitialize();
	for (size_t k=0; k<debugVct.size(); k++)
	{
		delete debugVct[k];
	}
}

void CDebugBaseDlg::open_tab_with_UDF(const char *fullname, int shownID)
{ // if shownID is -1, it adds the new tab and shows it at the last (most recent) spot.
  // if shownID is -2, it only adds the new tab without showing --use this inside a loop.
	CREATE_CDebugDlg transfer;
	CDebugDlg *newdbdlg = new CDebugDlg;
	transfer.dbDlg = newdbdlg;
	strcpy(transfer.fullfilename, fullname);
	newdbdlg->hParent = this;
	if (newdbdlg->hDlg = CreateDialogParam (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGE1), debugBase.hDlg, (DLGPROC)debugDlgProc, (LPARAM)&transfer))
		debugVct.push_back(newdbdlg);
	int res = newdbdlg->SetSize();
	_splitpath(fullname, NULL, NULL, fname, NULL);
	if (shownID==-1)
		shownID = (int)::SendMessage(mTab.hTab, TCM_GETITEMCOUNT, 0, 0);
	mTab.AddPage(newdbdlg->hDlg, fname);
	if (shownID!=-2)
	{
		TabCtrl_SetCurSel(mTab.hTab, shownID);
		TabCtrl_SetCurFocus(mTab.hTab, shownID);

		for (size_t k(0); k<mTab.titleStr.size(); k++)
			::ShowWindow(mTab.page[k], SW_HIDE);
		res = TabCtrl_GetCurSel(mTab.hTab);
		::ShowWindow(mTab.hCur=mTab.page[res], SW_SHOW);


#ifdef _DEBUG
		char buf[256];
//		//Hiding the old tabpage and showing the new one
//		int res2 = ::ShowWindow(mTab.hCur, SW_HIDE);
//		sprintf(buf,"old hCur=0x%x,hiding result=%d\n", mTab.hCur, res2);
//		mTab.hCur=mTab.page[shownID];
//		res = ::ShowWindow(mTab.hCur, SW_SHOW);
//		sprintf(buf,"new hCur=0x%x,showing result=%d\n", mTab.hCur, res);
//		strcat(buf,buf2);
		PRINTLOG("c:\\temp\\windows_log.txt", buf)
		char __buf[256];
		sprintf(__buf,"hCur set 0x%x (open_tab_with_UDF)\n", mTab.hCur);
		PRINTLOG("c:\\temp\\windows_log.txt", __buf)
#endif //_DEBUG
	}
}

BOOL CDebugBaseDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	CWndDlg::OnInitDialog(hwndFocus, lParam);
	fileDlg2.InitFileDlg(hDlg, hInst, "");

	hTab = GetDlgItem(IDC_TAB1);
	mTab.initTabcontrol(hTab);
	mTab.mDad = this;
	mTab.hTab = hTab;

#ifdef _DEBUG
		char __buf[256], __buf2[256];
		sprintf(__buf,"CDebugBaseDlg=0x%x\n", hDlg);
		sprintf(__buf2,"Tab Control=0x%x\n", hTab);
		strcat(__buf, __buf2);
 		RECT rt;
		::GetClientRect (hTab, &rt);
		sprintf(__buf2,"Tab size=(%d,%d)\n", rt.right,rt.bottom);
		strcat(__buf, __buf2);
		PRINTLOG("c:\\temp\\windows_log.txt", __buf)
#endif //_DEBUG	

	int firstPage2show(0);
	char errstr[256];
	string strRead;
	if (ReadINI (errstr, iniFile, "DEBUG VIEW", strRead)>0)
	{
		vector<string> tar;
		size_t count = str2vect(tar, strRead.c_str(), "\r\n");
		for (size_t k=0; k<count; k++)
			open_tab_with_UDF(tar[k].c_str(), k==count-1? -1 : -2 );
//		res =::ShowWindow(mTab.hCur, SW_HIDE);
//		res =::ShowWindow(mTab.hCur, SW_SHOW);
//		TabCtrl_SetCurSel (hTab, count-1);
//		mTab.hCur = mTab.page[count-1];
		//for (size_t k=0; k<count; k++)
		//	::ShowWindow(mTab.page[k], k==count-1? SW_SHOW : SW_HIDE);
	}
	else
	{
		fullfname[0]=fname[0]=0;
		if (fileDlg2.FileOpenDlg(fullfname, fname, "AUX UDF file (*.AUX)\0*.aux\0", "aux"))
//		if (FileOpen(fullfname))
			open_tab_with_UDF(fullfname, 0);
		else
		{
			if (GetLastError()!=0) {GetLastErrorStr(errstr); MessageBox (errstr, "File Open dialog box error"); return 0;}
		}
	}

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | 
        COINIT_DISABLE_OLE1DDE);
	return TRUE;

}

char *UnicodeToAnsi(LPCWSTR s, char *dest)
{
	if (s==NULL) return NULL;
	int cw=lstrlenW(s);
	if (cw==0) {CHAR *psz=new CHAR[1];*psz='\0';return psz;}
	int cc=WideCharToMultiByte(CP_ACP,0,s,cw,NULL,0,NULL,NULL);
	if (cc==0) return NULL;
	cc=WideCharToMultiByte(CP_ACP,0,s,cw,dest,cc,NULL,NULL);
	if (cc==0) {return NULL;}
	dest[cc]='\0';
	return dest;
}

int CDebugBaseDlg::FileOpen(char *fullfilename)
{
	int res(0);
	IFileOpenDialog *pFileOpen;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
	hr = pFileOpen->Show(NULL);
	if (SUCCEEDED(hr))
	{
		IShellItem *pItem;
		hr = pFileOpen->GetResult(&pItem);
		if (SUCCEEDED(hr))
		{
			PWSTR pszFilePath;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			// Display the file name to the user.
			if (SUCCEEDED(hr))
			{
				if (UnicodeToAnsi(pszFilePath, fullfilename))
					res=1;
				CoTaskMemFree(pszFilePath);
			}
			pItem->Release();
		}
	}
	pFileOpen->Release();
	return res;
}

void CDebugBaseDlg::OnShowWindow(BOOL fShow, UINT status)
{
 	RECT rt;
	::GetClientRect (hDlg, &rt);
	RECT rt2;
	int res2 = ::GetClientRect (mTab.hTab, &rt2);
//	::MoveWindow(hTab, 0, 0,  rt.right, rt.bottom, 1);
	int res = SetWindowPos(hTab, HWND_BOTTOM, 0, 0, rt.right-rt.left, rt.bottom-rt.top, SWP_SHOWWINDOW );
	res2 = ::GetClientRect (mTab.hTab, &rt2);
//	That is NOT the same as ::MoveWindow(hTab, 0, 0,  rt.right, rt.bottom, 1); ==> This changes the z-order and puts hTab on TOP
}

void CDebugBaseDlg::OnSize(UINT state, int cx, int cy)
{ // This is only in response to user re-sizing of debug window
	::MoveWindow(hTab, 0, 0, cx, 50, 1);
  //  SetWindowPos(hTab, HWND_TOPMOST, 0, 0, cx, cy, SWP_SHOWWINDOW);
// 	RECT rt;
//	::GetClientRect (hDlg, &rt);
//	int iSel = TabCtrl_GetCurSel(hTab);
	for (map<string, CDebugDlg*>::iterator it=dbmap.begin(); it!=dbmap.end(); it++)
	{
		it->second->SetSize();

	}
//	::ShowWindow(mTab.hCur, SW_SHOW);
	
}

void CDebugBaseDlg::OnClose()
{
	if (MessageBox("If you close this, all udfs opened will be closed.", "Are you sure?", MB_YESNO)==IDYES)
		OnDestroy();
}

void CDebugBaseDlg::OnDestroy()
{
//	::SendMessage(GetParent(hDlg), WM_DEBUG_CLOSE, 0, 0);
	PostThreadMessage(GetWindowThreadProcessId(hDlg, NULL), WM__ENDTHREAD, 0, 0);
}

void CDebugBaseDlg::OnCommand(int idc, HWND hwndCtl, UINT event)
{
	switch(idc)
	{
	case IDCANCEL:
		OnClose();
		break;
	}
}

void CDebugBaseDlg::OnNotify(HWND hwnd, int idcc, NMHDR *pnm)
{
	int res(0), iPage;
	LPNMLVKEYDOWN lvnkeydown = (LPNMLVKEYDOWN)pnm;
	if (idcc==IDC_TAB1) 
	{
		mTab.OnNotify(pnm);
	}
	switch(pnm->code)
	{
	case TCN_SELCHANGING:
        ::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG)FALSE); // TO RETURN FALSE
        return;
	case TCN_SELCHANGE:
		iPage = TabCtrl_GetCurSel(hTab); 
		break;
	case NM_DBLCLK:
		break;
	case LVN_ITEMCHANGED:
		break;
	case LVN_KEYDOWN:
		switch(lvnkeydown->wVKey)
		{
		case VK_SPACE:
			break;
		case VK_DELETE:
			break;
		}
		break;
	}
}

BOOL CALLBACK TabPage_DlgProc(HWND hwndDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	static int mouseclicked(0);
	mTab.hCur = hwndDlg;
	switch (umsg)
	{
		chHANDLE_DLGMSG(hwndDlg, WM_INITDIALOG, mTab.OnInitDialog);
		chHANDLE_DLGMSG(hwndDlg, WM_SIZE, mTab.OnSize);
		chHANDLE_DLGMSG(hwndDlg, WM_COMMAND, mTab.OnCommand);
		//// TODO: Add TabPage dialog message crackers here...

		//default:
		//{
		//	m_lptc->ParentProc(hwndDlg, msg, wParam, lParam);
		//	return FALSE;
		//}
	}
	return 0;
}

