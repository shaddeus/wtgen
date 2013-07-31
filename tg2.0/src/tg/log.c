/************************************************************************
 *									*
 *	File:  log.c							*
 *									*
 *	Routines to write out log entries.				*
 *									*
 *	Written 20-Sep-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/log.c,v 1.2 2002/01/24 23:23:39 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h> 
#include <sys/param.h>
#include <errno.h>
#include <utmp.h>

#include "config.h"
#include "distribution.h"
#include "protocol.h"
#include "decode.h"
#include "log.h"

/* Type definitions local to this file.					*/

#define FPRINTF		(void) fprintf
#ifdef SUNOS4 
#define MAXLOGNAME 9
#define HOSTNAMELEN MAXHOSTNAMELEN
#else 
#ifndef MAXLOGNAME 
#ifdef SYS_NMLN 
#define MAXLOGNAME (SYS_NMLN)
#else
#define MAXLOGNAME (UT_NAMESIZE + 1)
#endif 
#endif
#ifndef HOSTNAMELEN   
#ifdef SYS_NMLN 
#define HOSTNAMELEN (SYS_NMLN)
#else
#define HOSTNAMELEN (UT_HOSTSIZE + 1)
#endif 
#endif 
#endif

/* Functions exported from this file.					*/

FILE	*log_open();
int	log_init();

/* Functions local to this file.					*/

static char		*log_current_time();
static char		*log_sched_time();
static char		*log_time();
static void		log_write_error();

/* Variables exported from this file.					*/

/* Variables impored to this file					*/
extern int FlushOutput;

/* Variables local to this file.					*/

static  long		on_time_sec;
static	protocol	*prot;
static  FILE		*log_fp;



FILE *
log_open(filename)
	char		*filename;
{
	FILE		*fp;

        if(filename) {
		if ((fp = fopen(filename, "w")) == NULL) {
			perror("fopen: read");
			return (NULL);
		} 
        } else {
		fp = stdout; 
        } 
	return(fp);
}

/*
 * TG log file header is stored in ASCII form and contains the
 * following information:
 *
 *     o  Data file version identifier (e.g., 1.1)
 *     o  Script file name
 *     o  Start time of TG program 
 *     o  Address format identifier
 *     o  Name of protocol under test
 *     o  Header termination line
 */

log_init(fp, on_timeval, prot_name, af, script, prot2)
	
	FILE	*fp;			/* Handle to log file */
	struct timeval on_timeval;	/* Starting timeval structure */
	char    *prot_name;		/* Textual name of protocol under test*/
	short	af;			/* Address format specifier */
	char    *script;		/* name of script file */
	protocol *prot2;		/* Common protocol structure */
{

	char	username[MAXLOGNAME + 1];
	char	hostname[HOSTNAMELEN + 1];
	extern  char    *version;
	struct utsname buf; 
	static  char    machine[32];
        static  char    model[64];
        static  char    kern_version[256];
	int len, err,i; 

	/* Store time (in seconds) in global variable */
	on_time_sec = on_timeval.tv_sec;

	prot = prot2;

	log_fp = fp;
	
	/* cuserid returns the controlling terminal user's id. Not
	   sure what will happen if the controlling terminal does not
	   exist when tg is run as a daemon */

	if (1){ 
	  extern char *cuserid(char *);
	  char *x = (char *)cuserid(NULL);	  
	  if (x == NULL){ 
	    FPRINTF(stderr,
		    "log_init: fatal error: unable to fetch username \n");
	    abort();
	  } else {
	    strncpy(username, x, MAXLOGNAME); 
	  }
	}

	/* Build data file information header */
	FPRINTF(fp, BEGIN_HDR_STRING);
	FPRINTF(fp, "TG program version %s\n", version);
	FPRINTF(fp, "TG log file version %d.%d\n", LOG_VERSION, LOG_SUBVERSION);
	
	uname (&buf); 

#ifndef FREEBSD 	
	FPRINTF(fp, "Log file created by %s on %s \n", 
		username, buf.nodename); 
	FPRINTF(fp, "Machine configuration is %s %s %s %s\n", 
		buf.sysname, buf.release, buf.version, buf.machine);
#else 

	len = sizeof(machine) - 1;
	err = sysctlbyname("hw.machine", machine, &len, (void *)0, 0);
	if (err != 0 && errno != ENOMEM) {
	  FPRINTF(stderr,
		  "log_init: fatal error: unable to fetch machine type\n");
	  abort();
	}
	machine[len] = '\0';

	len = sizeof(model) - 1;
	err = sysctlbyname("hw.model", model, &len, (void *)0, 0);
	if (err != 0 && errno != ENOMEM) {
	  FPRINTF(stderr,
		  "log_init: fatal error: unable to fetch machine model\n");
	  abort();
	}	
	model[len] = '\0';

	len = sizeof(kern_version) - 1;
	err = sysctlbyname("kern.version", kern_version, &len, (void *)0, 0);
	if (err != 0 && errno != ENOMEM) {
	  FPRINTF(stderr,
		  "log_init: fatal error: unable to fetch kernel version\n");
	  abort();
	}

	/* fix any newlines that may be hidden in the code */
	for (i= 0 ; i< len; i++){ 
	  if (kern_version[i] == '\n')
	    kern_version[i] = ' '; 
	}

	FPRINTF(fp, "Log file created by %s on %s \n", 
		username, buf.nodename); 
	FPRINTF(fp, "Machine configuration is %s %s %s\n", 
		kern_version, machine, model);

	
#endif
	if (script) {
	  if (script[0] != '/') {
	    /* relative path - convert it to absolute path */
	    char buf[MAXPATHLEN];
	    char *result; 
	    int len; 
	    result = getcwd(buf, MAXPATHLEN); 
	    if ( result){ 
	      len = strlen(result); 
	      if (result[len-1] == '/') 
		result[len-1] = '\0'; 		
	      FPRINTF(fp, "Script filename is %s/%s\n", result, script);
	    } else { 
	      FPRINTF(fp, "Script filename is %s\n", script);
	    }
	  } else { 
	    FPRINTF(fp, "Script filename is %s\n", script);
	  }
	} else {
	  FPRINTF(fp, "Script read from stdin\n");
	}

	on_timeval.tv_usec = 0L;		/* Zero usec field */
	FPRINTF(fp, "Program start time from UNIX epoch %d\n",
		on_timeval.tv_sec);
	FPRINTF(fp, "Program start time: %s",
		ctime((time_t *) &on_timeval.tv_sec));
	
	FPRINTF(fp, "Address family identifier is %d\n", af);
	FPRINTF(fp, "Protocol under test is %s\n", prot_name);

	/* The following entry must immediately precede data records */
	FPRINTF(fp, END_HDR_STRING);

	return(0);				/* Normal return */
}


static void
log_write_error( /* type */ )
	/* int	type;		/* Not currently used */
{

	FPRINTF(stderr, "log_write_error: fatal error when writing log file\n");
	abort();
}


/* Log connection acceptance.  ``address2'' may be NULL to indicate	*/
/* that it is not present.  ``errno'' may be -1 to indicate		*/
/* exception-free transmission.						*/

void
log_accept(address1, address2, asn, lerrno)

	struct sockaddr	*address1;
	struct sockaddr	*address2;
	int		asn;
	int		lerrno;

	{
	char		buf[100];
	char		*cp = buf;
	char		ctl;

	/* Encode record type.						*/

	*cp++ = LOGTYPE_ACCEPT;

	/* Encode control byte.						*/

	ctl = 0;
	ctl |= LOGCTL_ADDR;
	if (address2 != NULL)
		ctl |= LOGCTL_2ADDR;
	if (lerrno >= 0)
		ctl |= LOGCTL_EXCEPT;
	*cp++ = ctl;
	
	/* Encode current time (accepts are always asynchronous).	*/

	cp = log_current_time(cp);

	/* Encode address(es) or asn.					*/

	cp = (*(prot->prot->addrtob))(address1, cp);
	if (address2 != NULL)
		cp = (*(prot->prot->addrtob))(address2, cp);
	cp = encode_ulong(cp, (unsigned long) asn);

	/* Encode error number, if one is present.			*/

	if (lerrno >= 0)
		cp = encode_ulong(cp, (unsigned long) lerrno);

	/* Write out the buffer.					*/

	if (fwrite(buf, cp - buf, 1, log_fp) != 1)
		log_write_error();
	else
	{    
	    if (FlushOutput)
		fflush(log_fp);
        }
}

/* Log program error.  ``address1'' and ``address2'' may be NULL	*/
/* to indicate that they are not present, ``asn'' may be -1 to indicate	*/
/* that it is not present.  ``errcode'' indicates the type of error.	*/

void
log_error(address1, address2, asn, errcode)

	struct sockaddr	*address1;
	struct sockaddr	*address2;
	int		asn;
	int		errcode;

	{
	char		buf[100];
	char		*cp = buf;
	char		ctl;

	/* Encode record type.						*/

	*cp++ = LOGTYPE_ERROR;

	/* Encode control byte.						*/

	ctl = 0;
	if (address1 != NULL)
		{
		ctl |= LOGCTL_ADDR;
		if (address2 != NULL)
			ctl |= LOGCTL_2ADDR;
		}
	*cp++ = ctl;
	
	/* Encode current time (errors are always asynchronous).	*/

	cp = log_current_time(cp);

	/* Encode address(es) or asn.					*/

	if (address1 == NULL)
		cp = encode_ulong(cp, (unsigned long) asn);
	else
		{
		cp = (*(prot->prot->addrtob))(address1, cp);
		if (address2 != NULL)
			cp = (*(prot->prot->addrtob))(address2, cp);
		}

	/* Encode error code.						*/

	cp = encode_ulong(cp, (unsigned long) errcode);

	/* Write out the buffer.					*/

	if (fwrite(buf, cp - buf, 1, log_fp) != 1)
		log_write_error();
	else
	{    
	    if (FlushOutput)
		fflush(log_fp);
        }
}

/* Log packet reception.  ``address1'' and ``address2'' may be NULL	*/
/* to indicate that they are not present, ``asn'' may be -1 to indicate	*/
/* that it is not present.  ``errno'' may be -1 to indicate		*/
/* exception-free transmission.						*/

void
log_rx(address1, address2, asn, pktid, len, lerrno)

	struct sockaddr	*address1;
	struct sockaddr	*address2;
	int		asn;
	unsigned long	pktid;
	unsigned long	len;
	int		lerrno;

	{
	char		buf[100];
	char		*cp = buf;
	char		ctl;

	/* Encode record type.						*/

	*cp++ = LOGTYPE_RX;

	/* Encode control byte.						*/

	ctl = 0;
	if (address1 != NULL)
		{
		ctl |= LOGCTL_ADDR;
		if (address2 != NULL)
			ctl |= LOGCTL_2ADDR;
		}
	if (lerrno >= 0)
		ctl |= LOGCTL_EXCEPT;
	*cp++ = ctl;
	
	/* Encode current time (receives are always asynchronous).	*/

	cp = log_current_time(cp);

	/* Encode address(es) or asn.					*/

	if (address1 == NULL)
		cp = encode_ulong(cp, (unsigned long) asn);
	else
		{
		cp = (*(prot->prot->addrtob))(address1, cp);
		if (address2 != NULL)
			cp = (*(prot->prot->addrtob))(address2, cp);
		}

	/* Encode packet ID and length.					*/

	cp = encode_ulong(cp, pktid);
	cp = encode_ulong(cp, len);

	/* Encode error number, if one is present.			*/

	if (lerrno >= 0)
		cp = encode_ulong(cp, (unsigned long) lerrno);

	/* Write out the buffer.					*/

	if (fwrite(buf, cp - buf, 1, log_fp) != 1)
		log_write_error();
	else
	{    
	    if (FlushOutput)
		fflush(log_fp);
        }
}

/* Log setup.  ``tvp'' may be NULL to indicate that this transmission	*/
/* was not explictly scheduled. ``lerrno'' may be -1 to indicate		*/
/* exception-free transmission.						*/

void
log_setup(tvp, lerrno)

	struct timeval	*tvp;
	int		lerrno;

	{
	char		buf[100];
	char		*cp = buf;
	char		ctl;

	/* Encode record type.						*/

	*cp++ = LOGTYPE_SETUP;

	/* Encode control byte.						*/

	ctl = 0;
	if (tvp != NULL)
		ctl |= LOGCTL_SCHED;
	if (lerrno >= 0)
		ctl |= LOGCTL_EXCEPT;
	*cp++ = ctl;

	/* Encode time, if present, otherwise encode current time.	*/

	if (tvp == NULL)
		cp = log_current_time(cp);
	else
		cp = log_sched_time(cp, tvp);

	/* Encode error number, if one is present.			*/

	if (lerrno >= 0)
		cp = encode_ulong(cp, (unsigned long) lerrno);

	/* Write out the buffer.					*/

	if (fwrite(buf, cp - buf, 1, log_fp) != 1)
		log_write_error();
	else
	{    
	    if (FlushOutput)
		fflush(log_fp);
        }
}

/* Log teardown.  ``tvp'' may be NULL to indicate that this		*/
/* transmission was not explictly scheduled. ``lerrno'' may be -1 to	*/
/* indicate exception-free transmission.				*/

void
log_teardown(tvp, lerrno)

	struct timeval	*tvp;
	int		lerrno;

	{
	char		buf[100];
	char		*cp = buf;
	char		ctl;

	/* Encode record type.						*/

	*cp++ = LOGTYPE_TEARDOWN;

	/* Encode control byte.						*/

	ctl = 0;
	if (tvp != NULL)
		ctl |= LOGCTL_SCHED;
	if (lerrno >= 0)
		ctl |= LOGCTL_EXCEPT;
	*cp++ = ctl;
	
	/* Encode time, if present, otherwise encode current time.	*/

	if (tvp == NULL)
		cp = log_current_time(cp);
	else
		cp = log_sched_time(cp, tvp);

	/* Encode error number, if one is present.			*/

	if (lerrno >= 0)
		cp = encode_ulong(cp, (unsigned long) lerrno);

	/* Write out the buffer.					*/

	if (fwrite(buf, cp - buf, 1, log_fp) != 1)
		log_write_error();
	else
	{    
	    if (FlushOutput)
		fflush(log_fp);
        }
}

/* Log packet transmission.  ``tvp'' may be NULL to indicate that this	*/
/* transmission was not explictly scheduled, ``address1'' and		*/
/* ``address2'' may be NULL to indicate that they are not present,	*/
/* ``asn'' may be -1 to indicate that it is not present.  ``lerrno'' may	*/
/* be -1 to indicate exception-free transmission.			*/

void
log_tx(tvp, address1, address2, asn, pktid, len, lerrno)

	struct timeval	*tvp;
	struct sockaddr	*address1;
	struct sockaddr	*address2;
	int		asn;
	unsigned long	pktid;
	unsigned long	len;
	int		lerrno;

	{
	char		buf[100];
	char		*cp = buf;
	char		ctl;

	/* Encode record type.						*/

	*cp++ = LOGTYPE_TX;

	/* Encode control byte.						*/

	ctl = 0;
	if (tvp != NULL)
		ctl |= LOGCTL_SCHED;
	if (address1 != NULL)
		{
		ctl |= LOGCTL_ADDR;
		if (address2 != NULL)
			ctl |= LOGCTL_2ADDR;
		}
	if (lerrno >= 0)
		ctl |= LOGCTL_EXCEPT;
	*cp++ = ctl;
	
	/* Encode time, if present, otherwise encode current time.	*/

	if (tvp == NULL)
		cp = log_current_time(cp);
	else
		cp = log_sched_time(cp, tvp);

	/* Encode address(es) or asn.					*/

	if (address1 == NULL)
		cp = encode_ulong(cp, (unsigned long) asn);
	else
		{
		cp = (*(prot->prot->addrtob))(address1, cp);
		if (address2 != NULL)
			cp = (*(prot->prot->addrtob))(address2, cp);
		}

	/* Encode packet ID and length.					*/

	cp = encode_ulong(cp, pktid);
	cp = encode_ulong(cp, len);

	/* Encode error number, if one is present.			*/

	if (lerrno >= 0)
		cp = encode_ulong(cp, (unsigned long) lerrno);

	/* Write out the buffer.					*/

	if (fwrite(buf, cp - buf, 1, log_fp) != 1)
		log_write_error();
	else
	{    
	    if (FlushOutput)
		fflush(log_fp);
        }
}

/* Log current time, as offset from on-time.				*/

static char *
log_current_time(buf)

	char		*buf;

	{
	struct timeval	t;

	/* Get the current time, adjust for on-time.			*/

	if (gettimeofday(&t, (struct timezone *)NULL) == -1)
		{
		perror("log_current_time: gettimeofday");
		abort();
		}
	t.tv_sec -= on_time_sec;

	return (log_time(buf, &t));
	}

/* Log scheduled time (as offset from on-time) and offset from current	*/
/* time.								*/

static char *
log_sched_time(buf, st)

	char		*buf;
	struct timeval	*st;

	{
	char		*cp;
	struct timeval	stc;
	struct timeval	t;

	/* Get current time.						*/
	stc = *st;

	if (gettimeofday(&t, (struct timezone *)NULL) == -1)
		{
		perror("log_sched_time: gettimeofday");
		abort();
		}

	/* Get delay between scheduled and actual time, in		*/
	/* microseconds.						*/

	timersub(&t, &stc, &t);
	t.tv_usec += t.tv_sec * 1000000L;

	/* Convert scheduled time to offset from on-time.		*/

	stc.tv_sec -= on_time_sec;

	/* Log the scheduled time.					*/

	cp = log_time(buf, &stc);

	/* Log the delay.						*/

	cp = encode_ulong(cp, (unsigned long)t.tv_usec);
	return (cp);
	}


/* Log specified time.  The time is assumed to be already converted to	*/
/* an offset from on-time.						*/

static char *
log_time(buf, t)

	char		*buf;
	struct timeval	*t;

	{
	char		*cp;

	/* Encode the seconds and microseconds.				*/

	cp = encode_ulong(buf, (unsigned long)t->tv_sec);
	cp = encode_ulong(cp, (unsigned long)t->tv_usec);
	return (cp);
	}


void
log_close() {

	extern FILE	*log_fp;
	
	if (log_fp != stdout &&  log_fp != stderr) 
	  (void) fclose (log_fp);

}
