#include "wxx_wincore.h" // Win32++ 8.2. This must be placed prior to <windows.h> to avoid including winsock.h
#include "histDlg.h"
#include "resource1.h"
#include "audfret.h"

extern CHistDlg mHistDlg;

void computeandshow(char *AppPath, int code, char *buf);
HWND CreateTT(HINSTANCE hInst, HWND hParent, RECT rt, char *string, int maxwidth=400);

BOOL CALLBACK historyDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	switch (umsg)
	{
	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, mHistDlg.OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, mHistDlg.OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, mHistDlg.OnClose);

	chHANDLE_DLGMSG (hDlg, WM_SHOWWINDOW, mHistDlg.OnShowWindow);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, mHistDlg.OnCommand);
	case WM_NOTIFY:
		mHistDlg.OnNotify(hDlg, (int)(wParam), lParam);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

CHistDlg::CHistDlg(void)
{

}

CHistDlg::~CHistDlg(void)
{

}

BOOL CHistDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	CWndDlg::OnInitDialog(hwndFocus, lParam);
//	FILE *fpp = fopen("track.txt","at"); fprintf(fpp,"from histview, parent = %x\n", GetParent(hDlg)); fclose(fpp);
	lvInit();
	char buf[256];
	DWORD dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(mHistDlg.logfilename, "%s%s%s_%s.log", AppPath, AppName, HISTORY_FILENAME, buf);

	vector<string> lines;
	FILE *fp = fopen(logfilename, "rt");
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
		FillupHist(lines);
	}
	else
	{
		// Keep the following lines until I am sure that the initial screen anomaly is gone. 10/2/2016 bjk
		//fp=fopen("hist_disappearing_track.txt","at");
		//fprintf(fp,"history file open error.\n");
		//fclose(fp);

		//fpp = fopen("track.txt","at"); fprintf(fpp,"histview = %x\n", hDlg); fclose(fpp);
		FillupHist(lines);
	}
	
	return TRUE;
}


void CHistDlg::lvInit()
{
	LvItem.mask=LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 256; // Max size of text
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	hList = GetDlgItem(IDC_LISTHIST);
	::SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);
	RECT rt;
	char buf[256];
	::GetClientRect (hList, &rt);
	LoadString (hInst, IDS_HELP_HISTORY, buf, sizeof(buf));
	CreateTT(hInst, hList, rt, buf, 270);
}

void CHistDlg::OnShowWindow(BOOL fShow, UINT status)
{

}

void CHistDlg::OnSize(UINT state, int cx, int cy)
{
	::MoveWindow(GetDlgItem(IDC_LISTHIST), 0, 0, cx, cy, 1);
	int res = ListView_SetColumnWidth(hList, 3, cx);
}

void CHistDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}

void CHistDlg::OnDestroy()
{

}

void CHistDlg::OnCommand(int idc, HWND hwndCtl, UINT event)
{
}

int char2INPUT_RECORD(char *buf, INPUT_RECORD ir[])
{
	unsigned int k(0), id(0);
	HKL hkl = GetKeyboardLayout (0);
	SHORT mix;
	for (; id<strlen(buf); k++, id++)
	{
		ir[k].EventType = KEY_EVENT;
		ir[k].Event.KeyEvent.bKeyDown = 1;
		mix = VkKeyScanEx(buf[id], hkl);

		(HIBYTE(mix)==1) ? ir[k].Event.KeyEvent.dwControlKeyState = SHIFT_PRESSED : ir[k].Event.KeyEvent.dwControlKeyState = 0;
		ir[k].Event.KeyEvent.uChar.AsciiChar = buf[id];
		ir[k].Event.KeyEvent.wVirtualKeyCode = LOBYTE(mix);
		ir[k].Event.KeyEvent.wVirtualScanCode = MapVirtualKey (LOBYTE(mix), MAPVK_VK_TO_VSC);
		ir[k].Event.KeyEvent.wRepeatCount = 1;

		if (HIBYTE(mix)==0) {
			k++;
			ir[k] = ir[k-1];
			ir[k].Event.KeyEvent.bKeyDown = 0;
		}
	}
	//This simply marks the end for WriteConsoleInput, not for real "return"
	ir[k].EventType = KEY_EVENT;
	ir[k].Event.KeyEvent.bKeyDown = TRUE;
	ir[k].Event.KeyEvent.dwControlKeyState = 0;
	ir[k].Event.KeyEvent.uChar.AsciiChar = VK_RETURN;
	ir[k].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
	ir[k].Event.KeyEvent.wRepeatCount = 1;
	ir[k].Event.KeyEvent.wVirtualScanCode = MapVirtualKey (VK_RETURN, MAPVK_VK_TO_VSC);
	return k+1;
}

void CHistDlg::OnNotify(HWND hwnd, int idcc, LPARAM lParam)
{
	int res(0);
	DWORD dw;
	static char buf[256];
	LPNMHDR pnm = (LPNMHDR)lParam;
	LPNMLISTVIEW pview = (LPNMLISTVIEW)lParam;
	LPNMLVKEYDOWN lvnkeydown;
	UINT code=pnm->code;
	LPNMITEMACTIVATE lpnmitem;
	static int clickedRow;
	int marked;
	INPUT_RECORD ir[256];
	switch(code)
	{
	case NM_CLICK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		break;
	case NM_DBLCLK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		marked = ListView_GetSelectionMark(lpnmitem->hdr.hwndFrom);
		ListView_GetItemText(lpnmitem->hdr.hwndFrom, clickedRow, 0, buf, 256);
		WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), ir, char2INPUT_RECORD(buf, ir), &dw);
		break;
	case LVN_KEYDOWN:
		lvnkeydown = (LPNMLVKEYDOWN)lParam;
		if (lvnkeydown->wVKey==VK_RETURN)
		{
			lpnmitem = (LPNMITEMACTIVATE) lParam;
			marked = ListView_GetSelectionMark(lpnmitem->hdr.hwndFrom);
			int nItems = ListView_GetSelectedCount(lpnmitem->hdr.hwndFrom);
			for (int k=marked; k<marked+nItems; k++)
			{
				ListView_GetItemText(lpnmitem->hdr.hwndFrom, k, 0, buf, 256);
				res = char2INPUT_RECORD(buf, ir);
				if (nItems>1 && k!=marked+nItems-1) 
					res = WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), ir, res, &dw);
				else
					res = WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), ir, res-1, &dw);
			}
			SetFocus(GetConsoleWindow());
		}
		break;
	case NM_CUSTOMDRAW:
        ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)ProcessCustomDraw(pnm));
        return;
	default:
		break;
	}
}

LRESULT CHistDlg::ProcessCustomDraw (NMHDR *lParam)
{
	return CDRF_DODEFAULT;
}

void CHistDlg::FillupHist(vector<string> in)
{
	char buf[256];
	int width[]={400, };
	for(int k = 0; k<1; k++)
	{
		LvCol.cx=width[k];
		::SendMessage(hList, LVM_INSERTCOLUMN,k,(LPARAM)&LvCol); 
	}
	LvItem.iItem=0;
	LvItem.iSubItem=0; //First item (InsertITEM)
	for (unsigned int k=0; k<in.size(); k++)
	{
		if (in[k].find("//")!=0)
		{
			strcpy(buf,in[k].c_str());
			LvItem.pszText=buf;
			LvItem.iItem=k;
			SendDlgItemMessage(IDC_LISTHIST,LVM_INSERTITEM,0,(LPARAM)&LvItem);
		}
	}
	ListView_Scroll(hList,0,ListView_GetItemCount(hList)*14);
}
void CHistDlg::AppendHist(char *in)
{
	LvItem.pszText=in;
	LvItem.iItem=ListView_GetItemCount(hList)+1;
	SendDlgItemMessage(IDC_LISTHIST,LVM_INSERTITEM,0,(LPARAM)&LvItem);
}
