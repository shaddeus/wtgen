/************************************************************************
 *									*
 *	File:  prot_stream.c						*
 *									*
 *	Common routines for interfacing to stream socket protocols	*
 *	suite via the normal user-level interface.			*
 *									*
 *	Written 09-Aug-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/prot_stream.c,v 1.1 2001/11/14 00:35:38 pingali Exp $";
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
#include <unistd.h>
#include <string.h>
#include "config.h"
#include "log.h"
#include "distribution.h"
#include "protocol.h"

/* Type definitions local to this file.					*/

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/

static int		fdmax = -1;	/* maximum value for client FD.	*/
static fd_set		fds;		/* FDS to clients.		*/
static int		firsttime = 1;	/* initialization flag.		*/
static unsigned long	*idr = NULL;	/* per-asn Packet IDs for read.	*/
static unsigned long	*idw = NULL;	/* per-asn Packet IDs for write.*/
static int		justrcvd = 0;	/* Must rcv before next send.	*/
static protocol		*prot = NULL;	/* pointer to protocol pars.	*/
static protocol_table	*prtab = NULL;	/* pointer to protocol table.	*/
static int		rcving = 0;	/* Receiving packets?		*/
static struct sockaddr	*sa = NULL;	/* per-asn socket addresses.	*/
static int		sfd = -1;	/* Socket file descriptor.	*/
static void             (*setup_qos)();
                                        /* Function to set up FD for    */
                                        /* desired quality of service.  */



/* Accept connections and packets while waiting for the opportunity to	*/
/* write (if wfd specified) or for specified timeout, whichever comes	*/
/* first.								*/

int
stream_get_packets(wfd, endtout)

	int		wfd;		/* FD for write, -1 if none.	*/
	struct timeval	*endtout;	/* end of timeout period.	*/

	{
	static char	*buf = NULL;	/* pointer to receive buffer.	*/
	int		fd;
	struct sockaddr_in from;	/* Socket structure client	*/
	int		i;
	int		len;
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
			fdmax = sfd;
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
			log_error(NULL, NULL, -1, LOGERR_SELECT);
			perror("stream_get_packets: gettimeofday");
			abort();
			}
		if (timercmp(endtout, &tv, >))
			{
			timersub(endtout, &tv, &tv);

			/* Wait at most one hour at a time.  SunOS has	*/
			/* some strange limitations on the timeout in	*/
			/* the select call, I guess they just don't	*/
			/* believe in waiting forever.  One hour is	*/
			/* shorter than forever.			*/

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
		if ((nitems = select(fdmax > wfd ? fdmax + 1 : wfd + 1,
				     &rfds,
				     &wfds,
				     (fd_set *)NULL,
				     tvp)) < 0)
			{
			if (errno != EINTR)
				{
				log_error(NULL, NULL, -1, LOGERR_SELECT);
				perror("stream_get_packets: select");
				abort();
				}
			}

		/* Accept any outstanding connections		*/
		if (sfd >=0)
		{  
		if ((nitems > 0) &&
		    FD_ISSET(sfd, &rfds))
			{
			FD_CLR(sfd, &rfds);
			nitems--;

			/* Each pass through the following loop accepts	*/
			/* one connection.				*/

			for (;;)
				{
				len = sizeof(from);
				fd = accept(sfd,
					    (struct sockaddr *)&from,
					    &len);

				if (fd >= 0)
					{
					int		namelen;

					FD_SET(fd, &fds);
					idr[fd] = 0;
					if (fd > fdmax)
						fdmax = fd;
					namelen = sizeof(sa[fd]);
					if (getpeername(fd,
							&(sa[fd]),
							&namelen) == -1)
						{
						log_error(NULL,
							  NULL,
							  fd,
							  LOGERR_GETPEER);
						perror("stream_get_packets: getpeername");
						abort();
						}
					/* Apply desired quality-of-service to nfd. */

					(*setup_qos)(fd);
					
					log_accept(&(sa[fd]), NULL, fd, -1);
					}
				else if (errno != EWOULDBLOCK)
					{
					log_accept(NULL, NULL, -1, errno);
					perror("stream_get_packets: accept");
					abort();
					}
				else
					break;
			      }
		      }
	        }
		/* Each pass through the following loop checks for	*/
		/* messages on one file descriptor.			*/

		for (i = 0; (i <= fdmax) && (nitems > 0); i++)
			{

			/* If current fd was not selected, ignore it.	*/

			if (!FD_ISSET(i, &rfds))
				continue;

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
						      "stream_get_packets: ",
						      "out of memory!");
					abort();
					}

				/* Receive the segment, scream and die	*/
				/* if error.				*/

				pklen = read(i, buf, MAX_PKT_SIZE);
				if ((pklen == 0) ||
				    ((pklen < 0) &
				     (errno != EWOULDBLOCK)))
					{

					/* Shut down and tell rcv if	*/
					/* EOF or error.		*/

					if (close(i) == -1)
						{
						log_rx(&(sa[i]), NULL, -1, 0, 0,
						       pklen == 0 ? -1 : errno);
						perror("close");
						log_teardown (NULL, -1);
						abort();
						}
					FD_CLR(i, &fds);
					len = 0;
					if (prtab->rcv != NULL)
						(void)(*(prtab->rcv))(i,
								      i,
								      NULL,
								      pklen,
								      0);
					break;
					}
				else if (pklen > 0)
					{

					/* Pass packet to receiver.	*/

					log_rx(&(sa[i]), NULL, -1, idr[i], pklen,
					       -1);
					if ((prtab->rcv != NULL) &&
					    ((*(prtab->rcv))(i,
							     i,
							     buf,
							     pklen,
							     idr[i])))
						buf = NULL;
					idr[i] += pklen;
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

		if ((nitems > 0) &&
		    (wfd > 0) &&
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
				perror("stream_get_packets: getttimeofday");
				abort();
				}
			if (timercmp(endtout, &tv, >))
				{
				timersub(endtout, &tv, &tv);

				/* Wait at most one hour per select.	*/

				if (tv.tv_sec > 3600)
					tv.tv_sec = 3600;
				}
			else
				{
				errno = ETIME;
				rcving = 0;
				return (0);
				}
			}

		}

	/* NOTREACHED */
	}

/* Send a packet, subject to the timeout.				*/

int
stream_send(asn, buf, len, endtout, pktid)

	long		asn;
	char		*buf;
	int		len;
	struct timeval	*endtout;
	unsigned long	*pktid;

{
	int		cc;
	int		fd = asn;
	static int	firsttime = 1;	/* Must wait for select to say	*/
					/* that connect has succeeded.	*/
	char		*tmpbuf = buf;

	/* Make sure that fd wasn't closed out from under the sender.	*/

	if (fd < 0) {
		errno = EINVAL;
		return (-1);
	}

	/* If we just got done allowing receives, try the write without	*/
	/* bothering to do another receive.				*/
	/* However, don't do this the first time around because we must	*/
	/* check for the connect completing.				*/

	if ((justrcvd && !firsttime) || rcving)
        {

  	    /* Try to write out the packet.				*/

	    cc = write(fd, tmpbuf, len);
	    if ((cc >= 0) || ((cc <0) && (errno != EWOULDBLOCK)) || rcving)
	    {
		log_tx(NULL, &(sa[fd]), NULL, -1, idw[fd],
			cc, cc == -1 ? errno : -1);
		if (cc >= 0) {
			*pktid = idw[fd];
			idw[fd] += cc;
		}
		if ((cc == len) || (cc < 0)) 
		{    
		    buffer_generic_free(buf);
		    justrcvd = 0;
		    return (cc);
		}    
	     }
		/* Adjust buffer pointers if partial write occurred.	*/

	    if (cc > 0)
  	    {
		tmpbuf += cc;
		len -= cc;
	    }

	}

	/* The write failed with an EWOULDBLOCK or we need to allow	*/
	/* some receives to happen. Thus, we must honor the timeout	*/
	/* period, unless we are being recursively invoked.		*/


	/* Each pass through the following loop attempts to write the	*/
	/* packet, more than one pass will be needed if partial writes	*/
	/* occur.							*/

        for (;;)
        {

	    /* Invoke the get_packets routine to receive packets and accept	*/
	    /* new connections while we are waiting to write.		*/

            if (!stream_get_packets(fd, endtout)) {
		return (-1);
            } else {

		/* Write out the packet!				*/

		firsttime = 0;
		cc = write(fd, tmpbuf, len);
		if (cc >= 0)
                {
  		    log_tx(NULL, &(sa[fd]), NULL, -1, idw[fd], cc, -1);
		    *pktid = idw[fd];
  		    idw[fd] += cc;
  		    if (cc != len)
   		    {
			tmpbuf += cc;
			len -= cc;
			continue;
   		    }
		} else if (errno != EWOULDBLOCK) {
			log_tx(NULL, &(sa[fd]), NULL, -1, idw[fd], 0, errno);
		}
		buffer_generic_free(buf);
		justrcvd = 0;
		return (cc);
	    }
	}
}

/* Record the fact that a setup has occurred.				*/

void
stream_setup(pp, fd,my_setup_qos)

	protocol	*pp;
	int		fd;
        void            (*my_setup_qos)();

	{
	int		nfds;

	if (sfd >= 0)
		{
		log_error(NULL, NULL, sfd, LOGERR_2SETUP);
		(void)fprintf(stderr,
			      "dgram_setup: %s %s\n",
			      "cannot set up new server without",
			      "tearing down old one first");
		abort();
		}

	/* Allocate memory for ID arrays.				*/

	if (idr == NULL)
		{
		nfds = getdtablesize();
		if ((idr =
		     (unsigned long *)malloc(nfds * sizeof(unsigned long))) ==
		    NULL)
			{
			log_error(NULL, NULL, sfd, LOGERR_MEM);
			(void)fprintf(stderr, "stream_setup: Out of memory!\n");
			abort();
			}
		else
		    memset( (char *)idr, 0, (nfds * sizeof(unsigned long)));
		}
	if (idw == NULL)
		{
		nfds = getdtablesize();
		if ((idw =
		     (unsigned long *)malloc(nfds * sizeof(unsigned long))) ==
		    NULL)
			{
			log_error(NULL, NULL, sfd, LOGERR_MEM);
			(void)fprintf(stderr, "stream_setup: Out of memory!\n");
			abort();
			}
		else
		    memset( (char *)idw, 0, (nfds * sizeof(unsigned long)));
		}
	if (sa == NULL)
		{
		nfds = getdtablesize();
		if ((sa =
		     (struct sockaddr *)malloc(nfds *
					       sizeof(struct sockaddr))) ==
		    NULL)
			{
			log_error(NULL, NULL, sfd, LOGERR_MEM);
			(void)fprintf(stderr, "stream_setup: Out of memory!\n");
			abort();
			}
		else
		    memset( (char *)sa, 0, (nfds * sizeof(unsigned long)));
		}

	/* Let stream_get_packets know of the change.			*/

	prot = pp;
	prtab = pp->prot;
	if (fd < 0)
		{
		int		cfd = -fd - 1;

		FD_SET(cfd, &fds);
		if (cfd > fdmax)
			fdmax = cfd;
		sa[cfd] = prot->dst;
		}
	else
		{
		sfd = fd;
		firsttime = 1;
 		}

	setup_qos = my_setup_qos;
	
	log_setup (NULL, -1);

	}

/* Suspend until the (absolute) time specified by waketime, processing	*/
/* any incoming packets or new connections that occur in that interval.	*/

void
stream_sleep_till(waketime)

	struct timeval	*waketime;

	{

	(void)stream_get_packets(-1, waketime);
	justrcvd = 1;
	}

/* Tear down connection.						*/

int
stream_teardown(asn)

	long		asn;

	{
	int		i;
	int		fd = asn;
	int		result;
	int		tmperrno;

	if (fd < 0)
		{

		/* Can't tear it down if it is already torn down!	*/

		errno = EINVAL;
		return (-1);
		}

	result = close(fd);
	log_teardown(NULL, result == 0 ? -1 : errno);

	if (fd == sfd)
		{

		/* Preserve errno from original close().		*/

		tmperrno = errno;

		/* Clear out sfd in order to force initialization if	*/
		/* it is later re-opened.				*/

		sfd = -1;

		/* This is the server socket, so make sure to also kill	*/
		/* current connections to any clients.			*/

		for (i = 0; i <= fdmax; i++)
			{
			if (FD_ISSET(i, &fds))
				(void)close(i);
			}

		/* Restore errno from original close().			*/

		errno = tmperrno;
		}

	return (result);
	}
