
#ifndef XCOM
#define XCOM

class xcom
{
public:
//	CAstSig cast;
	vector<CAstSig*> vecast;
	vector<const AstNode*> vecnodeUDF;

	xcom();
	virtual ~xcom();
	int computeandshow(const char *AppPath, int code, string input, const AstNode *pCall=NULL);
	void showarray(int code, const AstNode *pnode);
	int ClearVar(const char *var);
	int load_axl(FILE *fp, char *errstr);
	int save_axl(FILE *fp, const char * var, char *errstr);
	void out4console(string varname, CSignals *psig, string &out);
};

#else if //if XCOM was already defined, skip

#endif