#include "graffy.h" // this should come before the rest because of wxx820
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <vector>
#include "sigproc.h"
#include "audfret.h"
#include "audstr.h"
#include "resource1.h"
#include "showvar.h"
#include "histDlg.h"
#include "consts.h"

//#pragma comment(linker,"\"/manifestdependency:type='win32' \
//name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
//processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static FILE *fp1;

char iniFile[256];

double playbackblock(100.);
uintptr_t hShowvarThread(NULL);
uintptr_t hHistoryThread(NULL);
char udfpath[4096];

CWndDlg wnd;
CShowvarDlg mShowDlg;
CHistDlg mHistDlg;
CSignals tp;
CAstSig main;
void* soundplayPt;
double block;

int save_axl(FILE *fp, const char * var, char *errstr);
int load_axl(FILE *fp, char *errstr);

string typestring(int type);
BOOL CALLBACK showvarDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK historyDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CtrlHandler( DWORD fdwCtrlType );
LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
void EnumVariables(vector<string> &var);
void nonnulintervals(CSignal *psig, string &out, bool unit, bool clearout=false);

void nonnulintervals(CSignal *psig, string &out, bool unit, bool clearout)
{
	char buf[128];
	int kk(0), tint(psig->CountChains());
	if (clearout) out.clear();
	for (CSignal *xp=psig;kk<tint;kk++,xp=xp->chain) {
		if (unit) sprintf(buf,"(%gms~%gms) ", xp->tmark, xp->tmark + xp->dur());
		else sprintf(buf,"(%g~%g) ", xp->tmark, xp->tmark + xp->dur());
		out += buf;
	}
}

#define PRINTLAYER for (int k=0; k<layer; k++) fprintf(fp, "   "); 
void print_node_struct (int layer, FILE* fp, string str, AstNode *node)
{
	PRINTLAYER
	fprintf(fp, "%s", str.c_str());
	PRINTLAYER
	fprintf(fp, "(%d %s) ", node->type, typestring(node->type).c_str());
	if (node->str!=NULL) fprintf(fp, "%s, ", node->str);
	else fprintf(fp, "_, ", node->str);
	if (node->type==T_NUMBER) fprintf(fp, "%g\n", node->dval);
	else  fprintf(fp, "_\n");

	if (node->child!=NULL) 
	{	
		print_node_struct(++layer, fp, "child...\n", node->child); --layer;}
	if (node->next!=NULL) 
	{	
		print_node_struct(++layer, fp, "next...\n", node->next); --layer;}
	//if (node->LastChild!=NULL) 
	//{	
	//	print_node_struct(++layer, fp, "LastChild...\n", node->LastChild); --layer;}
}

void ClearVar(const char*var)
{
	CAstSigEnv *pe = main.pEnv;
	if (var[0]==0) // clear all
	{
		pe->Tags.erase(pe->Tags.begin(), pe->Tags.end());
		pe->UDFs.erase(pe->UDFs.begin(), pe->UDFs.end());
		mShowDlg.changed=true;
	}
	else
	{
		vector<string> vars;
		size_t res = str2vect(vars, var, " ");
		for (size_t k=0; k<res; k++)
		{
			for (map<string, CSignals>::iterator what=pe->Tags.begin(); what!=pe->Tags.end(); what++)
			{
				if (what->first==vars[k])
				{
					pe->Tags.erase(what);
					mShowDlg.changed=true;
					break;
				}
			}
		}
		for (size_t k=0; k<res; k++)
		{
			for (map<string, AstNode *>::iterator what=pe->UDFs.begin(); what!=pe->UDFs.end(); what++)
			{
				if (what->first==vars[k])
				{
					pe->UDFs.erase(what);
					mShowDlg.changed=true;
					break;
				}
			}
		}
	}
}


unsigned int WINAPI histThread (PVOID var) 
{
	mHistDlg.hDlg = CreateDialog (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY), GetConsoleWindow(), (DLGPROC)historyDlg);
	if (mHistDlg.hDlg==NULL) {MessageBox(NULL, "History Dlgbox creation failed.","AUXLAB",0); return 0;}
<<<<<<< HEAD
=======
//	mHistDlg.OnInitDialog(NULL, (LPARAM)var);
>>>>>>> origin/master
	RECT rc, rc2;
	int res = GetWindowRect(GetConsoleWindow(), &rc);
	rc2 = rc;
	int width = rc.right-rc.left;
	int height = rc.bottom-rc.top;
	rc2.left = rc.right;
	rc2.right = rc2.left + width/7*4;
	rc2.bottom = rc.bottom;
	rc2.top = rc2.bottom - height/5*4;
	int res2 = MoveWindow(mHistDlg.hDlg, rc2.left, rc2.top, width/7*4, height/5*4, 1);

 //   SYSTEMTIME lt;
 //   GetLocalTime(&lt);	
	//char buffer[256];
	//sprintf(buffer, "[%02d/%02d/%4d, %02d:%02d:%02d] wndHistory.hDlg:%x, GetWindowRect=%d, MoveWindow=%d\n", lt.wMonth, lt.wDay, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, wndHistory.hDlg, res, res2);
	//fp1=fopen("hist_disappearing_track.txt","at");
	//fprintf(fp1,buffer);
	//fclose(fp1);


	MSG         msg ;
	while (GetMessage (&msg, NULL, 0, 0))
	{ 
		if (msg.message==WM__ENDTHREAD) 
			_endthreadex(17);
		TranslateMessage (&msg) ;
		DispatchMessage (&msg) ;
	}
	hHistoryThread=NULL;
	return 0;
}


unsigned int WINAPI showvarThread (PVOID var) // Thread for variable show
{
	HINSTANCE hModule = GetModuleHandle(NULL);
<<<<<<< HEAD
=======
	//FILE *fpp=fopen("showvarlog.txt","wt");
	//fclose(fpp);
>>>>>>> origin/master

	mShowDlg.hDlg = CreateDialog (hModule, MAKEINTRESOURCE(IDD_SHOWVAR), GetConsoleWindow(), (DLGPROC)showvarDlg);
	mShowDlg.hList1 = GetDlgItem(mShowDlg.hDlg , IDC_LIST1);
	RECT rc;
	GetWindowRect(GetConsoleWindow(), &rc);
	int width = rc.right-rc.left;
	int height = rc.bottom-rc.top;
	RECT rc2(rc);
	rc2.right = rc.left;
	rc2.left = rc2.right - width/7*4+10;
	rc2.bottom = rc.bottom;
	rc2.top = rc2.bottom - height/5*4;

	if (rc2.left<0) 
	{
		rc2.right -= rc2.left;
		rc2.left = 0;
		MoveWindow(GetConsoleWindow(), rc2.right, rc.top, width, height, 1);
	}
	MoveWindow(mShowDlg.hDlg, rc2.left, rc2.top, width/7*4, height/5*4, 1);
//	SetFocus(GetConsoleWindow()); //This doesn't seem to work.
//	SetForegroundWindow (GetConsoleWindow());
	
	MSG msg ;
	HACCEL hAcc = LoadAccelerators (hModule, MAKEINTRESOURCE(IDR_XCOM_ACCEL));
	while (GetMessage (&msg, NULL, 0, 0))
	{ 
		if (msg.message==WM__ENDTHREAD) 
			_endthreadex(33);
		if (!TranslateAccelerator (mShowDlg.hDlg, hAcc, &msg))
		{
			TranslateMessage (&msg) ;
			DispatchMessage (&msg) ;
		}
	}
	hShowvarThread=NULL;
	return 0;
}


void out4console(const string varname, CSignals *psig, string &out)
{
	size_t count;
	char buf[256];
	string varname2;
	if (psig->GetType() != CSIG_CELL && varname.size()>0)	out += varname;
	switch(psig->GetType())
	{
	case CSIG_NULL:
		out += "NULL(0~";
		sprintf(buf,"%gms)\n",psig->tmark);
		out += buf;
		break;
	case CSIG_EMPTY:
		out += "(empty)\n";
		break;
	case CSIG_AUDIO:
		if (psig->next!=NULL) out += "audio(L) ";
		else	out += "audio ";
		nonnulintervals ((CSignal*)psig, out, true);
		out +="\n";
		if (psig->next!=NULL) {
			out += "audio(R) ";
			nonnulintervals (psig->next, out, true);
			out +="\n";
			}
		break;
	case CSIG_VECTOR:
	case CSIG_SCALAR:
	case CSIG_COMPLEX:
		if (psig->GetType()==CSIG_COMPLEX)
		{
			for (int k=0; k< min (psig->nSamples, DISPLAYLIMIT); k++) 
				{sprintf(buf,"%g %+gi  ", psig->buf[2*k], psig->buf[2*k+1]); out+=buf;}
		}
		else
			for (int k=0; k< min (psig->nSamples, DISPLAYLIMIT); k++) 
				{sprintf(buf,"%g  ", psig->buf[k]); out+=buf;}
		
		if (psig->nSamples>DISPLAYLIMIT) sprintf(buf,"....length=%d\n",psig->nSamples); 
		else sprintf(buf,"\n");
		out += buf;
		if (psig->next)
		{
			for (int k=0; k<min(psig->next->nSamples,DISPLAYLIMIT); k++)
			{	sprintf(buf,"%g  ", psig->next->buf[k]); out += buf;		}
			if (psig->next->nSamples>DISPLAYLIMIT) sprintf(buf,"....length=%d\n",psig->nSamples); 
			else sprintf(buf,"\n");
			out += buf;
		}
		break;
	case CSIG_STRING:
		out += "\"" + psig->string() + "\"\n";
		break;
	case CSIG_CELL:
		count = psig->cell.size();
		for (size_t k=0; k<count; k++)
		{
			CSignals nocellplz(psig->cell[k]);
			if (nocellplz.nSamples>0) 
				nocellplz.cell.clear(); // This is necessary because, after [] or at, a celled-object will have trailing cell members, if available.
			varname2 = varname + "{" + itoa((int)k+1,buf,10) + "} = ";
			out4console(varname2, &nocellplz, out);
		}
		break;
	}
}

#define	ISTHISSTR(STR) (!strcmp(pnode->str,STR))

bool need2echo(AstNode *pnode)
{
	//if there are multiple statements on one line, 

	//if node.str is one of the following, immediately return false
	if (pnode->str)
	{
		if (ISTHISSTR("play")) return false;
		if (ISTHISSTR("wavwrite")) return false;
		if (ISTHISSTR("show")) return false;
		if (ISTHISSTR("fprintf")) return false;
		if (ISTHISSTR("fdelete")) return false;
		if (ISTHISSTR("include")) return false;
		if (ISTHISSTR("eval")) return false;
		if (ISTHISSTR("eval")) return false;
	}
	return true;
}

void showarray(int code, CAstSig *main, AstNode *node)
{
	GRAFWNDDLGSTRUCT in;
	string varname, out;
	char buf[256], errstr[256];
	switch(node->type)
	{
	case T_IF:
	case T_WHILE:
	case T_SWITCH:
	case T_FOR:
	case T_SIGMA:
		break;
	case T_ID: // noncell_variable or cellvar{index}
	case NODE_CALL: // variable(index)
		if (need2echo(node))
		{
			tp = main->Sig;
			main->SetTag("ans", tp);
			out4console("ans =\n", &tp, out);
		}
		break;
	case NODE_INITCELL: // cell definition
		tp = main->Sig;
		out4console(node->str, main->RetrieveTag(node->str), out);
		mShowDlg.changed=true; 
		mShowDlg.SendMessage(WM__VAR_CHANGED, (WPARAM)node->str);
		break;
	case NODE_BLOCK: // MULTIPLE statements.
		{
			string script(main->GetScript());
			AstNode *p(node->child);
			while (p && p->child)
			{
				if (p->type==T_IF || p->type==T_WHILE || p->type==T_SWITCH || p->type==T_FOR || p->type==T_SIGMA )
					showarray (ID_DEFAULT, main, p);
				else if (p->str)
				{
					size_t found = script.find(';', p->column-1);
					if ( (p->next && (int)found > p->next->column) || // ; found but outside the current node 
						found==string::npos) // ; not found
					{
						// if p->type is NODE_CALL, p->str shows the name of function call (so don't call Eval)
						if (p->type != NODE_CALL && !main->RetrieveTag(p->str))
							main->Eval(p);
						if (p->type == NODE_CALL)
						{
							if (!p->next) // For function call, show only for the last one
								showarray (ID_DEFAULT, main, p);
						}
						else
							showarray (ID_DEFAULT, main, p);
					}
				}
				p=p->next;
			}
		}
		break;

	default: // any statement
		if (node->str)
		{
			map<string, CSignals>::iterator what;
			what = main->pEnv->Tags.find(node->str);
			if (what == main->pEnv->Tags.end())
				tp = main->Sig;
			else
			{
				tp = what->second;
				if (tp.cell.size()>0) out4console(node->str, &tp, out);
				else
				{
					varname = node->str; varname += " =\n";
					out4console(varname, &tp, out);
				}
			}
		}
		else // Extraction. Expression, no variable
		{
			main->SetTag("ans", main->Sig);
			out4console("ans =\n", &main->Sig, out);
		}
		break;
	}
	printf("%s",out.c_str());

	switch(code)
	{
	case ID_PLOT:
		{
			strcpy(buf, main->GetScript().c_str());
			CSignals gcf;
			CRect rt(0, 0, 500, 310);
			HANDLE fig = OpenGraffy(rt, buf, GetCurrentThreadId(), mShowDlg.hDlg, in);
			HANDLE ax = AddAxis (fig, .08, .18, .86, .72);
			CAxis *cax = static_cast<CAxis *>(ax);
			if (main->Sig.GetType()==CSIG_VECTOR)
				PlotCSignals(ax, main->Sig, 0xff0000, '*', LineStyle_noline); // plot for VECTOR, no line and marker is * --> change if you'd like
			else
				PlotCSignals(ax, main->Sig, 0xff0000);
			if (main->Sig.next)
				PlotCSignals(ax, *main->Sig.next, RGB(200,0,50));
			CFigure *cfig = static_cast<CFigure *>(fig);
		}
		break;

	case ID_PLAY:
		TerminatePlay();
	case ID_PLAYOVERLAP:
		if (main->Sig.GetType()==CSIG_AUDIO)
			main->Sig.PlayArray(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr);
		else if (main->Sig.GetType()!=CSIG_SCALAR)
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
		break;

	case ID_PLAYLOOP:
	case ID_PLAYENDLOOP:
		if (main->Sig.GetType()==CSIG_AUDIO)
		{
			LoopPlay(code==ID_PLAYLOOP);
			main->Sig.PlayArray(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr, 1); // looping
		}
		else
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
		break;

	case ID_PLAYCONTINUE:
		if (main->Sig.GetType()==CSIG_AUDIO)
			main->Sig.PlayArrayNext(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr);
		else
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
		break;

	case ID_STOP:
		TerminatePlay();
		break;
	case ID_SAVE:
		break;
	case ID_CLEARVAR:
		if (tp.getString(buf, 256)==NULL) printf("Invalid argument to clear variable\n");
		else
			ClearVar(buf);
		break;
	}
}



void computeandshow(char *AppPath, int code, char *buf)
{
	static AstNode node;
	static CSignals Sig;
	string fname;
	FILE *fp;
	string savesuccess;
	bool err(false);
	char errstr[256], filename[MAX_PATH], ext[MAX_PATH];
	size_t nItems;
	vector<string> varlist, tar;
	filename[0]=0;
	trimLeft(buf," \t");
	trimRight(buf," \t");
	switch(code)
	{
	case ID_SHOWVAR:
		mShowDlg.FillupShowVar();
		mShowDlg.changed=false;
		ShowWindow(mShowDlg.hDlg,SW_SHOW);
		SetForegroundWindow (mShowDlg.hDlg); 
		break;
	case ID_HISTORY:
		ShowWindow(mHistDlg.hDlg,SW_SHOW);
		break;
	case ID_SHOWFS:
		printf("Sampling Rate=%d\n", main.pEnv->Fs);
		break;
	case ID_LOAD:
		fname = buf;
		fp = main.OpenFileInPath(fname, "axl");
		if (fp==NULL)
			printf("File %s not found in %s\n", strcat(buf,".axl"), main.pEnv->AuxPath.c_str()); 
		else
		{
			if (load_axl(fp, errstr)==0) printf("File %s reading error----%s.\n", fname.c_str(), errstr);
			fclose(fp);
			mShowDlg.FillupShowVar();
		}
		break;
	case ID_SAVE:
		if ((nItems = str2vect(tar, buf, " "))==0)
		{	printf("#save (filename) var1 var2 var3...\n");		break;		}
		fname = tar[0];
		_splitpath(fname.c_str(), NULL, NULL, filename, ext);
		if (strlen(ext)==0) fname += ".axl";
		if (nItems==1) // no variables indicated.. save all
		{
			EnumVariables(varlist);
			size_t sz = varlist.size();
			nItems = sz+1;
			for (unsigned int k=1; sz>0 && k<nItems; k++)  
				tar.push_back(varlist[k-1]);
		}
		if ((fp = fopen(fname.c_str(), "wb"))==NULL) printf("File %s cannot be open for writing.\n", tar[0].c_str());
		else
		{
			printf("File %s saved with ...\n", tar[0].c_str());
			for (unsigned int k=1; k<nItems; k++)
			{
				if (save_axl(fp, tar[k].c_str(), errstr)==0)
				{
					if (savesuccess.length()>0)
						printf("Save error----%s.\n %s are saved OK.\n", errstr, savesuccess.c_str());
					else
						printf("Save error----%s.\n", errstr);
					fclose(fp);
					err = true;
					break;
				}
				savesuccess += tar[k].c_str() + string(" ");
				fclose(fp);
				fp=fopen(fname.c_str(), "ab");
			}
		}
		if (!err)	printf("%s \n", savesuccess.c_str());
		fclose(fp);
		break;
	default:
		if (strlen(buf)>0)
		{
		try {
			main.SetNewScript(buf,&node);
			main.Sig = main.Compute();
			//NODE_BLOCK: multiple statements on one line
			//NODE_CALL : function call, include call, eval call
			trimRight(buf," \t\r\n");
			if ( node.type==NODE_BLOCK || buf[strlen(buf)-1]!=';')
				showarray(code, &main, &node);

			//
			//{
			//	if (node.type!=NODE_CALL || strcmp(node.str,"plot") * strcmp(node.str,"play") * strcmp(node.str,"show")  ||
			//		strcmp(node.str,"sscanf") * strcmp(node.str,"fprintf") * strcmp(node.str,"fdelete") ||
			//		strcmp(node.str,"file") * strcmp(node.str,"include") * strcmp(node.str,"eval") )
			//}
			mShowDlg.changed=true;
			mShowDlg.FillupShowVar(); // this line was added on 10/2/2016.. probably all the other things about mShowDlg.changed is no longer necessary... bjk
			AstNode *p = (node.type==NODE_BLOCK) ? node.child : &node;
			for (; p; p = p->next)
				if (p->type!=T_ID && p->type!=T_NUMBER && p->type!=T_STRING) //For these node types, 100% chance that var was not changed---do not send WM__VAR_CHANGED
					mShowDlg.SendMessage(WM__VAR_CHANGED,  (WPARAM) (p->str ? p->str : "ans"));
			if (main.statusMsg.length()>0) cout << main.statusMsg.c_str() << endl;
			main.statusMsg.clear();
			}
		catch (const char *errmsg) {
			cout << "ERROR:" << errmsg << endl;	}
		}
	}
	printf("AUX>");
}

WORD readINI(const char *fname, CRect *rtMain, CRect *rtShowDlg, CRect *rtHistDlg)
{
	char errStr[256];
	int tar[4];
	WORD ret(0);
	string strRead;
	if (ReadINI (errStr, fname, "WINDOW POS", strRead)>=0 && str2array (tar, 4, strRead.c_str(), " ")==4)
	{
		rtMain->left = tar[0];
		rtMain->top = tar[1];
		rtMain->right = tar[2] + tar[0];
		rtMain->bottom = tar[3] + tar[1];
		ret += 1;
	}
	if (ReadINI (errStr, fname, "VAR VIEW POS", strRead)>=0 && str2array (tar, 4, strRead.c_str(), " ")==4)
	{
		rtShowDlg->left = tar[0];
		rtShowDlg->top = tar[1];
		rtShowDlg->right = tar[2] + tar[0];
		rtShowDlg->bottom = tar[3] + tar[1];
		ret += 2;
	}
	if (ReadINI (errStr, fname, "HIST POS", strRead)>=0 && str2array (tar, 4, strRead.c_str(), " ")==4)
	{
		rtHistDlg->left = tar[0];
		rtHistDlg->top = tar[1];
		rtHistDlg->right = tar[2] + tar[0];
		rtHistDlg->bottom = tar[3] + tar[1];
		ret += 4;
	}
	return ret;
}

int readINI(const char *fname, char *estr, int &fs, double &_block, char *path)
{//in, in, out, out, in/out
	char errStr[256];
	string strRead;
	int val;
	double dval;
	int res = ReadINI (errStr, fname, "SAMPLE RATE", strRead);
	if (res>0 && sscanf(strRead.c_str(), "%d", &val)!=EOF && val >10)	fs = val;
	else																fs = DEFAULT_FS;
	res = ReadINI (errStr, fname, "PLAYBACK BLOCK SIZE MILLISEC", strRead);
	if (res>0 && sscanf(strRead.c_str(), "%lf", &dval)!=EOF && dval >10.)	_block = dval;
	else																_block = DEFAULT_BLOCK_SIZE;

	if (ReadINI (errStr, fname, "PATH", strRead)>=0)
		strcat(path, strRead.c_str());

	return 1;
}

int writeINI(char *fname, char *estr, int fs, double _block, const char *path)
{
	char errStr[256];
	if (!printfINI (errStr, fname, "SAMPLE RATE", "%d", fs)) {strcpy(estr, errStr); 	return 0;}
	if (!printfINI (errStr, fname, "PLAYBACK BLOCK SIZE MILLISEC", "%.1f", _block))	{strcpy(estr, errStr);	return 0;}
	if (!printfINI (errStr, fname, "PATH", "%s", path)) {strcpy(estr, errStr); return 0;}
	return 1;
}

int writeINI(const char *fname, char *estr, CRect rtMain, CRect rtShowDlg, CRect rtHistDlg)
{
	char errStr[256];
	CString str;
	str.Format("%d %d %d %d", rtMain.left, rtMain.top, rtMain.Width(), rtMain.Height());
	if (!printfINI (errStr, fname, "WINDOW POS", "%s", str.c_str())) {strcpy(estr, errStr); return 0;}
	str.Format("%d %d %d %d", rtShowDlg.left, rtShowDlg.top, rtShowDlg.Width(), rtShowDlg.Height());
	if (!printfINI (errStr, fname, "VAR VIEW POS", "%s", str.c_str())) {strcpy(estr, errStr); return 0;}
	str.Format("%d %d %d %d", rtHistDlg.left, rtHistDlg.top, rtHistDlg.Width(), rtHistDlg.Height());
	if (!printfINI (errStr, fname, "HIST POS", "%s", str.c_str())) {strcpy(estr, errStr); return 0;}
	return 1;
}

void closeXcom(SYSTEMTIME *lt, const char *AppPath)
{
	//If CTRL_CLOSE_EVENT is pressed, you have 5 seconds (that's how Console app works in Windows.... Finish all cleanup work in this timeframe.
	char estr[256], buffer[256];
	const char *sss = main.GetPath();
	const char* pt = strstr(main.GetPath(), AppPath); 
	string pathnotapppath;
	size_t loc;
	if (pt!=NULL)
	{
		pathnotapppath.append(main.GetPath(),  main.GetPath()-pt);
		string  str(pt + strlen(AppPath));
		loc = str.find_first_not_of(';');
		pathnotapppath.append(pt + strlen(AppPath)+loc);
	}
	else
		pathnotapppath.append(main.GetPath());
	int res = writeINI(iniFile, estr, main.pEnv->Fs, block, pathnotapppath.c_str());

	CRect rt1, rt2, rt3;
	GetWindowRect(GetConsoleWindow(), &rt1);
	mShowDlg.GetWindowRect(rt2);
	mHistDlg.GetWindowRect(rt3);
	res = writeINI(iniFile, estr, rt1,rt2, rt3);
	sprintf(buffer, "//\t[%02d/%02d/%4d, %02d:%02d:%02d] AUXLAB closes------", lt->wMonth, lt->wDay, lt->wYear, lt->wHour, lt->wMinute, lt->wSecond);
	LOGHISTORY(buffer);
	if (hShowvarThread!=NULL) PostThreadMessage(GetWindowThreadProcessId(mShowDlg.hDlg, NULL), WM__ENDTHREAD, 0, 0);
	if (hHistoryThread!=NULL) PostThreadMessage(GetWindowThreadProcessId(mHistDlg.hDlg, NULL), WM__ENDTHREAD, 0, 0);
}

LRESULT CALLBACK sysmenuproc (int code, WPARAM wParam, LPARAM lParam)
{
	CWPSTRUCT *param = (CWPSTRUCT*)lParam;
	if (param->message==WM_MENUCOMMAND || param->message==WM_SYSCOMMAND)
	{
		int res=0;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CALLBACK proc (HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	switch (umsg)
	{
	case WM_CREATE:
		return 1;
	case WM_COMMAND:
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc( hwnd, umsg, wParam, lParam );
}


LRESULT CALLBACK wireKeyboardProc(int code, WPARAM wParam,LPARAM lParam) {  
    if (code < 0) {
    	return CallNextHookEx(0, code, wParam, lParam);
    }
    Beep(1000, 20);
    return CallNextHookEx(0, code, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int    code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	char estr[256], buf[256], *buff;
	int code, res;
	size_t nOut;
	string addp;
	vector<string> tar;
	int fs;
	char fullmoduleName[MAX_PATH], moduleName[MAX_PATH], AppPath[MAX_PATH];
 	char drive[16], dir[256], ext[8], fname[MAX_PATH], buffer[MAX_PATH], buffer2[MAX_PATH];
<<<<<<< HEAD
=======

	//FILE *fpp=fopen("msglog.txt","wt");
	//fclose(fpp);
>>>>>>> origin/master

	 _set_output_format(_TWO_DIGIT_EXPONENT);

	INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS; // ICC_LINK_CLASS will not work without common control 6.0 which I opted not to use
    InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    BOOL bRet = InitCommonControlsEx(&InitCtrls);

	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS) ;

	HWND hr = GetConsoleWindow();
	HMENU hMenu = GetSystemMenu(hr, FALSE);
	AppendMenu(hMenu, MF_SEPARATOR, 0, "");
	AppendMenu(hMenu, MF_STRING, 1010, "F1 will not work here. Click \"Settings & Workspace Variables\" and F1");

	HMODULE h = HMODULE_THIS;
 	GetModuleFileName(h, fullmoduleName, MAX_PATH);
 	_splitpath(fullmoduleName, drive, dir, fname, ext);
 	sprintf (AppPath, "%s%s", drive, dir);
 	sprintf (moduleName, "%s%s", fname, ext);
	getVersionStringAndUpdateTitle(GetConsoleWindow(), fullmoduleName, buffer, sizeof(buffer));
#ifndef WIN64
	sprintf(buf, "AUXLAB %s --- Audio Processing Made Easier with AUdio syntaX", buffer);
#else
	sprintf(buf, "AUXLAB %s (x64) --- Audio Processing Made Easier with AUdio syntaX", buffer);
#endif
	SetConsoleTitle(buf);
	RECT rt;
	GetWindowRect(hr, &rt);
<<<<<<< HEAD
	DWORD dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(iniFile, "%s%s_%s.ini", AppPath, fname, buf);
	res = readINI(iniFile, estr, fs, block, udfpath);
	main.Reset(fs,""); //	main.Sig.Reset(fs); is wrong...
=======
	dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(iniFile, "%s%s_%s.ini", AppPath, fname, buf);
	res = readINI(iniFile, estr, fs, block, udfpath);
//	main.Sig.Reset(fs); // This is not how to do it... see below 
	main.Reset(fs,"");
>>>>>>> origin/master
	addp = AppPath;
	if (strlen(udfpath)>0 && udfpath[0]!=';') addp += ';';	
	addp += udfpath;
	main.SetPath(addp.c_str());

	if ((hShowvarThread = _beginthreadex (NULL, 0, showvarThread, NULL, 0, NULL))==-1)
		MessageBox(NULL, "Showvar Thread Creation Failed.", "AUXLAB main", 0);
	if ((hHistoryThread = _beginthreadex (NULL, 0, histThread, (void*)AppPath, 0, NULL))==-1)
		MessageBox(NULL, "History Thread Creation Failed.", "AUXLAB main", 0);
	
	freopen( "CON", "w", stdout ) ;
	freopen( "CON", "r", stdin ) ;

	complex<double> x(1,-5), y(2,3);
	complex<double> c = x*y;

	HANDLE hStdin, hStdout; 
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD fdwMode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE; 
	SetConsoleMode(hStdin, fdwMode) ;
	if (!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE )) MessageBox (NULL, "Console control handler error", "AUXLAB", MB_OK);

	ShowWindow(mHistDlg.hDlg, SW_SHOW);

	printf("AUX>");

	while (mShowDlg.hDlg==NULL) {
		Sleep(100); 
		mShowDlg.FillupShowVar();
		mShowDlg.changed=false;
	}
	ShowWindow(mShowDlg.hDlg,SW_SHOW);
	ShowWindow(mHistDlg.hDlg,SW_SHOW);

	CRect rt1, rt2, rt3;
	res = readINI(iniFile, &rt1, &rt2, &rt3);
	if (res & 1)	
		MoveWindow(hr, rt1.left, rt1.top, rt1.Width(), rt1.Height(), TRUE);
	if (res & 2)	
		mShowDlg.MoveWindow(rt2);
	if (res & 4)	
		mHistDlg.MoveWindow(rt3);
	
	SYSTEMTIME lt;
    GetLocalTime(&lt);	
	sprintf(buffer2, "//\t[%02d/%02d/%4d, %02d:%02d:%02d] AUXLAB %s begins------", lt.wMonth, lt.wDay, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, buffer);

	dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(mHistDlg.logfilename, "%s%s%s_%s.log", AppPath, fname, HISTORY_FILENAME, buf);

	LOGHISTORY(buffer2);

	while(1) 
	{
		res = ReadConsole(hStdin, buf, 256, &dw, NULL);
		char *ff = strpbrk(buf,"\r\n");
		if (ff==NULL) { MessageBox(NULL, "Input to ReadConsole doesn't contain \\r\\n","Did you press Ctrl-C? It will not close the program.",0); continue;}
		buf[ff-buf]=0;
		LOGHISTORY(buf)
		mHistDlg.AppendHist(buf);
		
		if (buf[0]=='#')
		{
			nOut = str2vect (tar, &buf[1], " ");
			if (tar[0]=="plot")			code = ID_PLOT;
			else if (tar[0]=="play")	code = ID_PLAY;
			else if (tar[0]=="play+")	code = ID_PLAYOVERLAP;
			else if (tar[0]=="play++")	code = ID_PLAYCONTINUE;
			else if (tar[0]=="play<>")	code = ID_PLAYLOOP;
			else if (tar[0]=="play-")	code = ID_PLAYENDLOOP;
			else if (tar[0]=="stop")	code = ID_STOP;
			else if (tar[0]=="hist")	code = ID_HISTORY;
			else if (tar[0]=="fs")		code = ID_SHOWFS;
			else if (tar[0]=="ws" || tar[0]=="workspace")		code = ID_SHOWVAR;
			else if (tar[0]=="clear")	{ 
				if (nOut==1) tar.push_back(""); 
				ClearVar(tar[1].c_str()); printf("AUX>");
				continue;}
			else if (tar[0]=="save")	code = ID_SAVE;  
			else if (tar[0]=="load")	code = ID_LOAD;
			else if (tar[0]=="quit")	{LOGHISTORY("//\t""quit""") break;}
			else						code = ID_ERR;
		}
		else
			code = ID_DEFAULT;

		buff = buf;
		if (code != ID_DEFAULT) buff += tar[0].length()+1;
		computeandshow(AppPath, code, buff);
	}
    GetLocalTime(&lt);	
	closeXcom(&lt,AppPath);
	return 1;
} 

void show_node_analysis() //Run this to node analysis
{
	FILE *fp = fopen("results.txt","wt");
	FILE *fp2 = fopen("auxlines.txt","rt");

	CAstSig main;
	AstNode node;
	char buf[256];
	while(fgets(buf, 256, fp2))
	{
	try {	main.SetNewScript(buf,&node);
			print_node_struct(0, fp, buf, &node);
			fprintf(fp, "\n");
		}
	catch (const char *errmsg) {
		fprintf(fp, "%s\n", errmsg);
		}
	}
	fclose(fp2);
	fclose(fp);
}

