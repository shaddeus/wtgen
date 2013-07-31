/************************************************************************
 *									*
 *	File:  buffer_generic.c						*
 *									*
 *	Routines to manage a generic buffer pool.			*
 *									*
 *	Written 08-Aug-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/buffer_generic.c,v 1.1 2001/11/14 00:35:38 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#ifdef FREEBSD
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "config.h"

/* Type definitions local to this file.					*/

typedef struct generic_freelist_
	{
	struct generic_freelist_ *next;
	}		generic_freelist;

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

static generic_freelist	*generic_flist = NULL; /* generic bfr freelist.	*/

/* Get a buffer.  This does nothing fancy, a more sophisticated version	*/
/* might be able to avoid some packet copies.				*/

/* ARGSUSED */
char *
buffer_generic_get(maxlen)

	unsigned long	maxlen;

	{
	char		*buf;

	if (generic_flist == NULL)
		buf = (char *)malloc(MAX_PKT_SIZE);
	else
		{
		buf = (char *)generic_flist;
		generic_flist = generic_flist->next;
		}
	return (buf);
	}

/* Free up a buffer.							*/

void
buffer_generic_free(buf)

	char		*buf;

	{
	generic_freelist	*fp = (generic_freelist *)buf;

	fp->next = generic_flist;
	generic_flist = fp;
	}





