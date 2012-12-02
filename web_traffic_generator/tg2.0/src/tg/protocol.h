/************************************************************************
 *									*
 *	File:  protocol.h						*
 *									*
 *	Protocol definition structures.					*
 *									*
 *	Written 20-Jun-90 by Paul E. McKenney, SRI International.	*
 *									*
 ************************************************************************/

#ifndef lint 
static char protocol_h_rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/protocol.h,v 1.1 2002/01/24 23:25:19 pingali Exp $";
#endif 

/* Convert the double d to timer tvp.					*/

#define dtotimeval(d, tvp) \
	{ \
	(tvp)->tv_sec = floor(d); \
	(tvp)->tv_usec = ((d) - (tvp)->tv_sec) * 1000000; \
	}

/* Convert the double d to timer tvp, offsetting from current time.	*/

#define dtotimevalfromnow(d, tvp) \
	{ \
	unsigned long	seconds; \
	\
	if (gettimeofday(tvp, (struct timezone *)NULL) == -1) \
		{ \
		(void)perror("gettimeofday"); \
		abort(); \
		}\
	seconds = floor(d); \
	(tvp)->tv_sec += seconds; \
	(tvp)->tv_usec += ((d) - seconds) * 1000000; \
	if ((tvp)->tv_usec >= 1000000) \
		{ \
		(tvp)->tv_usec -= 1000000; \
		(tvp)->tv_sec++; \
		} \
	}

/* Convert the double d to timer tvp, offsetting from specified time.	*/

#define dtotimevalfromthen(then, d, tvp) \
	{ \
	unsigned long	seconds; \
	\
	seconds = floor(d); \
	(tvp)->tv_sec = (then)->tv_sec + seconds; \
	(tvp)->tv_usec = (then)->tv_usec + ((d) - seconds) * 1000000; \
	if ((tvp)->tv_usec >= 1000000) \
		{ \
		(tvp)->tv_usec -= 1000000; \
		(tvp)->tv_sec++; \
		} \
	}

/* Add tvp1 and tvp2, putting the result into result.  result may	*/
/* alias either tvp1 or tvp2 or both, if desired.			*/

#ifndef timeradd 
#define timeradd(tvp1, tvp2, result) \
	{ \
	(result)->tv_sec = (tvp1)->tv_sec + (tvp2)->tv_sec; \
	(result)->tv_usec = (tvp1)->tv_usec + (tvp2)->tv_usec; \
	if ((result)->tv_usec >= 1000000) \
		{ \
		(result)->tv_usec -= 1000000; \
		(result)->tv_sec++; \
		} \
	}
#endif

/* Subtract tvp2 from tvp1, putting the result into result.  result	*/
/* may alias either tvp1 or tvp2, if desired.				*/

#ifndef timersub 
#define timersub(tvp1, tvp2, result) \
	{ \
	(result)->tv_sec = (tvp1)->tv_sec - (tvp2)->tv_sec; \
	if ((tvp1)->tv_usec >= (tvp2)->tv_usec) \
		(result)->tv_usec = (tvp1)->tv_usec - (tvp2)->tv_usec; \
	else \
		{ \
		(result)->tv_usec = 1000000 + \
				    (tvp1)->tv_usec - \
				    (tvp2)->tv_usec; \
		(result)->tv_sec--; \
		} \
	}
#endif

/* Type definitions local to this file.					*/

typedef struct tg_action_		/* TG action structure.		*/
	{
	struct tg_action_ *next;
	int		tg_flags;	/* TG flags.			*/
	struct timeval	start_at;	/* time to begin this action.	*/
	struct timeval	stop_before;	/* time to be done w/ action.	*/
	distribution	arrival;	/* interarrival time.		*/
	long		data_limit;	/* max amt of data to send.	*/
	long		packet_limit;	/* max amt of data to send.	*/
	distribution	length;		/* packet length.		*/
	struct timeval	patience;	/* patience duration.		*/
	distribution	resplen;	/* response length distribution.*/
	long		seed;		/* new RNG seed.		*/
	struct timeval	time_limit;	/* max amt of time to be sending*/
        char            *log;           /* template for the log         */ 
	}		tg_action;

/* tg_flags definitions.						*/

#define TG_ARRIVAL	0x0001		/* Got arrival distribution.	*/
#define TG_DATA		0x0002		/* Got data send limit.		*/
#define TG_LENGTH	0x0004		/* Got packet length distr.	*/
#define TG_PATIENCE	0x0008		/* Got patience duration.	*/
#define TG_RESPLEN	0x0010		/* Got response length.		*/
#define TG_SEED		0x0020		/* Got RNG seed.		*/
#define TG_SETUP	0x0040		/* Got setup command.		*/
#define TG_START	0x0080		/* Got explicit start time.	*/
#define TG_STOP		0x0100		/* Got explicit stop time.	*/
#define TG_TIME		0x0200		/* Got time limit.		*/
#define TG_WAIT		0x0400		/* Got wait command.		*/
#define TG_PACKET	0x0800		/* Got packet limit.		*/
#define TG_RESET	0x1000		/* Got reset command.		*/
#define TG_LOG          0x2000          /* Got log command              */

/* Protocol switch table definition.					*/

typedef struct				/* Protocol table entry.	*/
	{
	char		*name;		/* name of protocol.		*/
	short		af;		/* Address family.		*/
	long		(*setup)();	/* connection setup function.	*/
					/*	protocol *		*/
					/* returns -1 if cannot setup.	*/
	int		(*teardown)();	/* connection teardown function.*/
					/*	unsigned long cxn	*/
					/* returns -1 if cannot teardown*/
	int		(*rcv)();	/* to receive incoming pkts.	*/
					/* TG-SUPPLIED!!!		*/
					/*	unsigned long rx	*/
					/*	unsigned long tx	*/
					/*	char *buf		*/
					/*	int len			*/
					/*	unsigned long pktid	*/
					/* The routine should return 1	*/
					/* if it has disposed of the	*/
					/* buffer (e.g., by modifying	*/
					/* it and passing it to send),	*/
					/* otherwise it should return 0.*/
	int		(*send)();	/* to send out a packet.	*/
					/* 	unsigned long tx	*/
					/*	char *buf		*/
					/*	char *len		*/
					/*	struct timeval *endtout	*/
					/*	unsigned long *pktid	*/
					/* NULL endtout says to wait	*/
					/* forever, if necessary.	*/
					/* Returns the number of bytes	*/
					/* actually sent, which will	*/
					/* normally be equal to len.	*/
					/* Returns -1 if an error	*/
					/* occurs (such as a timeout)	*/
					/* and sets errno appropriately.*/
	void		(*sleep_till)(); /* Suspends until the specified*/
					/* time, processing any packets	*/
					/* that arrive in the interim.	*/
					/* 	struct timeval *waketime*/
	char		*(*buffer_get)(); /* request buffer to be used	*/
					/* to compose packets to be	*/
					/* output.			*/
					/* This allows protocols to	*/
					/* avoid packet copying.	*/
					/*	unsigned long maxlen	*/
					/* Return NULL if no more bufs.	*/
	void		(*buffer_free)(); /* return buffer to freelist.	*/
					/* 	char *buf		*/
	int		(*atoaddr)();	/* parse an ascii string into	*/
					/* a sockaddr structure, return	*/
					/* false if unsuccessful.	*/
					/*	char *addr		*/
					/*	struct sockaddr *s	*/
	int		(*addrtoa)();	/* format a sockaddr structure	*/
					/* into an ascii string, return	*/
					/* false if unsuccessful.	*/
					/*	struct sockaddr *s	*/
					/*	char *addr		*/
	char		*(*btoaddr)();	/* parse a binary log address	*/
					/* a sockaddr structure, return	*/
					/* pointer to first byte of	*/
					/* addr that was not consumed.	*/
					/*	char *addr		*/
					/*	struct sockaddr *s	*/
	char		*(*addrtob)();	/* format a sockaddr structure	*/
					/* into an binary log address,	*/
					/* return pointer to first byte	*/
					/* of addr that was not		*/
					/* overwritten.			*/
					/*	struct sockaddr *s	*/
					/*	char *addr		*/
	}		protocol_table;

extern protocol_table *find_protocol();

/* Protocol connection definition struct.				*/

typedef struct				/* protocol defn structure.	*/
	{
	int		qos;		/* Quality of Service flags.	*/
	int		multicast_ttl;
	long		rcvwin;
	long		sndwin;
	struct sockaddr	src;
	struct sockaddr	dst;
	double		avg_bandwidth;
	double		peak_bandwidth;
	double		avg_delay;
	double		peak_delay;
	double		avg_loss;
	double		peak_loss;
	unsigned int    tos;
	double		interval;       /* used by stII */
	unsigned long	mtu;            /* used by stII */
	protocol_table	*prot;		/* pointer to protocol table	*/
					/* entry.			*/
	}		protocol;

/* Quality of service definitions.					*/

/* NEED TO ADD UNITS TO COMMENTS */

#define QOS_AVG_BANDWIDTH 0x0001	/* Got average bandwidth.	*/
#define QOS_PEAK_BANDWIDTH 0x0002	/* Got peak bandwidth.		*/
#define QOS_AVG_DELAY	0x0004		/* Got average delay.		*/
#define QOS_PEAK_DELAY	0x0008		/* Got peak delay.		*/
#define QOS_AVG_LOSS	0x0010		/* Got average loss rate.	*/
#define QOS_PEAK_LOSS	0x0020		/* Got peak loss rate.		*/
#define QOS_INTERVAL	0x0040		/* Got averaging interval.	*/
#define QOS_MTU		0x0080		/* Got max transmission unit.	*/


#define QOS_RCVWIN      0x0400		/* Got receive window size.     */
#define QOS_SNDWIN      0x0800          /* Got send window size.        */

#define QOS_TTL         0x4000          /* Got a multicast TTL          */

#define QOS_TOS         0x10000         /* Got IP TOS/DS BYTE field     */


#define QOS_INTERACTIVE	0x1000000       /* Simulate interactive session.*/
#define QOS_SRC		0x2000000       /* Got source address.		*/
#define QOS_DST		0x4000000       /* Got destination address.	*/
#define QOS_SERVER	0x8000000       /* Set up server socket to	*/
				        /* accept incoming connections.	*/
