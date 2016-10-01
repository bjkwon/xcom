#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <vector>
#include <map>
#include "sigproc.h"
#include "audfret.h"
#include "graffy.h"
#include "audstr.h"
#include "showvar.h"
#include "consts.h"

extern CAstSig main;
extern CShowvarDlg mShowDlg;
extern double block;

CSignals &GetSig(string varname);
CSignals *GetSig(char *var);

HANDLE plotCall(HANDLE fig, VARPLOT messenger)
{
	HANDLE ax;
	CAxis *cax;
	CFigure *cfig = static_cast<CFigure *>(fig);

	if (cfig->ax.size()==0)
	{
		ax = AddAxis (fig, .08, .18, .86, .72);
		cax = static_cast<CAxis *>(ax);
	}
	else
	{
		ax = cfig->ax.front();
		while(cfig->ax.front()->m_ln.size()>0)
			deleteObj(cfig->ax.front()->m_ln.front());
	}

	if (main.Sig.GetType()==CSIG_VECTOR)
		PlotCSignals(ax, main.Sig, 0xff0000, '*', LineStyle_noline); // do something here
	else
		PlotCSignals(ax, main.Sig, 0xff0000);
	if (main.Sig.next)
		PlotCSignals(ax, *main.Sig.next, RGB(200,0,50));
		
	cfig->m_dlg->InvalidateRect(NULL,0);
	return ax;
}

map<unsigned int, string> wmstr;
AUDFRET_EXP void makewmstr(map<unsigned int, string> &wmstr);

void plotThread (PVOID var) // plotting
{ // var is varname to plot
	MSG         msg ;
	HANDLE ax;
	HANDLE fig;
	HACCEL hAcc;
	CFigure *cfig;
	VARPLOT *messenger=(VARPLOT*)var;
	if (main.Sig.nSamples>0)
	{
		CRect rt(0, 0, 500, 310);
		if (strlen(messenger->varname)==0)
			fig = OpenFigure(&rt, main.Sig, GetHWND_SIGPROC(), 0, block);
		else
			fig = OpenFigure(&rt, messenger->varname, main.Sig, GetHWND_SIGPROC(), 0, block);
		ax = plotCall( fig, *messenger );
		cfig = static_cast<CFigure *>(fig);
		hAcc = GetAccel(fig);

		CSignals gcf;
		GetFigID(fig, gcf);
		main.SetTag("gcf", gcf);
		mShowDlg.changed=true;
	}
	else
		return;
	//For plots initiated by xcom main, messenger.hBase is NULL and PostMessage below is ignored.
	PostMessage(messenger->hBase, WM__PLOTDLG_CREATED, (WPARAM)messenger->varname, (LPARAM)GetCurrentThreadId());
//	HWND hFigDlg = GetHWND_PlotDlg((HANDLE)cfig); // not necessary; redundant with cfig->m_dlg->hDlg
	int temp1(0);
	CSignals gcf, *tp;
	makewmstr(wmstr);
	FILE *fpp=fopen("msglog.txt","at");
	fprintf(fpp,"Console=%x\n", GetConsoleWindow());
	fclose(fpp);
	while (GetMessage (&msg, NULL, 0, 0))
	{
		char msgbuf[32];
		bool cond = msg.message!= WM_NCMOUSEMOVE && msg.message!= WM_MOUSEMOVE && msg.message!= WM_TIMER && msg.message!= WM_PAINT;
		if(wmstr[msg.message].size()>0) strcpy(msgbuf,wmstr[msg.message].c_str()); else sprintf(msgbuf, "0x%04x", msg.message);
		if (cond)
			fpp=fopen("msglog.txt","at"),fprintf(fpp,"msg.hwnd=%x, cfig->m_dlg->hDlg=%x, msg.umsg=%s\n", msg.hwnd, cfig->m_dlg->hDlg, msgbuf),fclose(fpp);
		//check if msg is about the figure window created here; if so, call  TranslateAccelerator
		//exception: if WM_DESTROY, get out of message loop
		if (msg.message==WM_DESTROY || !cfig->m_dlg)	break;
		temp1=TranslateAccelerator(cfig->m_dlg->hDlg, hAcc, &msg);
		if (cond && temp1)
			fpp=fopen("msglog.txt","at"),fprintf(fpp,"TranslateAccelerator success\n"),fclose(fpp);
		//if not or TranslateAccelerator fails do the other usual processing here ---is it really necessary? probably
		if (!temp1)
		{
			switch (msg.message)
			{
			case WM__VAR_CHANGED:
				tp = GetSig(messenger->varname);
				if (tp && (tp->GetType()==CSIG_AUDIO || tp->GetType()==CSIG_VECTOR))
					plotCall( fig, *messenger );
				else // if the variable is no longer unavable, audio or vector, exit the thread (and delete the fig window)
					PostThreadMessage (GetCurrentThreadId(), WM_QUIT, 0, 0);
				break;
			default:
				if (msg.message==WM_KEYDOWN && msg.wParam==17 && GetParent(msg.hwnd)==cfig->m_dlg->hDlg) // Left control key for window size adjustment
					msg.hwnd = cfig->m_dlg->hDlg;
				if (!IsDialogMessage(messenger->hBase, &msg))
				{
					TranslateMessage (&msg) ;
					DispatchMessage (&msg) ;
				}
				else
					fpp=fopen("msglog.txt","at"),fprintf(fpp,"TranslateAccelerator2 success\n"),fclose(fpp);
			}
		}
	}
	PostMessage(messenger->hBase, WM__PLOTDLG_DESTROYED, (WPARAM)messenger->varname, 0);
	deleteObj(ax); // check if it's OK to forgo deleting ax here...
	deleteObj(fig);
}

