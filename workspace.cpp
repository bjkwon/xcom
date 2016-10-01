#include <vector>
#ifndef SIGPROC
#include "sigproc.h"
#endif
extern CAstSig main;

void EnumVariables(vector<string> &var)
{
	CAstSigEnv *pe = main.pEnv;
	var.clear();
	for (map<string, CSignals>::iterator what=pe->Tags.begin(); what!=pe->Tags.end(); what++)
		var.push_back(what->first);
}

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