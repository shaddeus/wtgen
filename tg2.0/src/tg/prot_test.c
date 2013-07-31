/************************************************************************
 *									*
 *	File:  prot_test.c						*
 *									*
 *	Test ``protocol'' that uses keyboard and screen to check out	*
 *	timing and scheduling at a coarse-grained level.		*
 *									*
 *	Written 24-Aug-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/prot_test.c,v 1.1 2001/11/14 00:35:38 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

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
static int		sfd = -1;

/* Accept packets while waiting for the opportunity to write (if wfd	*/
/* specified) or for specified timeout, whichever comes first.		*/

int
test_get_packets(prtab, wfd, endtout)

	protocol_table	*prtab;		/* PRotocol Table pointer.	*/
	int		wfd;		/* FD for write, -1 if none.	*/
	struct timeval	*endtout;	/* end of timeout period.	*/

	{
	static char	*buf = NULL;	/* pointer to receive buffer.	*/
	int		pklen;		/* Input packet length.		*/
	static unsigned long pktid = 0;	/* received packet id.		*/
	fd_set		rfds;		/* Read FDs for select.		*/
	struct timeval	tv;
	struct timeval	*tvp;
	fd_set		wfds;		/* Write FDs for select.	*/

	/* If we are passed a NULL prtab, the caller is trying to tell	*/
	/* us about a new server socket.  But that is too bad, since	*/
	/* we will ignore it in this test protocol.			*/

	if (prtab == NULL)
		{
		return (0);
		}

	/* Calculate initial timeout.  We force a select even if the	*/
	/* timeout has already expired in order to prevent the CPU from	*/
	/* starving the I/O.						*/

	if (endtout == NULL)
		{

		/* No timeout, so just use NULL pointer.		*/

		tvp = NULL;
		}
	else
		{

		/* Calculate timeout.					*/

		if (gettimeofday(&tv, (struct timezone *)NULL) == -1)
			{
			log_error(NULL, NULL, -1, LOGERR_GETTIME);
			perror("test_get_packets: gettimeofday");
			abort();
			}
		if (timercmp(endtout, &tv, >))
			{
			timersub(endtout, &tv, &tv);

			/* Kludge around strange SunOS restrictions.	*/

			if (tv.tv_sec > 3600)
				tv.tv_sec = 3600;
			}
		else
			{
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			}
		tvp = &tv;
		}

	/* Each pass through the following loop does one select call	*/
	/* to check the state of the fds.				*/

	for (;;)
		{

		/* Set up for select:  get fd bitmaps.			*/

		FD_ZERO(&rfds);
		if (sfd >= 0)
			FD_SET(sfd, &rfds);
		FD_ZERO(&wfds);
		if (wfd >= 0)
			{
			FD_SET(wfd, &rfds);
			FD_SET(wfd, &wfds);
			}
		if (select(wfd > sfd ? wfd + 1 : sfd + 1,
			   &rfds,
			   &wfds,
			   (fd_set *)NULL,
			   tvp) < 0)
			{
			if (errno != EINTR)
				{
				log_error(NULL, NULL, -1, LOGERR_SELECT);
				perror("test_get_packets: select");
				abort();
				}
			}

		/* If stdin fd was not selected, ignore it.		*/

		if ((sfd >= 0) &&
		    FD_ISSET(sfd, &rfds))
			{

			/* Each pass through the following loop		*/
			/* attempts to receive one segment.		*/

			for (;;)
				{

				/* Get a buffer if we do not already	*/
				/* have one on hand.			*/

				if ((buf == NULL) &&
				    ((buf =
				      (*(prtab->buffer_get))(MAX_PKT_SIZE)) ==
				     NULL))
					{
					log_error(NULL, NULL, -1, LOGERR_MEM);
					(void)fprintf(stderr,
						      "%s %s\n",
						      "test_get_packets: ",
						      "out of memory!");
					abort();
					}

				/* Receive the segment, scream and die	*/
				/* if error.				*/

				pklen = read(sfd, buf, MAX_PKT_SIZE);
				if ((pklen == 0) ||
				    ((pklen < 0) &
				     (errno != EWOULDBLOCK)))
					{

					/* Shut down and tell rcv if	*/
					/* EOF or error.		*/

					/*&&&&*/if (pklen < 0)
						perror("read");
					sfd = -1;
					log_rx(NULL,
					       NULL,
					       sfd,
					       0,
					       0,
					       pklen == 0 ? -1 : errno);
					if (prtab->rcv != NULL)
						(void)(*(prtab->rcv))(sfd,
								      sfd,
								      NULL,
								      pklen,
								      0);
					break;
					}
				else if (pklen > 0)
					{

					/* Pass packet to receiver.	*/

					log_rx(NULL,
					       NULL,
					       sfd,
					       pktid,
					       pklen,
					       -1);
					if ((prtab->rcv != NULL) &&
					    ((*(prtab->rcv))(sfd,
							     sfd,
							     buf,
							     pklen,
							     pktid++)))
						buf = NULL;
					}
				else
					{

					/* Nothing more to read right	*/
					/* now.				*/

					break;
					}
				}
			}

		/* Check for ability to write...			*/

		if ((wfd > 0) &&
		    (FD_ISSET(wfd, &wfds)))
			return (1);

		/* Calculate next timeout.  If the timeout has expired,	*/
		/* tell the caller the sad story.			*/

		if (endtout != NULL)
			{

			if (gettimeofday(&tv, (struct timezone *)NULL) == -1)
				{
				log_error(NULL, NULL, -1, LOGERR_GETTIME);
				perror("test_get_packets: gettimeofday");
				abort();
				}
			if (timercmp(endtout, &tv, >))
				{
				timersub(endtout, &tv, &tv);

				/* Kludge around strange SunOS		*/
				/* restrictions.			*/

				if (tv.tv_sec > 3600)
					tv.tv_sec = 3600;
				}
			else
				{
				errno = ETIME;
				return (0);
				}
			}

		}

	/* NOTREACHED */
	}

/* Test protocol connection setup function.				*/

long
test_setup(prot)

	protocol	*prot;

	{
	int		flags;

	/* Save pointer to protocol table.				*/

	prtab = prot->prot;

	/* Dump out QOS info. (debug only)				*/

#ifdef NOTDEF
	if ((prot->qos & QOS_BUCKET_RATE) != 0)
		(void)printf("Bucket Rate/Average bandwidth..%g\n", prot->bucket_rate);
	if ((prot->qos & QOS_PEAK_RATE) != 0)
		(void)printf("Peak Rate.....%g\n", prot->peak_rate);
	if ((prot->qos & QOS_BUCKET_DEPTH) != 0)
		(void)printf("Bucket depth......%g\n", prot->bucket_depth);
	if ((prot->qos & (QOS_RESERVED_RATE | QOS_SERVER)) != 0)
		(void)printf("Reserved rate for receiver......%g\n", prot->reserved_rate);
	if ((prot->qos & QOS_AVG_LOSS) != 0)
		(void)printf("Average loss.......%g\n", prot->avg_loss);
	if ((prot->qos & QOS_PEAK_LOSS) != 0)
		(void)printf("Peak loss..........%g\n", prot->peak_loss);

	if ((prot->qos & QOS_SLACK_TERM) != 0)
		(void)printf("Slack term for delay calcuation at receiver......%u\n", 
			     prot->slack_term);
	if ((prot->qos & QOS_MIN_UNIT) != 0)
		(void)printf("Minimum policed unit.........%u\n", prot->min_unit);

	if ((prot->qos & QOS_MAX_UNIT) != 0)
		(void)printf("Maximum policed unit.........%u\n", prot->max_unit);

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

        (void)printf("service model:...........");
        if ((prot->qos & QOS_CONTROLLED_LOAD) != 0)
                (void)printf("Controlled load\n");
        if ((prot->qos & QOS_GUARANTEED_SERVICE) != 0) 
                (void)printf("Guaranteed service\n");
 
	(void)printf("flags:.............");
	if ((prot->qos & QOS_INTERACTIVE) != 0)
		(void)printf(" INTERACTIVE");
	if ((prot->qos & QOS_SERVER) != 0)
		(void)printf(" SERVER");
	(void)printf("\n");

	(void)printf("filters:.............");
        if ((prot->qos & QOS_WILDCARD_FILTER) != 0)
                (void)printf("WF\n");
        if ((prot->qos & QOS_FIXED_FILTER) != 0)
                (void)printf("FF\n");
        if ((prot->qos & QOS_SHARED_EXPLICIT) != 0)
	        (void)printf("SE\n");

#endif /* NOTDEF */

	if (!(prot->qos & QOS_SERVER))
		{

		/* Handle client side.					*/

		return (open("/dev/tty", O_RDWR));
		}
	else
		{

		/* Handle server side.					*/

		sfd = open("/dev/tty", O_RDWR);

		/* Set no-delay mode.					*/

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

		return (sfd);
		}
	}

/* Tear down connection.						*/

int
test_teardown(asn)

	long		asn;

	{
	int		fd = asn;
	int		flags;

	if (fd < 0)
		{

		/* Can't tear it down if it is already torn down!	*/

		errno = EINVAL;
		return (-1);
		}

	/* Set no-delay mode.						*/

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		{
		log_error(NULL, NULL, -1, LOGERR_FCNTL);
		perror("fcntl F_GETFL");
		abort();
		}
	if (fcntl(fd, F_SETFL, flags & ~FNDELAY) == -1)
		{
		(void)close(fd);
		return(-1);
		}
	return (0);
	}

/* Send a packet, subject to the timeout.				*/

int
test_send(asn, buf, len, endtout, pktid)

	long		asn;
	char		*buf;
	int		len;
	struct timeval	*endtout;
	unsigned long	*pktid;

	{
	int		cc;
	int		fd = asn;
	static unsigned long id = 0;
	char		s[30];

	/* Make sure that fd wasn't closed out from under the sender.	*/

	if (fd < 0)
		{
		errno = EINVAL;
		return (-1);
		}

	/* Invoke the get_packets routine to receive packets and accept	*/
	/* new connections while we are waiting to write.		*/

	if (!test_get_packets(prtab, fd, endtout))
		{
		return (-1);
		}
	else
		{

		/* Write out the packet!				*/

		(void)sprintf(s, "%d,", len);
		cc = write(fd, s, strlen(s));
		log_tx(NULL,
		       NULL,
		       NULL,
		       fd,
		       id,
		       len,
		       cc >= 0 ? -1 : errno);
		if (cc >= 0)
			*pktid = id++;
		buffer_generic_free(buf);
		return (cc);
		}
	}

/* Suspend until the (absolute) time specified by waketime, processing	*/
/* any incoming packets or new connections that occur in that interval.	*/

void
test_sleep_till(waketime)

	struct timeval	*waketime;

	{

	(void)test_get_packets(prtab, -1, waketime);
	}
