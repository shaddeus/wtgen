/************************************************************************
 *									*
 *	File:  distribution.c						*
 *									*
 *	Contains distribution generation functions.			*
 *									*
 *	Written 17-Aug-90 by Paul E. McKenney, SRI International.	*
 *	Copyright (c) 1990 by SRI International.			*
 *									*
 ************************************************************************/

#ifndef lint 
static char rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/distribution.c,v 1.1 2001/11/14 00:35:38 pingali Exp $";
#endif 

/* Include files.							*/

#include <stdio.h>
#include <math.h>
#include "config.h"
#include "distribution.h"

/* Type definitions local to this file.					*/

/* Functions exported from this file.					*/

/* Functions local to this file.					*/

/* Variables exported from this file.					*/

/* Variables local to this file.					*/


/* Return constant ``random'' variate.					*/

double
dist_const_gen(dist)

	distribution	*dist;

	{

	return (dist->par1);
	}

/* Initialize constant ``random'' variate.				*/

char *
dist_const_init(dist, par)

	distribution	*dist;
	double		par;

	{

	dist->generate = dist_const_gen;
	dist->par1 = par;
	return (NULL);
	}

/* Return exponentially-distributed random variate.			*/

double
dist_exp_gen(dist)

	distribution	*dist;
	{

	double		value;

	do
		{
			value = -dist->par1 * log(((double)(random() + 1))/
				(double)((unsigned)MAX_RANDOM + 1));
		} while ((value < dist->par2) || (value > dist->par3));

	return(value);
	}

/* Initialize exponentially-distributed random variate.			*/

char *
dist_exp_init(dist, mean, min, max)

	distribution	*dist;
	double		mean;
	double		min;
	double		max;
	{

	dist->generate = dist_exp_gen;
	dist->par1 = mean;
	dist->par2 = min;
	dist->par3 = max;
	return (NULL);
	}

/* Return uniformly-distributed random variate in interval [0, par1).	*/	

double
dist_markov2_gen(dist)

	distribution	*dist;

	{
	dist_markov2	*dm2;
	int		state;
	distribution	*subdist;
	double		x;

	/* Get variate from current distribution.			*/

	dm2 = (dist_markov2 *)(dist->pars);
	state = dm2->state;
	subdist = dm2->dist[state];
	x = (*(subdist->generate))(subdist);

	/* Switch state, if necessary.					*/

	if (random() > dm2->p[state])
		dm2->state = !state;

	/* Pass back the variate.					*/

	return (x);
	}

/* Initialize two-state markov random variate.				*/

char *
dist_markov2_init(dist, t1, d1, t2, d2)

	distribution	*dist;
	double		t1;
	distribution	*d1;
	double		t2;
	distribution	*d2;

	{
	dist_markov2	*dm2;

	/* Set up generation function.					*/

	dist->generate = dist_markov2_gen;

	/* Get memory for markov2 struct.				*/

	dm2 = (dist_markov2 *)malloc(sizeof(dist_markov2));
	if (dm2 == NULL)
		{
		return ("distribution: Out of memory");
		}
	dist->pars = (char *)dm2;

	/* Get mean and distribution for state 1.			*/

	dm2->mean[0] = t1;
	if (dm2->mean[0] < 1.)
		return ("Mean state-residence time must be at least 1");
	else
		dm2->p[0] = (1. - 1. / dm2->mean[0]) *
			    (unsigned)0x80000000;
	dm2->dist[0] = (distribution *)malloc(sizeof(distribution));
	if (dm2->dist[0] == NULL)
		{
		return ("distribution: Out of memory");
		}
	*(dm2->dist[0]) = *d1;

	/* Get mean and distribution for state 2.			*/

	dm2->mean[1] = t2;
	if (dm2->mean[1] < 1.)
		return ("Mean state-residence time must be at least 1");
	else
		dm2->p[1] = (1. - 1. / dm2->mean[1]) *
			    (unsigned)0x80000000;
	dm2->dist[1] = (distribution *)malloc(sizeof(distribution));
	if (dm2->dist[1] == NULL)
		{
		return ("distribution: Out of memory");
		}
	*(dm2->dist[1]) = *d2;

	/* Initialize state.						*/

	dm2->state = 0;

	return (NULL);
	}

/* Return uniformly-distributed random variate in interval [0, par1).	*/	

double
dist_uniform_gen(dist)

	distribution	*dist;
	{

	/* dist->par1 is upper limit, dist->par2 is lower limit */
	return ((dist->par2 - dist->par1) * (((double)random()) /
		(double)((unsigned)MAX_RANDOM + 1)) + dist->par1);
	}

/* Initialize uniformly-distributed random variate in interval		*/
/* [0, par1).								*/

char *
dist_uniform_init(dist, min, max)

	distribution	*dist;
	double		min;
	double		max;

	{

	dist->generate = dist_uniform_gen;
	dist->par1 = min;
	dist->par2 = max;
	return (NULL);
	}
