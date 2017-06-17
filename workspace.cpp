#include <vector>
#ifndef SIGPROC
#include "sigproc.h"
#endif
extern CAstSig main;

CSignals &GetSig(string varname)
{
	map<string, CSignals>::iterator what;
	what = main.pEnv->Tags.find(varname.c_str());
	if (what == main.pEnv->Tags.end())
		return main.Sig;
	else
		return what->second;
}

CSignals *GetSig(char *var)
{
	string varname(var);
	map<string, CSignals>::iterator what;
	what = main.pEnv->Tags.find(varname.c_str());
	if (what == main.pEnv->Tags.end())
		return NULL;
	else
		return &(what->second);
}