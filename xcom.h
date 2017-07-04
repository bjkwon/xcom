
#ifndef XCOM
#define XCOM

class xcom
{
public:
	vector<CAstSig*> vecast;
	vector<const AstNode*> vecnodeUDF;

	vector<string> history;
	size_t nHistFromFile;
	size_t comid; // command ID
	char AppPath[256];
	INPUT_RECORD debug_command_frame[6];

	xcom();
	virtual ~xcom();
	void console();
	void gendebugcommandframe();
	bool isdebugcommand(INPUT_RECORD *in, int len);
	void hold_til_getline(char* readbuffer);
	size_t ReadHist();
	int computeandshow(int code, string input=string(""), const AstNode *pCall=NULL);
	void showarray(int code, const AstNode *pnode);
	int ClearVar(const char *var);
	int load_axl(FILE *fp, char *errstr);
	int save_axl(FILE *fp, const char * var, char *errstr);
	void out4console(string varname, CSignals *psig, string &out);
	size_t ctrlshiftright(const char *buf, DWORD offset);
	size_t ctrlshiftleft(const char *buf, DWORD offset);
};

#else if //if XCOM was already defined, skip

#endif