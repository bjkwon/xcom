#include <windows.h>
#include <commctrl.h>  // includes the common control header
#include "WndDlg0.h"
#include "msgCrack.h"
#include "graffy.h"
#ifndef SIGPROC
#include "sigproc.h"
#endif

#include <algorithm> // for remove_if

#define  WM__SOUND_EVENT  WM_APP+10

#define  SHOWFOREGROUND	989

typedef struct {
	HWND hBase;
	char varname[256];
} VARPLOT;

class CVectorsheetDlg : public CWndDlg
{
public:
	HWND hList1;
	LVCOLUMN LvCol;
	LVITEM LvItem; 
	

	HFONT eFont;

	CVectorsheetDlg(void);
	~CVectorsheetDlg(void);

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	void OnCommand(int idc, HWND hwndCtl, UINT event);
	void OnSize(UINT state, int cx, int cy);
	void OnMove(int cx, int cy);
	void OnClose();
	void OnDestroy();
	void OnShowWindow(BOOL fShow, UINT status);
	void OnNotify(HWND hwnd, int idcc, LPARAM lParam);
	LRESULT ProcessCustomDraw (NMHDR *lParam);
	void FillupVector(CSignals &sig);
};

class CShowvarDlg : public CWndDlg
{
public:
	HWND hList1, hList2;
	LVCOLUMN LvCol; // Make Coluom struct for ListView
	LVITEM LvItem;  // ListView Item struct
	CSignals Sig;
	CAstSig AstSig;

	vector<CWndDlg*> m_sheets;
	map<string, DWORD> plotDlgThread;

	HFONT eFont;
	int soundID;
	bool playing;
	bool changed;
	bool win7;

	CShowvarDlg(void);
	~CShowvarDlg(void);

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
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
	void OnSoundEvent(int index, int code);
	void OnPlotDlgCreated(char *varname, GRAFWNDDLGSTRUCT *pin);
	void OnPlotDlgDestroyed(char *varname);
	void OnVarChanged(char *varname);
	void OnCloseFig(int figID);
	void UpdateSheets();
	void FillupShowVar(CSignals *cell=NULL);
	void lvInit();
	HACCEL hAccel;
	HANDLE curFig;
	HANDLE *figwnd;
	int nFigureWnd;
	int fs;
	char VerStr[32];
};