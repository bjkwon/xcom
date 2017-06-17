#include <windows.h>
#include <commctrl.h>  // includes the common control header
#include "WndDlg0.h"
#include "msgCrack.h"
#include "sigproc.h"
#include "consts.h"

class CHistDlg : public CWndDlg
{
public:
	HWND hList;
	LVCOLUMN LvCol;
	LVITEM LvItem;
	char logfilename[256];

	HFONT eFont;

	CHistDlg(void);
	~CHistDlg(void);

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	void OnCommand(int idc, HWND hwndCtl, UINT event);
	void OnTimer(UINT id);
	void OnSize(UINT state, int cx, int cy);
	void OnClose();
	void OnDestroy();
	void OnShowWindow(BOOL fShow, UINT status);
	void OnNotify(HWND hwnd, int idcc, LPARAM lParam);
	LRESULT ProcessCustomDraw (NMHDR *lParam);
	void UpdateSheets();
	void lvInit();
	void FillupHist(vector<string> in);
	void AppendHist(char *in);
};