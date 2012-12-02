/************************************************************************
 *									*
 *	File:  decode.h							*
 *									*
 *	Header file for encode and decode compressed integers.		*
 *									*
 *	Written 12-Sep-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char decode_h_rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/decode.h,v 1.1 2002/01/24 23:25:19 pingali Exp $";
#endif 

extern char	*decode_ulong();
extern int	encode_response();
extern int	encode_special_response();
extern char	*encode_ulong();
