#include "graffy.h" // this should come before the rest because of wxx820
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <vector>
#include <map>
#include "sigproc.h"
#include "audfret.h"
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

//	cfig->m_dlg->OnCommand(32781, NULL, 0); // cannot be this way... virtual function CWndDlg::OnCommand is called, which is to be taken off. bjkwon 11/13/2016 ... 
	if (cfig->ax.size()>1)	
	{// IDM_SPECTRUM ..as if the F4 key was pressed.  Pressing twice
		cfig->m_dlg->SendMessage(WM_COMMAND, 32781); 
		cfig->m_dlg->SendMessage(WM_COMMAND, 32781); 
	}
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
	CSignals gcf;
	CFigure *cfig;
	VARPLOT *messenger=(VARPLOT*)var;
	if (main.Sig.nSamples>0)
	{
		CRect rt(0, 0, 500, 310);
		if (strlen(messenger->varname)==0)
			fig = OpenFigure(&rt, GetHWND_SIGPROC(), 0, block);
		else
<<<<<<< HEAD
			fig = OpenFigure(&rt, messenger->varname, GetHWND_SIGPROC(), 0, block);
=======
			fig = OpenFigure(&rt, messenger->varname, main.Sig, GetHWND_SIGPROC(), 0, block);
>>>>>>> origin/master
		if (fig==NULL) {MessageBox(NULL, "OpenFigure failed", "Auxlab", 0); return;}
		ax = plotCall( fig, *messenger );
		cfig = static_cast<CFigure *>(fig);
		hAcc = GetAccel(fig);

		GetFigID(fig, gcf);
		main.SetTag("gcf", gcf);
		mShowDlg.changed=true;
		mShowDlg.FillupShowVar();
	}
	else
		return;
	PostMessage(messenger->hBase, WM__PLOTDLG_CREATED, (WPARAM)messenger->varname, (LPARAM)GetCurrentThreadId());
	int temp1(0);
	CSignals *tp;
<<<<<<< HEAD
//	FILE *fpp = fopen("log.txt","at"); fprintf(fpp,"thread=%d, fig handle = %x, hAccel=%x\n", GetCurrentThreadId(), fig, hAcc); fclose(fpp);
	while (GetMessage (&msg, NULL, 0, 0))
	{
		if (msg.message==WM_DESTROY || !cfig->m_dlg)	
			break;
//		fpp = fopen("log.txt","at"); fprintf(fpp,"thread %d, TranslateAccelerator(%4x, %4x, (msg)%4x) ...", GetCurrentThreadId(), cfig->m_dlg->hDlg, hAcc, msg.message); 
		temp1=TranslateAccelerator(cfig->m_dlg->hDlg, hAcc, &msg);
//		if (temp1) 			{fprintf(fpp,"... success\n"); fclose(fpp);}
		if (!temp1)
		{
//			fprintf(fpp,"... 0\n"); fclose(fpp);
			if (msg.hwnd==cfig->m_dlg->hDlg || GetParent(msg.hwnd) == cfig->m_dlg->hDlg || GetParent(GetParent(msg.hwnd)) == cfig->m_dlg->hDlg )
			{
				if ((msg.message>=WM_NCLBUTTONDOWN && msg.message<=WM_NCMBUTTONDBLCLK) || ((msg.message>=WM_LBUTTONDOWN && msg.message<=WM_MBUTTONDBLCLK)) )
				{
					GetFigID(fig, gcf);
					main.SetTag("gcf", gcf);
					mShowDlg.FillupShowVar();
				}
=======
//	makewmstr(wmstr);
	//FILE *fpp=fopen("msglog.txt","at");
	//fprintf(fpp,"Console=%x\n", GetConsoleWindow());
	//fclose(fpp);
	while (GetMessage (&msg, NULL, 0, 0))
	{
//		char msgbuf[32];
//		bool cond = msg.message!= WM_NCMOUSEMOVE && msg.message!= WM_MOUSEMOVE && msg.message!= WM_TIMER && msg.message!= WM_PAINT;
//		if(wmstr[msg.message].size()>0) strcpy(msgbuf,wmstr[msg.message].c_str()); else sprintf(msgbuf, "0x%04x", msg.message);
//		if (cond)
//			fpp=fopen("msglog.txt","at"),fprintf(fpp,"msg.hwnd=%x, cfig->m_dlg->hDlg=%x, msg.umsg=%s\n", msg.hwnd, cfig->m_dlg->hDlg, msgbuf),fclose(fpp);
		//check if msg is about the figure window created here; if so, call  TranslateAccelerator
		//exception: if WM_DESTROY, get out of message loop
		if (msg.message==WM_DESTROY || !cfig->m_dlg)	break;
		temp1=TranslateAccelerator(cfig->m_dlg->hDlg, hAcc, &msg);
//		if (cond && temp1)
//			fpp=fopen("msglog.txt","at"),fprintf(fpp,"TranslateAccelerator success\n"),fclose(fpp);
		//if not or TranslateAccelerator fails do the other usual processing here ---is it really necessary? probably
		if (!temp1)
		{
			if (msg.hwnd==cfig->m_dlg->hDlg &&  ((msg.message>=WM_NCLBUTTONDOWN && msg.message<=WM_NCMBUTTONDBLCLK) || ((msg.message>=WM_LBUTTONDOWN && msg.message<=WM_MBUTTONDBLCLK)) ))
			{
				GetFigID(fig, gcf);
				main.SetTag("gcf", gcf);
				mShowDlg.changed=true;
	//			PostThreadMessage (GetCurrentThreadId(), WM__VAR_CHANGED, (WPARAM)gcf.string().c_str(), 0);
>>>>>>> origin/master
			}
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
				{
<<<<<<< HEAD
=======
//					fpp=fopen("msglog.txt","at");fprintf(fpp,"WM_KEYDOWN, msg.wParam=17, cfig->m_dlg->hDlg=%x, msg.hwnd=%x (later equalled)\n", cfig->m_dlg->hDlg, msg.hwnd);fclose(fpp);
>>>>>>> origin/master
					msg.hwnd = cfig->m_dlg->hDlg;
				}
 				if (!IsDialogMessage(messenger->hBase, &msg))
				{
					TranslateMessage (&msg) ;
					DispatchMessage (&msg) ;
				}
<<<<<<< HEAD
=======
//				else
//					fpp=fopen("msglog.txt","at"),fprintf(fpp,"IsDialogMessage success\n"),fclose(fpp);
>>>>>>> origin/master
			}
		}
	}
	HWND hp=GetParent(msg.hwnd);

	PostMessage(messenger->hBase, WM__PLOTDLG_DESTROYED, (WPARAM)messenger->varname, 0);
}

