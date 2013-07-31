/************************************************************************
 *									*
 *	File:  prot_dgram.c						*
 *									*
 *	Common routines for interfacing to datagram socket protocols	*
 *	suite via the normal user-level interface.			*
 *									*
 *	Written 17-Aug-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

/*
 *	$Log: prot_dgram.c,v $
 *	Revision 1.2  2002/01/24 23:23:39  pingali
 *	*** empty log message ***
 *
 *	Revision 1.1  2001/11/14 00:35:38  pingali
 *	Initial revision
 *
 * Revision 1.4  90/10/04  19:42:11  mckenney
 * Paul's final revision, prior to his departure.
 * 
 * Revision 1.3  90/09/18  20:23:14  mckenney
 * ckpt
 * 
 * Revision 1.2  90/09/04  11:27:46  mckenney
 * Checkpoint
 * 
 * Revision 1.1  90/08/26  23:41:06  mckenney
 * .
 * 
 */

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
#include "config.h"
#include "log.h"
#include "distribution.h"
#include "protocol.h"

/* Type definitions local to this file.					*/

typedef struct dgram_freelist_
	{
	struct dgram_freelist_ *next;
	}		dgram_freelist;

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

static dgram_freelist	*dgram_flist = NULL; /* dgram bfr freelist.	*/
static fd_set		fds;		/* FDS to clients.		*/
static int		firsttime = 1;	/* flag for get_packets.	*/
static struct sockaddr_in
			from;		/* source of last datagram.	*/
static int		fromlen = 0;	/* length of last dg's address.	*/
static int		justrcvd = 0;	/* rcv since last send?		*/
static protocol		*prot = NULL;	/* protocol attr. pointer.	*/
static protocol_table	*prtab = NULL;	/* PRotocol Table pointer.	*/
static int		rcving = 0;	/* currently receiving?		*/
static int		sfd = -1;	/* Socket file descriptor.	*/

/* Get a buffer.  This does nothing fancy, a more sophisticated version	*/
/* might be able to avoid some packet copies.  Leaves space for the	*/
/* packet ID at the front of the buffer.				*/

/* ARGSUSED */
char *
buffer_dgram_get(maxlen)

	unsigned long	maxlen;

	{
	char		*buf;

	if (dgram_flist == NULL)
		buf = (char *)malloc(MAX_PKT_SIZE);
	else
		{
		buf = (char *)dgram_flist;
		dgram_flist = dgram_flist->next;
		}
	return (buf + sizeof(unsigned long));
	}

/* Free up a buffer.							*/

void
buffer_dgram_free(buf)

	char		*buf;

	{
	dgram_freelist	*fp;

	fp = (dgram_freelist *)(buf - sizeof(unsigned long));
	fp->next = dgram_flist;
	dgram_flist = fp;
	}

/* Accept connections and packets while waiting for the opportunity to	*/
/* write (if wfd specified) or for specified timeout, whichever comes	*/
/* first.  A wfd of -2 means to send to whereever the previous datagram	*/
/* came from.								*/

int
dgram_get_packets(wfd, endtout)

	int		wfd;		/* FD for write, -1 if none.	*/
	struct timeval	*endtout;	/* end of timeout period.	*/

	{
	static char	*buf = NULL;	/* pointer to receive buffer.	*/
	int		fd;
	int		fdmax;
	struct sockaddr_in from;	/* Socket structure client	*/
	int		nitems;		/* Number of items selected.	*/
	int		pklen;		/* Input packet length.		*/
	fd_set		rfds;		/* Read FDs for select.		*/
	struct timeval	tv;
	struct timeval	*tvp;
	fd_set		wfds;		/* Write FDs for select.	*/

	/* Initialization on first pass, if necessary.			*/

	if (firsttime)
		{
		FD_ZERO(&fds);
		if (sfd >= 0)
			{
			FD_SET(sfd, &fds);
			}
		firsttime = 0;
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
			perror("dgram_get_packets: getttimeofday");
			abort();
			}
		if (timercmp(endtout, &tv, >))
			{
			timersub(endtout, &tv, &tv);

			/* Wait at most one hour at a time.  SunOS has	*/
			/* an annoying limitation on the select timeout,*/
			/* one hour is much less than this limitation.	*/

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

	rcving = 1;
	justrcvd = 1;
	for (;;)
		{

		/* Set up for select:  get fd bitmaps.			*/

		rfds = fds;
		FD_ZERO(&wfds);
		if (wfd >= 0)
			{
			FD_SET(wfd, &rfds);
			FD_SET(wfd, &wfds);
			}
		else if (wfd == -2)
			{
			FD_SET(sfd, &rfds);
			FD_SET(sfd, &wfds);
			}
		if ((nitems = select(fdmax = (sfd > wfd ? sfd + 1 : wfd + 1),
				     &rfds,
				     &wfds,
				     (fd_set *)NULL,
				     tvp)) < 0)
			{
			if (errno != EINTR)
				{
				log_error(NULL, NULL, -1, LOGERR_SELECT);
				perror("dgram_get_packets: select");
				abort();
				}
			}

		/* Each pass through the following loop checks for	*/
		/* messages on one file descriptor.			*/

		for (fd = 0; fd <= fdmax; fd++)
			{

			/* If current fd was not selected, ignore it.	*/

			if (!FD_ISSET(fd, &rfds))
				continue;

			/* Each pass through the following loop		*/
			/* attempts to receive one segment.		*/

			for (;;)
				{

				/* Get a buffer if we do not already	*/
				/* have one on hand.			*/

				if ( (buf == NULL)  &&
				   ((buf =
				      (*(prtab->buffer_get))(MAX_PKT_SIZE) -
				         sizeof(unsigned long)) ==
				       NULL))
					{
					log_error(NULL, NULL, -1, LOGERR_MEM);
					(void)fprintf(stderr,
						      "%s %s\n",
						      "dgram_get_packets: ",
						      "out of memory!");
					abort();
					}

				/* Receive the segment, scream and die	*/
				/* if error.				*/

				fromlen = sizeof(from);
				pklen = recvfrom(fd,
						 buf,
						 MAX_PKT_SIZE,
						 0,
						 (struct sockaddr *)&from,
						 &fromlen);
				if ((pklen == 0) ||
				    ((pklen < 0) &&
				     (errno != EWOULDBLOCK)))
					{

					/* Shut down and tell rcv if	*/
					/* EOF or error.		*/
					if (close(fd) == -1)
					    {
					    log_rx(NULL, NULL, fd, 0, 0, errno);
					    perror("dgram_get_packets: close");

					    log_teardown(NULL, errno);
					    abort();
					    }
					FD_CLR(fd, &fds);
					log_rx(NULL, NULL, fd, 0, 0, errno);
					log_teardown(NULL, -1);
					/* pklen includes beginning header */
					if (prtab->rcv != NULL)
						(void)(*(prtab->rcv))(fd,
								      -1,
								      NULL,
								      pklen,
								      0);
					}
				else if (pklen > 0)
					{

					/* Pass packet to receiver.	*/

					log_rx(&from,
					       NULL,
					       -1,
					       ntohl(*(unsigned long *)buf),
					       pklen - sizeof(unsigned long),
					       -1);
					if ((prtab->rcv != NULL) &&
					    ((*(prtab->rcv))
						       (fd,
							-2,
							buf + sizeof(long),
							pklen - sizeof(long),
							ntohl(*(unsigned long *)buf))))
						buf = NULL;
					}
				else
					{

					/* Nothing more to read right	*/
					/* now.				*/

					break;
					}
				}
			
			if (--nitems <= 0)
				break;
			}

		/* Check for ability to write...			*/

		if ((wfd > 0) &&
		    (FD_ISSET(wfd, &wfds)))
			{
			rcving = 0;
			return (1);
			}

		/* Calculate next timeout.  If the timeout has expired,	*/
		/* tell the caller the sad story.			*/

		if (endtout != NULL)
			{

			if (gettimeofday(&tv, (struct timezone *)NULL) == -1)
				{
				log_error(NULL, NULL, -1, LOGERR_SELECT);
				perror("dgram_get_packets: getttimeofday");
				abort();
				}
			if (timercmp(endtout, &tv, >))
				{
				timersub(endtout, &tv, &tv);

				/* Wait at most one hour at a time.	*/

				if (tv.tv_sec > 3600)
					tv.tv_sec = 3600;
				}
			else
				{
				rcving = 0;
				errno = ETIME;
				return (0);
				}
			}
		}

	/* NOTREACHED */
	}

/* Send a packet, subject to the timeout.  The special association	*/
/* numbered -2 means to return to the sender of the most-recently	*/
/* received datagram.  Note that the datagram ID is just a per-packet	*/
/* sequence number; no attempt is made to maintain separate consecutive	*/
/* number sequences for each destination.				*/

int
dgram_send(asn, buf, len, endtout, pktid)

	long		asn;
	char		*buf;
	int		len;
	struct timeval	*endtout;
	unsigned long	*pktid;

	{
	int		cc;
	int		fd = asn;


	/* Make sure that fd wasn't closed out from under the sender.	*/

	if (fd == -1)
		{
		errno = EINVAL;
		return (-1);
		}

	/* Put the packet ID into the packet.				*/

	((unsigned long *)buf)[-1] = htonl(*pktid);

	/* If we just got done allowing receives, try the write without	*/
	/* bothering to do another receive.				*/

	if (justrcvd ||
	    rcving)
		{

		/* Try to write out the packet.				*/

		if (fd != -2)
			{
			cc = write(fd,
				   buf - sizeof(unsigned long),
				   len + sizeof(unsigned long));
			}
		else
			{

			/* fd of -2 says to return a datagram to the	*/
			/* sender of the previously received datagram.	*/

			if (fromlen == 0)
				{
				errno = EBADF;
				return (-1);
				}
			cc = sendto(sfd,
				    buf - sizeof(unsigned long),
				    len + sizeof(unsigned long),
				    0,
				    (struct sockaddr *)&from,
				    fromlen);
			}

		/* If we succeeded, or if we failed for some reason	*/
		/* other than blocking, or if we are already receiving	*/
		/* packets (and thus do not want to recurse), clean up	*/
		/* and exit.						*/

		if ((cc >= 0) ||
		    (errno != EWOULDBLOCK) ||
		    rcving)
			{
			if (fd != -2)
				log_tx(NULL,  /* @@@@ fix definition of
						 send to include a pointer
						 to the time at which this
						 send was scheduled, or
						 NULL if it is asynchronous.
						 For now, treat all sends
						 as if they were async. */
				       &(prot->dst),
				       NULL,
				       -1,
				       ntohl(((unsigned long *)buf)[-1]),
				       (cc >= 0 ? 
				       cc - sizeof(unsigned long) : 0),
				       cc == -1 ? errno : -1);
			else
				log_tx(NULL,
				       &(from),
				       NULL,
				       -1,
				       ntohl(((unsigned long *)buf)[-1]),
				       (cc >= 0 ? 
				       cc - sizeof(unsigned long) : 0),
				       cc == -1 ? errno : -1);
			if (cc >= 0)
				(*pktid)++;
			(*(prtab->buffer_free))(buf);
			justrcvd = 0;
			return (cc);
			}
		}

	/* The write failed with an EWOULDBLOCK or we need to allow	*/
	/* some receives to happen. Thus, we must honor the timeout	*/
	/* period, unless this would recursively invoke			*/
	/* dgram_get_packets.						*/

	/* Invoke the dgram_get_packets routine to receive packets and	*/
	/* accept new connections while we are waiting to write.	*/

	if (!dgram_get_packets(fd, endtout))
		{
		return (-1);
		}
	else
		{

		/* Write out the packet!				*/

		if (fd != -2)
			cc = write(fd,
				   buf - sizeof(unsigned long),
				   len + sizeof(unsigned long));
		else
			{

			/* fd of -2 says to return a datagram to the	*/
			/* sender of the previously received datagram.	*/

			if (fromlen == 0)
				{
				errno = EBADF;
				return (-1);
				}
			cc = sendto(sfd,
				    buf - sizeof(unsigned long),
				    len + sizeof(unsigned long),
				    0,
				    (struct sockaddr *)&from,
				    fromlen);
			}
		if (fd != -2)
			log_tx(NULL,
			       &(prot->dst),
			       NULL,
			       -1,
			       ntohl(((unsigned long *)buf)[-1]),
			       cc - sizeof(unsigned long),
			       cc == -1 ? errno : -1);
		else
			log_tx(NULL,
			       &(from),
			       NULL,
			       -1,
			       ntohl(((unsigned long *)buf)[-1]),
			       cc - sizeof(unsigned long),
			       cc == -1 ? errno : -1);
		if (cc >= 0)
			(*pktid)++;
		(*(prtab->buffer_free))(buf);
		justrcvd = 0;
		return (cc);
		}
	}

/* Record the fact that a setup has occurred.				*/

void
dgram_setup(pp, fd)

	protocol	*pp;
	int		fd;

	{

	if (sfd >= 0)
		{
		log_error(NULL, NULL, sfd, LOGERR_SELECT);
		(void)fprintf(stderr,
			      "dgram_setup: %s %s\n",
			      "cannot set up new server without",
			      "tearing down old one first");
		abort();
		}

	/* Let dgram_get_packets know of the change.			*/

	prot = pp;
	prtab = pp->prot;
	if (fd != -1)
		sfd = fd;
	firsttime = 1;
	log_setup(NULL, -1);
	}

/* Suspend until the (absolute) time specified by waketime, processing	*/
/* any incoming packets or new connections that occur in that interval.	*/

void
dgram_sleep_till(waketime)

	struct timeval	*waketime;

	{

	(void)dgram_get_packets(-1, waketime);
	}

/* Tear down connection.						*/

int
dgram_teardown(asn)

	long		asn;

	{
	    int		fd = asn;
	    int		sta;

	if (fd < 0)
		{

		/* Can't tear it down if it is already torn down!	*/

		errno = EINVAL;
		return (-1);
		}

	fromlen = 0;
	justrcvd = 0;
	sta = close(fd);    
	log_teardown(NULL, sta == 0 ? -1 : sta);
	return (sta);
	}



