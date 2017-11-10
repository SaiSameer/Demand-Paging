/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	int pid = create(procaddr,ssize,priority,name,nargs,args);
	int store;
	struct pentry *pptr = &proctab[pid];
	if(get_bsm(&store) == SYSERR)
	{
		return SYSERR;
	}
	if(bsm_map(pid,VIRTUAL_BASE_ADDR,store, hsize) == SYSERR)
	{
		return SYSERR;
	}
	bsm_tab[store].bs_privacy = 1;
	pptr->store = store;
	pptr->vhpno = BACKING_STORE_BASE;
	pptr->vhpnpages = hsize;
	pptr->vmemlist = getmem(sizeof(struct mblock));
	pptr->vmemlist->mlen = hsize*NBPG;
	pptr->vmemlist->mnext = NULL;



	restore(ps);
	return OK;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
