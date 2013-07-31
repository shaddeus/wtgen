/************************************************************************
 *									*
 *	File:  prot_tcp.c						*
 *									*
 *	Routines for interfacing to the TCP protocol suite via the	*
 *	normal user-level interface.					*
 *									*
 *	Written 20-Jun-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/prot_tcp.c,v 1.2 2002/01/24 23:23:39 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include "log.h"
#include "config.h"
#include "distribution.h"
#include "protocol.h"

/* Type definitions local to this file.					*/

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

static protocol_table	*prtab = NULL;	/* pointer to protocol table.	*/
static protocol         *prot = NULL;   /* pointer to protocol struct.  */

/* Apply QOS parameters to an existing fd.				*/

void tcp_qos(fd)
int             fd;
{

    /* Handle QOS parameters, if any are set.                       */

#ifdef QOSDEBUG
	if ((prot->qos & QOS_AVG_BANDWIDTH) != 0)
		(void)printf("Average bandwidth..%g\n", prot->avg_bandwidth);
	if ((prot->qos & QOS_PEAK_BANDWIDTH) != 0)
		(void)printf("Peak bandwidth.....%g\n", prot->peak_bandwidth);
	if ((prot->qos & QOS_AVG_DELAY) != 0)
		(void)printf("Average delay......%g\n", prot->avg_delay);
	if ((prot->qos & QOS_PEAK_DELAY) != 0)
		(void)printf("Peak delay.........%g\n", prot->peak_delay);
	if ((prot->qos & QOS_AVG_LOSS) != 0)
		(void)printf("Average loss.......%g\n", prot->avg_loss);
	if ((prot->qos & QOS_PEAK_LOSS) != 0)
		(void)printf("Peak loss..........%g\n", prot->peak_loss);

	if ((prot->qos & QOS_TOS) != 0)
		(void)printf("IP TOS field.........%u\n", prot->tos);

	if ((prot->qos & QOS_INTERVAL) != 0)
		(void)printf("Interval...........%g\n", prot->interval);
	if ((prot->qos & QOS_MTU) != 0)
		(void)printf("MTU................%u\n", prot->mtu);

        if ((prot->qos & QOS_RCVWIN) != 0)
	        (void)printf("TCP RCVWIN.............%u\n", prot->rcvwin);
        if ((prot->qos & QOS_SNDWIN) !=0)
                (void)printf("TCP SNDWIN..............%u\n", prot->sndwin);

        if ((prot->qos & QOS_TTL) != 0) 
                (void)printf("Multicast TTL...............%u\n", prot->multicast_ttl);

	if ((prot->qos & QOS_INTERACTIVE) != 0)
		(void)printf(" INTERACTIVE");
	if ((prot->qos & QOS_SERVER) != 0)
		(void)printf(" SERVER");

	(void)printf("\n");

#endif /* QOSDEBUG */


    if ((prot->qos & QOS_RCVWIN) != 0)
    {
        /* Set up receive buffer size.                          */
        if (setsockopt(fd,SOL_SOCKET, SO_RCVBUF,(const void *)&(prot->rcvwin),
            sizeof(prot->rcvwin)) == -1)
        {
            perror("setsockopt SO_RCVBUF failed");
            abort();
        }
    }
    if ((prot->qos & QOS_SNDWIN) != 0)
    {
        /* Set up send buffer size.                             */
        if (setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(const void *)&(prot->sndwin),
           sizeof(prot->sndwin)) == -1)
        {
	    perror("setsockopt SO_SNDBUF failed");
	    abort();
	}
	    
     }

} 


/* TCP connection setup function.					*/

long
tcp_setup(proto)

	protocol	*proto;

	{
	int		flags;
	int		sfd;		/* Socket file descriptor.	*/
	struct sockaddr_in *tmpaddr;      

	/* Save pointer to protocol table.				*/

	prtab = proto->prot;

        /* Set up global pointer to protocol structure.                 */
        prot = proto;

	/* Create TCP socket.						*/

	if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		{
		return (-1);
		}

	if (!(prot->qos & QOS_SERVER))
		{

		/* Handle client side.  Connect up a socket and return	*/
		/* it to the caller.					*/

		/* Establish a connection.				*/

		if ((flags = fcntl(sfd, F_GETFL, 0)) == -1)
			{
			log_error(NULL, NULL, sfd, LOGERR_FCNTL);
			perror("fcntl F_GETFL");
			abort();
			}
		if (fcntl(sfd, F_SETFL, flags | FNDELAY) == -1)
			{
			(void)close(sfd);
			return(-1);
			}
		/* handle any QOS parameters */
#ifndef SUNOS4 
               if ((prot->qos & QOS_TOS) != 0)
               /* Set IP tos */
 	              if (setsockopt(sfd, IPPROTO_IP, IP_TOS, (char *)(&(prot->tos)), 
                         sizeof(prot->tos)) == -1)
		            perror("tcp_setup: can't set TOS");
#endif              

		tcp_qos(sfd);

		tmpaddr = (struct sockaddr_in *)&(prot->dst); 

		if  (IN_MULTICAST(tmpaddr->sin_addr.s_addr))
		{
			perror("tcp does not support multicast addresses");
			(void) close(sfd);
			return (-1);
		}	

		if ((connect(sfd, &(prot->dst), sizeof(prot->dst)) < 0) &
		    (errno != EINPROGRESS))
			{
			(void)close(sfd);
			return (-1);
			}

		/* Prevent slow connection-setup from killing us.	*/

		(void)signal(SIGPIPE, SIG_IGN);

		/* Tell the get_packets routine about the new client	*/
		/* socket.						*/

		stream_setup(prot, -sfd - 1,tcp_qos);
		}
	else
		{

		/* Handle server side.  This will need to be modified	*/
		/* to allow the server to accept multiple connections.	*/

		/* Bind socket.						*/

		tmpaddr = (struct sockaddr_in *)(&prot->src);    

		if  (IN_MULTICAST(tmpaddr->sin_addr.s_addr))
		{
			perror("tcp does not support multicast addresses");
			(void) close(sfd);
			return (-1);
		}	

		if (bind(sfd, &(prot->src), sizeof(prot->src)) < 0)
			{
			(void)close(sfd);
			return (-1);
			}

		/* Initialize socket to set no-delay mode.		*/

		if ((flags = fcntl(sfd, F_GETFL, 0)) == -1)
			{
			log_error(NULL, NULL, sfd, LOGERR_FCNTL);
			perror("fcntl F_GETFL");
			abort();
			}
		if (fcntl(sfd, F_SETFL, flags | FNDELAY) == -1)
			{
			log_error(NULL, NULL, sfd, LOGERR_FCNTL);
			perror("fcntl F_SETFL FNDELAY");
			abort();
			}

		/* handle any QOS parameters */
#ifndef SUNOS4
               if ((prot->qos & QOS_TOS) != 0)
               /* Set IP tos */
 	              if (setsockopt(sfd, IPPROTO_IP, IP_TOS, (char *)(&(prot->tos)), 
                         sizeof(prot->tos)) == -1)
		            perror("tcp_setup: can't set TOS");
#endif              

		/* Declare willingness to accept connections.		*/

		(void)listen(sfd, 3);

		/* Tell the get_packets routine about the new server	*/
		/* socket.						*/

		stream_setup(prot, sfd,tcp_qos);
		}

	return (sfd);
	}



