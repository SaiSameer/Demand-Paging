/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	int i;
	for(i =0 ;i< BS_COUNT; i++)
	{
		bsm_tab[i].bs_status = BSM_UNMAPPED;
		bsm_tab[i].bs_pid = -1;
		bsm_tab[i].bs_vpno = -1;
		bsm_tab[i].bs_npages = 0;
		bsm_tab[i].bs_sem = 0;
		bsm_tab[i].bs_privacy = 0;
		bsm_tab[i].bs_refcnt = 0;
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	int i = 0;
	for(i=0; i< BS_COUNT; i++)
	{
		if(bsm_tab[i].bs_status == BSM_UNMAPPED)
		{
			*avail = i;
			return OK;
		}
	}
	kprintf("All backing stores are in use");
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	bsm_tab[i].bs_status = BSM_UNMAPPED;
	bsm_tab[i].bs_pid = -1;
	bsm_tab[i].bs_vpno = -1;
	bsm_tab[i].bs_npages = 0;
	bsm_tab[i].bs_sem = 0;
	bsm_tab[i].bs_privacy = 0;
	bsm_tab[i].bs_refcnt = 0;
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	int i = 0;
	struct pentry * pptr = &proctab[pid];
	int vpno = vaddr/NBPG;
	for(i ; i< BS_COUNT; i++ )
	{
		if((pptr->bsm_tab[i].bs_status == BSM_MAPPED) )
		{
			if(pptr->bsm_tab[i].bs_vpno <= vpno && vpno < (pptr->bsm_tab[i].bs_vpno + pptr->bsm_tab[i].bs_npages)){
			*store = i;
			*pageth = vpno - pptr->bsm_tab[i].bs_vpno;
			return OK;
			}
		}
	}
	kprintf("Error in bsm lookup");
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	struct pentry *pptr = &proctab[pid];
	if(vpno < VIRTUAL_BASE_ADDR || source <0 || source > BS_COUNT || npages <= 0 || npages > 256)
	{
		kprintf("Invalid arguments for bsm_map");
		return SYSERR;
	}
	if(bsm_tab[source].bs_status == BSM_UNMAPPED){
		bsm_tab[source].bs_status = BSM_MAPPED;
		bsm_tab[source].bs_pid = pid;
		bsm_tab[source].bs_vpno = vpno;
		bsm_tab[source].bs_npages = npages;
		bsm_tab[source].bs_refcnt = 1;
	}
	else
	{
		bsm_tab[source].bs_refcnt++;
	}
	pptr->bsm_tab[source].bs_status = BSM_MAPPED;
	pptr->bsm_tab[source].bs_pid = pid;
	pptr->bsm_tab[source].bs_vpno = vpno;
	pptr->bsm_tab[source].bs_npages = bsm_tab[source].bs_npages;
	pptr->bsm_tab[source].bs_refcnt = 1;
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	struct pentry *pptr = &proctab[pid];
	if(vpno < VIRTUAL_BASE_ADDR)
	{
		kprintf("Invalid arguments for bsm_unmap");
		return SYSERR;
	}
	int  store, bs_offset;
	if(bsm_lookup(pid, vpno*VIRTUAL_BASE_ADDR, &store, &bs_offset) == SYSERR)
	{
		return SYSERR;
	}
	int i =0;
	for(i; i<NFRAMES; i++)
	{
		if(frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_PAGE)
		{
			if((frm_tab[i].fr_vpno > pptr->bsm_tab[store].bs_vpno)
					&& (frm_tab[i].fr_vpno < pptr->bsm_tab[store].bs_vpno + pptr->bsm_tab[store].bs_npages))
			{
				if(flag == 0)
				{
					if( free_frm(i) == SYSERR)
					{
						return SYSERR;
					}
				}
				else
				{
					if(free_prvt_frm(i) == SYSERR){
						return SYSERR;
					}
				}
			}
		}
	}
	pptr->bsm_tab[store].bs_status = BSM_UNMAPPED;
	pptr->bsm_tab[store].bs_pid = -1;
	pptr->bsm_tab[store].bs_vpno = -1;
	pptr->bsm_tab[store].bs_npages = 0;
	pptr->bsm_tab[store].bs_privacy =0;
	pptr->bsm_tab[store].bs_refcnt--;
	bsm_tab[store].bs_refcnt--;
	if(bsm_tab[store].bs_refcnt == 0)
	{
		bsm_tab[store].bs_status = BSM_UNMAPPED;
		bsm_tab[store].bs_privacy = 0;
	}
	return OK;

}

