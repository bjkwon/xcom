#include "wxx_wincore.h" // Win32++ 8.2. This must be placed prior to <windows.h> to avoid including winsock.h
#include "debugDlg.h"
#include "resource1.h"
#include "audfret.h"

#ifndef SIGPROC
#include "sigproc.h"
#endif

#include "xcom.h"
#include "consts.h"

extern xcom mainSpace;
extern HANDLE hStdin, hStdout; 

#define WM__DEBUG	WM_APP+3321 // do something later 6/4/2017 6pm

CDebugDlg mDebug;

map<string, CDebugDlg*> dbmap;

int spyWM(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam, char* const fname, vector<UINT> msg2excl, char* const tagstr);


BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	char buf0[256], fname[256];
	vector<string> lines;
	CREATE_CDebugDlg *ptransfer = (CREATE_CDebugDlg *)lParam;
	strcpy(buf0, "[Debug] ");
	strcat(buf0, ptransfer->fullfilename);
	SetWindowText(GetParent(hwndFocus), buf0);
	_splitpath(ptransfer->fullfilename, NULL, NULL, fname, buf0);
	dbmap[fname] = ptransfer->dbDlg;

	ptransfer->dbDlg->udfname = fname;

	ptransfer->dbDlg->hDlg = GetParent(hwndFocus);
	ptransfer->dbDlg->lvInit();

	FILE *fp = fopen(ptransfer->fullfilename, "rt");
	if (fp!=NULL)
	{
		fseek (fp, 0, SEEK_END);
		int filesize=ftell(fp);
		fseek (fp, 0, SEEK_SET);
		char *buf = new char[filesize+1];
		size_t res = fread(buf, 1, (size_t)filesize, fp);
		buf[res]=0;
		fclose(fp);
		size_t res2 = str2vect(lines, buf, "\r\n");
		delete[] buf;
	}
	else
	{
		//Error handle
	}
	ptransfer->dbDlg->FillupContent(lines);
	return TRUE;
}

extern vector<UINT> exc; //temp

BOOL CALLBACK debugDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
//	spyWM(hDlg, umsg, wParam, lParam, "c:\\temp\\rec", exc, "[debugDlgProc]");

	CDebugDlg *p_db(NULL); 
	int line(-1);
	for (map<string, CDebugDlg*>::iterator it = dbmap.begin(); it!=dbmap.end(); it++)
		if (hDlg == it->second->hDlg) p_db = it->second;
	if (!p_db && umsg!=WM_INITDIALOG) return FALSE;
	switch (umsg)
	{
	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, p_db->OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, p_db->OnClose);
	chHANDLE_DLGMSG (hDlg, WM_DESTROY, p_db->OnDestroy);
	chHANDLE_DLGMSG (hDlg, WM_SHOWWINDOW, p_db->OnShowWindow);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, p_db->OnCommand);
	case WM_NOTIFY:
		p_db->OnNotify(hDlg, (int)(wParam), lParam);
		break;
	case WM__DEBUG:
		if (wParam) p_db->inDebug = true;
		else p_db->inDebug = false;
		if ((DEBUG_STATUS)wParam==entering) 
			p_db->pabcast = (CAstSig *)lParam;
		else if ((DEBUG_STATUS)wParam==stepping) 
			line = (int)lParam;
		p_db->OnDebug((DEBUG_STATUS)wParam, line);

		break;
	default:
		return FALSE;
	}
	return TRUE;
}

CDebugDlg::CDebugDlg(void)
:inDebug(false)
{

}

CDebugDlg::~CDebugDlg(void)
{
//	dbmap.erase(udfname);
}

void CDebugDlg::OnDebug(DEBUG_STATUS type, int line)
{
	int curLine;
	switch(type)
	{
	case entering:
		break;
	case progress:
		ListView_SetItemState (hList, (lastLine = pabcast->endLine)-1,  LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		SetFocus(hList);
		break;
	case stepping:
		ListView_SetItemState (hList, lastLine-1,  0, 0x000F);
		ListView_SetItemState (hList, (lastLine = line)-1,  LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		SetFocus(hList);
		curLine = ListView_SetSelectionMark(hList, line-1);
		break;
	case exiting:
		ListView_SetItemState (hList, lastLine-1,  0, 0x000F);
		break;
	}
}

void CDebugDlg::FillupContent(vector<string> in)
{
	char buf[256];
	int width[]={50, 500, };
	LRESULT res;
	for(int k = 0; k<2; k++)
	{
		LvCol.cx=width[k];
		LoadString(hInst, IDS_DEBUGBOX1+k, buf, sizeof(buf));
		LvCol.pszText=buf;
		res = ::SendMessage(hList, LVM_INSERTCOLUMN,k,(LPARAM)&LvCol); 
	}
	for (LvItem.iItem=0; LvItem.iItem<(int)in.size(); LvItem.iItem++)
	{
		itoa(LvItem.iItem+1, buf, 10);
		LvItem.iSubItem=0; //First item (InsertITEM)
		LvItem.pszText=buf;
		::SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

		LvItem.iSubItem++; //Second column (SetITEM)
		strcpy(buf,in[LvItem.iItem].c_str());
		::SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
//	res = ListView_Scroll(hList,0,ListView_GetItemCount(hList)*14);
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
	CWndDlg::OnInitDialog(hDlg, NULL);
}

void CDebugDlg::OnSize(UINT state, int cx, int cy)
{
	::MoveWindow(GetDlgItem(IDC_LISTDEBUG), 0, 0, cx, cy, 1);
	int res = ListView_SetColumnWidth(hList, 3, cx);
}

void CDebugDlg::OnClose()
{
// Once debug window is created, it stays until the end of application (when closing, it becomes only hidden)
	ShowWindow(SW_HIDE);

	// Then bring it back ---ShowWindow(SW_SHOW); if it hits the breakpoint.... do this tomorrow 7/3/2017 bjk

}

void CDebugDlg::OnDestroy()
{
	::SendMessage(GetParent(hDlg), WM_DEBUG_CLOSE, (WPARAM)udfname.c_str(), 0);
}

void CDebugDlg::OnCommand(int idc, HWND hwndCtl, UINT event)
{
	char buf[256];
	DWORD dw;
	int res, marked, lineChecked;
	switch(idc)
	{
	case ID_F10: //from accelerator event should be 1
		strcpy(buf, "#step ");
	case ID_F5: //from accelerator event should be 1
		if (idc==ID_F5) 
			strcpy(buf, "#cont "), Beep(150,400);
		for (int k=0; k<6;k++)
			mainSpace.debug_command_frame[k].Event.KeyEvent.uChar.AsciiChar = buf[k];
		res = WriteConsoleInput(hStdin, mainSpace.debug_command_frame, 6, &dw);
		break;

	case ID_F9: //from accelerator event should be 1
		marked = ListView_GetSelectionMark(hList);
		lineChecked = ListView_GetCheckState(hList, marked);
		ListView_SetCheckState(hList, marked, lineChecked ? 0:1);
		break;

	case IDCANCEL:
	//	OnClose();
		break;
	}
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
	int marked, clickedRow;
	int res(0), lineChecked;
	switch(code)
	{
	case NM_CLICK:
		//lpnmitem = (LPNMITEMACTIVATE) lParam;
		//clickedRow = lpnmitem->iItem;
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
		if (clickedRow==0) {ListView_SetCheckState(hList, 0, 0); break;}
		lineChecked = ListView_GetCheckState(hList, clickedRow);
		if (lpnmitem->uNewState>=LVIF_DI_SETITEM && !lpnmitem->uOldState) break;
		if ( (lpnmitem->uNewState & LVIS_SELECTED) && !lineChecked) 	break;
		it = find(breakpoint.begin(), breakpoint.end(), clickedRow+1);
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
		for (vector<CAstSig*>::iterator it=mainSpace.vecast.begin(); it!=mainSpace.vecast.end(); it++)
		{
			CAstSig *past = *it;
			if (breakpoint.empty())
				past->pEnv->DebugBreaks.erase(udfname);
			else 
				past->pEnv->DebugBreaks[udfname] = breakpoint;
		}
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
        ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)ProcessCustomDraw(pnm));
        return;
	default:
		break;
	}
}

LRESULT CDebugDlg::ProcessCustomDraw (NMHDR *lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	switch(lplvcd->nmcd.dwDrawStage) 
	{
	case CDDS_PREPAINT : //Before the paint cycle begins
	//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;
            
	case CDDS_ITEMPREPAINT: //Before an item is drawn
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
		switch(lplvcd->iSubItem)
		{
		case 0:
			lplvcd->clrText   = RGB(255,255,0);
			lplvcd->clrTextBk = RGB(0,0,0);
			return CDRF_NEWFONT;
		case 1:
		case 2:
			lplvcd->clrText   = RGB(20,26,158);
			lplvcd->clrTextBk = RGB(200,200,10);
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