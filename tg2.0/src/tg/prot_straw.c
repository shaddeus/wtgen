/************************************************************************
 *									*
 *	File:  prot_straw.c						*
 *									*
 *	Routines for interfacing to the raw ST-II protocol suite via	*
 *	the normal user-level interface.				*
 *									*
 *	Modified 11-Jun-91 by C.Lynn, BBN.				*
 *	Copyright (c) 1991 by  BBN Systems and Technologies,		*
 *		A Division of Bolt Beranek and Newman Inc.		*
 *	Written 20-Jun-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/prot_straw.c,v 1.1 2002/01/24 23:23:39 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h> 
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "log.h"
#include "distribution.h"
#include "protocol.h"
#include <sys/uio.h>		/* iov for struct msg */
#include "st2_api.h"

/* Type definitions local to this file.					*/

#define SAPLEN 2
#define Bytesof(x) Bytes2(x)		/* !@#$ cpp lacks an "eval" */
#define DEFAULT_PROTOCOL 255
#define DEFAULT_PORT 1234

int	st2_options	= STOptLBit | STTSRYes;	/* Ought to fix parser ... */

Instsockaddr_st2(local,InstaApplEntity(here,SAPLEN,/*no srcrut*/);) = {
    Initsockaddr_st2(STReqUnspec,0/*opt_xxx*/),
    InitaApplEntity(STLclAppEnt,local.here,
	INADDR_ANY,DEFAULT_PROTOCOL,SAPLEN,Bytesof(DEFAULT_PORT),/*no srcrut*/)
};

Instsockaddr_st2(remote,
	InstaFlowSpec3(tos);
	InstaTargetList(targlst,
	    InstaTarget(targ1,SAPLEN,);
	);
) = {
    Initsockaddr_st2(STReqUnspec,0/*opt_xxx*/),
    InitaFlowSpec3(STpFlowSpec,
		   /*min*/  128/*bytes*/,  2/*pps*/, 128*2/*bandwidth*/,
		   /*des*/ 1024/*bytes*/, 10/*pps*/),
    InitaTargetList(remote.targlst,1/*targets*/,
	InitaTarget(remote.targlst.targ1,
		INADDR_ANY,SAPLEN,0,)
    )
};


/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

static protocol_table	*prtab = NULL;	/* pointer to protocol table.	*/

static Instsockaddr_st2(from,
	unsigned char parm0[(MAX_ST_NAM-sizeofsockaddr_st2header)/4];);
					/* Socket structure client	*/
static struct msghdr	 msg;		/* For recvmsg/sendmsg ... */
static struct iovec	 ctliov;	/* For recvfrom data descriptor */
static char		 ctlnodata[8];	/* Broken recvmsg! */
static union {				/* First component of msg */
    struct cmsghdr	 cm;
    char		 buf[ MAX_ST_CTL ]; /* For control info from ST-II */
}			 ctl;
static struct cmsghdr	*ctlp = &( ctl.cm ); /* Contents of ctlbuf */

/* ST-II stream setup function.						*/

  long
straw_setup( protp )			/* from tg.y: do_actions */
   protocol		*protp;
{
    int			 flags,
			 sfd,		/* Socket file descriptor.	*/
			 len;


    /* Save pointer to protocol table.					*/

    prtab = protp->prot;

    /* Create ST-II socket.						*/

    if ( (sfd = socket(AF_COIP, SOCK_RAW, 0)) == -1 )
    {
	return (-1);
    }

    if ( ! (protp->qos & QOS_SERVER) )
    {
	unsigned long	 bytes,
			 ratex10,
			 bandwidthx10;


	/* Handle client side.  Connect up a socket and return		*/
	/* it to the caller.						*/

	if ( (protp->qos & QOS_MTU) != 0 )
	{
	    bytes = protp->mtu;
	}
	else
	{
	    printf( "ST-II: packet size not specified (mtu)\n" );
	    return (-1);
	}

	if ( (protp->qos & QOS_INTERVAL) != 0 )
	{
	    ratex10 = 10.0 / protp->interval;
	    if ( ratex10 == 0 )
	    {
		printf( "ST-II: specified interval is less than 0.1 pps, 0.1 pps assumed\n" );
		ratex10 = 1;
	    }
	}
	else
	{
	    printf( "ST-II: packet interval not specified (interval)\n" );
	    return (-1);
	}

	if ( (protp->qos & QOS_PEAK_BANDWIDTH) != 0 )
	{
	    bandwidthx10 = 10 * protp->peak_bandwidth;
	}
	else if ( (protp->qos & QOS_AVG_BANDWIDTH) != 0 )
	{
	    bandwidthx10 = 10 * protp->avg_bandwidth;
	}
	else
	{
	    bandwidthx10 = bytes * ratex10;
	}

	/* Establish a connection.					*/

	local.here.IPAdr = * (unsigned long *) &( protp->src.sa_data[2] );
	local.here.NextPcol = DEFAULT_PROTOCOL;
	local.here.SAP[0] = protp->src.sa_data[0];
	local.here.SAP[1] = protp->src.sa_data[1];

	/* Bind socket.							*/

	if ( bind( sfd, &( local ), sizeof (local) ) == -1 )
	{
	    (void) close( sfd );
	    return (-1);
	}

	remote.st_options = st2_options;

	remote.targlst.targ1.IPAdr = * (unsigned long *) &( protp->dst.sa_data[2] );
	remote.targlst.targ1.SAP[0] = protp->dst.sa_data[0];
	remote.targlst.targ1.SAP[1] = protp->dst.sa_data[1];
	remote.targlst.targ1.SAPBytes = 2;

	remote.tos.LimitOnPDUBytes = bytes;
	remote.tos.DesPDUBytes     = bytes;
	remote.tos.LimitOnPDURate  = ratex10;
	remote.tos.DesPDURate      = ratex10;
	remote.tos.MinBytesXRate   = bandwidthx10;

	if (   (connect( sfd, (struct sockaddr *) &( remote ),
		       sizeof (remote) ) == -1 )
	    && (errno != EINPROGRESS) )
	{
	    (void) close( sfd );
	    return (-1);
	}

	/* Prevent slow connection-setup from killing us.		*/

	(void) signal( SIGPIPE, SIG_IGN );

	ctliov.iov_base = (caddr_t) ctlnodata;
	ctliov.iov_len = sizeof (ctlnodata);
	InitMsg(&(msg),&( from ),sizeof (from), &ctliov,1,
		&( ctl.cm ),sizeof (ctl));
	len = recvmsg( sfd, &msg, 0/*flags*/ );
	if ( len == -1 )
	{
	    log_error(NULL, NULL, sfd, 17);
	    perror( "st2_get_packets: recvmsg Accept" );
	    abort();
	}

	if ( from.st_request != STReqAccept )
	{
	    (void) close( sfd );
	    return ( -1 );
	}

	if ( (flags = fcntl( sfd, F_GETFL, 0 )) == -1 )
	{
	    log_error( NULL, NULL, sfd, LOGERR_FCNTL );
	    perror( "fcntl F_GETFL" );
	    abort();
	}
	if (fcntl( sfd, F_SETFL, flags | FNDELAY ) == -1 )
	{
	    (void) close( sfd );
	    return (-1);
	}

	/* Tell the get_packets routine about the new client socket.	*/

	st2_setup( protp, -sfd - 1 );
    }
    else
    {

	/* Handle server side.  This will need to be modified to allow	*/
	/* the server to accept multiple connections.			*/

	local.here.IPAdr = * (unsigned long *) &( protp->src.sa_data[2] );
	local.here.NextPcol = DEFAULT_PROTOCOL;
	local.here.SAP[0] = protp->src.sa_data[0];
	local.here.SAP[1] = protp->src.sa_data[1];

	/* Bind socket.							*/

	if ( bind( sfd, &( local ), sizeof (local) ) == -1 )
	{
	    (void) close( sfd );
	    return (-1);
	}

	/* Initialize socket to set no-delay mode.			*/

	if ( (flags = fcntl( sfd, F_GETFL, 0 )) == -1 )
	{
	    log_error( NULL, NULL, sfd, LOGERR_FCNTL );
	    perror( "fcntl F_GETFL" );
	    abort();
	}

	if ( fcntl( sfd, F_SETFL, flags | FNDELAY ) == -1 )
	{
	    log_error( NULL, NULL, sfd, LOGERR_FCNTL );
	    perror( "fcntl F_SETFL FNDELAY" );
	    abort();
	}

	/* Declare willingness to accept connections.			*/

	(void) listen( sfd, 3 );

	/* Tell the get_packets routine about the new server socket.	*/

	st2_setup( protp, sfd );
    }

    return (sfd);
}

/*
 Local Variables:
 c-indent-level: 4
 c-brace-offset: -4
 c-brace-imaginary-offset: 0
 c-continued-statement-offset: 4
 comment-column: 40
 End:
*/
