/************************************************************************
 *									*
 *	File:  distribution.h						*
 *									*
 *	Structs defining addtional parameters for those probability	*
 *	distributions that need them.					*
 *									*
 *	Written 20-Jun-90 by Paul E. McKenney, SRI International.	*
 *									*
 ************************************************************************/

#ifndef lint 
static char distribution_h_rcsid[] = "$Header: /nfs/ruby/pingali/tg/isitg/dist/tg2.0/src/tg/RCS/distribution.h,v 1.1 2002/01/24 23:25:19 pingali Exp $";
#endif 

/* Type definitions local to this file.					*/

typedef struct
	{
	double		(*generate)();	/* generate a new variable.	*/
	double		par1;		/* distribution parameter 1.	*/
	double		par2;		/* distribution parameter 2.	*/
	double		par3;		/* distribution parameter 3.	*/
	double		par4;		/* distribution parameter 4.	*/
	char		*pars;		/* pointer to more parameters	*/
					/* for those distributions that	*/
					/* need them.			*/
	}		distribution;

typedef struct
	{
	double		mean[2];	/* mean time in each state.	*/
	unsigned long	p[2];		/* probability of remaining in	*/
					/* same state.			*/
	distribution	*dist[2];	/* distribution in each state.	*/
	int		state;		/* current state.		*/
	}		dist_markov2;	/* 2-state markov distribution.	*/

extern char		*dist_const_init();
extern char		*dist_exp_init();
extern char		*dist_markov2_init();
extern char		*dist_uniform_init();
