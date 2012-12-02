#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28.2.1 2001/07/19 05:46:39 peter Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
#if defined(__cplusplus) || __STDC__
static int yygrowstack(void);
#else
static int yygrowstack();
#endif
#define YYPREFIX "yy"
#line 2 "tg.y"
/************************************************************************
 *									*
 *	File:  tg.y							*
 *									*
 *	Traffic Generation command grammar.				*
 *									*
 *	Written 19-Jun-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1989 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/tg.y,v 1.2 2002/01/24 23:25:28 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#ifdef FREEBSD
#include <stdlib.h>
#else
#include <search.h>
#include <malloc.h>
#endif
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <math.h>
#include <time.h> 
#include "config.h"
#include "distribution.h"
#include "protocol.h"
#include "log.h"

/* Type definitions local to this file.					*/

#define MAXSYM		512

#define SYMcpy(a, b)	{ \
			(void)strncpy((a), (b), MAXSYM); \
			(a)[MAXSYM - 1] = '\0'; \
			}

#define SYMcat(a, b)	{ \
			(void)strncat((a), (b), MAXSYM - strlen(a) - 1); \
			}

typedef enum srvr_state_flag_		/* server packet parse state.	*/
	{
	srvr_len,			/* Accumulate reply length.	*/
	srvr_skip,			/* Accumulate skip distance.	*/
	srvr_skipping			/* Skip to next reply length.	*/
	} srvr_state_flag;

typedef struct srvr_state_		/* per-asn state variables.	*/
	{
	struct srvr_state_ *next;
	long		asn;		/* asn that this state is for.	*/
	int		bad;		/* asn has corrupted data.	*/
	srvr_state_flag	state;		/* Current state.		*/
	unsigned long	acc;		/* accumulate compressed number.*/
	int		nbytes;		/* # bytes accumulated so far.	*/
	int		special;	/* acc contains special number.	*/
	unsigned long	skip;		/* how many bytes to skip.	*/
	} srvr_state;

typedef struct				/* Yacc Symbol table entry TYPE.*/
	{
	int		flags;		/* FLAGS, same as symbol below.	*/
	int		lineno;		/* Line number for construct.	*/
	double		d;		/* token numeric value.		*/
	char		n[MAXSYM];	/* token name.			*/
	distribution	tmpdist;	/* Distribution when we aren't	*/
					/* sure which one we have.	*/
	struct sockaddr	tmpaddr;	/* Address when we aren't sure	*/
					/* which one we have.		*/
	tg_action	action;		/* Action descriptor.		*/
	protocol	prot;		/* Protocol descriptor.		*/
	}		strbuf;

#define YYSTYPE strbuf

/* Each entry of the expected-replies queue represents a packet that	*/
/* is expected to be received from a server.  If a given packet is	*/
/* not received by its timeout, we scream and die.  Packets are		*/
/* identified by the sequence number of the first byte following them,	*/
/* the sequence number of the first byte of the first packet is zero.	*/

typedef struct xpctd_replies_		/* expected-replies queue.	*/
	{
	struct xpctd_replies_ *next;
	unsigned long	byte_seqno;	/* seqno of 1st byte after	*/
					/* packet represented by this	*/
					/* struct.			*/
	struct timeval	timeout;	/* time at which patience gives	*/
					/* out.				*/
	} xpctd_replies;

/* Functions exported from this file.					*/

extern void		yyerror();

/* Functions local to this file.					*/

extern void		fix_times();
extern void		generate();
extern void		generate_interactive();
extern int		check_deadline();
extern int		start_log(struct timeval, char *format);
extern void		do_actions();
extern void		node_init();
extern int		rcv_pkt();
extern int		rcv_pkt_interactive();
extern int		rcv_pkt_interactive_srvr();
extern srvr_state	*srvr_state_get();
extern void		tg_append_element();
extern void		wait_start();
extern xpctd_replies	*xpctd_replies_get();
extern void		xpctd_replies_free();
extern void		yyerror();

/* Variables exported from this file.					*/

double			global_start;	/* global start time.		*/
struct timeval		global_start_tv; /* global start timeval.	*/
protocol		lexprot;	/* lex's protocol definition.	*/
protocol		prot;			/* Protocol definition.	*/
tg_action		*tg_first = NULL;	/* first TG action.	*/
tg_action		**tg_last = &tg_first;	/* last TG action.	*/
char                    *version = "2.0";	/* TG program version.	*/
char			*ofile = NULL;
char			*ifile = NULL;
 int			FlushOutput = 0;      /* whether to flush after each*/


/* Variables local to this file.					*/

static int		got_errors = 0;	/* Got errors during parse?	*/
static int		got_setup = 0;	/* Got a setup clause?		*/
static int		got_setup_implicit = 0;	/* an implicit one?	*/
static srvr_state	*srvr_state_h = NULL; /* per-asn srvr state list*/
static xpctd_replies	*xpctd_replies_flist = NULL;
					/* xr freelist.			*/
static xpctd_replies	*xpctd_replies_qh = NULL; /* xr queue header.	*/
static xpctd_replies	**xpctd_replies_qt = &xpctd_replies_qh;
					/* xr queue tail-pointer.	*/

char			logfilename[1024];
char			prefix[1024];
char			suffix[1024];

/* Symbol table */
struct sym_entry {
	char		*sym_name;
	int		sym_type;
	union {
	    int		intval;
	    char	*strval;
	    float	fltval;
	    double	dblval;
	} sym;

#define sym_intval	sym.intval
#define sym_strval	sym.strval
#define sym_fltval	sym.fltval
#define sym_dblval	sym.dblval

};

enum {
	SYM_INT_TYPE = 1,
	SYM_STR_TYPE,
	SYM_FLT_TYPE,
	SYM_DBL_TYPE,
};

#define SYM_TBL_SIZE	100

struct sym_entry sym_tbl[SYM_TBL_SIZE];

int	sym_tbl_index = 0;

#line 206 "y.tab.c"
#define YYERRCODE 256
#define INCLUDE 257
#define EQUAL 258
#define DIST_CONST 259
#define DIST_EXP 260
#define DIST_MARKOV 261
#define DIST_MARKOV2 262
#define DIST_UNIFORM 263
#define ARRIVAL 264
#define AT 265
#define AVERAGE 266
#define BANDWIDTH 267
#define DATA 268
#define DELAY 269
#define INTERACTIVE 270
#define INTERVAL 271
#define LENGTH 272
#define LOG 273
#define LOSS 274
#define MTU 275
#define ON 276
#define PATIENCE 277
#define PEAK 278
#define RCVWIN 279
#define RESPONSELENGTH 280
#define SEED 281
#define SERVER 282
#define SETUP 283
#define SNDWIN 284
#define TIME 285
#define TOS 286
#define TTL 287
#define WAIT 288
#define ADDR 289
#define FILENAME 290
#define FLOATING_POINT 291
#define HEX_INTEGER 292
#define IDENTIFIER 293
#define INTEGER 294
#define OCTAL_INTEGER 295
#define PROTOCOL 296
#define SMALL_INTEGER 297
#define STRING_LITERAL 298
#define ADDRESS 299
#define PACKET 300
#define RESET 301
const short yylhs[] = {                                        -1,
    0,    0,    3,    3,    3,    1,    4,    2,    6,    7,
   10,   10,   10,   12,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,    8,    8,
   14,   14,   15,   16,   16,   16,   16,   16,   16,   17,
   18,   18,   19,   20,   21,   23,   24,   22,   22,   26,
   26,   26,   26,   26,   25,   25,   25,   25,   25,   25,
   25,   27,   27,   27,   27,   27,   27,    9,    9,    9,
   28,   28,   13,   13,    5,    5,    5,   29,   29,
};
const short yylen[] = {                                         2,
    2,    1,    1,    1,    1,    2,    3,    3,    2,    2,
    2,    3,    3,    1,    0,    4,    4,    4,    4,    4,
    4,    3,    3,    2,    3,    3,    3,    3,    0,    2,
    2,    1,    2,    1,    1,    1,    3,    4,    5,    1,
    1,    2,    2,    2,    2,    2,    2,    0,    2,    2,
    1,    2,    2,    2,    1,    2,    2,    4,    5,    2,
    3,    1,    1,    1,    1,    1,    1,    1,    3,    5,
    1,    1,    1,    1,    1,    1,    1,    1,    1,
};
const short yydefred[] = {                                      0,
    0,    0,    0,    0,    2,    0,    6,   71,   79,   78,
    9,   68,    0,    1,   14,   29,   15,    0,    0,    0,
    0,    0,   69,    0,    0,    0,    0,   40,    0,   30,
    0,   32,   34,   35,   36,    0,    0,   24,    0,    0,
    0,    0,    0,    0,    0,   12,   13,    0,    0,    0,
    0,    0,   73,   75,   76,   77,   74,   55,   44,   33,
   43,   42,   31,    0,    0,    0,    0,    0,   25,   26,
    0,    0,    0,   22,   23,   28,   27,   70,   72,   56,
    0,    0,    0,   45,    0,    0,    0,   16,   18,   20,
   17,   19,   21,    0,    0,   61,   46,    0,    0,    0,
    0,   51,   49,    0,    0,   48,   58,    0,   50,   53,
   54,   52,   47,    0,   59,
};
const short yydgoto[] = {                                       3,
    4,    5,    0,    0,   57,    6,   16,   20,   11,   17,
   21,   18,   58,   30,   31,   32,   33,   34,   35,   36,
   65,   86,   87,  106,   59,  103,    0,   12,   13,
};
const short yysindex[] = {                                   -249,
 -286, -205,    0, -249,    0, -272,    0,    0,    0,    0,
    0,    0,  -33,    0,    0,    0,    0, -260, -205, -188,
 -168, -209,    0,  -25, -246, -205, -263,    0, -205,    0,
 -204,    0,    0,    0,    0, -228, -235,    0, -201, -236,
 -212, -236, -236, -236, -236,    0,    0, -205, -201, -201,
 -201, -201,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -246, -210, -201, -201, -201,    0,    0,
 -201, -201, -201,    0,    0,    0,    0,    0,    0,    0,
 -201, -246, -201,    0, -246, -218, -202,    0,    0,    0,
    0,    0,    0, -201, -201,    0,    0, -201, -236, -205,
 -201,    0,    0, -205, -218,    0,    0, -246,    0,    0,
    0,    0,    0, -218,    0,
};
const short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   26,    0,    0,    0,    0,    0,    0,   81,
   66,   72,    0,   26,    0,    0,    0,    0,   88,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   40,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    1,    0,   15,    0,    0,   99,   40,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  101,    0,    0,    0,    0,    0,
    0,    0,    0,  105,    0,
};
const short yygindex[] = {                                     83,
    0,    0,    0,    0,  -38,    0,    0,    0,  -26,    0,
    0,    0,  -30,    0,    0,   65,    0,    0,    0,    0,
    0,  -76,    0,    0,  -54,    0,    0,   -1,    4,
};
#define YYTABLESIZE 393
const short yytable[] = {                                      60,
   57,   70,   62,   74,   75,   76,   77,    1,   69,   84,
  105,    7,   49,   50,   60,   51,   52,   23,   80,   81,
   82,   83,   24,   15,   19,   72,    2,   95,   22,  114,
   97,   66,   48,   67,   61,   88,   89,   90,   68,   48,
   91,   92,   93,   64,   53,   54,   78,   55,   56,   98,
   94,   79,   96,  115,   71,   54,   72,   55,   56,   25,
  110,   73,   99,  107,  108,   10,  100,  109,   27,   85,
  112,   11,   46,  111,  104,   25,   26,  113,   28,   47,
    8,  101,  102,   29,   27,    8,   14,   41,    9,   53,
   54,   10,   55,   56,   28,   63,    0,   37,   37,   29,
   38,   38,   39,    0,   39,    0,   40,    0,    0,   41,
   42,    0,    0,    0,    0,   43,    0,   44,   45,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   57,   57,    0,    0,   57,    0,
    0,    0,   57,   57,    0,    0,    0,   57,   60,   60,
   57,   57,   60,   57,    0,   57,   60,   60,   57,   72,
   72,   60,    0,   72,   60,   60,    0,   60,   72,   60,
   57,   57,   60,   48,   48,    0,   72,   48,   72,    0,
   72,    0,   48,   72,   60,   60,    0,    0,    0,    0,
   48,   72,   48,    0,   48,   72,   72,   48,    0,   10,
   10,    0,    0,    0,    0,   11,   11,   11,   10,   48,
   48,   11,   11,    0,   11,    0,   11,    0,   10,   11,
   11,   41,   41,   10,   11,   11,    0,   11,   11,   11,
   41,    0,   37,   37,   38,   38,    0,    0,   39,   39,
   41,   37,    0,   38,    0,   41,    0,   39,    0,    0,
    0,   37,    0,   38,    0,    0,   37,   39,   38,    0,
    0,    0,   39,
};
const short yycheck[] = {                                      26,
    0,   40,   29,   42,   43,   44,   45,  257,   39,   64,
   87,  298,  259,  260,    0,  262,  263,   19,   49,   50,
   51,   52,   19,  296,   58,    0,  276,   82,  289,  106,
   85,  267,   58,  269,  298,   66,   67,   68,  274,    0,
   71,   72,   73,  272,  291,  292,   48,  294,  295,  268,
   81,   48,   83,  108,  267,  292,  269,  294,  295,  264,
   99,  274,  281,   94,   95,    0,  285,   98,  273,  280,
  101,    0,  282,  100,  277,  264,  265,  104,  283,  289,
    0,  300,  301,  288,  273,  291,    4,    0,  294,  291,
  292,  297,  294,  295,  283,   31,   -1,  266,    0,  288,
    0,  270,  271,   -1,    0,   -1,  275,   -1,   -1,  278,
  279,   -1,   -1,   -1,   -1,  284,   -1,  286,  287,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  264,  265,   -1,   -1,  268,   -1,
   -1,   -1,  272,  273,   -1,   -1,   -1,  277,  264,  265,
  280,  281,  268,  283,   -1,  285,  272,  273,  288,  264,
  265,  277,   -1,  268,  280,  281,   -1,  283,  273,  285,
  300,  301,  288,  264,  265,   -1,  281,  268,  283,   -1,
  285,   -1,  273,  288,  300,  301,   -1,   -1,   -1,   -1,
  281,  296,  283,   -1,  285,  300,  301,  288,   -1,  264,
  265,   -1,   -1,   -1,   -1,  264,  265,  266,  273,  300,
  301,  270,  271,   -1,  273,   -1,  275,   -1,  283,  278,
  279,  264,  265,  288,  283,  284,   -1,  286,  287,  288,
  273,   -1,  264,  265,  264,  265,   -1,   -1,  264,  265,
  283,  273,   -1,  273,   -1,  288,   -1,  273,   -1,   -1,
   -1,  283,   -1,  283,   -1,   -1,  288,  283,  288,   -1,
   -1,   -1,  288,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 301
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"':'",0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"INCLUDE","EQUAL",
"DIST_CONST","DIST_EXP","DIST_MARKOV","DIST_MARKOV2","DIST_UNIFORM","ARRIVAL",
"AT","AVERAGE","BANDWIDTH","DATA","DELAY","INTERACTIVE","INTERVAL","LENGTH",
"LOG","LOSS","MTU","ON","PATIENCE","PEAK","RCVWIN","RESPONSELENGTH","SEED",
"SERVER","SETUP","SNDWIN","TIME","TOS","TTL","WAIT","ADDR","FILENAME",
"FLOATING_POINT","HEX_INTEGER","IDENTIFIER","INTEGER","OCTAL_INTEGER",
"PROTOCOL","SMALL_INTEGER","STRING_LITERAL","ADDRESS","PACKET","RESET",
};
const char * const yyrule[] = {
"$accept : statements",
"statements : include statements",
"statements : commands",
"statement : include",
"statement : macro",
"statement : commands",
"include : INCLUDE STRING_LITERAL",
"macro : IDENTIFIER EQUAL integer",
"commands : start_time association_spec tg_entry_list",
"start_time : ON time_literal",
"association_spec : protocol_and_addresses quality_of_service",
"protocol_and_addresses : protocol ADDR",
"protocol_and_addresses : protocol ADDR SERVER",
"protocol_and_addresses : protocol ADDR ADDR",
"protocol : PROTOCOL",
"quality_of_service :",
"quality_of_service : quality_of_service AVERAGE BANDWIDTH number",
"quality_of_service : quality_of_service PEAK BANDWIDTH number",
"quality_of_service : quality_of_service AVERAGE DELAY number",
"quality_of_service : quality_of_service PEAK DELAY number",
"quality_of_service : quality_of_service AVERAGE LOSS number",
"quality_of_service : quality_of_service PEAK LOSS number",
"quality_of_service : quality_of_service RCVWIN integer",
"quality_of_service : quality_of_service SNDWIN integer",
"quality_of_service : quality_of_service INTERACTIVE",
"quality_of_service : quality_of_service INTERVAL number",
"quality_of_service : quality_of_service MTU integer",
"quality_of_service : quality_of_service TTL integer",
"quality_of_service : quality_of_service TOS integer",
"tg_entry_list :",
"tg_entry_list : tg_entry_list tg_entry",
"tg_entry : at_clause tg_action",
"tg_entry : tg_action",
"at_clause : AT time_literal",
"tg_action : tg_action_setup",
"tg_action : tg_action_wait",
"tg_action : tg_action_log",
"tg_action : tg_action_arrival tg_action_length tg_action_modifier_list",
"tg_action : tg_action_arrival tg_action_length tg_action_resplen tg_action_modifier_list",
"tg_action : tg_action_arrival tg_action_length tg_action_resplen tg_action_patience tg_action_modifier_list",
"tg_action_setup : SETUP",
"tg_action_wait : WAIT",
"tg_action_wait : WAIT time_literal",
"tg_action_log : LOG STRING_LITERAL",
"tg_action_arrival : ARRIVAL distribution",
"tg_action_length : LENGTH distribution",
"tg_action_resplen : RESPONSELENGTH distribution",
"tg_action_patience : PATIENCE time_literal",
"tg_action_modifier_list :",
"tg_action_modifier_list : tg_action_modifier_list tg_action_modifier",
"tg_action_modifier : DATA number",
"tg_action_modifier : RESET",
"tg_action_modifier : PACKET number",
"tg_action_modifier : SEED integer",
"tg_action_modifier : TIME time_literal",
"distribution : number",
"distribution : DIST_CONST number",
"distribution : DIST_EXP number",
"distribution : DIST_EXP number number number",
"distribution : DIST_MARKOV2 number distribution number distribution",
"distribution : DIST_UNIFORM number",
"distribution : DIST_UNIFORM number number",
"symbol : IDENTIFIER",
"symbol : decimal_number",
"symbol : number",
"symbol : integer",
"symbol : decimal_integer",
"symbol : integer",
"time_literal : decimal_number",
"time_literal : decimal_integer ':' decimal_number",
"time_literal : decimal_integer ':' decimal_integer ':' decimal_number",
"decimal_number : FLOATING_POINT",
"decimal_number : decimal_integer",
"number : FLOATING_POINT",
"number : integer",
"integer : HEX_INTEGER",
"integer : INTEGER",
"integer : OCTAL_INTEGER",
"decimal_integer : SMALL_INTEGER",
"decimal_integer : INTEGER",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 845 "tg.y"
#include "lex.yy.c"

/* MAINprogram for extractdoc.						*/

main(argc, argv)

	int		argc;
	char		*argv[];

	{
	extern int	yydebug;
	void		sigint();
	FILE		*fp;

	/* Set debugging if it is desired.				*/

#	if YYDEBUG
	/*yydebug = 1; */
#	endif

	/* Initialize lex state.					*/

	lex_init(argc, argv);
	
	/* fix the outputfile format */ 
	if (ofile != NULL) {
	  int i,j; 

	  prefix[0] = '\0'; 
	  suffix[0] = '\0'; 
	  for (i = strlen(ofile) - 1 ; i >= 0 ; i-- ) 
	    if ( ofile[i] == '.')
	      break; 
	  for (j =0 ; j < i ; j++ ) 
	    prefix[j] = ofile[j]; 
	  for (j = i+1 ; j < strlen(ofile) ; j++ ) 
	    suffix[j - i -1] = ofile[j]; 

	  if (0) fprintf(stderr, "ofile = %s, prefix = %s, suffix = %s \n", 
		  ofile, prefix, suffix ); 
	  
	}

	BEGIN LEX_NORMAL;

	/* Convert input.						*/

	if (yyparse() != 0)
		exit(1);
	if (got_errors)
		exit(1);

	(void) signal(SIGINT, sigint);
	(void) signal(SIGTERM, sigint);

	/*
	 * Set up logging.  If ofile is set, then we write to the 
	 * specified file.
	 */

	/* Start logging						*/

	start_log(global_start_tv, NULL);

	/* Fix up start and stop times.					*/

	fix_times();

	/* Wait for global start time, scream if it has already passed.	*/

	wait_start();

	/* Generate traffic.						*/

	do_actions();
	return (0);
	}

start_log(on_timeval, format)
	struct timeval on_timeval;	/* Starting timeval structure */
	char *format; 
	{
	FILE		*fp;
	time_t          now; 
	char buf[1024]; 
	int i; 
	
	if (ofile != NULL) { 
	  if (format != NULL ) { 
	    
	    now = on_timeval.tv_sec;
#ifdef SUNOS4
	    /* good old sunos 4.x */ 
	    if (1) {	      
	      struct tm *timebuf;
	      timebuf = gmtime(&now);
	      strftime(&buf[0], 1024,format, timebuf);
	    }
#else 
	    if (1) { 
	      struct tm timebuf;
	      (void) gmtime_r(&now, &timebuf);
	      strftime(&buf[0], 1024,format, &timebuf);
	    }
#endif
	    /* replace the forward slashes...*/ 
	    for (i= 0 ; i < strlen(buf); i++) 
	      if ( buf[i] == '/') 
		buf[i] = '_'; 
	    sprintf(logfilename, "%s.%s.%s", prefix, buf, suffix); 	
	    
	  } else {
	    
	    sprintf(logfilename, "%s.%s", prefix, suffix); 	
	    
	  }
	  
	  if (0) fprintf(stderr, "Log:  before editing %s\n", logfilename);

	  /*
	   * Set up logging.  If ofile is set, then we write to the 
	   * specified file.
	   */
	  if ((fp = log_open(logfilename)) == (FILE *) NULL) {
	    exit (-1);
	  }

	} else {

	  /* output file undefined */
	  if ((fp = log_open(NULL)) == (FILE *) NULL) {
	    exit (-1);
	  }	  

	}

	log_init(fp, on_timeval, prot.prot->name, prot.prot->af,
		ifile, &prot);
	}


/* Perform tasks based on list of actions.				*/

void
do_actions()

	{
	tg_action	*cur_tg;
	struct timeval	lasttime;
	long		tx_asn = -1;

	/* Set up receive routine.					*/

	if ((prot.qos & QOS_INTERACTIVE) == 0)
		prot.prot->rcv = rcv_pkt;
	else if ((prot.qos & QOS_SERVER) == 0)
		prot.prot->rcv = rcv_pkt_interactive;
	else
		prot.prot->rcv = rcv_pkt_interactive_srvr;

	/* If there is no explicit setup clause, do an immediate setup.	*/

	if (got_setup_implicit &&
	    ((tx_asn = (*(prot.prot->setup))(&prot)) == -1))
		{

		/* log the setup error. */

		perror("do_actions: protocol setup");
		exit(-1);
		}

	/* Each pass through the following loop processes one tg_action	*/
	/* element from the list.					*/

	for (cur_tg = tg_first; cur_tg != NULL; cur_tg = cur_tg->next)
		{

		/* Wait for start time, if one was specified.  Remember	*/
		/* the start time (or the current time, if the start	*/
		/* time is not specified) in ``lasttime''.		*/

		if ((cur_tg->tg_flags & TG_START) != 0)
			{
			(*(prot.prot->sleep_till))(&(cur_tg->start_at));
			lasttime = cur_tg->start_at;
			}
		else
			{
			if (gettimeofday(&lasttime,
					 (struct timezone *)NULL) == -1)
				{
				perror("do_actions: gettimeofday");
				abort();
				}
			}

		/* Compute stop time on the fly, if needed.		*/

		if ((cur_tg->tg_flags & TG_STOP) == 0)
			{
			if ((cur_tg->tg_flags & TG_TIME) == 0)
				{

				/* No definite stop time, set it for	*/
				/* some time in the year 2038...	*/

				cur_tg->stop_before.tv_sec = 0x7fffffff;
				}
			else
				{

				/* Add the time limit to the current	*/
				/* time to get the stop time.		*/

				timeradd(&lasttime,
					 &cur_tg->time_limit,
					 &cur_tg->stop_before);
				}
			}

		/* Perform the specified action.			*/

		if ((cur_tg->tg_flags & TG_SETUP) != 0)
			{

			/* Perform setup phase.				*/

			if ((tx_asn = (*(prot.prot->setup))(&prot)) == -1)
				{

				/* log the setup error. */

				perror("do_actions: protocol setup");
				exit(-1);
				}
			}
		else if ((cur_tg->tg_flags & TG_WAIT) != 0)
			{

			/* If we would run out of patience before the	*/
			/* end of the wait, wait for the patience	*/
			/* interval.					*/

			while ((xpctd_replies_qh != NULL) &&
			       (timercmp(&(xpctd_replies_qh->timeout),
					 &(cur_tg->stop_before),
					 <)))
				{

				/* Check to see if timeout has already	*/
				/* passed..				*/

				if (check_deadline
						(&(xpctd_replies_qh->timeout)))
					{

					/* @@@@ log frustration!	*/

					(void)fprintf(stderr,
						      "Patience exceeded!\n");
					exit(-1);
					}
				
				/* Wait for the timeout if not.		*/

				(*(prot.prot->sleep_till))
					(&(xpctd_replies_qh->timeout));
				}

			/* Just wait until the interval is over.	*/

			(*(prot.prot->sleep_till))(&(cur_tg->stop_before));
			}
	else if ((cur_tg->tg_flags & TG_LOG) != 0)
			{
			/* (Re-)Start logging to a new file */
			time_t now;
			struct timeval on_timeval;
			
			log_close();
			(void) time(&now);
			on_timeval.tv_sec = now;
			on_timeval.tv_usec = 0;
			start_log(on_timeval, cur_tg->log);
			}
		else if ((prot.qos & QOS_INTERACTIVE) != 0)
			{

			/* Generate traffic as specified by arrival	*/
			/* and length.					*/

			generate_interactive(tx_asn, cur_tg, lasttime);
			}
		else
			{

			/* Generate traffic as specified by arrival	*/
			/* and length.					*/

			generate(tx_asn, cur_tg, lasttime);
			}
		}

	/* Finished, tear down connection.				*/

	if ((*(prot.prot->teardown))(tx_asn) == -1)
		{

		/* log the teardown error. */

		perror("do_actions: protocol teardown");
		exit(-1);
		}

	return;
	}

/* Fix up start and stop times and perform semantic checks.		*/

void
fix_times()

	{
	tg_action	*cur_tg;

	/* Scan action list, computing all times that can be computed	*/
	/* beforehand.							*/

	for (cur_tg = tg_first; cur_tg != NULL; cur_tg = cur_tg->next)
		{

		/* If we aren't interactive and someone is trying to	*/
		/* do a responselength, scream.				*/

		if (((prot.qos & QOS_INTERACTIVE) == 0) &&
		    ((cur_tg->tg_flags & TG_RESPLEN) != 0))
			{
			(void)fprintf(stderr,
				      "resplen legal only if %s specified",
				      "interactive");
			exit(-1);
			}

		/* If there is no stop time, try to infer one.		*/

		if ((cur_tg->tg_flags & TG_STOP) == 0)
			{

			if ((cur_tg->tg_flags & (TG_START | TG_TIME)) ==
			    (TG_START | TG_TIME))
				{

				/* Infer from start time and limit.	*/

				cur_tg->tg_flags |= TG_STOP;
				timeradd(&(cur_tg->start_at),
					 &(cur_tg->time_limit),
					 &(cur_tg->stop_before));
				}
			else if ((cur_tg->next != NULL) &&
				 ((cur_tg->next->tg_flags & TG_START) != 0))
				{

				/* Infer from next actions start time.	*/

				if (!timercmp(&(cur_tg->next->start_at),
					      &(cur_tg->start_at),
					      >))
					{
					(void)fprintf(stderr,
						      "fix_times: %s\n",
						      "time clash");
					}
				cur_tg->tg_flags |= TG_STOP;
				cur_tg->stop_before = cur_tg->next->start_at;
				}
			}

		/* If there is now a stop time, make sure that it	*/
		/* does not conflict with the next item's start time.	*/
		/* If the next item does not have a start time, then	*/
		/* copy in the stop time from this item.		*/

		if (((cur_tg->tg_flags & TG_STOP) != 0) &&
		    (cur_tg->next != NULL))
			{
			if ((cur_tg->next->tg_flags & TG_START) == 0)
				{
				cur_tg->next->tg_flags |= TG_START;
				}
			else
				{
				if (timercmp(&(cur_tg->stop_before),
					     &(cur_tg->next->start_at),
					     >))
					{
					(void)fprintf(stderr,
						      "fix_times: %s\n",
						      "time clash");
					}
				}
			}
		}
	}

/* Check to see if current time has exceeded deadline.			*/

int
check_deadline(deadline)

	struct timeval	*deadline;

	{
	struct timeval	tv;

	if (gettimeofday(&tv, (struct timezone *)NULL) == -1)
		{
		perror("check_deadline: gettimeofday");
		abort();
		}
	return (timercmp(deadline, &tv, <));
	}

/* Generate traffic as specified by cur_tg.				*/

void
generate(tx_asn, cur_tg, lasttime)

	long		tx_asn;
	tg_action	*cur_tg;
	struct timeval	lasttime;
	{
	unsigned long	datasent = 0, pktsent = 0;
	static unsigned long	pktid = 0;

	/* Set random-number generator seed, if so  specified.		*/

	if ((cur_tg->tg_flags & TG_SEED) != 0) {
		(void)srand48(cur_tg->seed);
	}

	/* Reset packet counter */

	if ((cur_tg->tg_flags & TG_RESET) != 0) {
		pktid = 0;
	}

	/* Generate traffic until either packet, data, or time limit	*/
	/* is exceeded.							*/

	for (;;)
		{
		char		*buf;
		struct timeval	nextpkt_tv;
		unsigned long	pktlen;
		double		arrival;

		/* Find arrival time for next packet.			*/

		arrival = (*(cur_tg->arrival.generate))(&cur_tg->arrival);
		if (arrival != 0.)
			{

			/* The interarrival time is not exactly zero,	*/
			/* count the time to start when the last packet	*/
			/* was scheduled to leave, rather than when it	*/
			/* actually left.				*/

			dtotimevalfromthen(&lasttime, arrival, &nextpkt_tv);
			}
		else
			{

			/* The interarrival time is zero, so get the	*/
			/* current time of day to check for stop times.	*/

			if (gettimeofday(&nextpkt_tv,
					 (struct timezone *)NULL) == -1)
				{
				perror("generate: gettimeofday");
				abort();
				}
			}

		/* If we are to stop this action before the next packet	*/
		/* is to arrive, just wait for the stop time.		*/

		if (!timercmp(&(cur_tg->stop_before), &nextpkt_tv, >))
			{
			(*(prot.prot->sleep_till))(&(cur_tg->stop_before));
			break;
			}

		/* Otherwise, wait until nextpkt_tv to transmit the	*/
		/* packet.						*/

		if (arrival != 0)
			(*(prot.prot->sleep_till))(&nextpkt_tv);
		lasttime = nextpkt_tv;

		/* Did we exceed the limit on the number of packets to send? */

		if (((cur_tg->tg_flags & TG_PACKET) != 0) &&
		    (++pktsent > cur_tg->packet_limit)) {

			break;
		}

		/* Get the packet length and see if the data limit has	*/
		/* been exceeded.					*/

		pktlen = (*(cur_tg->length.generate))(&cur_tg->length);
		if (((cur_tg->tg_flags & TG_DATA) != 0) &&
		    ((datasent += pktlen) > cur_tg->data_limit)) {

			/* The current packet would put us over the	*/
			/* limit, quit!					*/

			break;
		}

		/* Get a buffer for the packet and transmit it.		*/

		if ((buf = (*(prot.prot->buffer_get))(pktlen)) == NULL)
			{

			log_error(NULL, NULL, tx_asn, LOGERR_MEM);
			(void)fprintf(stderr,
				      "main: buffer %s\n",
				      "allocation failure");
			}
		else
			{

			(void)(*(prot.prot->send))(tx_asn,
						   buf,
						   pktlen,
						   &(cur_tg->stop_before),
						   &pktid);
			}
		}
	}

/* Generate interactive-like traffic as specified by cur_tg.		*/

void
generate_interactive(tx_asn, cur_tg, lasttime)

	long		tx_asn;
	tg_action	*cur_tg;
	struct timeval	lasttime;

	{
	unsigned long	datasent = 0, pktsent = 0;
	int		encodelen;
	static unsigned long minbuflen = 0;
	unsigned long	pktid;
	static unsigned long replydatawaiting = 0;
	unsigned long	resplen;

	/* Get minimum buffer length if we don't yet know it.		*/

	if (minbuflen == 0)
		minbuflen = encode_special_response((char *)NULL, 0, 0);

	/* Set random-number generator seed, if so specified.		*/

	if ((cur_tg->tg_flags & TG_SEED) != 0)
		{
		(void)srand48(cur_tg->seed);
		}

	/* Generate traffic until either data limit or time limit	*/
	/* exceeded.							*/

	for (;;)
		{
		char		*buf;
		struct timeval	nextpkt_tv;
		unsigned long	pktlen;
		double		arrival;

		/* Find arrival time for next packet.			*/

		arrival = (*(cur_tg->arrival.generate))(&cur_tg->arrival);
		if (arrival != 0.)
			{

			/* The interarrival time is not exactly zero,	*/
			/* count the time to start when the last packet	*/
			/* was scheduled to leave, rather than when it	*/
			/* actually left.				*/

			dtotimevalfromthen(&lasttime, arrival, &nextpkt_tv);
			}
		else
			{

			/* The interarrival time is zero, so get the	*/
			/* current time of day to check for stop times.	*/

			if (gettimeofday(&nextpkt_tv,
					 (struct timezone *)NULL) == -1)
				{
				perror("generate: gettimeofday");
				abort();
				}
			}

		/* If we would run out of patience before the stop	*/
		/* time and before the next packet transmission time,	*/
		/* wait for the patience interval.			*/

		while ((xpctd_replies_qh != NULL) &&
		       (timercmp(&(xpctd_replies_qh->timeout),
				 &(cur_tg->stop_before),
				 <)) &&
		       (timercmp(&(xpctd_replies_qh->timeout), &nextpkt_tv, <)))
			{

			/* Check to see if timeout has already passed..	*/

			if (check_deadline(&(xpctd_replies_qh->timeout)))
				{

				/* @@@@ log frustration!		*/

				(void)fprintf(stderr, "Patience exceeded!\n");
				exit(-1);
				}
			
			/* Wait for the timeout if not.			*/

			(*(prot.prot->sleep_till))
				(&(xpctd_replies_qh->timeout));
			}

		/* If we are to stop this action before the next packet	*/
		/* is to arrive, just wait for the stop time.		*/

		if (!timercmp(&(cur_tg->stop_before), &nextpkt_tv, >))
			{
			(*(prot.prot->sleep_till))(&(cur_tg->stop_before));
			break;
			}

		/* Otherwise, wait until nextpkt_tv to transmit the	*/
		/* packet.						*/

		if (arrival != 0)
			(*(prot.prot->sleep_till))(&nextpkt_tv);
		lasttime = nextpkt_tv;

		/* Did we exceed the limit on the number of packets to send? */

		if (((cur_tg->tg_flags & TG_PACKET) != 0) &&
		    (++pktsent > cur_tg->packet_limit)) {

			break;
		}

		/* Get the packet length and see if the data limit has	*/
		/* been exceeded.					*/

		pktlen = (*(cur_tg->length.generate))(&cur_tg->length);
		if (((cur_tg->tg_flags & TG_DATA) != 0) &&
		    ((datasent += pktlen) > cur_tg->data_limit)) {

			/* The current packet would put us over the	*/
			/* limit, quit!					*/

			break;
		}

		/* Get a buffer for the packet and transmit it.		*/

		if ((buf = (*(prot.prot->buffer_get))(pktlen > minbuflen ?
						      pktlen : 
						      minbuflen)) == NULL)
			{

			log_error(NULL, NULL, tx_asn, LOGERR_MEM);
			(void)fprintf(stderr,
				      "main: buffer %s\n",
				      "allocation failure");
			}
		else
			{

			if ((cur_tg->tg_flags & TG_RESPLEN) == 0)
				{

				/* Encode a special zero to say that	*/
				/* we want no response.			*/

				encodelen = encode_special_response(buf,
								    pktlen,
								    0);
				}
			else
				{

				/* Generate desired response length.	*/

				resplen = (*(cur_tg->resplen.generate))
						(&(cur_tg->resplen));

				/* Encode the response length into the	*/
				/* packet and try to send it.		*/

				encodelen = encode_response(buf,
							    pktlen,
							    resplen);
				}

			/* Send the packet.				*/

			for (;;)
				{
				struct timeval	*tvp;

				/* Get a pointer to a timeout, either	*/
				/* the patience timeout or the end of	*/
				/* this action, whichever comes first.	*/

				if (xpctd_replies_qh == NULL)
					tvp = &(cur_tg->stop_before);
				else if (timercmp(&(cur_tg->stop_before),
						  &(xpctd_replies_qh->timeout),
						  >))
					tvp = &(xpctd_replies_qh->timeout);
				else
					tvp = &(cur_tg->stop_before);
				if ((*(prot.prot->send))(tx_asn,
							 buf,
							 pktlen > encodelen ?
								pktlen :
								encodelen,
							 tvp,
							 &pktid) == -1)
					{

					/* If we had a hard error or if	*/
					/* we hit the end of the action,*/
					/* quit.			*/

					if ((errno != ETIME) ||
					    (tvp == &(cur_tg->stop_before)))
						break;

					/* Check to see if patience	*/
					/* timeout has already passed..	*/

					if ((xpctd_replies_qh != NULL) &&
					    (check_deadline
						(&(xpctd_replies_qh->timeout))))
						{

						/* @@@@ log frustration!*/

						(void)fprintf(stderr,
							      "%s exceeded!\n",
							      "Patience");
						exit(-1);
						}
					}
				else
					{

					/* If this action is impatient,	*/
					/* make an xpctd-replies entry.	*/

					if ((cur_tg->tg_flags &
					     TG_PATIENCE) != 0)
						{
						xpctd_replies *p =
							xpctd_replies_get();

						p->byte_seqno =
							replydatawaiting;
						timeradd(&nextpkt_tv,
							 &(cur_tg->patience),
							 &(p->timeout));
						*xpctd_replies_qt = p;
						xpctd_replies_qt = &(p->next);

						/* Accumulate the total	*/
						/* amount of outstanding*/
						/* reply data expected.	*/

						replydatawaiting += resplen;
						}

					break;
					}
				}
			}
		}
	}

/* NODE INITialize.  Forces all fields to zero/NULL.			*/

void
node_init(node)

	YYSTYPE		*node;

	{

	(void)memset((char *)node, 0, sizeof(*node));
	}

/* Receive and log packet.						*/

int
rcv_pkt(rx_asn, tx_asn, buf, len, pktid)

	long		rx_asn;
	long		tx_asn;
	char		*buf;
	int		len;
	unsigned long	pktid;

	{

	/* Do nothing, since the protocol logs it.  This serves as a	*/
	/* debugging hook.						*/

	return (0);
	}

/* Receive a response from an interactive server (we sent a packet	*/
/* requesting a response, the server responded, and we just received	*/
/* that response).  Log the packet and modify the xpctd_replies list	*/
/* to account for that packet.						*/

int
rcv_pkt_interactive(rx_asn, tx_asn, buf, len, pktid)

	long		rx_asn;
	long		tx_asn;
	char		*buf;
	int		len;
	unsigned long	pktid;

	{
	static unsigned long replydatarcvd = 0;
	int		retval;
	xpctd_replies	*xp = xpctd_replies_qh;

	/* Invoke rcv_pkt to log the packet's arrival.			*/

	retval = rcv_pkt(rx_asn, tx_asn, buf, len, pktid);

	/* Each pass through the following loop removes one		*/
	/* xpctd_replies entry from the list.  Note that stream		*/
	/* protocols such as TCP can fuse packets; this can result in	*/
	/* one receive removing several xpctd_replies.			*/

	replydatarcvd += len;
	while (xp != NULL)
		{

		/* Quit if no more xpctd_replies are covered.		*/

		if (replydatarcvd <= xp->byte_seqno)
			break;

		/* Remove the current xpctd_replies entry.		*/

		xpctd_replies_qh = xp->next;
		xpctd_replies_free(xp);
		if ((xp = xpctd_replies_qh) == NULL)
			xpctd_replies_qt = &xpctd_replies_qh;
		}

	/* Tell the caller what rcv_pkt did with the buffer.		*/

	return (retval);
	}

/* Receive a packet from an interactive client that must be responded	*/
/* to.  Since packets may be glued and chopped, the commands from the	*/
/* client must be pasted back together, in general.  Therefore, a	*/
/* separate state structure is maintained for each association.		*/

int
rcv_pkt_interactive_srvr(rx_asn, tx_asn, buf, len, pktid)

	long		rx_asn;
	long		tx_asn;
	char		*buf;
	int		len;
	unsigned long	pktid;

	{
	char		*bufend;
	char		*cp = buf;
	srvr_state	*p;
	int		rplpktid;
	char		*replbuf;
	int		retval;

	/* Invoke rcv_pkt to log the packet's arrival.			*/

	retval = rcv_pkt(rx_asn, tx_asn, buf, len, pktid);

	/* If we really have a packet, get a pointer to the asn's	*/
	/* state information.  If we have an EOF or error, delete	*/
	/* the asn's state information.					*/

	if (buf != NULL)
		p = srvr_state_get(rx_asn, 0);
	else
		{
		p = srvr_state_get(rx_asn, 1);
		return (retval);
		}

	/* If this association has given us bad data in the past,	*/
	/* ignore it.							*/

	if (p->bad)
		return (retval);

	/* Each pass through the following loop consumes one byte of	*/
	/* control information or skips as much filler as possible.	*/

	bufend = &(buf[len]);
	while (cp < bufend)
		{

		/* Handle current byte as specified by current state.	*/

		switch (p->state)
		{
		case srvr_len :

			/* Accumulate the response-length field.	*/

			p->acc |= (*cp & 0x7f) << (p->nbytes * 7);
			if (++p->nbytes > 5)
				{

				/* Failed plausibility check, so ignore	*/
				/* this association from here on out.	*/

				p->bad = 1;
				log_error(NULL, NULL, rx_asn, LOGERR_INTFMT);
				}

			/* If this is the last byte, send the reply.	*/
			/* The special form of zero represented by	*/
			/* 0x80 0x00 says ``send no packet''.  This	*/
			/* can be detected by *cp==0x00, since putting	*/
			/* a zero at the end of a number does not	*/
			/* change its value.				*/

			if ((*cp & 0x80) == 0)
				{
				if (*cp == 0)
					;
				else if ((replbuf =
				          (*(prot.prot->buffer_get))(p->acc)) ==
				         NULL)
					{
					log_error(NULL, NULL, -1, LOGERR_MEM);
					(void)fprintf(stderr,
						      "%s%s: Out of memory\n",
						      "rcv_pkt_",
						      "interactive_srvr");
					abort();
					}
				else
					{
					(void)(*(prot.prot->send))(tx_asn,
								   replbuf,
								   p->acc,
								   NULL,
								   &rplpktid);
					}

				/* Set the state to pick up the skip	*/
				/* count.				*/

				p->state = srvr_skip;
				p->acc = 0;
				p->nbytes = 0;
				}

			cp++;
			break;

		case srvr_skip :

			/* Accumulate the skip field.			*/

			p->acc |= (*cp & 0x7f) << (p->nbytes * 7);

			/* If we have the full field, skip the		*/
			/* specified number of bytes.			*/

			if ((*cp & 0x80) != 0)
				{
				if (++p->nbytes > 5)
					{

					/* Failed plausibility check,	*/
					/* so ignore this association	*/
					/* from here on out.		*/

					p->bad = 1;
					log_error(NULL,
						  NULL,
						  rx_asn,
						  LOGERR_INTFMT);
					}
				cp++;
				break;
				}
			else
				{
				p->skip = p->acc - p->nbytes;
				if ((p->skip & ~0x7fffffff) != 0)
					{

					/* Failed plausibility check,	*/
					/* so ignore this association	*/
					/* from here on out.		*/

					p->bad = 1;
					log_error(NULL,
						  NULL,
						  rx_asn,
						  LOGERR_INTFMT);
					}
				p->state = srvr_skipping;
				p->nbytes = 0;

				/* drop into next leg of switch...	*/

				}

		case srvr_skipping :

			/* Skip the bytes.				*/

			cp += p->skip;

			/* If past the end of the packet, adjust the	*/
			/* count and skip the first part of the next	*/
			/* packet.  Otherwise, just skip the specified	*/
			/* length.					*/

			if ((cp > bufend) ||
			    (cp < buf))
				p->skip = cp - bufend;
			else
				{
				p->state = srvr_len;
				p->acc = 0;
				}
			break;


		}
		}

	return (retval);
	}

/* Get the state entry for the specified association, creating it if	*/
/* necessary, and returning a pointer to it.  If the delete flag is	*/
/* set, the entry is deleted if it is present and a NULL pointer	*/
/* returned.								*/

srvr_state *
srvr_state_get(asn, delete)

	long		asn;
	int		delete;

	{
	srvr_state	*p;
	srvr_state	**q;

	/* Search the list for the specified entry, if found, move it	*/
	/* to the front of the list.					*/

	for (p = srvr_state_h, q = &srvr_state_h;
	     p != NULL;
	     q = &(p->next), p = *q)
		{
		if (p->asn == asn)
			{
			*q = p->next;
			if (delete)
				{
				(void)free((char *)p);
				return (NULL);
				}
			else
				{
				p->next = srvr_state_h;
				srvr_state_h = p;
				break;
				}
			}
		}

	/* If no entry was found, create one (unless it was to be	*/
	/* deleted anyway...).						*/

	if ((p == NULL) &&
	    !delete)
		{
		if ((p = (srvr_state *)malloc(sizeof(srvr_state))) == NULL)
			{

			log_error(NULL, NULL, -1, LOGERR_MEM);
			(void)fprintf(stderr,
				      "srvr_state_get: Out of memory\n");
			abort();
			}
		p->next = srvr_state_h;
		srvr_state_h = p;
		p->state = srvr_len;
		p->asn = asn;
		p->acc = 0;
		p->nbytes = 0;
		p->bad = 0;
		}

	/* Return a pointer to the entry.				*/

	return (p);
	}

/* Append new element to action structure.				*/

void
tg_append_element(ap1, ap2)

	tg_action	*ap1;
	tg_action	*ap2;

	{

	ap1->tg_flags |= ap2->tg_flags;
	if ((ap2->tg_flags & TG_ARRIVAL) != 0)
		ap1->arrival = ap2->arrival;
	if ((ap2->tg_flags & TG_DATA) != 0)
		ap1->data_limit = ap2->data_limit;
	if ((ap2->tg_flags & TG_PACKET) != 0)
		ap1->packet_limit= ap2->packet_limit;
	if ((ap2->tg_flags & TG_LENGTH) != 0)
		ap1->length = ap2->length;
	if ((ap2->tg_flags & TG_PATIENCE) != 0)
		ap1->patience = ap2->patience;
	if ((ap2->tg_flags & TG_RESPLEN) != 0)
		ap1->resplen = ap2->resplen;
	if ((ap2->tg_flags & TG_SEED) != 0)
		ap1->seed = ap2->seed;
	if ((ap2->tg_flags & TG_TIME) != 0)
		ap1->time_limit = ap2->time_limit;
	}

/* Wait for start time.  Currently we assume that initial page-faults	*/
/* are not a performance problem.  If this is not the case, we need	*/
/* to add code to reference all pages in order to fault them in.	*/

void
wait_start()

	{

	if (check_deadline(&global_start_tv))
		{
		(void)fprintf(stderr, "Starting time already passed!\n");
		(void)fprintf(stderr, "Restart program!\n");
		exit(-1);
		}
	(*(prot.prot->sleep_till))(&global_start_tv);
	}

/* Get an expected-replies queue element.				*/

xpctd_replies *
xpctd_replies_get()

	{
	xpctd_replies	*p;

	/* If the freelist is empty, malloc up another entry.		*/

	if (xpctd_replies_flist == NULL)
		{
		p = (xpctd_replies *)malloc(sizeof(xpctd_replies));
		if (p == NULL)
			{
			(void)fprintf(stderr,
				      "xpctd_replies_get: out of memory!\n");
			abort();
			}
		p->next = NULL;
		return (p);
		}

	/* Freelist is nonempty, just grab the next element.		*/

	p = xpctd_replies_flist;
	xpctd_replies_flist = p->next;
	return (p);
	}

/* Free up an expected-replies queue element.				*/

void
xpctd_replies_free(p)

	xpctd_replies	*p;

	{

	p->next = xpctd_replies_flist;
	xpctd_replies_flist = p;
	return;
	}

/* Yacc error routine.							*/

void
yyerror(s)

	char		*s;

	{
	(void)fflush(stdout);
	(void)fprintf(stderr, "%s, line %d: %s\n", filename, lineno, s);
	/* (void)fprintf(stderr, "%s, line %d: %s\n", 
             	   current_config_file, lineno, s); */
	(void)fflush(stderr);
	got_errors = 1;
	}

void
sigint(sig, code, scp, addr)

	int		sig, code;
	struct sigcontext *scp;
	char		*addr;
{

	log_close();
}
#line 1801 "y.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 6:
#line 234 "tg.y"
{
		FILE	*fd;

		fprintf(stderr, "including \"%s\"\n", yyvsp[0].n); 

		if (include_level > MAX_INCLUDE_LEVEL) {
			fprintf(stderr, "too many levels of inclusion\n");
			exit(-1);
		}
		if ((fd = fopen(yyvsp[0].n, "r")) == NULL) {
			fprintf(stderr, "can't open include file \"%s\"\n",
				yyvsp[0].n);
			exit(-1);
		}
		file_tbl[include_level].fd = yyin;
		file_tbl[include_level].cnt = yylineno;
		strcpy(file_tbl[include_level].name, current_config_file);
#if defined(FREEBSD) ||  defined(LINUX)
		file_tbl[include_level].stack = YY_CURRENT_BUFFER; 
#endif		
		yyin = fd;
		yylineno = 0;
		strcpy(current_config_file, yyvsp[0].n);
#if defined(FREEBSD) || defined(LINUX)
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE)); 
#endif		

		include_level++;
		}
break;
case 7:
#line 267 "tg.y"
{
		extern char	*malloc();
		char		*cp;

		if (!(cp = malloc (strlen(yyvsp[-2].n) + 1))) {
			perror("malloc");
			exit (-1);
		}
		strcpy (cp, yyvsp[-2].n);
		sym_tbl[sym_tbl_index].sym_name = cp;
		sym_tbl[sym_tbl_index].sym_type = SYM_INT_TYPE;
		sym_tbl[sym_tbl_index].sym_intval = (int) yyvsp[0].d;

#ifdef DEBUG
		fprintf(stderr, "sym_tbl_index [%d] macro [%s], value [%d]\n",
			sym_tbl_index, cp, (int) yyvsp[0].d);
#endif
		sym_tbl_index++;
		}
break;
case 8:
#line 290 "tg.y"
{
		prot = yyvsp[-1].prot;
		}
break;
case 9:
#line 297 "tg.y"
{
		struct timeval	tp;
		unsigned long	modulus = yyvsp[0].d;
		if (modulus == 0 ) 
		        {
			  yyerror("Start time must be greater than 0");
			  exit(1);			  
			}
		if (modulus != yyvsp[0].d)
			{
			yyerror("start_time: ON value must be integral");
			}
		if (gettimeofday(&tp, (struct timezone *)NULL) == -1)
			{
			perror("gettimeofday");
			exit(-1);
			}
		global_start = ((tp.tv_sec + modulus - 1) / modulus) * modulus;
		global_start_tv.tv_sec = global_start;
		global_start_tv.tv_usec = 0;
		}
break;
case 10:
#line 322 "tg.y"
{
		yyval = yyvsp[0];
		yyval.prot.qos |= yyvsp[-1].prot.qos;
		yyval.prot.src = yyvsp[-1].prot.src;
		yyval.prot.dst = yyvsp[-1].prot.dst;
		yyval.prot.prot = yyvsp[-1].prot.prot;
		}
break;
case 11:
#line 333 "tg.y"
{
		yyval = yyvsp[-1];
		yyval.prot.dst = yyvsp[0].tmpaddr;
		yyval.prot.qos = QOS_DST;
		}
break;
case 12:
#line 339 "tg.y"
{
		yyval = yyvsp[-2];
		yyval.prot.src = yyvsp[-1].tmpaddr;
		yyval.prot.qos = QOS_SRC | QOS_SERVER;
		}
break;
case 13:
#line 345 "tg.y"
{
		yyval = yyvsp[-2];
		yyval.prot.src = yyvsp[-1].tmpaddr;
		yyval.prot.dst = yyvsp[0].tmpaddr;
		yyval.prot.qos = QOS_SRC | QOS_DST;
		}
break;
case 14:
#line 355 "tg.y"
{
		node_init(&yyval);
		yyval.prot.prot = yyvsp[0].prot.prot;
		lexprot = yyval.prot;
		BEGIN LEX_ADDRESS;
		}
break;
case 15:
#line 365 "tg.y"
{
		node_init(&yyval);
		}
break;
case 16:
#line 370 "tg.y"
{
		yyval = yyvsp[-3];
		yyval.prot.avg_bandwidth = yyvsp[0].d;
		yyval.prot.qos |= QOS_AVG_BANDWIDTH;
		}
break;
case 17:
#line 376 "tg.y"
{
		yyval = yyvsp[-3];
		yyval.prot.peak_bandwidth = yyvsp[0].d;
		yyval.prot.qos |= QOS_PEAK_BANDWIDTH;
		}
break;
case 18:
#line 382 "tg.y"
{
		yyval = yyvsp[-3];
		yyval.prot.avg_delay = yyvsp[0].d;
		yyval.prot.qos |= QOS_AVG_DELAY;
		}
break;
case 19:
#line 388 "tg.y"
{
		yyval = yyvsp[-3];
		yyval.prot.peak_delay = yyvsp[0].d;
		yyval.prot.qos |= QOS_PEAK_DELAY;
		}
break;
case 20:
#line 394 "tg.y"
{
		yyval = yyvsp[-3];
		yyval.prot.avg_loss = yyvsp[0].d;
		yyval.prot.qos |= QOS_AVG_LOSS;
		}
break;
case 21:
#line 400 "tg.y"
{
		yyval = yyvsp[-3];
		yyval.prot.peak_loss = yyvsp[0].d;
		yyval.prot.qos |= QOS_PEAK_LOSS;
		}
break;
case 22:
#line 406 "tg.y"
{
                    yyval = yyvsp[-2];
		    yyval.prot.rcvwin = yyvsp[0].d;
		    yyval.prot.qos |= QOS_RCVWIN;
		}
break;
case 23:
#line 412 "tg.y"
{
                    yyval = yyvsp[-2];
		    yyval.prot.sndwin = yyvsp[0].d;
		    yyval.prot.qos |= QOS_SNDWIN;
		}
break;
case 24:
#line 418 "tg.y"
{
		yyval = yyvsp[-1];
		yyval.prot.qos |= QOS_INTERACTIVE;
		}
break;
case 25:
#line 423 "tg.y"
{
		yyval = yyvsp[-2];
		yyval.prot.interval = yyvsp[0].d;
		yyval.prot.qos |= QOS_INTERVAL;
		}
break;
case 26:
#line 429 "tg.y"
{
		yyval = yyvsp[-2];
		yyval.prot.mtu = yyvsp[0].d;
		yyval.prot.qos |= QOS_MTU;
		}
break;
case 27:
#line 435 "tg.y"
{
		yyval = yyvsp[-2];
		yyval.prot.multicast_ttl = yyvsp[0].d;
		yyval.prot.qos |= QOS_TTL;
		}
break;
case 28:
#line 441 "tg.y"
{
		#ifdef SUNOS4
		  fprintf(stderr,"SunOS does not support setting TOS field.\n");
		  exit(1); 
                #endif	
		yyval = yyvsp[-2];
		yyval.prot.tos = yyvsp[0].d;
		yyval.prot.qos |= QOS_TOS;
		}
break;
case 30:
#line 455 "tg.y"
{

		/* Add the new entry to the linked list.  Note that	*/
		/* the chain represented by tg_entry_list has already	*/
		/* been added to the list, and need not be referred to,	*/
		/* thus there is no need for node_init in the above	*/
		/* empty clause.					*/

		*tg_last = (tg_action *)malloc(sizeof(tg_action));
		if (*tg_last == NULL)
			{
			yyerror("tg_entry_list: Out of memory");
			}
		**tg_last = yyvsp[0].action;
		tg_last = &((*tg_last)->next);
		}
break;
case 31:
#line 475 "tg.y"
{
		yyval = yyvsp[0];
		yyval.action.tg_flags |= yyvsp[-1].action.tg_flags;
		yyval.action.start_at = yyvsp[-1].action.start_at;
		}
break;
case 32:
#line 481 "tg.y"
{
		yyval = yyvsp[0];
		}
break;
case 33:
#line 488 "tg.y"
{
		double		tmp;

		node_init(&yyval);
		tmp = global_start + yyvsp[0].d;
		dtotimeval(tmp, &yyval.action.start_at);
		yyval.action.tg_flags |= TG_START;
		}
break;
case 34:
#line 500 "tg.y"
{
		yyval = yyvsp[0];
		}
break;
case 35:
#line 504 "tg.y"
{
		yyval = yyvsp[0];
		}
break;
case 36:
#line 508 "tg.y"
{
		yyval = yyvsp[0];
		}
break;
case 37:
#line 512 "tg.y"
{
		yyval = yyvsp[-2];
		tg_append_element(&yyval.action, &yyvsp[-1].action);
		tg_append_element(&yyval.action, &yyvsp[0].action);
		}
break;
case 38:
#line 519 "tg.y"
{
		yyval = yyvsp[-3];
		tg_append_element(&yyval.action, &yyvsp[-2].action);
		tg_append_element(&yyval.action, &yyvsp[-1].action);
		tg_append_element(&yyval.action, &yyvsp[0].action);
		}
break;
case 39:
#line 527 "tg.y"
{
		yyval = yyvsp[-4];
		tg_append_element(&yyval.action, &yyvsp[-3].action);
		tg_append_element(&yyval.action, &yyvsp[-2].action);
		tg_append_element(&yyval.action, &yyvsp[-1].action);
		tg_append_element(&yyval.action, &yyvsp[0].action);
		}
break;
case 40:
#line 538 "tg.y"
{
		node_init(&yyval);
		if (got_setup)
			{
			yyerror("Multiple setup clauses not supported");
			YYERROR;
			}
		if (got_setup_implicit)
			{
			yyerror("setup clause not legal after implicit setup");
			YYERROR;
			}
		got_setup = 1;
		yyval.action.tg_flags |= TG_SETUP;
		}
break;
case 41:
#line 557 "tg.y"
{
		node_init(&yyval);
		if (!got_setup)
			got_setup_implicit = 1;
		yyval.action.tg_flags |= TG_WAIT;
		}
break;
case 42:
#line 564 "tg.y"
{
		node_init(&yyval);
		if (!got_setup)
			got_setup_implicit = 1;
		yyval.action.tg_flags |= TG_WAIT | TG_TIME;
		dtotimeval(yyvsp[0].d, &yyval.action.time_limit);
		}
break;
case 43:
#line 575 "tg.y"
{
		char		*cp;

		node_init(&yyval);
		if (!got_setup)
			got_setup_implicit = 1;
		yyval.action.tg_flags |= TG_LOG;
		if (!(cp = malloc (strlen(yyvsp[0].n) + 1))) {
			perror("malloc");
			exit (-1);
		}
		strcpy (cp, yyvsp[0].n);
		yyval.action.log = cp;
		}
break;
case 44:
#line 592 "tg.y"
{
		node_init(&yyval);
		if (!got_setup)
			got_setup_implicit = 1;
		yyval.action.tg_flags |= TG_ARRIVAL;
		yyval.action.arrival = yyvsp[0].tmpdist;
		}
break;
case 45:
#line 603 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_LENGTH;
		yyval.action.length = yyvsp[0].tmpdist;
		}
break;
case 46:
#line 612 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_RESPLEN;
		yyval.action.resplen = yyvsp[0].tmpdist;
		}
break;
case 47:
#line 620 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_PATIENCE;
		dtotimeval(yyvsp[0].d, &yyval.action.patience);
		}
break;
case 48:
#line 628 "tg.y"
{
		node_init(&yyval);
		}
break;
case 49:
#line 632 "tg.y"
{
		yyval = yyvsp[-1];
		tg_append_element(&yyval.action, &yyvsp[0].action);
		}
break;
case 50:
#line 640 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_DATA;
		yyval.action.data_limit = yyvsp[0].d;
		}
break;
case 51:
#line 646 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_RESET;
		}
break;
case 52:
#line 651 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_PACKET;
		yyval.action.packet_limit = yyvsp[0].d;
		}
break;
case 53:
#line 657 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_SEED;
		yyval.action.seed = yyvsp[0].d;
		}
break;
case 54:
#line 663 "tg.y"
{
		node_init(&yyval);
		yyval.action.tg_flags |= TG_TIME;
		dtotimeval(yyvsp[0].d, &yyval.action.time_limit);
		}
break;
case 55:
#line 672 "tg.y"
{
		char		*cp;

		if ((cp = dist_const_init(&yyval.tmpdist, yyvsp[0].d)) != NULL)
			yyerror(cp);
		}
break;
case 56:
#line 679 "tg.y"
{
		char		*cp;

		if ((cp = dist_const_init(&yyval.tmpdist, yyvsp[0].d)) != NULL)
			yyerror(cp);
		}
break;
case 57:
#line 686 "tg.y"
{
		char		*cp;

		if ((cp = dist_exp_init(&yyval.tmpdist, yyvsp[0].d, (double) 0,
					(double) MAX_RANDOM)) != NULL)
			yyerror(cp);
		}
break;
case 58:
#line 694 "tg.y"
{
		char		*cp;
		
		if (yyvsp[-1].d < yyvsp[0].d)
			{
			if ((cp = dist_exp_init(&yyval.tmpdist, yyvsp[-2].d, yyvsp[-1].d,
						yyvsp[0].d)) != NULL)
				yyerror(cp);
			}
		else 
			{
			if ((cp = dist_exp_init(&yyval.tmpdist, yyvsp[-2].d, yyvsp[0].d,
						yyvsp[-1].d)) != NULL)
				yyerror(cp);
			}
		}
break;
case 59:
#line 711 "tg.y"
{
		char		*cp;

		if ((cp = dist_markov2_init(&yyval.tmpdist,
					    yyvsp[-3].d,
					    &(yyvsp[-2].tmpdist),
					    yyvsp[-1].d,
					    &(yyvsp[0].tmpdist))) != NULL)
			yyerror(cp);
		}
break;
case 60:
#line 722 "tg.y"
{
		char		*cp;

		if ((cp = dist_uniform_init(&yyval.tmpdist,
					    (double) 0, yyvsp[0].d)) != NULL)
			yyerror(cp);
		}
break;
case 61:
#line 730 "tg.y"
{
		char		*cp;

		if (yyvsp[-1].d < yyvsp[0].d)
			{
			if ((cp = dist_uniform_init(&yyval.tmpdist,
						    yyvsp[-1].d, yyvsp[0].d)) != NULL)
				yyerror(cp);
			}
		else
			{
			if ((cp = dist_uniform_init(&yyval.tmpdist,
						    yyvsp[0].d, yyvsp[-1].d)) != NULL)
				yyerror(cp);
			}
		}
break;
case 62:
#line 751 "tg.y"
{
		int	i, found;

		/* Perform symbol lookup if we're working with a string */
		for (i = 0, found = 0; i < sym_tbl_index; i++) {
			if ((strcmp(sym_tbl[i].sym_name, yyvsp[0].n) == 0) && 
				(sym_tbl[i].sym_type == SYM_INT_TYPE)) {
				found++;
				break;
			}
		}

		if (found) {
			yyval.d = sym_tbl[i].sym_intval;
		} else {
			fprintf(stderr, "reference to unknown symbol \"%s\"",
				yyvsp[0].n);
			exit(1);
		}
	}
break;
case 67:
#line 776 "tg.y"
{
		yyval = yyvsp[0];
		}
break;
case 68:
#line 783 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 69:
#line 787 "tg.y"
{
		yyval.d = yyvsp[-2].d * 60 + yyvsp[0].d;
		}
break;
case 70:
#line 791 "tg.y"
{
		yyval.d = (yyvsp[-4].d * 60 + yyvsp[-2].d) * 60 + yyvsp[0].d;
		}
break;
case 71:
#line 798 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 72:
#line 802 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 73:
#line 809 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 74:
#line 813 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 75:
#line 820 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 76:
#line 824 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 77:
#line 828 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 78:
#line 835 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
case 79:
#line 839 "tg.y"
{
		yyval.d = yyvsp[0].d;
		}
break;
#line 2656 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
