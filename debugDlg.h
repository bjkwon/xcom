#include <windows.h>
#include <commctrl.h>  // includes the common control header
#include "WndDlg0.h"
#include "msgCrack.h"
#include <set>

#ifndef SIGPROC
#include "sigproc.h"
#endif

enum DEBUG_STATUS
{
    entering,
    progress,
	stepping,
    exiting,
};

using namespace std;

#define WM_DEBUG_CLOSE	WM_APP+9821

class CDebugDlg : public CWndDlg
{
public:

	string udfname;
	CAstSig *pabcast;
	int lastLine;
	vector<int> breakpoint;
	bool inDebug;
	HWND hList;
	LVCOLUMN LvCol; // Make Coluom struct for ListView
	LVITEM LvItem;  // ListView Item struct

	HFONT eFont;

	CDebugDlg(void);
	~CDebugDlg(void);

//	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	void OnCommand(int idc, HWND hwndCtl, UINT event);
	void OnSysCommand(UINT cmd, int x, int y);
	void OnSize(UINT state, int cx, int cy);
	void OnClose();
	void OnDestroy();
	void OnShowWindow(BOOL fShow, UINT status);
	HBRUSH OnCtlColorStatic(HDC hdc, HWND hCtrl, int id);
	HBRUSH OnCtlColorEdit(HDC hdc, HWND hCtrl, int id);
	HBRUSH OnCtlColor(HDC hdc, HWND hCtrl, int id);
	void OnNotify(HWND hwnd, int idcc, LPARAM lParam);
	LRESULT ProcessCustomDraw (NMHDR *lParam);
	void lvInit();
	void FillupContent(vector<string> in);
	int GetBPandUpdate();


	void OnDebug(DEBUG_STATUS type, int line);

};

//To create CDebugDlg, make data like the following
struct CREATE_CDebugDlg
{
	CDebugDlg *dbDlg;
	char fullfilename[256];
};

