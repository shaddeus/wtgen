/************************************************************************
 *									*
 *	File:  protocol.c						*
 *									*
 *	Contains protocol table and functions that search this table.	*
 *									*
 *	Written 20-Jun-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/protocol.c,v 1.1 2001/11/14 00:35:38 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <math.h>
#include "config.h"
#include "distribution.h" 
#include "protocol.h"

extern int		stream_teardown();
extern int		stream_send();
extern void		stream_sleep_till();

extern long		tcp_setup();

extern int		dgram_teardown();
extern int		dgram_send();
extern void		dgram_sleep_till();

extern long		udp_setup();

extern long		test_setup();
extern int		test_teardown();
extern int		test_send();
extern void		test_sleep_till();


extern char		*buffer_dgram_get();
extern void		buffer_dgram_free();

extern char		*buffer_generic_get();
extern void		buffer_generic_free();

extern int		ipport_atoaddr();
extern int		ipport_addrtoa();
extern char		*ipport_btoaddr();
extern char		*ipport_addrtob();

/* Type definitions local to this file.					*/

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

/* NOTE:  The protocol table must be kept in alphabetical order.	*/

static protocol_table	ptab[] =
				{
					{
					"tcp", AF_INET,
						tcp_setup, stream_teardown,
						NULL, stream_send,
						stream_sleep_till,
						buffer_generic_get,
						buffer_generic_free,
						ipport_atoaddr,
						ipport_addrtoa,
						ipport_btoaddr,
						ipport_addrtob,
					},
					{
					"test", AF_INET,
						test_setup, test_teardown,
						NULL, test_send,
						test_sleep_till,
						buffer_generic_get,
						buffer_generic_free,
						ipport_atoaddr,
						ipport_addrtoa,
						ipport_btoaddr,
						ipport_addrtob,
					},
					{
					"udp", AF_INET,
						udp_setup, dgram_teardown,
						NULL, dgram_send,
						dgram_sleep_till,
						buffer_dgram_get,
						buffer_dgram_free,
						ipport_atoaddr,
						ipport_addrtoa,
						ipport_btoaddr,
						ipport_addrtob,

					},
					{
					NULL, AF_UNSPEC,
						NULL, NULL,
						NULL, NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
					},
				};


/* search ptab for the specified protocol, return a pointer to the	*/
/* entry or NULL if no match.						*/

protocol_table *
find_protocol(name)

	char		*name;

	{
	int		cmp;
	protocol_table	*p;

	for (p = ptab; p->name != NULL; p++)
		{
		cmp = strcmp(name, p->name);
		if (cmp == 0)
			return (p);
		if (cmp < 0)
			return (NULL);
		}

	return (NULL);
	}
