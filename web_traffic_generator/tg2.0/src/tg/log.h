/************************************************************************
 *									*
 *	File:  log.h							*
 *									*
 *	Header file for handling log files				*
 *									*
 *	Written 12-Sep-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char log_h_rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/log.h,v 1.1 2002/01/24 23:25:19 pingali Exp $";
#endif 

#define LOG_VERSION             2
#define LOG_SUBVERSION          0
 
/* 
 * A log file entry consists of a tuple consisting of the following fields:
 *
 *	<Record type> <Record control> <Record value>
 */

/* Record type field enumerations */

#define LOGTYPE_RX		1
#define LOGTYPE_TX		2
#define LOGTYPE_SETUP		3
#define LOGTYPE_TEARDOWN	4
#define LOGTYPE_ACCEPT		5
#define LOGTYPE_ERROR		6

/* Control field modifier bit definitions */

#define	LOGCTL_SCHED		(0x1<<0)
#define	LOGCTL_ADDR		(0x1<<1)
#define	LOGCTL_2ADDR		(0x1<<2)
#define	LOGCTL_EXCEPT		(0x1<<3)

/* Error codes when record type is set to LOGTYPE_ERROR */

#define LOGERR_INTFMT		1	/* Script format error */
#define LOGERR_MEM		2	/* Out of memory */
#define LOGERR_2SETUP		3	/* Two connections were established */
#define LOGERR_GETTIME		4	/* gettimeofday() failed. */
#define LOGERR_SELECT		5	/* select() failed. */
#define LOGERR_FCNTL		6	/* fcntl() failed. */
#define LOGERR_GETPEER		7	/* getpeername() failed. */

#define BEGIN_HDR_STRING	"<Begin TG Header>\n"
#define END_HDR_STRING		"<End TG Header>\n"

/* The following routines are exported */

FILE	*log_open();
int	log_init();
void	log_tx();
void	log_rx();
void	log_accept();
void	log_setup();
void	log_teardown();
void	log_error();

