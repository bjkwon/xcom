#define DEFAULT_FS	22050
#define DEFAULT_BLOCK_SIZE 200.

#define HISTORY_FILENAME	"_history"

#define LOGHISTORY(MSG) { FILE* __fp_ = fopen(mHistDlg.logfilename,"at"); if (__fp_) fprintf(__fp_,"%s\n", (MSG)); else ::MessageBox(mHistDlg.hDlg, "null fp", "", 0); fclose(__fp_); } 

#define PRINTF_WIN(MSG) WriteFile(hStdout, (MSG), strlen((MSG)), &dw, NULL);

#define ID_DEFAULT	99
#define ID_ERR		-111
#define ID_PLOT		100
#define ID_PLAY		101
#define ID_PLAYCONTINUE	111
#define ID_PLAYOVERLAP 112
#define ID_PLAYLOOP 113
#define ID_PLAYENDLOOP 114
#define ID_STOP		115
#define ID_SHOWFS	102
#define ID_SHOWVAR	103
#define ID_CLEARVAR	104 
#define ID_SAVE		105 
#define ID_LOAD		106
#define ID_HISTORY	107
#define ID_DEBUG_STEP 108
#define ID_DEBUG_CONTINUE 109
#define ID_DEBUG_EXIT 110
#define ID_DEBUG 111



#define DISPLAYLIMIT 10
#define TEXTLINEDISPLAYLIMIT 30

#define NODE_BLOCK		10000
#define NODE_ARGS		10001
#define NODE_MATRIX		10002
#define NODE_VECTOR		10003
#define NODE_CALL		10004
#define NODE_FUNC		10005
#define NODE_IDLIST		10006
#define NODE_EXTRACT	10007
#define NODE_INITCELL	10008
#define NODE_IXASSIGN	10009
#define NODE_INTERPOL	10010

#define  T_EOF 0
#define  T_UNKNOWN 258
#define  T_NEWLINE 259
#define  T_IF 260
#define  T_ELSE 261
#define  T_ELSEIF 262
#define  T_END 263
#define  T_WHILE 264
#define  T_FOR 265
#define  T_BREAK 266
#define  T_CONTINUE 267
#define  T_SWITCH 268
#define  T_CASE 269
#define  T_OTHERWISE 270
#define  T_FUNCTION 271
#define  T_ENDFUNCTION 272
#define  T_RETURN 273
#define  T_SIGMA 274
#define  T_OP_SHIFT 275
#define  T_OP_SHIFT2 276
#define  T_OP_CONCAT 277
#define  T_COMP_EQ 278
#define  T_COMP_NE 279
#define  T_COMP_LE 280
#define  T_COMP_GE 281
#define  T_LOGIC_AND 282
#define  T_LOGIC_OR 283
#define  T_NUMBER 284
#define  T_STRING 285
#define  T_ID 286
#define  T_DUR 287
#define  T_LENGTH 288
#define  T_NEGATIVE 289
#define  T_POSITIVE 290
#define  T_LOGIC_NOT 291

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HMODULE_THIS  ((HINSTANCE)&__ImageBase)

#define WM__ENDTHREAD	WM_APP+342