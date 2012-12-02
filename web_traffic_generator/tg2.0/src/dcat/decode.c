/************************************************************************
 *									*
 *	File:  decode.c							*
 *									*
 *	Encode and decode compressed integers.				*
 *									*
 *	Written 11-Sep-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/tg/RCS/decode.c,v 1.1 2001/11/14 00:35:38 pingali Exp pingali $";
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
#include "decode.h"

/* Type definitions local to this file.					*/

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/


/* Decode an unsigned long from a buffer.  The number is packed seven	*/
/* bits per byte in little-endian order, with the sign bit indicating	*/
/* that more is to come.						*/

char *
decode_ulong(buf, n, len)

	char 		*buf;
	int		*n;
	int		len;

	{
	char		*bufend = &(buf[len]);
	unsigned int	curbyte;
	int		shift = 0;
	unsigned long	tmp = 0;

	/* Each pass though the following loop decodes one byte.	*/

	for (;;)
		{

		/* Pick up the next byte.				*/

		curbyte = *(buf++);

		/* Shift and mask it into the accumulated value.	*/

		tmp |= (curbyte & 0x7f) << shift;

		/* If the top bit is not set, this was the last byte.	*/

		if (curbyte <= 0x7f)
			break;

		/* Increment the shift count, scream if more than 32	*/
		/* bits are to be read.					*/

		shift += 7;
		if (shift > 32)
			{

			/*@@@@ Log bad response...			*/

			(void)fprintf(stderr,
				      "decode_ulong: bad format\n");
			abort();
			}

		/* If the encoded number runs off the end of the	*/
		/* buffer, return NULL so that the caller can try	*/
		/* again when he gets more input.			*/

		if (buf >= bufend)
			return ((char *)NULL);
		}

	/* Return the decoded number.					*/

	*n = tmp;
	return (buf);
	}


decode_ulong2(fp, result)

	FILE		*fp;
	int		*result;	/* Integer is returned here */

	{
	unsigned int	curbyte;
	int		shift = 0, rvalue;
	unsigned long	tmp = 0;

	/* Each pass though the following loop decodes one byte.	*/

	for (; (rvalue = getc(fp)) != EOF;)
		{

		/* Pick up the next byte.				*/

		curbyte = (char) rvalue;

		/* Shift and mask it into the accumulated value.	*/

		tmp |= (curbyte & 0x7f) << shift;

		/* If the top bit is not set, this was the last byte.	*/

		if (curbyte <= 0x7f)
			break;

		/* Increment the shift count, scream if more than 32	*/
		/* bits are to be read.					*/

		shift += 7;
		if (shift > 32)
			{

			/*@@@@ Log bad response...			*/

			(void)fprintf(stderr,
				      "decode_ulong: bad format\n");
			abort();
			}
		}
	if (rvalue == EOF) {
		return (-1);
	}
	/* Return the decoded number.					*/

	*result = tmp;
	return (0);
	}

/* Encode a response request.  This consists of the length of the	*/
/* desired response in bytes, followed by the number of bytes to skip	*/
/* in order to find the next response-length request.  Lose synch, and	*/
/* you die!  But does not require touching every byte of a long packet.	*/
/* Returns the length of the buffer consumed.  If the buffer pointer	*/
/* is NULL, returns the maximum length of buffer that can be consumed.	*/

int
encode_response(buf, len, n)

	char		*buf;
	unsigned int	len;
	unsigned int	n;

	{
	char		*cp = buf;
	static int	maxlen = -1;
	int		remainder;

	/* Set up maximum lengths if first time through.		*/

	if (maxlen < 0)
		maxlen = 2 * (int)encode_ulong((char *)NULL, 0);
	
	/* Just return maximum length if NULL buffer.			*/

	if (buf == NULL)
		return (maxlen);

	/* Encode the desired value and the length to skip.		*/

	cp = encode_ulong(buf, n);
	remainder = len - (cp - buf);
	if (remainder <= 0)
		cp = encode_ulong(cp, 1);
	else
		cp = encode_ulong(cp, remainder);
	return (cp - buf);
	}

/* Encode a special response request.  This consists of the length of	*/
/* the desired response in bytes, followed by a zero byte, followed by	*/
/* the number of bytes to skip in order to find the next		*/
/* response-length request.						*/

int
encode_special_response(buf, len, n)

	char		*buf;
	unsigned int	len;
	unsigned int	n;

	{
	char		*cp = buf;
	static int	maxlen = -1;
	int		remainder;

	/* Set up maximum lengths if first time through.		*/

	if (maxlen < 0)
		maxlen = 2 * (int)encode_ulong((char *)NULL, 0) + 1;
	
	/* Just return maximum length if NULL buffer.			*/

	if (buf == NULL)
		return (maxlen);

	/* Encode the desired value and the length to skip.		*/

	cp = encode_ulong(buf, n);
	*cp++ = 0x00;
	remainder = len - (cp - buf);
	if (remainder <= 0)
		cp = encode_ulong(cp, 1);
	else
		cp = encode_ulong(cp, remainder);
	return (cp - buf);
	}

/* Encode an unsigned long.  This consists of seven bits of number per	*/
/* byte of buffer, in little-endian order, with the sign bit indicating	*/
/* that more is to come.						*/

char *
encode_ulong(buf, n)

	char		*buf;
	unsigned long	n;

	{

	/* If no buffer, return maximum length.				*/

	if (buf == NULL)
		return ((char *)5);
	
	/* Each pass through the following loop encodes seven bits,	*/
	/* low-order bits first.					*/

	while (n > 127)
		{
		*(buf++) = (n & 0x7f) | 0x80;
		n >>= 7;
		}

	/* Encode the last bits -- leave sign bit clear.		*/

	*(buf++) = n;
	return (buf);
	}
