#ifndef TABCONTROL
#define TABCONTROL

class CTabControl 
{
public:
	CWndDlg *mDad;
	HWND hTab;
	HWND hCur;
	HWND hVisiblePage;
	int tabPageCount;
	BOOL blStretchTabs;
	vector<string> titleStr;
	vector<HWND> page;

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	void OnSize(UINT state, int cx, int cy);
	void OnCommand(INT id, HWND hwndCtl, UINT codeNotify);

	void initTabcontrol(HWND);
	int AddPage(HWND hNewpage, string title);
	int GetCurrentPageID();
	void FirstTabstop_SetFocus(HWND h);
	void TabControl_GetClientRect(RECT * prc);
	BOOL CenterTabPage(int iPage);
	BOOL StretchTabPage(int iPage);
	void TabCtrl_OnKeyDown(LPARAM lParam);
	BOOL TabCtrl_OnSelChanged();
	BOOL OnNotify(LPNMHDR pnm);

 	CTabControl();
	virtual ~CTabControl() {};
private:
	TCITEM tie;
};

#define TabCtrl_SelectTab(hTab,iSel) { \
	TabCtrl_SetCurSel(hTab,iSel); \
	NMHDR nmh = { hTab, GetDlgCtrlID(hTab), TCN_SELCHANGE }; \
	SendMessage(nmh.hwndFrom,WM_NOTIFY,(WPARAM)nmh.idFrom,(LPARAM)&nmh); }

#endif //DEBUG_DLG