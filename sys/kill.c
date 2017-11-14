/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);

	int i=0;
	for(i; i< BS_COUNT; i++)
	{
		if(proctab[pid].bsm_tab[i].bs_status == BSM_MAPPED)
		{
			if(proctab[pid].bsm_tab[i].bs_vpno < VIRTUAL_BASE_ADDR){
				kprintf("Invalid bs_vpno is %d\n", proctab[pid].bsm_tab[i].bs_vpno);
				kprintf("for process id %d \n",pid);
				break;
			}
			if(bsm_unmap(pid, proctab[pid].bsm_tab[i].bs_vpno, proctab[pid].bsm_tab[i].bs_privacy) == SYSERR)
			{
				kprintf("BSM UNMAP error in kill\n");
				return SYSERR;
			}
		}
	}
	
	int pd_frm = proctab[pid].pdbr / NBPG - FRAME0;
	frm_tab[pd_frm].fr_status = FRM_UNMAPPED;
	frm_tab[pd_frm].fr_pid = -1;
	frm_tab[pd_frm].fr_vpno = -1;
	frm_tab[pd_frm].fr_refcnt = 0;
	frm_tab[pd_frm].fr_type = -1;
	frm_tab[pd_frm].fr_dirty = 0;
	frm_tab[pd_frm].fr_sc = 1;

	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}

	restore(ps);
	return(OK);
}
