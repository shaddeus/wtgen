/************************************************************************
 *                                                                      *
 *      File:  dcat.c                                                   *
 *                                                                      *
 *      A filter for reading TG log files                               *
 *                                                                      *
 *      Written 12-Sep-90 by Danny Lee, SRI International.              *
 *      Copyright (c) 1990 by SRI International.                        *
 *                                                                      *
 *      Contributions from Steve Casner                                 *
 *                                                                      *
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "log.h"

#ifndef TRUE
#define TRUE 1
#endif  

#ifndef FALSE
#define FALSE 0
#endif  

unsigned long	on_time_sec = 0;

main(argc, argv)
    int			argc;
    char		**argv;
    
{
    register char	*buff;
    int			match;
    char		*malloc();
    unsigned long 	result;
    int			filestat;
    FILE		*fi;
    extern int		optind;
    char c; 
    int absolute = FALSE;
    int errflg = 0; 

    /* 
     * We use the standard getopt() call to parse
     * command line options.
     */
    while ((c = getopt(argc, argv, "a")) != -1) {
	switch (c) {
	  case 'a':
	    absolute = TRUE;
	    break;
	  case '?':
	    errflg++;
	    break;
	  default: 
	    errflg++;
	    fprintf (stderr, "dcat: unknown option [%s]\n",
		     argv[optind]);
	}
    }

    if (errflg) {
	fprintf (stderr, "Usage : dcat [-a] <filename> \n");
	exit (-1);
    }

    /* getopt permutes the command line arguments */ 
    argc -= optind;
    argv += optind;

    /* Open specified file */
    if ( argc < 1 )
	fi = stdin;
    else {
	if ((fi = fopen(*argv, "r")) == NULL) {
	    perror("dcat cant open input ");
	    exit (-1);
	}
    }

    /* Allocate temporary buffer space */
    if ((buff = malloc (BUFSIZ)) == NULL) {
	perror ("dcat: malloc");
	exit (-1);
    }

    /* Read past the header portion of the file. */
    for (match = FALSE; 
	 (match == FALSE) && (fgets (buff, BUFSIZ, fi) != NULL); 
	 ){

      /* Casner - Snatch epoch base time if it is present and absolute
	 time requested.*/  
      if (absolute) {
	(void) sscanf(buff,"Program start time from UNIX epoch %lu", 
		      &on_time_sec);
      }
	if (!strcmp (buff, END_HDR_STRING)) 
	    match = TRUE;
	 else 
	    printf ("%s", buff);
    }

    /*
     * End of file was detected without finding the
     * special end of header entry.
     */
    if (match == FALSE) {
	fprintf (stderr, "Error: premature end of file\n");
	exit (-1);
    }

    /* Print a header for the forthcoming table */
    puts ("\n\nEvent Time\tType\t\tAddress\t\tId\tLength");
    printf ("--------------------------------------------------");
    printf ("---------------\n\n");

    /* Read the record type until we run out */
    for (; (filestat = decode_ulong2(fi, &result)) != EOF; ) {
	log_parse (fi, result);
    }

    free(buff);

    if (fi != stdin) {
	fclose (fi);
    } else {
	clearerr (fi);			/* reset sticky eof */
    }
    if (ferror (stdout)) {
	fprintf (stderr, "cat: output write error\n");
	exit (-1);
    }
    exit (0);
}

/*
 *
 *
 */

log_parse (fp, rec_type) 

	FILE		*fp;
	unsigned long	rec_type;
{
	int		ctrl;
	unsigned long	ac_t_usec, t_sec, t_usec, errno, assoc;

	/* Fetch record control/modifier word */
	if ((ctrl = getc (fp)) == EOF) {	
		fprintf (stderr, "log_parse: premature EOF\n");
		fprintf (stdout, "log_parse: premature EOF\n");
		exit (-1);
	}
	/* Print event time */
	if (ctrl & LOGCTL_SCHED) {
		decode_ulong2 (fp, &t_sec);	/* time in seconds */
		decode_ulong2 (fp, &t_usec);	/* time in useconds */
		decode_ulong2 (fp, &ac_t_usec);	/* time in useconds */
		t_sec += on_time_sec; 
		printf ("%u.%06u + %u\t", t_sec, t_usec, ac_t_usec);
	} else {
		decode_ulong2 (fp, &t_sec);	/* time in seconds */
		decode_ulong2 (fp, &t_usec);	/* time in useconds */
		t_sec += on_time_sec; 
		printf ("%u.%06u\t", t_sec, t_usec);
	}

	switch(rec_type) {
		case LOGTYPE_RX:
			printf ("Receive  ");
			{
				unsigned long pkt_id, pkt_len, assoc, lerrno;

				/* Extract address information */
				log_parse_address (fp, ctrl);

				/* Extract packet length and packet id */
				decode_ulong2(fp, &pkt_id);
				decode_ulong2(fp, &pkt_len);
				printf ("\t%u\t%u", pkt_id, pkt_len);
			}
			break;
		case LOGTYPE_TX:
			printf ("Transmit ");
			{
				unsigned long pkt_id, pkt_len, assoc, lerrno;

				/* Extract address information */
				log_parse_address (fp, ctrl);

				/* Extract packet length and packet id */
				decode_ulong2(fp, &pkt_id);
				decode_ulong2(fp, &pkt_len);
				printf ("\t%u\t%u", pkt_id, pkt_len);
			}
			break;
		case LOGTYPE_SETUP:
			printf ("Setup    ");
			break;
		case LOGTYPE_TEARDOWN:
			printf ("Teardown ");
			break;
		case LOGTYPE_ACCEPT:
			printf ("Accept   ");
			{
				/* Extract address information */
				log_parse_address(fp, ctrl);

				/* Association is supplied */
				decode_ulong2(fp, &assoc);
				printf ("\tAssociation %d", assoc);
			}
			break;
		case LOGTYPE_ERROR:
			printf ("Error    ");
			{
				/* Extract and print address information */
				log_parse_address (fp, ctrl);

				/* Extract Unix errno.  */
				decode_ulong2(fp, &errno);
				printf ("\tError Entry: Unix Error Code %u",
					errno);

				/* log_parse_error (fi, (char) ctrl); */
			}
			break;
		default:
			fprintf (stderr, "log_parse: unknown record type\n");
	}
	/* Check for error */
	if (ctrl & LOGCTL_EXCEPT) {
		decode_ulong2 (fp, &errno);
		printf ("\tUnix Errno %d\n", errno); 
	} else {
		putchar('\n');
	}
}

/*
 * Print either the association identification number or the
 * packet source-destination pair
 *
 */


log_parse_address(fp, ctrl)
	FILE		*fp;
	unsigned char	ctrl;
{
	char  buffer[30];      /* contains dotted decimal ip address + port */
	struct sockaddr_in	sin;
	unsigned long assoc;

	if (ctrl & LOGCTL_ADDR) {
		fread (buffer, sizeof (sin.sin_port) + sizeof (sin.sin_addr),
			1, fp);
		ipport_btoaddr(buffer, &sin);
		ipport_addrtoa(&sin, buffer);
		printf ("\t%s", buffer);		/* Source address */
		if (ctrl & LOGCTL_2ADDR) {
			fread (buffer, sizeof (sin.sin_port) +
				sizeof (sin.sin_addr), 1, fp);
			ipport_btoaddr (buffer, &sin);
			ipport_addrtoa (&sin, buffer);
			printf ("\t%s", buffer);	/* Destination address */
		}
	} else {
		decode_ulong2(fp, &assoc);
		printf ("\tAssociation %d", assoc);
	}
}
