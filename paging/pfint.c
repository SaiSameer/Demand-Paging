/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{

  STATWORD ps;
  disable(ps);
  struct pentry *pptr = &proctab[currpid];
  unsigned long a = read_cr2();
  virt_addr_t * virt_addr_a =(virt_addr_t *)&a;
  pd_t *pd = pptr->pdbr + sizeof(pd_t) * virt_addr_a->pd_offset;
  int store, pageth;
  if(bsm_lookup(currpid, a, &store, &pageth) == SYSERR)
  {
	  kill(currpid);
	  restore(ps);
	  return SYSERR;
  }
  if(pd->pd_pres == 0)
  {
	  int i =0;
	  int frame;
	  get_frm(&frame);
	  if(frame == -1)
	  {
		  kprintf("Invalid frame returned");
		  restore(ps);
		  return SYSERR;
	  }
	  frm_tab[frame].fr_status = FRM_MAPPED;
	  frm_tab[frame].fr_pid = currpid;
	  frm_tab[frame].fr_vpno = -1;
	  frm_tab[frame].fr_refcnt = 0;
	  frm_tab[frame].fr_type = FR_TBL;
	  frm_tab[frame].fr_dirty = 0;
	  frm_tab[frame].fr_sc =1;
	  pt_t *pt=(pt_t*)((FRAME0+frame)*NBPG);
	  for(i=0; i< P_SIZE; i++)
	  {
		  pt[i].pt_pres = 0;
		  pt[i].pt_dirty = 0;
	  }

	  pd->pd_pres = 1;
	  pd->pd_write = 1;
	  pd->pd_global = 0;
	  pd->pd_base =	FRAME0 + frame;
  }
  pt_t *pt = (pt_t *)(pd->pd_base * NBPG + sizeof(pt_t) * virt_addr_a->pt_offset );
  if(pt->pt_pres == 0)
  {
	  int i=0;
	  int frame;
	  get_frm(&frame);
	  if(frame == -1)
	  {
		  kprintf("Invalid frame returned");
		  restore(ps);
		  return SYSERR;
	  }
	  frm_tab[frame].fr_status = FRM_MAPPED;
	  frm_tab[frame].fr_pid = currpid;
	  frm_tab[frame].fr_vpno = a / NBPG;
	  frm_tab[frame].fr_refcnt = 1;
	  frm_tab[frame].fr_type = FR_PAGE;
	  frm_tab[frame].fr_dirty = 0;
	  frm_tab[frame].fr_sc = 1;

	  frm_tab[pd->pd_base - FRAME0].fr_refcnt++;
	  pt->pt_pres = 1;
	  pt->pt_write = 1;
	  pt->pt_global = 0;
	  pt->pt_dirty =0;
	  pt->pt_base = FRAME0 + frame;
	  read_bs((FRAME0 + frame)* NBPG,store, pageth);
  }
  write_cr3(pptr->pdbr);
  restore(ps);
  return OK;
}


