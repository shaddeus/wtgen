/************************************************************************
 *									*
 *	File:  config.h							*
 *									*
 *	Protocol definition structures.					*
 *									*
 *	Written 08-Aug-90 by Paul E. McKenney, SRI International.	*
 *									*
 ************************************************************************/

#ifndef lint 
static char config_h_rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/config.h,v 1.1 2002/01/24 23:25:19 pingali Exp $";
#endif 

/* Maximum packet buffer size.						*/

#define MAX_PKT_SIZE		8192 /* 3072	/* Sized for Ethernet.	*/

/* Maximum value from random-number generator.				*/

#define MAX_RANDOM		0x7fffffff

/*
 * FD_SET and associated macros are not defined in SunOS 3.5
 * We need to defined them here, if not previously defined.
 */
#ifndef NFDBITS

#define NFDBITS (30)    /* bits per mask */

#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      memset((char *)(p), 0, sizeof (*(p)))
#endif


#if !defined (ETIME)    /* FREEBSD doesn't have ETIME (timer expired error) */
#define ETIME 250       /* pick one.... see errno.h */ 
#endif


