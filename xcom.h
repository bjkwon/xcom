	
#ifndef XCOM
#define XCOM

#define MAIN_PROMPT "AUX>"
#define DEBUG_PROMPT "K>"

enum DEBUG_KEY
{
	non_debug=0,
    debug_F5,
    debug_F10,
    debug_F11,
    debug_Shift_F5,
    debug_Ctrl_F10,
};

class xcom
{
public:
	vector<CAstSig*> vecast;

	vector<string> history;
	size_t nHistFromFile;
	size_t comid; // command ID
	char AppPath[256];
	INPUT_RECORD debug_command_frame[6];
	string comPrompt;

	xcom();
	virtual ~xcom();
	void console();
	void checkdebugkey(INPUT_RECORD *in, int len);
	DEBUG_KEY getinput(char* readbuffer);
	size_t ReadHist();
	void ShowWS_CommandPrompt(CAstSig *pcast);
	int computeandshow(const char *input, const AstNode *pCall=NULL);
	int cleanup_debug();
	void showarray(const AstNode *pnode);
	int ClearVar(const char *var);
	int load_axl(FILE *fp, char *errstr);
	int save_axl(FILE *fp, const char * var, char *errstr);
	void out4console(string varname, CSignals *psig, string &out);
	size_t ctrlshiftright(const char *buf, DWORD offset);
	size_t ctrlshiftleft(const char *buf, DWORD offset);
	int hook(CAstSig *ast, string HookName, const char* args);
	void LogHistory(vector<string> input);
	bool dbmapfind(const char* udfname);
	void breakpoint(CAstSig *past, const AstNode *pnode, bool freepass=false);
	void debug_appl_manager(CAstSig *debugAstSig, DEBUG_STATUS debug_status, int line=-1);
	bool IsThisBreakpoint(CAstSig *past, const AstNode *pnode);
};

#define PRINTLOG(FNAME,STR) \
{ FILE*__fp=fopen(FNAME,"at"); fprintf(__fp,STR);	fclose(__fp); }


#else if //if XCOM was already defined, skip



#endif