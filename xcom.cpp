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

static FILE *fp1;

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

extern uintptr_t hDebugThread;
extern map<string, CDebugDlg*> dbmap;

HANDLE hStdin, hStdout; 
string typestring(int type);
BOOL CALLBACK showvarDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK historyDlg (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CtrlHandler( DWORD fdwCtrlType );
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

unsigned int WINAPI histThread (PVOID var) 
{
	HWND hhh = GetConsoleWindow();
	mHistDlg.hDlg = CreateDialog (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY), GetConsoleWindow(), (DLGPROC)historyDlg);
	if (mHistDlg.hDlg==NULL) {MessageBox(NULL, "History Dlgbox creation failed.","AUXLAB",0); return 0;}

	ShowWindow(mHistDlg.hDlg, SW_SHOW);

	CRect rt1, rt2, rt3;
	int res = readINI(iniFile, &rt1, &rt2, &rt3);
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

extern HWND hDebugList; //temp

unsigned int WINAPI showvarThread (PVOID var) // Thread for variable show
{
	HINSTANCE hModule = GetModuleHandle(NULL);

	mShowDlg.hDlg = CreateDialog (hModule, MAKEINTRESOURCE(IDD_SHOWVAR), GetConsoleWindow(), (DLGPROC)showvarDlg);
	ShowWindow(mHistDlg.hDlg,SW_SHOW);
	mShowDlg.hList1 = GetDlgItem(mShowDlg.hDlg , IDC_LIST1);
	
	CRect rt1, rt2, rt3;
	int res = readINI(iniFile, &rt1, &rt2, &rt3);
	if (res & 2)	
		mShowDlg.MoveWindow(rt2);
	else
	{
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
	}
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
:nHistFromFile(50)
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

void xcom::showarray(int code, const AstNode *pnode)
{
	CAstSig *pabteg = vecast.back();
	CSignals tp;
	AstNode *p;
	GRAFWNDDLGSTRUCT in;
	string varname, out;
	char buf[256], errstr[256];
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
				out4console(varname, &pabteg->GetTag(pnode->str), out);
				// notice that this break is inside the bracket 
				break; // That means.... if cellvar{index}, passing through
			}
		case NODE_CALL: // variable(index)
			if (need2echo(pnode))
			{
				tp = pabteg->Sig;
				pabteg->SetTag("ans", tp);
				out4console("ans =\n", &tp, out);
			}
			break;
		case NODE_INITCELL: // cell definition
			mShowDlg.changed=true; 
			if (pnode->str)
			{
				out4console(pnode->str, pabteg->RetrieveTag(pnode->str), out);
				mShowDlg.SendMessage(WM__VAR_CHANGED, (WPARAM)pnode->str, (LPARAM)&pabteg->Sig);
			}
			else
			{
				pabteg->SetTag("ans", pabteg->Sig);
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
						showarray (ID_DEFAULT, p);
					else if (p->str)
					{
						size_t found = script.find(';', p->column-1);
						if ( (p->next && (int)found > p->next->column) || // ; found but outside the current node 
							found==string::npos) // ; not found
						{
							// if p->type is NODE_CALL, p->str shows the name of function call (so don't call Eval)
							if (p->type != NODE_CALL && !pabteg->RetrieveTag(p->str))
								pabteg->Eval(p);
							if (p->type == NODE_CALL)
							{
								if (!p->next) // For function call, show only for the last one
									showarray (ID_DEFAULT, p);
							}
							else
								showarray (ID_DEFAULT, p);
						}
					}
				}
			}
			break;

		default: // any statement
			if (pnode->str)
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
						showarray (ID_DEFAULT, p);
				}
				else
				{
					pabteg->SetTag("ans", pabteg->Sig);
					out4console("ans =\n", &pabteg->Sig, out);
				}
			}
			break;
		}
	if (out.length()>0) printf("%s",out.c_str());
	switch(code)
	{
	case ID_PLOT:
		{
			strcpy(buf, pabteg->GetScript().c_str());
			CSignals gcf;
			CRect rt(0, 0, 500, 310);
			HANDLE fig = OpenGraffy(rt, buf, GetCurrentThreadId(), mShowDlg.hDlg, in);
			HANDLE ax = AddAxis (fig, .08, .18, .86, .72);
			CAxis *cax = static_cast<CAxis *>(ax);
			if (pabteg->Sig.GetType()==CSIG_VECTOR)
				PlotCSignals(ax, pabteg->Sig, 0xff0000, '*', LineStyle_noline); // plot for VECTOR, no line and marker is * --> change if you'd like
			else
				PlotCSignals(ax, pabteg->Sig, 0xff0000);
			if (pabteg->Sig.next)
				PlotCSignals(ax, *pabteg->Sig.next, RGB(200,0,50));
			CFigure *cfig = static_cast<CFigure *>(fig);

		}
		break;

	case ID_PLAY:
		TerminatePlay();
	case ID_PLAYOVERLAP:
		if (pabteg->Sig.GetType()==CSIG_AUDIO)
			pabteg->Sig.PlayArray(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr);
		else if (tp.GetType()!=CSIG_SCALAR)
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
		break;

	case ID_PLAYLOOP:
	case ID_PLAYENDLOOP:
		if (pabteg->Sig.GetType()==CSIG_AUDIO)
		{
			LoopPlay(code==ID_PLAYLOOP);
			pabteg->Sig.PlayArray(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr, 1); // looping
		}
		else
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
		break;

	case ID_PLAYCONTINUE:
		if (pabteg->Sig.GetType()==CSIG_AUDIO)
			pabteg->Sig.PlayArrayNext(0, WM_APP+100, GetConsoleWindow(), &playbackblock, errstr);
		else
			MessageBox(GetConsoleWindow(), "non audio signal cannot be played through audio output.", "error", MB_OK);
		break;

	case ID_STOP:
		TerminatePlay();
		break;
	}
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
	}
	return false;
}

void xcom::hold_til_getline(char* readbuffer)
{
	char buf1[4096]={0}, buf[4096]={0};
	DWORD dw, dw2;
	size_t nRemove(0);
	size_t cumError(0);
	vector<string> tar;
	int res;
	INPUT_RECORD in[32];
	CHAR read;
	SHORT delta;
	bool showscreen(true);
	size_t histOffset;
	size_t num; // total count of chracters typed in
	size_t offset; // how many characters shifted left from the last char
	size_t off; // how much shift of the current cursor is from the command prompt
	int line, len;
	CONSOLE_SCREEN_BUFFER_INFO coninfo0, coninfo;
	GetConsoleScreenBufferInfo(hStdout, &coninfo0);
	bool loop(true);
	bool replacemode(true);
	CONSOLE_CURSOR_INFO concurinf;
	num = offset = histOffset = buf[0] = 0;
	while (loop)
	{		
		res = ReadConsoleInput(hStdin, in, 32, &dw);
		if (res==0) { dw = GetLastError(); 	GetLastErrorStr(buf);
		sprintf(buf1, "code=%d", dw);	MessageBox(GetConsoleWindow(), buf, buf1, 0); 
		if (cumError++==2) 	loop=false;
		else	continue; }
		//dw can be greater than one when 1) control-v is pressed, or 2) debug command string is dispatched from OnNotify of debugDlg, or 3) maybe in other occassions
		if (dw>1) showscreen = !isdebugcommand(in, dw);
		for (UINT k=0; k<dw; k++)
		{
			if ( in[k].EventType == KEY_EVENT && in[k].Event.KeyEvent.bKeyDown)
			{
				read = in[k].Event.KeyEvent.uChar.AsciiChar;
				GetConsoleScreenBufferInfo(hStdout, &coninfo);
				off = (coninfo.dwCursorPosition.Y-coninfo0.dwCursorPosition.Y)*coninfo0.dwMaximumWindowSize.Y + coninfo.dwCursorPosition.X-coninfo0.dwCursorPosition.X;
				switch(in[k].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_RETURN:
					loop=false;
					buf1[0] = histOffset = 0; 
					buf[num] = '\n';
					strcpy(readbuffer, buf);
					WriteConsole (hStdout, strcpy(buf1, "\r\n"), 2, &dw2, NULL);
					return;
				case VK_BACK:
					if (!(num-offset)) break;
					buf1[0] = '\b';
					memcpy(buf1+1, buf+num-offset, offset+1);
					memcpy(buf+num---offset-1, buf1+1, offset+1);
					if (showscreen) WriteConsole (hStdout, buf1, strlen(buf1+1)+2, &dw2, NULL);
				case VK_LEFT:
					if (!(num-offset)) break;
					if (in[k].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || in[k].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
						delta = strlen(buf) - ctrlshiftleft(buf, offset) - offset;
					else
						delta = 1;
					for (int pp=0; pp<delta; pp++)
					{
						if ( (coninfo.dwCursorPosition.Y-coninfo0.dwCursorPosition.Y) && !coninfo.dwCursorPosition.X)
						{
							coninfo.dwCursorPosition.X = coninfo0.dwMaximumWindowSize.X-1; 
							coninfo.dwCursorPosition.Y--;
						}
						else
							coninfo.dwCursorPosition.X--;
					}
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					if (in[k].Event.KeyEvent.wVirtualKeyCode==VK_LEFT) offset += delta;
					break;
				case VK_RIGHT:
					if (in[k].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || in[k].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
					{
						len = ctrlshiftright(buf, offset);
						delta = len - off;
					}
					else
						delta = 1;
					//first determine if current location is inside the range of num, if not break
					for (int pp=0; pp<delta; pp++)
						if (off<num)
						{
							coninfo.dwCursorPosition.X++;
							if (coninfo.dwCursorPosition.X>coninfo.dwMaximumWindowSize.X)  coninfo.dwMaximumWindowSize.Y++, coninfo.dwCursorPosition.X++;
							// if the current cursor is on the bottom, the whole screen should be scrolled---do this later.
							SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
							offset--;
						}
					break;
				case VK_SHIFT:
				case VK_CONTROL:
				case VK_MENU:
				case VK_PAUSE:
				case VK_CAPITAL:
				case VK_HANGUL:
				case 0x16:
				case 0x17:
				case 0x18:
				case 0x19:
				case 0x1c:
				case VK_SNAPSHOT:
					break;
				case VK_ESCAPE:
					SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
					memset(buf, 0, num);
					WriteConsole (hStdout, buf, num, &dw2, NULL);
					SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
					num=histOffset=offset=0;
					break;

				case VK_UP:
				case VK_DOWN:
					if (in[k].Event.KeyEvent.wVirtualKeyCode==VK_UP)
					{
						if (comid==histOffset) break;
						histOffset++;
						if (history.size()>comid-histOffset+1)
							nRemove=history[comid-histOffset+1].size();
					}
					else
					{
						if (histOffset==0) break;
						histOffset--;
						nRemove=history[comid-histOffset-1].size();
					}
					memset(buf, 0, nRemove);
					if (histOffset==0) buf[0]=0;
					else strcpy(buf, history[comid-histOffset].c_str());
					SetConsoleCursorPosition (hStdout, coninfo0.dwCursorPosition);
					num = strlen(buf);
					if (showscreen) WriteConsole (hStdout, buf, max(nRemove, num), &dw2, NULL);
					coninfo.dwCursorPosition.X = coninfo0.dwCursorPosition.X + num;
					coninfo.dwCursorPosition.Y = coninfo0.dwCursorPosition.Y;
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					break;
				case VK_HOME:
					coninfo = coninfo0;
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					offset = num;
					break;
				case VK_END:
					line = num / coninfo.dwMaximumWindowSize.X;
					coninfo.dwCursorPosition.X = mod(coninfo0.dwCursorPosition.X + num, coninfo.dwMaximumWindowSize.X);
					coninfo.dwCursorPosition.Y += line;
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					offset = 0;
					break;
				case VK_INSERT:
					replacemode = !replacemode;
					concurinf.bVisible = 1;
					if (replacemode)	concurinf.dwSize = 70;
					else concurinf.dwSize = 25;
					SetConsoleCursorInfo (hStdout, &concurinf);
					break;
				case VK_DELETE:
					if (!offset) break;
					memcpy(buf1, buf+num-offset+1, offset);
					memcpy(buf+num---offset--, buf1, offset+1);
					if (showscreen) WriteConsole (hStdout, buf1, strlen(buf1)+1, &dw2, NULL);
					SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
					break;
				default:
					if (in[k].Event.KeyEvent.wVirtualKeyCode>=VK_F1 && in[k].Event.KeyEvent.wVirtualKeyCode<=VK_F24) break;
					//default is replace mode (not insert mode)
					// if cursor is in the middle
					if (showscreen) res = WriteConsole (hStdout, &read, 1, &dw2, NULL);
					if (replacemode && offset>0)
						buf[num-offset--] = read;
					else 
					{
						if (offset)
						{
							GetConsoleScreenBufferInfo(hStdout, &coninfo);
							strcpy(buf1, &buf[num-offset]);
							buf[num++-offset] = read;
							strcpy(buf+num-offset, buf1);
							if (showscreen) res = WriteConsole (hStdout, &buf1, strlen(buf1), &dw2, NULL);
							GetConsoleScreenBufferInfo(hStdout, &coninfo);
							coninfo.dwCursorPosition.X -= dw2;
							SetConsoleCursorPosition (hStdout, coninfo.dwCursorPosition);
						}
						else
							buf[num++-offset] = read;
					}
					break;

				}
				if (!loop) k=dw+1, strcat(buf, "\n");
			}
		}
	}
}
	
void xcom::console()
{
	char buf1[4096]={0}, *buff, buf[4096]={0};
	vector<string> tar;
	int code;
	HWND hr = GetConsoleWindow();
	while(1) 
	{
		hold_til_getline(buf); // this is a holding line.
		char *ff = strpbrk(buf,"\r\n");
		if (ff==NULL) { ::MessageBox(hr, "Input to ReadConsole doesn't contain \\r\\n","Did you press Ctrl-C? It will not close the program.", 0); continue;}
		buf[ff-buf]=0;

		LOGHISTORY(buf)
		mHistDlg.AppendHist(buf);
		history.push_back(buf);
		comid++;
		
		if (buf[0]=='#')
		{
			str2vect (tar, &buf[1], " ");
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
			else if (tar[0]=="save")	code = ID_SAVE;  
			else if (tar[0]=="load")	code = ID_LOAD;
			else if (tar[0]=="quit")	{LOGHISTORY("//\t""quit""") break;}
			else if (tar[0]=="clear")	code = ID_CLEARVAR;
			else						code = ID_ERR;
		}
		else
			code = ID_DEFAULT;
		buff = buf;
		if (code != ID_DEFAULT) buff += tar[0].length()+1;
		mainSpace.computeandshow(code, string (buff));
	}
}

int xcom::computeandshow(int code, string input, const AstNode *pCall)
{
	CAstSig *pabteg = vecast.back();
	string fname;
	bool caught(false);
	FILE *fp;
	string savesuccess;
	bool err(false);
	char errstr[256], buffer[MAX_PATH], drive[64], dir[MAX_PATH], filename[MAX_PATH], ext[MAX_PATH];
	size_t k(0), nItems;
	int success(0);
	vector<string> varlist, tar;
	trim(input, " \t");
	switch(code)
	{
	case ID_SHOWVAR:
		mShowDlg.Fillup();
		mShowDlg.changed=false;
		ShowWindow(mShowDlg.hDlg,SW_SHOW);
		SetForegroundWindow (mShowDlg.hDlg); 
		break;
	case ID_HISTORY:
		ShowWindow(mHistDlg.hDlg,SW_SHOW);
		break;
	case ID_SHOWFS:
		printf("Sampling Rate=%d\n", pabteg->pEnv->Fs);
		break;
	case ID_CLEARVAR:
		nItems = str2vect(tar, input.c_str(), " ");
		if (nItems>0) savesuccess = "Variable(s) cleared: ";
		for (; k<nItems; k++)
			if (pabteg->ClearVar(tar[k].c_str()))
				success++, sprintf(ext, "%s ", tar[k].c_str()), savesuccess+= ext;
		if (success) printf("%s\n", savesuccess.c_str()), mShowDlg.Fillup(&pabteg->Tags);
		break;
	case ID_LOAD:
		fname = input;
		fp = pabteg->OpenFileInPath(fname, "axl");
		if (fp)
		{
			if (load_axl(fp, errstr)==0) 
				printf("File %s reading error----%s.\n", fname.c_str(), errstr);
			fclose(fp);
			mShowDlg.Fillup(&pabteg->Tags);
		}
		else
			printf("File %s not found in %s\n", string(input+".axl").c_str(), pabteg->pEnv->AuxPath.c_str()); 
		break;
	case ID_SAVE:
		if ((nItems = str2vect(tar, input.c_str(), " "))==0)
		{	printf("#save (filename) var1 var2 var3...\n");		break;		}
		_splitpath(tar[0].c_str(), drive, dir, filename, ext);
		if (strlen(drive)+strlen(dir)==0) 
			FulfillFile(buffer, AppPath, filename), fname=buffer;
		else
			fname = tar[0];
		if (strlen(ext)==0) fname += ".axl";
		else fname += ext;
		if (nItems==1) // no variables indicated.. save all
		{
			pabteg->EnumVar(varlist);
			size_t sz = varlist.size();
			nItems = sz+1;
			for (unsigned int k=1; sz>0 && k<nItems; k++)  
				tar.push_back(varlist[k-1]);
		}
		if (fp = fopen(fname.c_str(), "wb")) 
		{
			printf("File %s saved with ...\n", fname.c_str());
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
		else
			printf("File %s cannot be open for writing.\n", tar[0].c_str());
		if (!err)	printf("%s \n", savesuccess.c_str());
		fclose(fp);
		break;
	default:
		if (code==ID_STOP)
			showarray(code, NULL);
		else if (input.size()>0 || code==ID_DEBUG_CONTINUE)
		{
		try {
			pabteg->SetNewScript(input.c_str());
			pabteg->Sig = pabteg->Compute();
			//NODE_BLOCK: multiple statements on one line
			//NODE_CALL : function call, include call, eval call
			trimr(input, "\r\n");
			bool suppress ( input.back()==';' );
			if ( code!=ID_DEBUG && pabteg->pAst->type==NODE_BLOCK || !suppress)
				showarray(code, pabteg->pAst);
			}
		catch (const char *errmsg) { caught=true;
			cout << "ERROR:" << errmsg << endl;	 }
		}
		if (!caught)
		{
			mShowDlg.pcast = pabteg;
			mShowDlg.Fillup();
			if(pabteg->pAst) 
			{ // for non-debug cases, this is always true
			AstNode *p = (pabteg->pAst->type==NODE_BLOCK) ? pabteg->pAst->child : pabteg->pAst;
			for (; p; p = p->next)
				if (p->type!=T_ID && p->type!=T_NUMBER && p->type!=T_STRING) //For these node types, 100% chance that var was not changed---do not send WM__VAR_CHANGED
					mShowDlg.SendMessage(WM__VAR_CHANGED,  (WPARAM) (p->str ? p->str : "ans"));
			}
			if (pabteg->statusMsg.length()>0) cout << pabteg->statusMsg.c_str() << endl;
			pabteg->statusMsg.clear();
		}
		if ( (pabteg->pAst && pabteg->pAst->type == NODE_BLOCK) || pabteg->pnodeLast)
			printf("K>");
		else
			printf("AUX>");
	}
	return pCall? 1:0;
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

	CAstSig cast = mainSpace.vecast.front();

	//If CTRL_CLOSE_EVENT is pressed, you have 5 seconds (that's how Console app works in Windows.... Finish all cleanup work in this timeframe.
	char estr[256], buffer[256];
	const char *sss = cast.GetPath();
	const char* pt = strstr(cast.GetPath(), AppPath); 
	string pathnotapppath;
	size_t loc;
	if (pt!=NULL)
	{
		pathnotapppath.append(cast.GetPath(),  cast.GetPath()-pt);
		string  str(pt + strlen(AppPath));
		loc = str.find_first_not_of(';');
		pathnotapppath.append(pt + strlen(AppPath)+loc);
	}
	else
		pathnotapppath.append(cast.GetPath());
	int res = writeINI(iniFile, estr, cast.pEnv->Fs, block, pathnotapppath.c_str());

	CRect rt1, rt2, rt3;
	GetWindowRect(GetConsoleWindow(), &rt1);
	mShowDlg.GetWindowRect(rt2);
	mHistDlg.GetWindowRect(rt3);
	res = writeINI(iniFile, estr, rt1,rt2, rt3);

	SYSTEMTIME lt;
	GetLocalTime(&lt);	

	sprintf(buffer, "//\t[%02d/%02d/%4d, %02d:%02d:%02d] AUXLAB closes------", lt.wMonth, lt.wDay, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
	LOGHISTORY(buffer);
	if (hShowvarThread!=NULL) PostThreadMessage(GetWindowThreadProcessId(mShowDlg.hDlg, NULL), WM__ENDTHREAD, 0, 0);
	if (hHistoryThread!=NULL) PostThreadMessage(GetWindowThreadProcessId(mHistDlg.hDlg, NULL), WM__ENDTHREAD, 0, 0);
	if (hDebugThread!=NULL)
	{
		map<string, CDebugDlg*>::iterator it=dbmap.begin(); 
		if (it!=dbmap.end())
			PostThreadMessage(GetWindowThreadProcessId((it->second)->hDlg, NULL), WM__ENDTHREAD, 0, 0);
	}
	fclose (stdout);
	fclose (stdin);
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
    DWORD fdwMode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE; 
	SetConsoleMode(hStdin, fdwMode) ;
	if (!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE )) ::MessageBox (hr, "Console control handler error", "AUXLAB", MB_OK);

	fdwMode = ENABLE_PROCESSED_OUTPUT |  ENABLE_WRAP_AT_EOL_OUTPUT ;
	res = SetConsoleMode(hStdout, fdwMode) ;

	dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf(mHistDlg.logfilename, "%s%s%s_%s.log", mainSpace.AppPath, fname, HISTORY_FILENAME, buf);
	size_t nHistFromFile = mainSpace.ReadHist();
	mainSpace.comid = nHistFromFile;

	WriteConsole (hStdout, "AUX>", 4, &dw, NULL);

	while (mShowDlg.hDlg==NULL) {
		Sleep(100); 
		mShowDlg.pcast = &cast;
		mShowDlg.Fillup();
		mShowDlg.changed=false;
	}
	mShowDlg.pcast = &cast;
	ShowWindow(mShowDlg.hDlg,SW_SHOW);
	ShowWindow(mHistDlg.hDlg,SW_SHOW);

	CRect rt1, rt2, rt3;
	res = readINI(iniFile, &rt1, &rt2, &rt3);
	if (res & 1)	
		MoveWindow(hr, rt1.left, rt1.top, rt1.Width(), rt1.Height(), TRUE);
	
	SYSTEMTIME lt;
    GetLocalTime(&lt);	
	sprintf(buffer2, "//\t[%02d/%02d/%4d, %02d:%02d:%02d] AUXLAB %s begins------", lt.wMonth, lt.wDay, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, buffer);

	LOGHISTORY(buffer2);

	mainSpace.vecast.push_back(&cast);
	mainSpace.vecnodeUDF.push_back(NULL);

	CONSOLE_CURSOR_INFO concurinf;

	concurinf.bVisible = 1;
	concurinf.dwSize = 70;
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

