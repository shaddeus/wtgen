%{
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

%}

/* Lex token definitions.						*/

%token INCLUDE EQUAL

%token DIST_CONST DIST_EXP DIST_MARKOV DIST_MARKOV2 DIST_UNIFORM

%token ARRIVAL AT AVERAGE BANDWIDTH DATA DELAY INTERACTIVE INTERVAL 
%token LENGTH LOG LOSS MTU ON PATIENCE PEAK RCVWIN RESPONSELENGTH SEED SERVER
%token SETUP SNDWIN TIME TOS TTL WAIT 

%token ADDR FILENAME FLOATING_POINT HEX_INTEGER IDENTIFIER INTEGER
%token OCTAL_INTEGER PROTOCOL SMALL_INTEGER STRING_LITERAL

%token ADDRESS

%token PACKET RESET

/* Starting state.							*/

%start statements 


%%

/*
  changed the grammar to reflect the correct semantics. 

statements
	: statement
	| statements statement
	;
*/

statements
	: include statements
	| commands
	;

statement
	: include
	| macro
	| commands
	;

include
	: INCLUDE STRING_LITERAL
		{
		FILE	*fd;

		fprintf(stderr, "including \"%s\"\n", $2.n); 

		if (include_level > MAX_INCLUDE_LEVEL) {
			fprintf(stderr, "too many levels of inclusion\n");
			exit(-1);
		}
		if ((fd = fopen($2.n, "r")) == NULL) {
			fprintf(stderr, "can't open include file \"%s\"\n",
				$2.n);
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
		strcpy(current_config_file, $2.n);
#if defined(FREEBSD) || defined(LINUX)
		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE)); 
#endif		

		include_level++;
		}
	;

macro
	: IDENTIFIER EQUAL integer
		{
		extern char	*malloc();
		char		*cp;

		if (!(cp = malloc (strlen($1.n) + 1))) {
			perror("malloc");
			exit (-1);
		}
		strcpy (cp, $1.n);
		sym_tbl[sym_tbl_index].sym_name = cp;
		sym_tbl[sym_tbl_index].sym_type = SYM_INT_TYPE;
		sym_tbl[sym_tbl_index].sym_intval = (int) $3.d;

#ifdef DEBUG
		fprintf(stderr, "sym_tbl_index [%d] macro [%s], value [%d]\n",
			sym_tbl_index, cp, (int) $3.d);
#endif
		sym_tbl_index++;
		}
	;

commands
	: start_time association_spec tg_entry_list
		{
		prot = $2.prot;
		}
	;

start_time
	: ON time_literal
		{
		struct timeval	tp;
		unsigned long	modulus = $2.d;
		if (modulus == 0 ) 
		        {
			  yyerror("Start time must be greater than 0");
			  exit(1);			  
			}
		if (modulus != $2.d)
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
	;

association_spec
	: protocol_and_addresses quality_of_service
		{
		$$ = $2;
		$$.prot.qos |= $1.prot.qos;
		$$.prot.src = $1.prot.src;
		$$.prot.dst = $1.prot.dst;
		$$.prot.prot = $1.prot.prot;
		}
	;

protocol_and_addresses
	: protocol ADDR
		{
		$$ = $1;
		$$.prot.dst = $2.tmpaddr;
		$$.prot.qos = QOS_DST;
		}
	| protocol ADDR SERVER
		{
		$$ = $1;
		$$.prot.src = $2.tmpaddr;
		$$.prot.qos = QOS_SRC | QOS_SERVER;
		}
	| protocol ADDR ADDR
		{
		$$ = $1;
		$$.prot.src = $2.tmpaddr;
		$$.prot.dst = $3.tmpaddr;
		$$.prot.qos = QOS_SRC | QOS_DST;
		}
	;

protocol
	: PROTOCOL
		{
		node_init(&$$);
		$$.prot.prot = $1.prot.prot;
		lexprot = $$.prot;
		BEGIN LEX_ADDRESS;
		}
	;

quality_of_service
	:
		{
		node_init(&$$);
		}

	| quality_of_service AVERAGE BANDWIDTH number
		{
		$$ = $1;
		$$.prot.avg_bandwidth = $4.d;
		$$.prot.qos |= QOS_AVG_BANDWIDTH;
		}
	| quality_of_service PEAK BANDWIDTH number
		{
		$$ = $1;
		$$.prot.peak_bandwidth = $4.d;
		$$.prot.qos |= QOS_PEAK_BANDWIDTH;
		}
	| quality_of_service AVERAGE DELAY number
		{
		$$ = $1;
		$$.prot.avg_delay = $4.d;
		$$.prot.qos |= QOS_AVG_DELAY;
		}
	| quality_of_service PEAK DELAY number
		{
		$$ = $1;
		$$.prot.peak_delay = $4.d;
		$$.prot.qos |= QOS_PEAK_DELAY;
		}
	| quality_of_service AVERAGE LOSS number
		{
		$$ = $1;
		$$.prot.avg_loss = $4.d;
		$$.prot.qos |= QOS_AVG_LOSS;
		}
	| quality_of_service PEAK LOSS number
		{
		$$ = $1;
		$$.prot.peak_loss = $4.d;
		$$.prot.qos |= QOS_PEAK_LOSS;
		}
         | quality_of_service RCVWIN integer
                {
                    $$ = $1;
		    $$.prot.rcvwin = $3.d;
		    $$.prot.qos |= QOS_RCVWIN;
		}
        | quality_of_service SNDWIN integer
                {
                    $$ = $1;
		    $$.prot.sndwin = $3.d;
		    $$.prot.qos |= QOS_SNDWIN;
		}
	| quality_of_service INTERACTIVE
		{
		$$ = $1;
		$$.prot.qos |= QOS_INTERACTIVE;
		}
	| quality_of_service INTERVAL number
		{
		$$ = $1;
		$$.prot.interval = $3.d;
		$$.prot.qos |= QOS_INTERVAL;
		}
	| quality_of_service MTU integer
		{
		$$ = $1;
		$$.prot.mtu = $3.d;
		$$.prot.qos |= QOS_MTU;
		}
	| quality_of_service TTL integer
		{
		$$ = $1;
		$$.prot.multicast_ttl = $3.d;
		$$.prot.qos |= QOS_TTL;
		}
	| quality_of_service TOS integer
		{
		#ifdef SUNOS4
		  fprintf(stderr,"SunOS does not support setting TOS field.\n");
		  exit(1); 
                #endif	
		$$ = $1;
		$$.prot.tos = $3.d;
		$$.prot.qos |= QOS_TOS;
		}
   	;

tg_entry_list
	:
	| tg_entry_list tg_entry
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
		**tg_last = $2.action;
		tg_last = &((*tg_last)->next);
		}
	;

tg_entry
	: at_clause tg_action
		{
		$$ = $2;
		$$.action.tg_flags |= $1.action.tg_flags;
		$$.action.start_at = $1.action.start_at;
		}
	| tg_action
		{
		$$ = $1;
		}
	;

at_clause
	: AT time_literal
		{
		double		tmp;

		node_init(&$$);
		tmp = global_start + $2.d;
		dtotimeval(tmp, &$$.action.start_at);
		$$.action.tg_flags |= TG_START;
		}
	;

tg_action
	: tg_action_setup
		{
		$$ = $1;
		}
	| tg_action_wait
		{
		$$ = $1;
		}
        | tg_action_log 
                {
		$$ = $1;
		}
	| tg_action_arrival tg_action_length tg_action_modifier_list
		{
		$$ = $1;
		tg_append_element(&$$.action, &$2.action);
		tg_append_element(&$$.action, &$3.action);
		}
	| tg_action_arrival tg_action_length tg_action_resplen
	  tg_action_modifier_list
		{
		$$ = $1;
		tg_append_element(&$$.action, &$2.action);
		tg_append_element(&$$.action, &$3.action);
		tg_append_element(&$$.action, &$4.action);
		}
	| tg_action_arrival tg_action_length tg_action_resplen
	  tg_action_patience tg_action_modifier_list
		{
		$$ = $1;
		tg_append_element(&$$.action, &$2.action);
		tg_append_element(&$$.action, &$3.action);
		tg_append_element(&$$.action, &$4.action);
		tg_append_element(&$$.action, &$5.action);
		}
	;

tg_action_setup
	: SETUP
		{
		node_init(&$$);
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
		$$.action.tg_flags |= TG_SETUP;
		}
	;

tg_action_wait
	: WAIT
		{
		node_init(&$$);
		if (!got_setup)
			got_setup_implicit = 1;
		$$.action.tg_flags |= TG_WAIT;
		}
	| WAIT time_literal
		{
		node_init(&$$);
		if (!got_setup)
			got_setup_implicit = 1;
		$$.action.tg_flags |= TG_WAIT | TG_TIME;
		dtotimeval($2.d, &$$.action.time_limit);
		}
	;

tg_action_log
	: LOG STRING_LITERAL
		{
		char		*cp;

		node_init(&$$);
		if (!got_setup)
			got_setup_implicit = 1;
		$$.action.tg_flags |= TG_LOG;
		if (!(cp = malloc (strlen($2.n) + 1))) {
			perror("malloc");
			exit (-1);
		}
		strcpy (cp, $2.n);
		$$.action.log = cp;
		}

tg_action_arrival
	: ARRIVAL distribution
		{
		node_init(&$$);
		if (!got_setup)
			got_setup_implicit = 1;
		$$.action.tg_flags |= TG_ARRIVAL;
		$$.action.arrival = $2.tmpdist;
		}
	;

tg_action_length
	: LENGTH distribution
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_LENGTH;
		$$.action.length = $2.tmpdist;
		}
	;

tg_action_resplen
	: RESPONSELENGTH distribution
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_RESPLEN;
		$$.action.resplen = $2.tmpdist;
		}

tg_action_patience
	: PATIENCE time_literal
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_PATIENCE;
		dtotimeval($2.d, &$$.action.patience);
		}

tg_action_modifier_list
	:
		{
		node_init(&$$);
		}
	| tg_action_modifier_list tg_action_modifier
		{
		$$ = $1;
		tg_append_element(&$$.action, &$2.action);
		}
	;

tg_action_modifier
	: DATA number
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_DATA;
		$$.action.data_limit = $2.d;
		}
	| RESET 
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_RESET;
		}
	| PACKET number
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_PACKET;
		$$.action.packet_limit = $2.d;
		}
	| SEED integer
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_SEED;
		$$.action.seed = $2.d;
		}
	| TIME time_literal
		{
		node_init(&$$);
		$$.action.tg_flags |= TG_TIME;
		dtotimeval($2.d, &$$.action.time_limit);
		}
	;

distribution
	: number
		{
		char		*cp;

		if ((cp = dist_const_init(&$$.tmpdist, $1.d)) != NULL)
			yyerror(cp);
		}
	| DIST_CONST number
		{
		char		*cp;

		if ((cp = dist_const_init(&$$.tmpdist, $2.d)) != NULL)
			yyerror(cp);
		}
	| DIST_EXP number
		{
		char		*cp;

		if ((cp = dist_exp_init(&$$.tmpdist, $2.d, (double) 0,
					(double) MAX_RANDOM)) != NULL)
			yyerror(cp);
		}
	| DIST_EXP number number number		/* exp mean min max */
		{
		char		*cp;
		
		if ($3.d < $4.d)
			{
			if ((cp = dist_exp_init(&$$.tmpdist, $2.d, $3.d,
						$4.d)) != NULL)
				yyerror(cp);
			}
		else 
			{
			if ((cp = dist_exp_init(&$$.tmpdist, $2.d, $4.d,
						$3.d)) != NULL)
				yyerror(cp);
			}
		}
	| DIST_MARKOV2 number distribution number distribution
		{
		char		*cp;

		if ((cp = dist_markov2_init(&$$.tmpdist,
					    $2.d,
					    &($3.tmpdist),
					    $4.d,
					    &($5.tmpdist))) != NULL)
			yyerror(cp);
		}
	| DIST_UNIFORM number			/* max */
		{
		char		*cp;

		if ((cp = dist_uniform_init(&$$.tmpdist,
					    (double) 0, $2.d)) != NULL)
			yyerror(cp);
		}
	| DIST_UNIFORM number number		/* min max */
		{
		char		*cp;

		if ($2.d < $3.d)
			{
			if ((cp = dist_uniform_init(&$$.tmpdist,
						    $2.d, $3.d)) != NULL)
				yyerror(cp);
			}
		else
			{
			if ((cp = dist_uniform_init(&$$.tmpdist,
						    $3.d, $2.d)) != NULL)
				yyerror(cp);
			}
		}
	;


symbol
	: IDENTIFIER
	{
		int	i, found;

		/* Perform symbol lookup if we're working with a string */
		for (i = 0, found = 0; i < sym_tbl_index; i++) {
			if ((strcmp(sym_tbl[i].sym_name, $1.n) == 0) && 
				(sym_tbl[i].sym_type == SYM_INT_TYPE)) {
				found++;
				break;
			}
		}

		if (found) {
			$$.d = sym_tbl[i].sym_intval;
		} else {
			fprintf(stderr, "reference to unknown symbol \"%s\"",
				$1.n);
			exit(1);
		}
	}
	| decimal_number
	| number
	| integer
	| decimal_integer
	| integer
		{
		$$ = $1;
		}
	;

time_literal
	: decimal_number
		{
		$$.d = $1.d;
		}
	| decimal_integer ':' decimal_number
		{
		$$.d = $1.d * 60 + $3.d;
		}
	| decimal_integer ':' decimal_integer ':' decimal_number
		{
		$$.d = ($1.d * 60 + $3.d) * 60 + $5.d;
		}
	;

decimal_number
	: FLOATING_POINT
		{
		$$.d = $1.d;
		}
	| decimal_integer
		{
		$$.d = $1.d;
		}
	;

number
	: FLOATING_POINT
		{
		$$.d = $1.d;
		}
	| integer
		{
		$$.d = $1.d;
		}
	;

integer
	: HEX_INTEGER
		{
		$$.d = $1.d;
		}
	| INTEGER
		{
		$$.d = $1.d;
		}
	| OCTAL_INTEGER
		{
		$$.d = $1.d;
		}
	;

decimal_integer
	: SMALL_INTEGER
		{
		$$.d = $1.d;
		}
	| INTEGER
		{
		$$.d = $1.d;
		}
	;

%%
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
