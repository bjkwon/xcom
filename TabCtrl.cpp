#include "wxx_wincore.h" // Win32++ 8.2. This must be placed prior to <windows.h> to avoid including winsock.h
#include "showvar.h"

#include <vector>
#include <string>
#include "TabCtrl.h"

using namespace std;


static VOID TabPageMessageLoop(HWND hwnd)
{
	MSG msg;
	int status;
	BOOL handled = FALSE;
	BOOL fFirstStop = FALSE;
	HWND hFirstStop;

	while ((status = GetMessage(&msg, NULL, 0, 0)))
	{
		if (-1 == status)	// Exception
		{
			return;
		}
		else
		{
			//This message is explicitly posted from TabCtrl_OnSelChanged() to
			// indicate the closing of this page.  Stop the Loop.
			if (WM_SHOWWINDOW == msg.message && FALSE == msg.wParam)
				return;

			//IsDialogMessage() dispatches WM_KEYDOWN to the tab page's child controls
			// so we'll sniff them out before they are translated and dispatched.
			if (WM_KEYDOWN == msg.message && VK_TAB == msg.wParam)
			{
				//Tab each tabstop in a tab page once and then return to to
				// the tabCtl selected tab
				if (!fFirstStop)
				{
					fFirstStop = TRUE;
					hFirstStop = msg.hwnd;
				}
				else if (hFirstStop == msg.hwnd)
				{
					// Tab off the tab page
					HWND hTab = (HWND)GetWindowLongPtr(GetParent(msg.hwnd), GWLP_USERDATA);
					if (NULL == hTab)
						hTab = hTab;
					SetFocus(hTab);
					return;
				}
			}
			// Perform default dialog message processing using IsDialogM. . .
			handled = IsDialogMessage(hwnd, &msg);

			// Non dialog message handled in the standard way.
			if (!handled)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	// If we get here window is closing
	PostQuitMessage(0);
	return;
}

CTabControl::CTabControl()
{
}

void CTabControl::FirstTabstop_SetFocus(HWND h)
{
	FORWARD_WM_NEXTDLGCTL(h, 1, FALSE, SendMessage);
	FORWARD_WM_KEYDOWN(GetFocus(), VK_TAB, 0, 0, PostMessage);
}

void CTabControl::initTabcontrol(HWND h)
{
	hTab = h;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
}

int CTabControl::AddPage(HWND hNewpage, string title)
{
	char buf[256];
	strcpy(buf, title.c_str());
	char *pt=strchr(buf, '.');
	if (pt) pt[0]=0;
	tie.pszText = buf;
	TabCtrl_InsertItem(hTab, TabCtrl_GetItemCount(hTab), &tie);
	page.push_back(hNewpage);
	titleStr.push_back(buf);
	return 1;
}

BOOL CTabControl::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{

	return 1;
}

int CTabControl::GetCurrentPageID()
{
	CDebugBaseDlg *tp = (CDebugBaseDlg *)mDad;
	for (map<int,HWND>::iterator it=tp->dlgPage_hwnd.begin(); it!=tp->dlgPage_hwnd.end(); it++)
	{
		int id = it->first;
		if (hCur== it->second)
			return id;
	}
	for (size_t k(0); k<page.size(); k++)
		if (hCur==page[k]) 
			return (int)k;
	return -1;
}

void CTabControl::OnSize(UINT state, int cx, int cy)
{
	RECT wndrct;
	int wndWidth, wndHeight;
	int recWidth, recHeight;
	switch(GetCurrentPageID())
	{
	case 1:
		GetClientRect(hCur, &wndrct);
		wndWidth=wndrct.right-wndrct.left, wndHeight=wndrct.bottom-wndrct.top;
		break;
	case 2: // freq range
		GetClientRect(hCur, &wndrct);
		wndWidth=wndrct.right-wndrct.left;
		wndHeight=wndrct.bottom-wndrct.top;
		recWidth = wndWidth/15;
		recHeight = wndHeight/7;
		CPoint pt1(wndrct.left + recWidth, wndrct.top+recHeight*3);
		CPoint pt2(pt1.x + recWidth, pt1.y+recHeight);
		break;
	}
}


void CTabControl::OnCommand(INT id, HWND hwndCtl, UINT codeNotify)
{
	vector<string> str4pipe;
	string argstr, argstr2;
	int tabID;
	CRect rct;
	switch((tabID=GetCurrentPageID()))
	{
	case 1:
		break;
	case 2:
		break;
	}

	//Forward all commands to the parent proc.
	//Note: We don't use SendMessage because on the receiving end,
	// that is: _OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify),
	// hwnd would = addressee and not the parent of the control.  Intuition
	// favors the notion that hwnd is the parent of hwndCtl.
//	FORWARD_WM_COMMAND(hTab, id, hwndCtl, codeNotify, mappyDlgProc);

	// If this WM_COMMAND message is a notification to parent window
	// ie: EN_SETFOCUS being sent when an edit control is initialized
	// do not engage the Message Loop or send any messages.
	if (codeNotify != 0)
		return;

	// Mouse clicks on a control should engage the Message Loop
	SetFocus(hwndCtl);
	FirstTabstop_SetFocus(hTab);
	TabPageMessageLoop(hTab);
}




void CTabControl::TabControl_GetClientRect(RECT * prc)
{
	RECT rtab_0;
	LONG lStyle = GetWindowLong(hTab, GWL_STYLE);

	// Calculate the tab control's display area
	GetWindowRect(hTab, prc);
	ScreenToClient(GetParent(hTab), (POINT *) & prc->left);
	ScreenToClient(hTab, (POINT *) & prc->right);
	TabCtrl_GetItemRect(hTab, 0, &rtab_0);	//The tab itself

	if ((lStyle & TCS_BOTTOM) && (lStyle & TCS_VERTICAL))	//Tabs to Right
	{
		prc->top = prc->top + 6;	//x coord
		prc->left = prc->left + 4;	//y coord
		prc->bottom = prc->bottom - 12;	// height
		prc->right = prc->right - (12 + rtab_0.right - rtab_0.left);	// width
	}
	else if (lStyle & TCS_VERTICAL)	//Tabs to Left
	{
		prc->top = prc->top + 6;	//x coord
		prc->left = prc->left + (4 + rtab_0.right - rtab_0.left);	//y coord
		prc->bottom = prc->bottom - 12;	// height
		prc->right = prc->right - (12 + rtab_0.right - rtab_0.left);	// width
	}
	else if (lStyle & TCS_BOTTOM)	//Tabs on Bottom
	{
		prc->top = prc->top + 6;	//x coord
		prc->left = prc->left + 4;	//y coord
		prc->bottom = prc->bottom - (16 + rtab_0.bottom - rtab_0.top);	// height
		prc->right = prc->right - 12;	// width
	}
	else	//Tabs on top
	{
		prc->top = prc->top + (6 + rtab_0.bottom - rtab_0.top);	//x coord
		prc->left = prc->left + 4;	//y coord
		prc->bottom = prc->bottom - (16 + rtab_0.bottom - rtab_0.top);	// height
		prc->right = prc->right - 12;	// width
	}
}

BOOL CTabControl::CenterTabPage(int iPage)
{
	RECT rect, rclient;

	TabControl_GetClientRect(&rect);	// left, top, width, height

	// Get the tab page size
	GetClientRect(page[iPage], &rclient);
	rclient.right = rclient.right - rclient.left;	// width
	rclient.bottom = rclient.bottom - rclient.top;	// height
	rclient.left = rect.left;
	rclient.top = rect.top;

	// Center the tab page, or cut it off at the edge of the tab control(bad)
	if (rclient.right < rect.right)
		rclient.left += (rect.right - rclient.right) / 2;

	if (rclient.bottom < rect.bottom)
		rclient.top += (rect.bottom - rclient.bottom) / 2;

	// Move the child and put it on top
	return SetWindowPos(page[iPage], HWND_TOP, rclient.left, rclient.top, rclient.right, rclient.bottom, 0);
}

BOOL CTabControl::StretchTabPage(int iPage)
{
	RECT rect, rectBase;
	::GetClientRect (mDad->hDlg, &rectBase);
	TabControl_GetClientRect(&rect);	// left, top, width, height

	// Move the child and put it on top
	return SetWindowPos(page[iPage], HWND_TOP, rect.left, rect.top, rectBase.right, rectBase.bottom, 0);
}

BOOL CTabControl::OnNotify(LPNMHDR pnm)
{
	//page[0].OnNotify(pnm); // probably this is not necessary. 7/31/2016 bjk

	switch (pnm->code)
	{
		case TCN_KEYDOWN:
			TabCtrl_OnKeyDown((LPARAM)pnm);

		// fall through to call TabCtrl_OnSelChanged() on each keydown
		case TCN_SELCHANGE:
			return TabCtrl_OnSelChanged();
	}
	return FALSE;
}

void CTabControl::TabCtrl_OnKeyDown(LPARAM lParam)
{
	TC_KEYDOWN *tk = (TC_KEYDOWN *)lParam;
	int itemCount = TabCtrl_GetItemCount(tk->hdr.hwndFrom);
	int currentSel = TabCtrl_GetCurSel(tk->hdr.hwndFrom);

	if (itemCount <= 1)
		return;	// Ignore if only one TabPage

	BOOL verticalTabs = GetWindowLong(hTab, GWL_STYLE) & TCS_VERTICAL;

	if (verticalTabs)
	{
		switch (tk->wVKey)
		{
			case VK_PRIOR:	//select the previous page
				if (0 == currentSel)
					return;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel - 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel - 1);
				break;

			case VK_UP:	//select the previous page
				if (0 == currentSel)
					return;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel - 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel);
				break;

			case VK_NEXT:	//select the next page
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel + 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel + 1);
				break;

			case VK_DOWN:	//select the next page
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel + 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel);
				break;

			case VK_LEFT:	//navagate within selected child tab page
			case VK_RIGHT:
				SetFocus(page[currentSel]);
				FirstTabstop_SetFocus(page[currentSel]);
				TabPageMessageLoop(page[currentSel]);
				break;

			default:
				return;
		}
	}	

	else	// horizontal Tabs
	{
		switch (tk->wVKey)
		{
			case VK_NEXT:	//select the previous page
				if (0 == currentSel)
					return;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel - 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel - 1);
				break;

			case VK_LEFT:	//select the previous page
				if (0 == currentSel)
					return;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel - 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel);
				break;

			case VK_PRIOR:	//select the next page
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel + 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel + 1);
				break;

			case VK_RIGHT:	//select the next page
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel + 1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel);
				break;

			case VK_UP:	//navagate within selected child tab page
			case VK_DOWN:
				SetFocus(page[currentSel]);
				FirstTabstop_SetFocus(page[currentSel]);
				TabPageMessageLoop(page[currentSel]);
				break;

			default:
				return;
		}
	}	//else // horizontal Tabs
}

BOOL CTabControl::TabCtrl_OnSelChanged()
{
	int iSel = TabCtrl_GetCurSel(hTab);

	//ShowWindow() does not seem to post the WM_SHOWWINDOW message
	// to the tab page.  Since I use the hiding of the window as an
	// indication to stop the message loop I post it explicitly.
	//PostMessage() is necessary in the event that the Loop was started
	// in response to a mouse click.

	// ???????????????????????????//
//	FORWARD_WM_SHOWWINDOW(m_lptc->hVisiblePage,FALSE,0,PostMessage);


	for (size_t k(0); k<titleStr.size(); k++)
	{
		if (k==iSel)
		{
			ShowWindow(hCur=page[k], SW_SHOW);
			FILE*fp=fopen("c:\\temp\\windows_log.txt","at");
			fprintf(fp,"hCur set 0x%x (TabCtrl_OnSelChanged)\n", hCur);
			fclose(fp);
		}
		else		 ShowWindow(page[k], SW_HIDE);
	}

	return TRUE;
}