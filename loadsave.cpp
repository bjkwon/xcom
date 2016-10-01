#include "audfret.h"
#ifndef SIGPROC
#include "sigproc.h"
#endif

#define RETURN0IFINVALID(X) if ((X)<=0) {sprintf(errstr, "Error in ( "#X " )"); return 0;}
#define RETURN0IFINVALID0(X) if ((X)!=0) {sprintf(errstr, "Error in ( "#X " )"); return 0;}
#define RETURNWITHERRMSG(X) {sprintf(errstr, "Error in ( "#X " )"); return 0;}
#define RETURNWITHERRMSG2(X,Y) {sprintf(errstr, "Error in ( "#X #Y" )"); return 0;}

#define VERSIONSTR	"0.9"

extern CAstSig main;

void nonnulintervals(CSignals *psig, string &out);


int save_axl(FILE *fp, const char * var, char *errstr)
{
	char *buffer, buf[256], fullmoduleName[256];
	CSignals tp;
	size_t sz;
	unsigned char dummy(0), type;
	char header[64];
	int wherenow, cellcount(-1), count(0);
 	GetModuleFileName(GetModuleHandle(NULL), fullmoduleName, MAX_PATH);
	getVersionString (fullmoduleName, buf, sizeof(buf));
	buf[4]=0;
	sprintf(header, "AXL %s", buf+1); // the first character is buf is 'v'--we take from second char
	map<string, CSignals>::iterator what;
	what = main.pEnv->Tags.find(var);
	if (what == main.pEnv->Tags.end())
	{
		sprintf(errstr, "Variable '%s' not found.", var);
		return 0;
	}
	else
	{
		RETURN0IFINVALID( fwrite((void*)&header, strlen(header), 1, fp) )
		tp = what->second;
		type=(unsigned char)tp.GetType();
		RETURN0IFINVALID( fwrite((void*)&type, sizeof(char), 1, fp) )
		RETURN0IFINVALID( fwrite((void*)var, strlen(var)+1, 1, fp) )
		do	{
			sz = tp.nSamples;
			switch(type)
			{
			case CSIG_EMPTY:
			case CSIG_STRING:
				RETURN0IFINVALID( fwrite((void*)&sz, sizeof(sz), 1, fp) )
				if (sz>0)
				{
					buffer = new char[sz+1];
					tp.getString(buffer,sz+1);
					RETURN0IFINVALID( fwrite((void*)buffer, sz, 1, fp) )
					delete[] buffer;
				}
				break;
			case CSIG_SCALAR:
			case CSIG_VECTOR:
				RETURN0IFINVALID( fwrite((void*)&sz, sizeof(sz), 1, fp) )
				RETURN0IFINVALID( fwrite((void*)tp.GetBuffer(), sizeof(double), tp.nSamples, fp) )
				break;
			case CSIG_AUDIO:
				wherenow = ftell(fp);
				sz = tp.IsStereo();//0 for mono, 1 for stereo...
				RETURN0IFINVALID( fwrite((void*)&sz, sizeof(sz), 1, fp) )
				wherenow = ftell(fp);
				tp.WriteAXL(fp);
				break;
			case CSIG_CELL:
				cellcount = tp.cell.size();
				RETURN0IFINVALID( fwrite((void*)&cellcount, sizeof(cellcount), 1, fp) )
				break;
			}
			if (count<cellcount)
			{
				tp = what->second.cell[count];
				type=(unsigned char)tp.GetType();
				RETURN0IFINVALID( fwrite((void*)&type, sizeof(char), 1, fp) )
				count++;
			}
			else
				break;
		} while (1) ;
	}
	errstr[0]=0;
	return 1;
}

int load_axl(FILE *fp, char *errstr)
{
	unsigned char type;
	char header[8], readbuf[256], varname[256]; // let's limit variable name to 255 characters.
	size_t res, sz;
	int nChannels;
	char *buffer;
	int wherenow(0), nElem(0), count(0);
	CSignals tp;
	fseek (fp, 0, SEEK_END);
    int filesize=ftell(fp);
	fseek (fp, 0, SEEK_SET);
	while(wherenow<filesize)
	{
		RETURN0IFINVALID( res = fread((void*)&header, 1, 7, fp) )
		header[res]=0;
		if (strncmp(header,"AXL",3)!=0) RETURNWITHERRMSG("AXL header not found")
		RETURN0IFINVALID( fread((void*)&type, 1,1,fp) )
		wherenow = ftell(fp);
		RETURN0IFINVALID( (res=fread((void*)&readbuf, 1,sizeof(readbuf),fp)) )
		strncpy(varname, readbuf, sizeof(readbuf)); varname[255]=0;
		if (strlen(varname)>=sizeof(readbuf)-1) { sprintf(errstr,"varname longer than specified."); return -1;}
		RETURN0IFINVALID0( res = fseek(fp, wherenow+strlen(varname)+1, SEEK_SET) )
//		RETURN0IFINVALID( fscanf(fp, "%s", varname) ) //#load xy wipes off fp in the second time around (for y) here..  I will need to get rid of fscanf and use fread...
//		RETURN0IFINVALID0( res = fseek(fp, wherenow+strlen(varname)+1, SEEK_SET) )
		while (1)
		{
			RETURN0IFINVALID( res = fread((void*)&sz, sizeof(size_t),1,fp) )
			wherenow = ftell(fp);
			switch(type)
			{
			case CSIG_EMPTY:
			case CSIG_STRING:
				buffer = new char[sz];
				res = fread((void*)buffer, sz, 1,fp);
				tp.Reset(2);
				tp.UpdateBuffer((int)sz);
				for (size_t i=0, j=0; i<sz; i++, j++)
					tp.buf[j] = buffer[i];
				delete[] buffer;
				break;
			case CSIG_SCALAR:
			case CSIG_VECTOR:
				tp.Reset(1);
				tp.UpdateBuffer((int)sz);
				RETURN0IFINVALID( res = fread((void*)&tp.GetBuffer()[0], sizeof(double),sz,fp) )
				break;
			case CSIG_CELL: // for a cell variable, sz means the element counts
				nElem = sz;
				break;
			case CSIG_AUDIO:
				nChannels = (int)sz; 
				tp.ReadAXL(fp);
				if (nChannels) // stereo
				{
					CSignals sec;
					if (sec.ReadAXL(fp)) tp.SetNextChan(&sec);
				}
				break;
			default:
				sprintf(errstr, "Invalid CSignal type.");
				return -1;
				break;			
			}
			if (nElem==0) break;
			else
			{
				if (count>0) 
					main.AddCell(varname, tp);
				if (count++==nElem)
				{
					tp = main.Sig;
					main.Sig.cell.clear(); //reset here so that successive cell variables can be processed properly.
					break;
				}
				else
					RETURN0IFINVALID( fread((void*)&type, 1,1,fp) )
			}
		} 
		main.SetTag(varname, tp);
		wherenow = ftell(fp);
		count=nElem=0; //reset here so that successive cell variables can be processed properly.
	}
	return 1;
}