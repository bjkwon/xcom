#include <string>
using namespace std;

string typestring(int type)
{
	char buf[256];
	switch(type)
	{
	case 0:
		return  "T_EOF";
	case 258:
		return  "T_UNKNOWN";
	case 259:
		return  "T_NEWLINE";
	case 260:
		return  "T_IF";
	case 261:
		return  "T_ELSE";
	case 262:
		return  "T_ELSEIF";
	case 263:
		return  "T_END";
	case 264:
		return  "T_WHILE";
	case 265:
		return  "T_FOR";
	case 266:
		return  "T_BREAK";
	case 267:
		return  "T_CONTINUE";
	case 268:
		return  "T_SWITCH";
	case 269:
		return  "T_CASE";
	case 270:
		return  "T_OTHERWISE";
	case 271:
		return  "T_FUNCTION";
	case 272:
		return  "T_ENDFUNCTION";
	case 273:
		return  "T_RETURN";
	case 274:
		return  "T_SIGMA";
	case 275:
		return  "T_OP_SHIFT";
	case 276:
		return  "T_OP_SHIFT2";
	case 277:
		return  "T_OP_CONCAT";
	case 278:
		return  "T_COMP_EQ";
	case 279:
		return  "T_COMP_NE";
	case 280:
		return  "T_COMP_LE";
	case 281:
		return  "T_COMP_GE";
	case 282:
		return  "T_LOGIC_AND";
	case 283:
		return  "T_LOGIC_OR";
	case 284:
		return  "T_NUMBER";
	case 285:
		return  "T_STRING";
	case 286:
		return  "T_ID";
	case 287:
		return  "T_NEGATIVE";
	case 288:
		return  "T_POSITIVE";
	case 289:
		return  "T_LOGIC_NOT";
	case 10000:
		return  "NODE_BLOCK";
	case 10001:
		return  "NODE_ARGS";
	case 10002:
		return  "NODE_MATRIX";
	case 10003:
		return  "NODE_VECTOR";
	case 10004:
		return  "NODE_CALL";
	case 10005:
		return  "NODE_FUNC";
	case 10006:
		return  "NODE_IDLIST";
	case 10007:
		return  "NODE_EXTRACT";
	case 10008:
		return  "NODE_INITCELL";
	case 10009:
		return  "NODE_IXASSIGN";
	case 10010:
		return  "NODE_INTERPOL";
	default:
		if ((type>31)&&(type<128))
		{
			sprintf(buf, "%c", type);
			return &buf[0];
		}
		else
			return "ERROR---------";
	}
}