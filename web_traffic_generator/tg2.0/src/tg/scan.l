%{
/************************************************************************
 *									*
 *	File:  scan.l							*
 *									*
 *	lexical analyser for CATE traffic generator.			*
 *	Hacked from extractdoc lexer, in turn hacked from an ANSI C	*
 *	lexer.								*
 *									*
 *	Written 18-Jun-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 SRI International.				*
 *									*
 ************************************************************************/

#ifndef lint 
static char scan_l_rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/scan.l,v 1.2 2002/01/24 23:25:28 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <strings.h>
#include "y.tab.h"

/* Type definitions local to this file.					*/

#ifndef FLEX_SCANNER      /* for now tg won't handle include files under FreeBSD */
#undef input
#define input()	\
	(((yytchar = \
	   yysptr > yysbuf ? \
		U(*--yysptr) : \
		(curarg < 0 ? \
			getc(yyin) : \
			((yytchar = *(curchar++)) != 0 ? \
				yytchar : \
				(gargv[++curarg] != NULL) ? \
					curchar = &(gargv[curarg][0]), \
					'\n' : \
					(strcpy(filename, "<stdin>"), \
					 curarg = -1, \
					 lineno = 1, \
					 '\n' \
					) \
			) \
		) \
	  ) == 10 ? \
		(yylineno++, yytchar) : \
		yytchar \
	 ) == EOF ? \
		0 : \
		yytchar \
	)

#endif

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

static void		count();	/* COUNT columns for error use.	*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

static int		column = 0;	/* input COLUMN number.		*/
static int		curarg;		/* arglist arg pointer.		*/
static char		*curchar;	/* arglist char pointer.	*/
static char		**gargv;	/* global version of argv.	*/
static int		lineno = 1;	/* input LINE NOmber.		*/
static char		filename[BUFSIZ] = "<arglist>";

%}

	/* Definitions.							*/

%a 8000
%e 2000
%n 1000
%o 20000
%p 10000

O		[0-7]
D		[0-9]
L		[a-zA-Z_]
H		[a-fA-F0-9]
E		[Ee][+-]?{D}+
W		[ \t\v\f]
WW		{W}*
ID		{L}({L}|{D})*

%START LEX_NORMAL LEX_ADDRESS

%%

"include"		{ count(); BEGIN LEX_NORMAL; return(INCLUDE); }
"="			{ count(); BEGIN LEX_NORMAL; return(EQUAL); }

"server"		{ count(); BEGIN LEX_NORMAL; return(SERVER); }

"average"		{ count(); BEGIN LEX_NORMAL; return(AVERAGE); }
"bandwidth"		{ count(); BEGIN LEX_NORMAL; return(BANDWIDTH); }
"delay"			{ count(); BEGIN LEX_NORMAL; return(DELAY); }
"interactive"		{ count(); BEGIN LEX_NORMAL; return(INTERACTIVE); }
"interval"		{ count(); BEGIN LEX_NORMAL; return(INTERVAL); }
"loss"			{ count(); BEGIN LEX_NORMAL; return(LOSS); }
"MTU"			{ count(); BEGIN LEX_NORMAL; return(MTU); }
"mtu"			{ count(); BEGIN LEX_NORMAL; return(MTU); }
"peak"			{ count(); BEGIN LEX_NORMAL; return(PEAK); }
"rcvwin"                { count(); BEGIN LEX_NORMAL; return(RCVWIN); }
"sndwin"                { count(); BEGIN LEX_NORMAL; return(SNDWIN); }
"tos"                   { count(); BEGIN LEX_NORMAL; return(TOS); }

"constant"		{ count(); BEGIN LEX_NORMAL; return(DIST_CONST); }
"exponential"		{ count(); BEGIN LEX_NORMAL; return(DIST_EXP); }
"exp"			{ count(); BEGIN LEX_NORMAL; return(DIST_EXP); }
"markov"		{ count(); BEGIN LEX_NORMAL; return(DIST_MARKOV); }
"markov2"		{ count(); BEGIN LEX_NORMAL; return(DIST_MARKOV2); }
"uniform"		{ count(); BEGIN LEX_NORMAL; return(DIST_UNIFORM); }

"arrival"		{ count(); BEGIN LEX_NORMAL; return(ARRIVAL); }
"at"			{ count(); BEGIN LEX_NORMAL; return(AT); }
"data"			{ count(); BEGIN LEX_NORMAL; return(DATA); }
"length"		{ count(); BEGIN LEX_NORMAL; return(LENGTH); }
"on"			{ count(); BEGIN LEX_NORMAL; return(ON); }
"patience"		{ count(); BEGIN LEX_NORMAL; return(PATIENCE); }
"responselength"	{ count(); BEGIN LEX_NORMAL; return(RESPONSELENGTH); }
"resplen"		{ count(); BEGIN LEX_NORMAL; return(RESPONSELENGTH); }
"seed"			{ count(); BEGIN LEX_NORMAL; return(SEED); }
"setup"			{ count(); BEGIN LEX_NORMAL; return(SETUP); }
"time"			{ count(); BEGIN LEX_NORMAL; return(TIME); }
"wait"			{ count(); BEGIN LEX_NORMAL; return(WAIT); }

"packet"		{ count(); BEGIN LEX_NORMAL; return(PACKET); }
"reset"			{ count(); BEGIN LEX_NORMAL; return(RESET); }
"TTL"			{ count(); BEGIN LEX_NORMAL; return(TTL); }
"ttl"			{ count(); BEGIN LEX_NORMAL; return(TTL); }
"log"			{ count(); BEGIN LEX_NORMAL; return(LOG); }


<LEX_NORMAL>":"		{ count(); return(':'); }

<LEX_NORMAL>{ID}	{
			count();
			if ((yylval.prot.prot = find_protocol(yytext)) != NULL)
				return(PROTOCOL);
			else
				{
				SYMcpy(yylval.n, yytext);
				return(IDENTIFIER);
				}
			}


<LEX_NORMAL>"/"({ID}("/"{ID})*)?|{ID}("/"{ID})+ {

			/* Note that filenames must contain at least	*/
			/* one slash.					*/

			count();
			SYMcpy(yylval.n, yytext);
			return(FILENAME);
			}

<LEX_NORMAL>0[xX]{H}+	{
			unsigned long	tmp;

			count();
			SYMcpy(yylval.n, &yytext[2]);	/* skip past 0[xX] */
			(void)sscanf(yylval.n, "%lx", &tmp);
			yylval.d = tmp;
			return(HEX_INTEGER);
			}
<LEX_NORMAL>0+{O}{O}+	{
			unsigned long	tmp;

			count();
			SYMcpy(yylval.n, &yytext[1]);	/* skip initial 0*/
			(void)sscanf(yytext, "%lo", &tmp);
			yylval.d = tmp;
			return(OCTAL_INTEGER);
			}
<LEX_NORMAL>0+{O}	{
			unsigned long	tmp;

			count();
			SYMcpy(yylval.n, yytext);
			(void)sscanf(yytext, "%lu", &tmp);
			yylval.d = tmp;
			return(SMALL_INTEGER);
			}
<LEX_NORMAL>{D}+	{
			unsigned long	tmp;

			count();
			SYMcpy(yylval.n, yytext);
			(void)sscanf(yytext, "%lu", &tmp);
			yylval.d = tmp;
			return(INTEGER);
			}
<LEX_NORMAL>'(\\.|[^\\'])*' {
			char		*qp;

			count();

			/* Strip quotes from character constant.	*/

			qp = &yytext[strlen(yytext) - 1];
			if (*qp == '\'')
				*qp = '\0';
			SYMcpy(yylval.n, yytext + 1);
			return(STRING_LITERAL);
			}

<LEX_NORMAL>{D}+{E}	{
			count();
			SYMcpy(yylval.n, yytext);
			(void)sscanf(yytext, "%lf", &yylval.d);
			return(FLOATING_POINT);
			}
<LEX_NORMAL>{D}*"."{D}+({E})? {
			count();
			SYMcpy(yylval.n, yytext);
			(void)sscanf(yytext, "%lf", &yylval.d);
			return(FLOATING_POINT);
			}
<LEX_NORMAL>{D}+"."{D}*({E})? {
			count();
			SYMcpy(yylval.n, yytext);
			(void)sscanf(yytext, "%lf", &yylval.d);
			return(FLOATING_POINT);
			}

<LEX_NORMAL>\"(\\.|[^\\"])*\" {
			char		*qp;

			count();
			
			/* Strip quotes from string.			*/
			
			qp = &yytext[strlen(yytext) - 1];
			if (*qp == '"')
				*qp = '\0';
                        SYMcpy(yylval.n, yytext + 1); 
			return(STRING_LITERAL);
			}
<LEX_NORMAL>\"[^"\n]*	{
			count();
			yyerror("Unterminated string");
			exit(-1);
			}

<LEX_NORMAL>[ \t\n\014]	{ count(); }
<LEX_NORMAL>.		{
			count();
			yyerror("Illegal character");
			exit(-1);
			}

<LEX_NORMAL>^#.*$	{
			/* Comment lines...				*/
			count();
			}

<LEX_ADDRESS>[ \t\n\014] { count(); }

<LEX_ADDRESS>[\41-\176]* {
			count();
			if ((*(lexprot.prot->atoaddr))(yytext,
						       &(yylval.tmpaddr)))
				return (ADDR);
			else
				{
				BEGIN LEX_NORMAL;
				REJECT;
				}
			}
%%

/* File record structure used for holding file include state            */
struct file_entry {
	FILE		*fd;
	char		name[MAXSYM];
	int		cnt;
#if defined(FREEBSD) || defined(LINUX)
        YY_BUFFER_STATE stack; 
#endif

};

#define MAX_INCLUDE_LEVEL       10
struct file_entry file_tbl[MAX_INCLUDE_LEVEL];
char			current_config_file[MAXSYM];
int			include_level = 0;	/* Current inclusion depth */

yywrap()
{

    /*
     * If we are currently in an included file, don't exit, but pop back
     * to the calling file.
     */
    if (include_level > 0) {
	include_level--;
	yyin = file_tbl[include_level].fd;
	yylineno = file_tbl[include_level].cnt;
	strcpy(current_config_file, file_tbl[include_level].name);

#if defined(FREEBSD) || defined(LINUX)
        yy_delete_buffer(YY_CURRENT_BUFFER); 
        yy_switch_to_buffer(file_tbl[include_level].stack); 
#endif
	return(0);
    } else {
	return(1);
    }
}

static void
count()
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			{
			column = 0;
			lineno++;
			}
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
}

/* Lex initialization.							*/

lex_init(argc, argv)

	int		argc;
	char		*argv[];
{
	int		c;
	int		errflg = 0;

        extern char	*optarg, *ifile, *ofile;
        extern int	optind;

	gargv = argv;

	/* 
	 * We should use LEX to parse the command line.
	 * For now, we use the standard getopt() call to parse
	 * command line options.
         */
        while ((c = getopt(argc, argv, "fi:o:")) != -1) {
		switch (c) {
		    case 'i':
			if (ifile) {
				errflg++;
				break;
			}
			ifile = optarg;
			break;
		    case 'f':
			FlushOutput = 1;
			break;
                    case 'o':
			if (ofile) {
				errflg++;
				break;
			}
			ofile = optarg;
                        break;
		    case '?':
                        errflg++;
                    default: 
                        errflg++;
                        fprintf (stderr, "lex_init: unknown option [%s]\n",
				argv[optind]);
               }
	}
	if (errflg) {
		fprintf (stderr, "lex_init: command line error\n");
		exit (-1);
	}

	if(ifile) {
                strncpy(current_config_file, ifile, MAXSYM); 
		if (freopen (ifile, "r", stdin) == (FILE *) NULL) {
			exit (-1);
		}
	}
#ifdef stuff
	if (argc < 2) {
		curarg = -1;
	} else {
		curarg = 1;
		curchar = &(gargv[curarg][0]);
	}
#endif 
	curarg = -1;
}
