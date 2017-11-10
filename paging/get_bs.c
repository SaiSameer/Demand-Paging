#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
    STATWORD ps;
    disable(ps);
	if((npages<1 || npages >128) || (bs_id < 0 || bs_id >= MAX_ID))
	{
		kprintf("Backing store ID or pages is out of bound");
		restore(ps);
		return SYSERR;	
	}
	
	if(bsm_tab[bs_id].bs_status == BSM_MAPPED){
		if(bsm_tab[bs_id].bs_privacy == 1)
		{
			kprintf("Requested backing store is in use for private heap");
			restore(ps);
			return SYSERR;
		}
		npages = bsm_tab[bs_id].bs_npages;		
	}
	bsm_tab[bs_id].bs_npages = npages;
	bsm_tab[bs_id].bs_pid = currpid;
	
    restore(ps);
    return npages;

}


