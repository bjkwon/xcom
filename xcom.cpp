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
#include "xcom.h"

//#pragma comment(linker,"\"/manifestdependency:type='win32' \
//name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
//processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

char iniFile[256];

double playbackblock(100.);
uintptr_t hShowvarThread(NULL);
uintptr_t hHistoryThread(NULL);
char udfpath[4096];

vector<UINT> exc; // temp


xcom mainSpace;

CWndDlg wnd;
CShowvarDlg mShowDlg;
CHistDlg mHistDlg;
void* soundplayPt;
double block;

extern uintptr_t hDebugThread2;
extern map<string, CDebugDlg*> dbmap;
extern CDebugBaseDlg debugBase;
unsigned int WINAPI debugThread2 (PVOID var) ;


HANDLE hStdin, hStdout; 
string typestring(int type);
BOOL CALLBACK showvarDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK historyDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CtrlHandler( DWORD fdwCtrlType );
void nonnulintervals(CSignal *psig, string &out, bool unit, bool clearout=false);
size_t ReadThisLine(string &linebuf, HANDLE hCon, CONSOLE_SCREEN_BUFFER_INFO coninfo0, SHORT thisline, size_t promptoffset);

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

WORD readINI(const char *fname, CRect *rtMain, CRect *rtShowDlg, CRect *rtHistDlg, CRect *rtDebugDlg)
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
	if (ReadINI (errStr, fname, "DEBUG VIEW POS", strRead)>=0 && str2array (tar, 4, strRead.c_str(), " ")==4)
	{
		rtDebugDlg->left = tar[0];
		rtDebugDlg->top = tar[1];
		rtDebugDlg->right = tar[2] + tar[0];
		rtDebugDlg->bottom = tar[3] + tar[1];
		ret += 8;
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


int writeINI(const char *fname, char *estr, int fs, double _block, const char *path)
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

void closeXcom(const char *AppPath)
{
	//When CTRL_CLOSE_EVENT is pressed, you have 5 seconds. That's how Console app works in Windows.
	// Finish all cleanup work during this time.

	CAstSig *pcast = mainSpace.vecast.front();
	char estr[256], buffer[256];
	const char *sss = pcast->GetPath();
	const char* pt = strstr(pcast->GetPath(), AppPath); 
	string pathnotapppath;
	size_t loc;
	if (pt!=NULL)
	{
		pathnotapppath.append(pcast->GetPath(),  pcast->GetPath()-pt);
		string  str(pt + strlen(AppPath));
		loc = str.find_first_not_of(';');
		pathnotapppath.append(pt + strlen(AppPath)+loc);
	}
	else
		pathnotapppath.append(pcast->GetPath());
	int res = writeINI(iniFile, estr, pcast->pEnv->Fs, block, pathnotapppath.c_str());

	CRect rt1, rt2, rt3;
	GetWindowRect(GetConsoleWindow(), &rt1);
	mShowDlg.GetWindowRect(rt2);
	mHistDlg.GetWindowRect(rt3);
	res = writeINI(iniFile, estr, rt1,rt2, rt3);

	string debugudfs;
	for (map<string, CDebugDlg*>::iterator it=dbmap.begin(); it!=dbmap.end(); it++)
	{
		debugudfs += it->second->fullUDFpath; debugudfs += "\r\n";
	}
	if (debugudfs.size()>0)
	{
		if (!printfINI (estr, iniFile, "DEBUG VIEW", "%s", debugudfs.c_str())) strcpy(buffer, estr); 
		map<string, CDebugDlg*>::iterator it=dbmap.begin(); 
		GetWindowRect(it->second->hParent->hDlg, &rt1);
		CString str;
		str.Format("%d %d %d %d", rt1.left, rt1.top, rt1.Width(), rt1.Height());
		if (!printfINI (estr, iniFile, "DEBUG VIEW POS", "%s", str.c_str())) {strcpy(buffer, estr);/*do something*/ }

	}

	SYSTEMTIME lt;
	GetLocalTime(&lt);	
	vector<string> in;
	sprintf(buffer, "//\t[%02d/%02d/%4d, %02d:%02d:%02d] AUXLAB closes------", lt.wMonth, lt.wDay, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
	in.push_back(buffer);
	mainSpace.LogHistory(in);
	if (hShowvarThread!=NULL) PostThreadMessage(GetWindowThreadProcessId(mShowDlg.hDlg, NULL), WM__ENDTHREAD, 0, 0);
	if (hHistoryThread!=NULL) PostThreadMessage(GetWindowThreadProcessId(mHistDlg.hDlg, NULL), WM__ENDTHREAD, 0, 0);
	if (hDebugThread2!=NULL)
	{
		map<string, CDebugDlg*>::iterator it=dbmap.begin(); 
		if (it!=dbmap.end())
			PostThreadMessage(GetWindowThreadProcessId((it->second)->hDlg, NULL), WM__ENDTHREAD, 0, 0);
	}
	fclose (stdout);
	fclose (stdin);
	for (vector<CAstSig*>::iterator it = mainSpace.vecast.begin()+1; it != mainSpace.vecast.end(); it++)
		delete *it;

}

unsigned int WINAPI histThread (PVOID var) 
{
	HWND hhh = GetConsoleWindow();
	mHistDlg.hDlg = CreateDialog (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY), GetConsoleWindow(), (DLGPROC)historyDlg);
	if (mHistDlg.hDlg==NULL) {MessageBox(NULL, "History Dlgbox creation failed.","AUXLAB",0); return 0;}

	ShowWindow(mHistDlg.hDlg, SW_SHOW);

	CRect rt1, rt2, rt3, rt4;
	int res = readINI(iniFile, &rt1, &rt2, &rt3, &rt4);
	if (res & 4)	
		mHistDlg.MoveWindow(rt3);
	else
	{
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
	}

	SetFocus(GetConsoleWindow()); //This seems to work.
	SetForegroundWindow (GetConsoleWindow());

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

	mShowDlg.hDlg = CreateDialog (hModule, MAKEINTRESOURCE(IDD_SHOWVAR), GetConsoleWindow(), (DLGPROC)showvarDlg);
	ShowWindow(mHistDlg.hDlg,SW_SHOW);
	mShowDlg.hList1 = GetDlgItem(mShowDlg.hDlg , IDC_LIST1);
	
	RECT rc;
	CRect rt1, rt2, rt3, rt4;
	int res = readINI(iniFile, &rt1, &rt2, &rt3, &rt4);
	if (res & 2)	
		mShowDlg.MoveWindow(rt2);
	else
	{
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
	}
	if (!(res & 8)) // if rt4 is not available from readINI() above, place it bottom right corner of main window
	{
		GetWindowRect(GetConsoleWindow(), &rc);
		rt4 = rc;
		CRect tp(rt4.CenterPoint(), rt4.BottomRight());
		rt4 = tp + CPoint(30,30);
	}
	if ((hDebugThread2 = _beginthreadex (NULL, 0, debugThread2, (void*)&rt4, 0, NULL))==-1)
		MessageBox (GetConsoleWindow(), "Debug Thread Creation Failed.", 0, 0);

	SetFocus(GetConsoleWindow()); //This seems to work.
	SetForegroundWindow (GetConsoleWindow());

	MSG msg ;
	HACCEL hAcc = LoadAccelerators (hModule, MAKEINTRESOURCE(IDR_XCOM_ACCEL));

	//FILE *fpp = fopen("c:\\temp\\rec","wt"); 
	//fclose(fpp);

	exc.push_back(WM_NCHITTEST);
	exc.push_back(WM_SETCURSOR);
	exc.push_back(WM_MOUSEMOVE);
	exc.push_back(WM_NCMOUSEMOVE);
	exc.push_back(WM_WINDOWPOSCHANGING);
	exc.push_back(WM_WINDOWPOSCHANGED);
	exc.push_back(WM_CTLCOLORDLG);
	exc.push_back(WM_NCPAINT);
	exc.push_back(WM_GETMINMAXINFO);
	exc.push_back(WM_MOVE);
	exc.push_back(WM_MOVING);
	exc.push_back(WM_PAINT);
	exc.push_back(WM_NCMOUSEMOVE);
	exc.push_back(WM_ERASEBKGND);
	exc.push_back(WM_TIMER);
//	bool printthis;

	while (GetMessage (&msg, NULL, 0, 0))
	{
		if (msg.message==WM__ENDTHREAD) 
			_endthreadex(33);
//		printthis = spyWM(msg.hwnd, msg.message, msg.wParam, msg.lParam, "c:\\temp\\rec", exc, "showvar MessageLoop")>0;
//		if (printthis) fpp=fopen("c:\\temp\\rec","at"); 
		if (!TranslateAccelerator (mShowDlg.hDlg, hAcc, &msg))
		{
			TranslateMessage (&msg) ;
			DispatchMessage (&msg) ;
//			if (printthis)  fprintf(fpp, "TranslateMessage/DispatchMessage\n"); 
		}
		//else
		//{
		//	if (printthis)  fprintf(fpp, "TranslateAccelerator_mShowDlg.hDlg success.\n"); 
		//}
//		if (printthis) fclose(fpp);
	}
	hShowvarThread=NULL;
	return 0;
}


#define	ISTHISSTR(STR) (!strcmp(pnode->str,STR))

bool need2echo(const AstNode *pnode)
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

xcom::xcom()
:nHistFromFile(50), comPrompt("AUX>")
{

}

xcom::~xcom()
{

}

void xcom::out4console(string varname, CSignals *psig, string &out)
{
	size_t count;
	char buf[256];
	varname += " ";
	string varname2;
	if (psig->IsLogical()) 
	{
		out += "(logical) "; out += varname; 
		for (int k=0; k< min (psig->nSamples, DISPLAYLIMIT*4); k++) 
			{sprintf(buf,"%d ", psig->logbuf[k]); out+=buf;}
		out +="\n";
		return;
	}
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
			out += " audio(R) ";
			nonnulintervals (psig->next, out, true);
			out +="\n";
			}
		break;
	case CSIG_VECTOR:
	case CSIG_SCALAR:
		if (psig->IsComplex())
		{
			for (int k=0; k< min (psig->nSamples, DISPLAYLIMIT); k++) 
			{
				if (psig->buf[2*k+1]==1.)
					sprintf(buf,"%g+i ", psig->buf[2*k]), out+=buf;
				else
					sprintf(buf,"%g%+gi ", psig->buf[2*k], psig->buf[2*k+1]), out+=buf;
			}
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

void xcom::showarray(const AstNode *pnode)
{
	CAstSig *pabteg = vecast.back();
	CSignals tp;
	AstNode *p;
	GRAFWNDDLGSTRUCT in;
	string varname, out;
	if (pnode)
		switch(pnode->type)
		{
		case T_IF:
		case T_WHILE:
		case T_SWITCH:
		case T_FOR:
		case T_SIGMA:
			break;
		case T_ID: // noncell_variable or cellvar{index}
			if (!pnode->child) // this means noncell_variable 
			{
				varname = pnode->str; varname += " =\n";
				out4console(varname, pabteg->GetVar(pnode->str), out);
				// notice that this break is inside the bracket 
				break; // That means.... if cellvar{index}, passing through
			}
		case NODE_CALL: // variable(index)
			if (need2echo(pnode))
			{
				tp = pabteg->Sig;
				pabteg->SetVar("ans", tp);
				out4console("ans =\n", &tp, out);
			}
			break;
		case NODE_INITCELL: // cell definition
			mShowDlg.changed=true; 
			if (pnode->str)
			{
				out4console(pnode->str, pabteg->RetrieveVar(pnode->str), out);
				mShowDlg.SendMessage(WM__VAR_CHANGED, (WPARAM)pnode->str, (LPARAM)&pabteg->Sig);
			}
			else
			{
				pabteg->SetVar("ans", pabteg->Sig);
				out4console("ans =\n", &pabteg->Sig, out);
				mShowDlg.SendMessage(WM__VAR_CHANGED, (WPARAM)"ans", (LPARAM)&pabteg->Sig);
			}
			break;
		case NODE_BLOCK: // MULTIPLE statements.
			{
				string script(pabteg->GetScript());
				for (p = pnode->child; p && p->child; p=p->next)
				{
					if (p->type==T_IF || p->type==T_WHILE || p->type==T_SWITCH || p->type==T_FOR || p->type==T_SIGMA )
						showarray (p);
					else if (p->str)
					{
						size_t found = script.find(';', p->col-1);
						if ( (p->next && (int)found > p->next->col) || // ; found but outside the current node 
							found==string::npos) // ; not found
						{
							// if p->type is NODE_CALL, p->str shows the name of function call (so don't call Eval)
							if (p->type != NODE_CALL && !pabteg->RetrieveVar(p->str))
								pabteg->Eval(p);
							if (p->type == NODE_CALL)
							{
								if (!p->next) // For function call, show only for the last one
									showarray (p);
							}
							else
								showarray (p);
						}
					}
				}
			}
			break;

		default: // any statement
			if (pnode->str && pnode->type!=T_STRING)
			{
				CSignals *tpp = pabteg->GetSig(pnode->str);
				if (!tpp)
					tp = pabteg->Sig;
				else
				{
					tp = *tpp;
					if (tp.cell.size()>0) out4console(pnode->str, &tp, out);
					else
					{
						varname = pnode->str; varname += " =\n";
						out4console(varname, &tp, out);
					}
				}
			}
			else // Extraction. Expression, no variable
			{
				if (pnode->alt) // alt is used for multiple outputs ... (only UDF for now) 
				{
					for (p = pnode->alt->child; p; p=p->next)
						showarray (p);
				}
				else
				{
					pabteg->SetVar("ans", pabteg->Sig);
					out4console("ans =\n", &pabteg->Sig, out);
				}
			}
			break;
		}
	if (out.length()>0) printf("%s",out.c_str());
}

bool isUpperCase(char c)
{
	return (c>64 && c<91);
}
bool isLowerCase(char c)
{
	return (c>96 && c<123);
}

char alphas[]={"QWERTYUIOPASDFGHJKLZXCVBNMqwertyuioplkjhgfdsazxcvbnm"};
char nums[]={"1234567890."};
char alphanums[]={"1234567890.QWERTYUIOPASDFGHJKLZXCVBNMqwertyuioplkjhgfdsazxcvbnm"};

size_t xcom::ctrlshiftleft(const char *buf, DWORD offset)
{
	//RETURNS the position satisfying the condition (zero-based)
	size_t len = strlen(buf);
	char copy[4096];
	strcpy(copy, buf);
	copy[len-offset]=0;
	string str(copy);
	size_t res, res1, res2, res12(0);
	if (isalpha(copy[len-offset-1]))
	{
		if ((res = str.find_last_not_of(alphas))!=string::npos)
			return str.find_first_of(alphas, res);
		else					
			return str.find_first_of(alphas);
	}
	else if (isdigit(copy[len-offset-1]))
	{
		//return the idx of first numeric char
		res = str.find_last_not_of(nums);
			return str.find_first_of(nums, res);
	}
	else
	{
		res1 = str.find_last_of(alphas);
		res2 = str.find_last_of(nums);
		if (res1==string::npos)
		{
			if (res2!=string::npos) 
				res12 = res2;
		}
		else
		{
			if (res2!=string::npos) 
				res12 = max(res1,res2)+1;
			else
				res12 = res1;
		}
		return res12;
	}
	return 0;
}

size_t xcom::ctrlshiftright(const char *buf, DWORD offset)
{
	//RETURNS the position satisfying the condition (zero-based)
	size_t len = strlen(buf);
	string str(buf);
	size_t res, res1, res2, res12(0);
	size_t inspt = len-offset+1;
	if (isalpha(buf[inspt]))
	{
		res = str.find_first_not_of(alphas,len-offset);
		return str.find_last_of(alphas, res);
	}
	else if (isdigit(buf[inspt]))
	{
		//return the idx of first numeric char
		res = str.find_first_not_of(nums,len-offset);
		return str.find_last_of(nums, res);
	}
	else
	{
		res1 = str.find_first_of(alphas,inspt);
		res2 = str.find_first_of(nums,inspt);
		if (res1==string::npos)
		{
			if (res2!=string::npos) 
				res12 = res2;
		}
		else
		{
			if (res2!=string::npos) 
				res12 = min(res1,res2);
			else
				res12 = res1;
		}
		return res12;
	}
	return 0;
}

void xcom::gendebugcommandframe()
{
	for (int k=0; k<6;k++)
	{
		debug_command_frame[k].EventType = KEY_EVENT;
		debug_command_frame[k].Event.KeyEvent.bKeyDown = debug_command_frame[k].Event.KeyEvent.wRepeatCount = 1;
		debug_command_frame[k].Event.KeyEvent.dwControlKeyState = 0x20;
	}
	debug_command_frame[0].Event.KeyEvent.dwControlKeyState = 0x30;
	debug_command_frame[5].Event.KeyEvent.dwControlKeyState = 0x120;
	debug_command_frame[5].Event.KeyEvent.uChar.AsciiChar = 0x0d;
	debug_command_frame[5].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
}


bool xcom::isdebugcommand(INPUT_RECORD *in, int len)
{
	char readbuf[32];
	if (len==6)
	{
		for (int k=0; k<6;k++)
		{
			if ( debug_command_frame[k].Event.KeyEvent.bKeyDown != in[k].Event.KeyEvent.bKeyDown ) return false;
			if ( debug_command_frame[k].Event.KeyEvent.dwControlKeyState != in[k].Event.KeyEvent.dwControlKeyState ) return false;
			if ( debug_command_frame[k].Event.KeyEvent.wRepeatCount != in[k].Event.KeyEvent.wRepeatCount ) return false;
		}
		if (in[5].Event.KeyEvent.wVirtualKeyCode != debug_command_frame[5].Event.KeyEvent.wVirtualKeyCode) return false;
		if (in[5].Event.KeyEvent.uChar.AsciiChar != debug_command_frame[5].Event.KeyEvent.uChar.AsciiChar) return false;

		int p=0;
		for (; p<5;p++)	readbuf[p] = in[p].Event.KeyEvent.uChar.AsciiChar;
		readbuf[p]=0;
		if (!strcmp(readbuf, "#cont")) return true;
		if (!strcmp(readbuf, "#step")) return true;
		if (!strcmp(readbuf, "#stin")) return true;
		if (!strcmp(readbuf, "#abor")) return true;
		if (!strcmp(readbuf, "#r2cu")) return true;
	}
	return false;
}

bool xcom::debugcommand(const char* cmd)
{
	char buf[2048];
	strcpy_s(buf, 2048, cmd);
	char *ff = strpbrk(buf,"\r\n");
	if (ff) ff[0]=0;
	if (!strcmp(buf,"#step")) return true;
	if (!strcmp(buf,"#cont")) return true;
	if (!strcmp(buf,"#exit")) return true;
	if (!strcmp(buf,"#stin")) return true;
	if (!strcmp(buf,"#abor")) return true;
	if (!strcmp(buf,"#r2cu")) return true;
	return false;
}


void xcom::console()
{
	char buf[4096];
	memset(buf,0,4096);
	while(1) 
	{
		trimLeft(buf,"\xff");
		trimRight(buf,"\xff");
		getinput(buf); // this is a holding line.
		trimLeft(buf,"\xff");
		trimRight(buf,"\r\n\xff");
		if (mainSpace.computeandshow(buf)==-1) break;
	}
}

int xcom::cleanup_debug()
{
	while (mShowDlg.SendDlgItemMessage(IDC_DEBUGSCOPE, CB_DELETESTRING, 1)!=CB_ERR) {}
	mShowDlg.SendDlgItemMessage(IDC_DEBUGSCOPE, CB_SETCURSEL, 0);

	// Do memory clean up of sub in CallUDF()
	return 0;
}

bool IsConditionalLoopType(const AstNode *p)
{
	switch(p->type)
	{
	case T_IF:
	case T_WHILE:
	case T_SWITCH:
	case T_FOR:
	case T_SIGMA:
		return true;
	default:
		return false;
	}
}

int xcom::computeandshow(const char *in, const AstNode *pCall)
{
	CAstSig *pabteg = vecast.back();
	bool err(false);
	size_t nItems, k(0);
	string input(in);
	trim(input, " \t\r\n");
	trimr(input, "\r\n");
	if (input.size()>0)
	try {
		pabteg->SetNewScript(input.c_str());
		if (pabteg->pAst->type==NODE_BLOCK && !IsConditionalLoopType(pabteg->pAst))
		{
			AstNode *p = pabteg->pAst->next;
			while (p)
			{
				try {
				pabteg->Sig = pabteg->Compute(p);
				if (!p->suppress)
					showarray(p);
				p=p->next;
				}
				catch (const CAstException &e) {
					char errmsg[2048];
					strncpy(errmsg, e.getErrMsg().c_str(), sizeof(errmsg)/sizeof(*errmsg));
					errmsg[sizeof(errmsg)/sizeof(*errmsg)-1] = '\0';
					throw errmsg;
				}
			}
		}
		else
		{
			pabteg->Sig = pabteg->Compute();
			if ( !pabteg->pAst->suppress)
				showarray(pabteg->pAst);
		}
	}
	catch (const char *errmsg) {
		cout << "ERROR: " << errmsg << endl;	 
	}
	catch (CAstSig *ast) 
	{ // this was thrown by aux_HOOK
		if (ast->dstatus == aborting)
		{
			ast->cleanup_sons();
			cleanup_debug();
		}
		else
		{
		try {
			string HookName;
			char buf[2048];
			vector<string> tar;
			input = ast->GetScript();
			nItems = str2vect(tar, input.c_str(), " ");
			if (nItems==0) return 0;
			if (tar[0]=="#")
			{
				if (nItems==1) return 0;
				HookName = tar[1];
			}
			else
				HookName = tar[0].substr(1);
			if (nItems==1)
				buf[0]=0;
			else
				strcpy(buf, input.substr(input.find(tar[1])).c_str());
	
		if (hook(ast, HookName, buf)==-1)	
				return -1;	
		}
		catch (const char *errmsg) {
		cout << "ERROR:" << errmsg << endl;	 }	
		}
	}
	if (input.size()==0)
		ShowWS_CommandPrompt(NULL);
	else
		ShowWS_CommandPrompt(pabteg);
	return pCall? 1:0;
}

int xcom::hook(CAstSig *ast, string HookName, const char* argstr)
{
	char errstr[256], buf[MAX_PATH], drive[64], dir[MAX_PATH], filename[MAX_PATH], ext[MAX_PATH], buffer[MAX_PATH];
	string name, fname;
	size_t k(0), nItems;
	vector<string> varlist, tar;
	if (HookName=="play" || HookName=="stopplay" || HookName=="playnext" || HookName=="playloop")
	{
		if (!argstr[0]) return 0;
		if (str2vect(tar, argstr, " ")>1)
			throw "only one argument allowed";
		CAstSig tp(ast);
		tp.SetNewScript(argstr);
		tp.Compute(); 
		if (HookName=="stopplay") TerminatePlay();
		if (tp.Sig.GetType()==CSIG_AUDIO)
		{
			if (HookName=="playnext")
				tp.Sig.PlayArrayNext(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr);
			else if (HookName!="playloop")
				tp.Sig.PlayArray(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr);
			else
			{
				LoopPlay(true);
				tp.Sig.PlayArray(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr, 1); // looping
			}
		}
		else
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
	}
	else if (HookName=="stop")
	{
		TerminatePlay();
	}
	else if (HookName=="hist")
	{
		ShowWindow(mHistDlg.hDlg,SW_SHOW);
	}
	else if (HookName=="quit")
	{
		return -1;
	}
	else if (HookName=="load")
	{
		if (str2vect(tar, argstr, " ")>1)
			throw "only one argument allowed";
		FILE *fp = ast->OpenFileInPath(argstr, "axl", name);
		if (fp)
		{
			if (load_axl(fp, errstr)==0) 
				printf("File %s reading error----%s.\n", argstr, errstr);
			fclose(fp);
			mShowDlg.Fillup(&ast->Vars);
		}
		else
		{
			strcpy(errstr, argstr); strcat(errstr,".axl");
			printf("File %s not found in %s\n", errstr, ast->pEnv->AuxPath.c_str()); 
		}
	}
	else if (HookName=="save")
	{
		string savesuccess("");
		if ((nItems = str2vect(tar, argstr, " "))==0)
			throw "#save (filename) var1 var2 var3...\n";
		_splitpath(tar[0].c_str(), drive, dir, filename, ext);
		if (strlen(drive)+strlen(dir)==0) 
			FulfillFile(buffer, AppPath, filename), fname=buffer;
		else
			fname = tar[0];
		if (strlen(ext)==0) fname += ".axl";
		else fname += ext;

		if (!argstr[0]) // no variables indicated.. save all
		{
			ast->EnumVar(varlist);
			size_t sz = varlist.size();
			nItems = sz+1;
			for (unsigned int k=1; sz>0 && k<nItems; k++)  
				tar.push_back(varlist[k-1]);
		}
		//arg must be variable (i.e., T_ID)
		//for (args = args->child; args; args = args->next)
		//	if (args->type !=T_ID)  
		//		throw "Argument must be a variable";
		bool err;
		FILE *fp ;
		if (fp = fopen(fname.c_str(), "wb")) 
		{
			printf("File %s saved with ", fname.c_str());
			for (unsigned int k=1; k<nItems; k++)
			{
				if (save_axl(fp, tar[k].c_str(), errstr)==0)
				{
					if (savesuccess.length()>0)
						printf("\nSave error----%s.\n %s are saved OK.", errstr, savesuccess.c_str());
					else
						printf("\nSave error----%s.", errstr);
					fclose(fp);
					err = true;
					break;
				}
				else
					printf("%s ", tar[k].c_str());
				savesuccess += tar[k].c_str() + string(" ");
				fclose(fp);
				fp=fopen(fname.c_str(), "ab");
			}
			printf("\n");
		}
		else
			printf("File %s cannot be open for writing.\n", tar[0].c_str());
		if (!err)	printf("%s \n", savesuccess.c_str());
		fclose(fp);
	}
	else if (HookName=="fs")
	{
		printf("Sampling Rate=%d\n", ast->pEnv->Fs);
	}
	else if (HookName=="workspace" || HookName=="ws")
	{
		mShowDlg.Fillup();
		mShowDlg.changed=false;
		ShowWindow(mShowDlg.hDlg,SW_SHOW);
		SetForegroundWindow (mShowDlg.hDlg); 
	}
	else if (HookName=="clear")
	{
		string savesuccess("");
		nItems = str2vect(tar, argstr, " ");
		int success(0);
		if (nItems>0) savesuccess = "Variable(s) cleared: ";
		for (; k<nItems; k++)
			if (ast->ClearVar(tar[k].c_str()))
				success++, sprintf(buf, "%s ", tar[k].c_str()), savesuccess+= buf;
		if (success) printf("%s\n", savesuccess.c_str()), mShowDlg.Fillup(&ast->Vars);
	}
	else if (HookName=="plot")
	{
		nItems = str2vect(tar, argstr, " ");
		CAstSig tp(ast);
		GRAFWNDDLGSTRUCT in;
		for (size_t k=0; k<nItems; k++)
		{
			tp.SetNewScript(tar[k].c_str());
			tp.Compute(); 
			strcpy(buf, tar[k].c_str());
			CSignals gcf;
			CRect rt(0, 0, 500, 310);
			HANDLE fig = OpenGraffy(rt, buf, GetCurrentThreadId(), mShowDlg.hDlg, in);
			HANDLE ax = AddAxis (fig, .08, .18, .86, .72);
			CAxis *cax = static_cast<CAxis *>(ax);
			if (ast->Sig.GetType()==CSIG_VECTOR)
				PlotCSignals(ax, tp.Sig, 0xff0000, '*', LineStyle_noline); // plot for VECTOR, no line and marker is * --> change if you'd like
			else
				PlotCSignals(ax, tp.Sig, 0xff0000);
			if (tp.Sig.next)
				PlotCSignals(ax, *tp.Sig.next, RGB(200,0,50));
			CFigure *cfig = static_cast<CFigure *>(fig);
		}
	}
	else
	{
		sprintf(errstr, "Undefined HOOK name: %s", HookName.c_str());
		throw errstr;
	}
	return 0;
}

bool xcom::dbmapfind(const char* udfname)
{
	map<string, CDebugDlg*>::iterator it;
	return (it = dbmap.find(udfname))!=dbmap.end(); 
}

void xcom::LogHistory(vector<string> input)
{
	FILE* logFP = fopen(mHistDlg.logfilename,"at"); 
	if (logFP) 
	{
		for (size_t k=0; k<input.size(); k++)
		{
			if (input[k].size()>0)
				fprintf(logFP, "%s\n", input[k].c_str()); 
		}
		fclose(logFP); 
	}
	else
	{
		char temp[256]; 
		sprintf(temp, "fopen error: %s", mHistDlg.logfilename); 
		::MessageBox(mHistDlg.hDlg, temp, "LOGHISTORY", 0); 
	}
}

void xcom::ShowWS_CommandPrompt(CAstSig *pcast)
{
	if (pcast && pcast->pAst)
	{
		mShowDlg.pcast = pcast;
		mShowDlg.Fillup();
		//if during debugging, redraw all figure windows with the varname in the debugging scope
		if (pcast->pCall) // is this right keyword for debugging??
			mShowDlg.OnVarChanged(pcast);
		else
		{
			AstNode *p = (pcast->pAst->type==NODE_BLOCK) ? pcast->pAst->next : pcast->pAst;
			for (; p; p = p->next)
				if (p->type!=T_ID && p->type!=T_NUMBER && p->type!=T_STRING) //For these node types, 100% chance that var was not changed---do not send WM__VAR_CHANGED
					mShowDlg.OnVarChanged(p->str ? p->str : "ans", NULL);
		}
		if (pcast->statusMsg.length()>0) cout << pcast->statusMsg.c_str() << endl;
		pcast->statusMsg.clear();
	}
	if (vecast.size()==1)
	{
		string line;
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		GetConsoleScreenBufferInfo(hStdout, &coninfo);
		size_t res = ReadThisLine(line, hStdout, coninfo, coninfo.dwCursorPosition.Y, 0);
		if (res>0) printf("\n");
		mainSpace.comPrompt = "AUX>";
		printf(mainSpace.comPrompt.c_str());
	}
	else
	{
		if (!pcast || pcast->dstatus==null)
		{
			mainSpace.comPrompt = "K>";
			printf(mainSpace.comPrompt.c_str());
		}
	}
}

size_t xcom::ReadHist()
{
	DWORD dw;
	HANDLE hFile = CreateFile(mHistDlg.logfilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile==INVALID_HANDLE_VALUE) return 0;
	LARGE_INTEGER fsize;
	if (!GetFileSizeEx(hFile, &fsize)) 
	{ CloseHandle(hFile); MessageBox(mHistDlg.hDlg, "GetFileSizeEx error", "ReadHist()", 0);  return 0;	}

	__int64 size = fsize.QuadPart;
	int sizelimit = 0xffff; //65535;
	if (fsize.HighPart>0)
	{
		LONG high(-fsize.HighPart);
		dw = SetFilePointer (hFile, sizelimit, &high, FILE_END);
	}
	else
		dw = SetFilePointer (hFile, -sizelimit, NULL, FILE_END);
	char *buffer = new char[sizelimit+1];
	BOOL res = ReadFile(hFile, buffer, sizelimit, &dw, NULL);
	buffer[dw]=0;

	string str(buffer), tempstr;
	size_t pos, pos0 = str.find_last_of("\r\n");
	size_t count(0);
	vector<string> _history;
	while (count<nHistFromFile)
	{
		pos = str.find_last_of("\r\n", pos0-2);
		tempstr = str.substr(pos+1, pos0-pos-2);
		trim(tempstr, " ");
		if (!tempstr.empty())
		{
			if (tempstr[0]!='/' || tempstr[1]!='/')
				count++, _history.push_back(tempstr);
		}
		pos0 = pos;
	}
	delete[] buffer;
	CloseHandle(hFile); 
	history.reserve(nHistFromFile*10);
	for (vector<string>::reverse_iterator rit=_history.rbegin(); rit!=_history.rend(); rit++)
		history.push_back(*rit);
	return count;
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
	char estr[256], buf[256];
	int res;
	string addp;
	vector<string> tar;
	int fs;
	char fullmoduleName[MAX_PATH], moduleName[MAX_PATH];
 	char drive[16], dir[256], ext[8], fname[MAX_PATH], buffer[MAX_PATH], buffer2[MAX_PATH];

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
 	sprintf (mainSpace.AppPath, "%s%s", drive, dir);
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
	DWORD dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(iniFile, "%s%s_%s.ini", mainSpace.AppPath, fname, buf);
	res = readINI(iniFile, estr, fs, block, udfpath);
	CAstSigEnv globalEnv(fs);
	CAstSig cast(&globalEnv);
//	cast.Reset(fs,""); //	mainSpace.cast.Sig.Reset(fs); is wrong...
	addp = mainSpace.AppPath;
	if (strlen(udfpath)>0 && udfpath[0]!=';') addp += ';';	
	addp += udfpath;
	globalEnv.SetPath(addp.c_str());

	if ((hShowvarThread = _beginthreadex (NULL, 0, showvarThread, NULL, 0, NULL))==-1)
		::MessageBox (hr, "Showvar Thread Creation Failed.", "AUXLAB mainSpace", 0);
	if ((hHistoryThread = _beginthreadex (NULL, 0, histThread, (void*)mainSpace.AppPath, 0, NULL))==-1)
		::MessageBox (hr, "History Thread Creation Failed.", "AUXLAB mainSpace", 0);
	
	freopen( "CON", "w", stdout ) ;
	freopen( "CON", "r", stdin ) ;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD fdwMode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS; 
//    DWORD fdwMode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE; 
	SetConsoleMode(hStdin, fdwMode) ;
	if (!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE )) ::MessageBox (hr, "Console control handler error", "AUXLAB", MB_OK);

	fdwMode = ENABLE_PROCESSED_OUTPUT |  ENABLE_WRAP_AT_EOL_OUTPUT ;
	res = SetConsoleMode(hStdout, fdwMode) ;

	dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(mHistDlg.logfilename, "%s%s%s_%s.log", mainSpace.AppPath, fname, HISTORY_FILENAME, buf);
	size_t nHistFromFile = mainSpace.ReadHist();
	mainSpace.comid = nHistFromFile;

	WriteConsole (hStdout, mainSpace.comPrompt.c_str(), (DWORD)mainSpace.comPrompt.size(), &dw, NULL);

	while (mShowDlg.hDlg==NULL) {
		Sleep(100); 
		mShowDlg.pcast = &cast;
		mShowDlg.Fillup();
		mShowDlg.changed=false;
	}
	mShowDlg.pcast = &cast;
	ShowWindow(mShowDlg.hDlg,SW_SHOW);
	ShowWindow(mHistDlg.hDlg,SW_SHOW);

	CRect rt1, rt2, rt3, rt4;
	res = readINI(iniFile, &rt1, &rt2, &rt3, &rt4);
	if (res & 1)	
		MoveWindow(hr, rt1.left, rt1.top, rt1.Width(), rt1.Height(), TRUE);
	
	SYSTEMTIME lt;
    GetLocalTime(&lt);	
	sprintf(buffer2, "//\t[%02d/%02d/%4d, %02d:%02d:%02d] AUXLAB %s begins------", lt.wMonth, lt.wDay, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, buffer);
	vector<string> in;
	in.push_back(buffer2);
	mainSpace.LogHistory(in);

	mainSpace.vecast.push_back(&cast);
	mainSpace.vecnodeUDF.push_back(NULL);

	CONSOLE_CURSOR_INFO concurinf;

	concurinf.bVisible = 1;
	concurinf.dwSize = 25;
	SetConsoleCursorInfo (hStdout, &concurinf);

	mainSpace.gendebugcommandframe();
	mainSpace.console();
	closeXcom(mainSpace.AppPath);
	return 1;
} 

void show_node_analysis() //Run this to node analysis
{
	FILE *fp = fopen("results.txt","wt");
	FILE *fp2 = fopen("auxlines.txt","rt");

	CAstSig main(mainSpace.vecast.front()->pEnv);
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

