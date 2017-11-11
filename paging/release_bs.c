#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
	int i=0;
	for(i ; i < NPROC; i++)
	{
		if(proctab[i].bsm_tab[bs_id].bs_status == BSM_MAPPED)
		{
			proctab[i].bsm_tab[bs_id].bs_status == BSM_UNMAPPED;
			proctab[i].bsm_tab[bs_id].bs_pid = -1;
			proctab[i].bsm_tab[bs_id].bs_vpno = -1;
			proctab[i].bsm_tab[bs_id].bs_npages = 0;
			proctab[i].bsm_tab[bs_id].bs_sem = 0;
			proctab[i].bsm_tab[bs_id].bs_privacy = 0;
			proctab[i].bsm_tab[bs_id].bs_refcnt = 0;
			bsm_tab[bs_id].bs_refcnt--;
		}
	}
	bsm_tab[bs_id].bs_status = BSM_UNMAPPED;
	bsm_tab[bs_id].bs_pid = -1;
	bsm_tab[bs_id].bs_vpno = -1;
	bsm_tab[bs_id].bs_npages = 0;
	bsm_tab[bs_id].bs_sem = 0;
	bsm_tab[bs_id].bs_privacy = 0;
	bsm_tab[i].bs_refcnt = 0;

	return OK;

}

