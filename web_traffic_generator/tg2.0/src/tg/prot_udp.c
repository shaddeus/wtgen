/************************************************************************
 *									*
 *	File:  prot_udp.c						*
 *									*
 *	Routines for interfacing to the UDP protocol suite via the	*
 *	normal user-level interface.					*
 *									*
 *	Written 04-Sep-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/prot_udp.c,v 1.2 2002/01/24 23:23:39 pingali Exp $";
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

/* Apply QOS parameters to an existing fd.                              */

static void udp_qos(fd, prot)
int             fd;
protocol       *prot;
{
    /* Handle QOS parameters here if UDP ever gets any.             */
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

#endif /* QOSDEBUG  */

#ifndef SUNOS4
	if ((prot->qos & QOS_TOS) != 0)
		if (setsockopt(fd, IPPROTO_IP, IP_TOS, (char *)(&(prot->tos)), 
                    sizeof(prot->tos)) == -1)
		    perror("udp_qos: can't set TOS");
#endif              

	if ((prot->qos & QOS_RCVWIN) != 0)
		{
		/* Set up receive socket buffer size.			*/
		if (setsockopt(fd,SOL_SOCKET, SO_RCVBUF,(const void *)&(prot->rcvwin),
			       sizeof(prot->rcvwin)) == -1)
			{
			perror("setsockopt SO_RCVBUF failed");
			abort();
			}
		}
	if ((prot->qos & QOS_SNDWIN) != 0)
		{
		/* Set up send socket buffer size.			*/
		if (setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(const void *)&(prot->sndwin),
			       sizeof(prot->sndwin)) == -1)
			{
			perror("setsockopt SO_SNDBUF failed");
			abort();
			}
		}
    
}



/* UDP  connection setup function.					*/

long
udp_setup(prot)

	protocol	*prot;

	{
	int		flags;
	int		sfd;		/* Socket file descriptor.	*/
	struct sockaddr_in *addr;
	unsigned char   ttl;		/* multicast ttl */ 
        struct sockaddr_in multicast_localaddr; 	


	/* Save pointer to protocol table.				*/

	prtab = prot->prot;

	/* Create UDP socket.						*/

	if ((sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
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
			log_error(NULL, NULL, -1, LOGERR_FCNTL);
			perror("fcntl F_GETFL");
			abort();
			}
		if (fcntl(sfd, F_SETFL, flags | FNDELAY) == -1)
			{
			(void)close(sfd);
			return(-1);
			}
                /* Handle any QOS */
		udp_qos(sfd, prot);
               
                   
		addr = (struct sockaddr_in *) &(prot->dst);

		if ( ( IN_MULTICAST(addr->sin_addr.s_addr) ) && (prot->qos & QOS_TTL))
    	        {
		    
#ifdef IP_MULTICAST_TTL
		    ttl = (unsigned char)prot->multicast_ttl;
		    if (setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_TTL, &(ttl),
			       sizeof(ttl)) < 0)
		    {
			perror("multicast ttl failure");
			(void) close(sfd);
			return (-1);
		     }	

#else
		     perror("multicast ttl not supported");
		     (void) close(sfd);
		     return (-1);
#endif

		}    

		if (prot->qos & QOS_SRC)
			{
			if (bind(sfd, &(prot->src), sizeof(prot->src)) < 0)
				{
				(void)close(sfd);
				return (-1);
				}
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

		dgram_setup(prot, -1);
		}
	else
		{

		/* Handle server side.  This will need to be modified	*/
		/* to allow the server to accept multiple connections.	*/





		    /* don't think this is correect */
		addr = (struct sockaddr_in *)&(prot->src);
		if ( IN_MULTICAST(ntohl(addr->sin_addr.s_addr)))		
	        {
#ifdef IP_MULTICAST_TTL 
		    struct ip_mreq mreq;

 		    /* join the group */
		    mreq.imr_multiaddr.s_addr = addr->sin_addr.s_addr;
		    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		    if (setsockopt(sfd, IPPROTO_IP,
			   IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0 )
		    {
			perror("membership failed");
			(void)close(sfd);
			return (-1);
                    }

		    /* multicast uses INADDR_ANY for binding to local port */

		     multicast_localaddr.sin_family = addr->sin_family;
		     multicast_localaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
		     multicast_localaddr.sin_port = addr->sin_port;
	             if ( bind(sfd, (struct sockaddr *)&(multicast_localaddr), sizeof(multicast_localaddr) ) < 0)
		     {
			 perror("multicast bind failed");
			 (void)close(sfd);
			 return (-1);
	 	     }

#else
		    perror("multicast join not supported");
		    (void) close(sfd);
		    return (-1);
#endif
		 }   
		 else
		 {    
		        if (bind(sfd, &(prot->src), sizeof(prot->src)) < 0)
			{
			    (void)close(sfd);
			    return (-1);
			}
		 }

		/* Initialize socket to set no-delay mode.		*/

		if ((flags = fcntl(sfd, F_GETFL, 0)) == -1)
			{
			log_error(NULL, NULL, -1, LOGERR_FCNTL);
			perror("fcntl F_GETFL");
			abort();
			}

		if (fcntl(sfd, F_SETFL, flags | FNDELAY) == -1)
			{
			log_error(NULL, NULL, -1, LOGERR_FCNTL);
			perror("fcntl F_SETFL FNDELAY");
			abort();
			}

                /* Handle any QOS */
		udp_qos(sfd, prot);


		/* Declare willingness to accept connections.		*/

		(void)listen(sfd, 3);

		/* Tell the get_packets routine about the new server	*/
		/* socket.						*/

		dgram_setup(prot, sfd);
		}


	/* Pass the association ID back to the caller.			*/

	return (sfd);
	}

