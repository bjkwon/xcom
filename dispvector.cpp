#include "wxx_wincore.h" // Win32++ 8.2. This must be placed prior to <windows.h> to avoid including winsock.h

#include <process.h>
#include "showvar.h"
#include "resource1.h"
#include "audfret.h"
#include "audstr.h"

extern CWndDlg wnd;
extern CShowvarDlg mShowDlg;
extern char udfpath[4096];
extern void* soundplayPt;
extern double block;
extern vector<CWndDlg*> cellviewdlg;

map<string, CRect> dlgpos;

BOOL CALLBACK vectorsheetDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	// This calls original OnInitDialog(), setting up hinst, AppPath, etc.. Derived OnInitDialog is to be called separately after CreateWindow
	if (umsg == WM_INITDIALOG) return wnd.OnInitDialog((HWND)(wParam), lParam);

	bool alreadymade(false);
	CVectorsheetDlg *cvDlg(NULL);
	for (vector<CWndDlg*>::iterator it=mShowDlg.m_sheets.begin(); it!=mShowDlg.m_sheets.end(); it++)
		if ( (*it)->hDlg == hDlg) 
		{	cvDlg = (CVectorsheetDlg *)*it; alreadymade=true; 	break;}
	if (cvDlg==NULL)
	{
		for (vector<CWndDlg*>::iterator it=cellviewdlg.begin(); it!=cellviewdlg.end(); it++)
		{
			for (vector<CWndDlg*>::iterator jt= ((CShowvarDlg *)(*it))->m_sheets.begin(); jt!= ((CShowvarDlg *)(*it))->m_sheets.end(); jt++)
//				if ( (*jt)->hDlg == hDlg) 
				{	cvDlg = (CVectorsheetDlg *)*jt; alreadymade=true; 	break;}
		}
	}
	if (cvDlg!=NULL) 
	{
		switch (umsg)
		{
		chHANDLE_DLGMSG (hDlg, WM_MOVE, cvDlg->OnMove);
		chHANDLE_DLGMSG (hDlg, WM_SIZE, cvDlg->OnSize);
		chHANDLE_DLGMSG (hDlg, WM_CLOSE, cvDlg->OnClose);
		chHANDLE_DLGMSG (hDlg, WM_SHOWWINDOW, cvDlg->OnShowWindow);
		chHANDLE_DLGMSG (hDlg, WM_COMMAND, cvDlg->OnCommand);
		case WM_NOTIFY:
			cvDlg->OnNotify(hDlg, (int)(wParam), lParam);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	else
		return FALSE; // this is in fact impossible case, but just keep it....
}

CVectorsheetDlg::CVectorsheetDlg(void)
{

}

CVectorsheetDlg::~CVectorsheetDlg(void)
{

}

BOOL CVectorsheetDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	SetWindowText((char*)lParam);
	RECT rt,rt2,rt3,rt4;
	GetWindowRect(&rt);
	GetClientRect(hDlg, &rt2);
	::GetWindowRect(GetDlgItem(IDC_LISTVECTOR),&rt3);
	::MoveWindow(GetDlgItem(IDC_LISTVECTOR), rt2.left, rt2.top, rt2.right-rt2.left, rt2.bottom-rt2.top, TRUE);
	hParent->GetWindowRect(&rt4);
	CRect rt0(rt4.right+10, (rt4.top*2+rt4.bottom)/3, rt4.right+10+145, (rt4.top*2+rt4.bottom)/3+(rt.bottom-rt.top));
	for (map<string, CRect>::reverse_iterator it=dlgpos.rbegin(); it!=dlgpos.rend(); it++)
	{
		if (rt0.left==it->second.left && rt0.top==it->second.top) { rt0.OffsetRect(20,32); it=dlgpos.rbegin();}
	}
	MoveWindow(&rt0, TRUE);
	dlgpos[(char*)lParam] = rt0;
	return TRUE;
}

void CVectorsheetDlg::FillupVector(CSignals &sig)
{
	int width[]={45, 100,};
	char buf[64];
	SendDlgItemMessage(IDC_LISTVECTOR,LVM_DELETEALLITEMS,0,0);
	while (SendDlgItemMessage(IDC_LISTVECTOR,LVM_DELETECOLUMN,0,0)) {};
	LvItem.mask=LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 48; // Max size of text
	SendDlgItemMessage(IDC_LISTVECTOR,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);

	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	LvCol.cx=width[0];	LvCol.pszText="index";
	SendDlgItemMessage(IDC_LISTVECTOR,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); 
	LvCol.cx=width[1];	LvCol.pszText="value";
	SendDlgItemMessage(IDC_LISTVECTOR,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol); 
	LvItem.iItem=0; LvItem.iSubItem=0; 
	for(int k = sig.length()-1; k>=0; k--)
	{
		LvItem.pszText=itoa(k+1,buf,10);
		SendDlgItemMessage(IDC_LISTVECTOR,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	}
	bool ch(sig.GetType()==CSIG_STRING);
	bool cmplx(sig.IsComplex());
	bool logic(sig.IsLogical());
	int swsdf= sig.length();
	for(int k = sig.length()-1; k>=0; k--)
	{
		LvItem.iItem=k; 
		LvItem.iSubItem=1; 
		if (ch) sprintf(buf,"%c (%u)", (char)sig.strbuf[k], (unsigned char)sig.strbuf[k]);
		else if (cmplx) sprintf(buf,"%g %+g * i", sig.buf[2*k], sig.buf[2*k+1]);
		else if (logic) sprintf(buf,"%d", sig.logbuf[k]);
		else sprintf(buf,"%g", sig.buf[k]);
		LvItem.pszText=buf;
		SendDlgItemMessage(IDC_LISTVECTOR,LVM_SETITEM,0,(LPARAM)&LvItem);
	}
}

void CVectorsheetDlg::OnShowWindow(BOOL fShow, UINT status)
{
}

void CVectorsheetDlg::OnMove(int cx, int cy)
{
	char buf[64];
	GetWindowText(buf, sizeof(buf));
	RECT rt;
	GetWindowRect(&rt);
	dlgpos[buf] = rt;
}

void CVectorsheetDlg::OnSize(UINT state, int cx, int cy)
{
	::MoveWindow(hList1, 0, 30, cx, cy-30, NULL);
	int width[4];
	int ewas(0);
	for (int k=0; k<4; k++) width[k] = ListView_GetColumnWidth(hList1, k);
	for (int k=0; k<3; k++)  ewas += width[k];
	int newwidth = cx - ewas-20;
	int res = ListView_SetColumnWidth(hList1, 3, newwidth);
	RECT rt,rt2,rt3;
	GetWindowRect(&rt);
	GetClientRect(hDlg, &rt2);
	::GetWindowRect(GetDlgItem(IDC_LISTVECTOR),&rt3);
	::MoveWindow(GetDlgItem(IDC_LISTVECTOR), rt2.left, rt2.top, rt2.right-rt2.left, rt2.bottom-rt2.top, TRUE);
	char buf[64];
	GetWindowText(buf, sizeof(buf));
	dlgpos[buf] = rt;
}

void CVectorsheetDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}

void CVectorsheetDlg::OnDestroy()
{

}

void CVectorsheetDlg::OnCommand(int idc, HWND hwndCtl, UINT event)
{
	string addp;
	switch(idc)
	{
	case IDCANCEL:
		OnClose();
		break;
	}
}

void CVectorsheetDlg::OnNotify(HWND hwnd, int idcc, LPARAM lParam)
{
	int res(0);
	static char buf[256];
	LPNMHDR pnm = (LPNMHDR)lParam;
	LPNMLISTVIEW pview = (LPNMLISTVIEW)lParam;
	LPNMLVKEYDOWN lvnkeydown;
	UINT code=pnm->code;
	LPNMITEMACTIVATE lpnmitem;
	static int clickedRow;
	switch(code)
	{
	case NM_CLICK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		break;
	case NM_DBLCLK:
		lpnmitem = (LPNMITEMACTIVATE) lParam;
		clickedRow = lpnmitem->iItem;
		break;
	case LVN_KEYDOWN:
		lvnkeydown = (LPNMLVKEYDOWN)lParam;
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

LRESULT CVectorsheetDlg::ProcessCustomDraw (NMHDR *lParam)
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