/************************************************************************
 *									*
 *	File:  prot_ipport.c						*
 *									*
 *	Convert ASCII IP address of the form a.b.c.d.port to		*
 *	sockaddr_in.							*
 *									*
 *	Written 04-Sep-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/prot_ipport.c,v 1.1 2001/11/14 00:35:38 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include "config.h"
#include "distribution.h"
#include "protocol.h"

/* Type definitions local to this file.					*/

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/


/* Convert an ascii address to a sockaddr.				*/

int
ipport_atoaddr(addr, s)

	char		*addr;
	struct sockaddr	*s;

	{
	struct sockaddr_in *sin = (struct sockaddr_in *)s;
	unsigned int	a;
	unsigned int	b;
	unsigned int	c;
	unsigned int	d;

	memset((char *)s, 0, sizeof(*s));
	sin->sin_family = AF_INET;
	if (sscanf(addr, "%u.%u.%u.%u.%hu",
		   &a, &b, &c, &d, &(sin->sin_port)) != 5)
		return (0);
	if ((a > 255) ||
	    (b > 255) ||
	    (c > 255) ||
	    (d > 255))
		return (0);
	sin->sin_addr.s_addr = (a << 24) | (b << 16) | (c << 8) | d;

	sin->sin_addr.s_addr = htonl(sin->sin_addr.s_addr);;
        sin->sin_port = htons(sin->sin_port); 				     
	return (1);
	}

/* Convert a sockaddr structure to an ascii address.			*/

int
ipport_addrtoa(s, addr)

	struct sockaddr	*s;
	char		*addr;


	{
	struct sockaddr_in *sin = (struct sockaddr_in *)s;
	unsigned int	a;
	unsigned int	b;
	unsigned int	c;
	unsigned int	d;
	unsigned long	ipaddr;
        unsigned short  port;     

	ipaddr = ntohl(sin->sin_addr.s_addr);
        port = ntohs(sin->sin_port);

	d = ipaddr & 0xff;
	c = (ipaddr >>= 8) & 0xff;
	b = (ipaddr >>= 8) & 0xff;
	a = (ipaddr >>= 8) & 0xff;
	(void)sprintf(addr, "%u.%u.%u.%u.%u", a, b, c, d, port);
	return (1);
	}

/* Convert a binary log address to a sockaddr.				*/

char *
ipport_btoaddr(addr, s)

	char		*addr;
	struct sockaddr	*s;

	{
	struct sockaddr_in *sin = (struct sockaddr_in *)s;
	unsigned long      ipaddr;
	unsigned short     port;

	sin->sin_family = AF_INET;

#ifdef OLD
	if (1) {
	  static int count = 5; 
	  if (count > 0 ) { 
	    printf("decoding addr[0] = %x \n", *addr); 
	    printf("Encoding addr[1] = %x \n", *(addr + 1)); 
	    count --; 
	   }
	}
	(void)memcpy((char *)&(port), addr, sizeof(port));
	(void)memcpy((char *)&(ipaddr),
		     (char *)&(addr[sizeof(sin->sin_port)]),
		      sizeof(ipaddr));
#else 
 	port    = (*(addr +0) & 0xff) << 8; 
 	port   |= (*(addr +1) & 0xff) << 0; 
 	ipaddr  = (*(addr +2) & 0xff) << 24; 
 	ipaddr |= (*(addr +3) & 0xff) << 16; 
 	ipaddr |= (*(addr +4) & 0xff) << 8; 
 	ipaddr |= (*(addr +5) & 0xff) << 0; 
#endif
	sin->sin_port = ntohs(port);
	sin->sin_addr.s_addr = ntohl(ipaddr); 


	return (&(addr[sizeof(port) + sizeof(ipaddr)]));
	}

/* Convert a sockaddr binary log address.				*/

char *
ipport_addrtob(s, addr)

	struct sockaddr	*s;
	char		*addr;

	{
	struct sockaddr_in *sin = (struct sockaddr_in *)s;
	unsigned short     port;
	unsigned long      ipaddr;

        port = htons(sin->sin_port);
        ipaddr = htonl(sin->sin_addr.s_addr); 


#ifdef OLD
	if (1) {
	  static int count = 5; 
	  if (count > 0 ) { 
	    printf("Encoding addr[0] = %x \n", *(char *)&port); 
	    printf("Encoding addr[1] = %x \n", *((char *)&port + 1)); 
	    count --; 
	   }
	}
	(void)memcpy(addr, (char *)&(port),  sizeof(port));
	(void)memcpy((char *)&(addr[sizeof(sin->sin_port)]),
		     (char *)&(ipaddr),
		      sizeof(ipaddr));
#else 
 	*(addr + 0)  = (unsigned char)((port   >>   8) & 0xff) ; 
 	*(addr + 1)  = (unsigned char)((port   >>   0) & 0xff); 
 	*(addr + 2)  = (unsigned char)((ipaddr >>  24) & 0xff) ; 
 	*(addr + 3)  = (unsigned char)((ipaddr >>  16) & 0xff) ; 
 	*(addr + 4)  = (unsigned char)((ipaddr >>   8) & 0xff) ; 
 	*(addr + 5)  = (unsigned char)((ipaddr >>   0) & 0xff) ; 
#endif

	return (&(addr[sizeof(port) + sizeof(ipaddr)]));
	}



