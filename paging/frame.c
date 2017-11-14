/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int sc_start_frm = 0;
int qhead =0;
queue q[NFRAMES];
extern int page_replace_policy;


/*-------------------------------------------------------------------------
 * init_queue - initialize queue
 *-------------------------------------------------------------------------
 */
void init_queue()
{
  int i = 0;
  for(i ; i< NFRAMES; i++)
  {
	  q[i].next = -1;
	  q[i].prev = -1;
  }
  return OK;
}


/*-------------------------------------------------------------------------
 * remove_qframe - remove a frame from queue
 *-------------------------------------------------------------------------
 */
void remove_qframe(i)
{

  return OK;
}


/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  int i = 0;
  for(i ; i< NFRAMES; i++)
  {
	  frm_tab[i].fr_status = FRM_UNMAPPED;
	  frm_tab[i].fr_pid = -1;
	  frm_tab[i].fr_vpno = -1;
	  frm_tab[i].fr_refcnt = 0;
	  frm_tab[i].fr_type = -1;
	  frm_tab[i].fr_dirty = 0;
	  frm_tab[i].fr_sc = 1;
  }
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD ps;
  disable(ps);
  struct pentry *pptr = &proctab[frm_tab[i].fr_pid];
  int i =0;
  for(i; i<NFRAMES; i++)
  {
	  if(frm_tab[i].fr_status == FRM_UNMAPPED)
	  {
		  *avail = i;
		  restore(ps);
		  return OK;
	  }
  }
  if(page_replace_policy == SC)
  {
	  i = qhead;
	  while(1)
	  {
		  virt_addr_t * vf = (virt_addr_t *)(frm_tab[i].fr_vpno<<12);
		  pd_t * pd =  pptr->pdbr + sizeof(pd_t) * vf->pd_offset ;
		  pt_t * pt = pd->pd_base * NBPG + sizeof(pt_t) * vf->pt_offset;
		  if(pt->pt_acc == 1)
		  {
			  pt->pt_acc = 0;
		  }
		  else
		  {
			  free_frm(i);
			  kprintf("Frame %d is being replaced through SC", i);
			  *avail = i;
			  qhead = q[i].next;
			  remove_qframe(i);
			  restore(ps);
			  return OK;
		  }
		  i = q[i].next;
	  }
  }
  if(page_replace_policy == AGING)
  {

  }
  restore(ps);
  return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  STATWORD ps;
  disable(ps);
  struct pentry * fptr = &proctab[frm_tab[i].fr_pid];
  if(frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_type == FR_PAGE)
  {
	  virt_addr_t * vf = (virt_addr_t *)(frm_tab[i].fr_vpno<<12);
	  pd_t * pd =  fptr->pdbr + sizeof(pd_t) * vf->pd_offset ;
	  pt_t * pt = pd->pd_base * NBPG + sizeof(pt_t) * vf->pt_offset;
	  if(frm_tab[i].fr_dirty == 1)
	  {
		  int store,pageth;
		  if(bsm_lookup(frm_tab[i].fr_pid,frm_tab[i].fr_vpno/NBPG,store,pageth))
		  {
			  restore(ps);
			  return SYSERR;
		  }
		  write_bs((FRAME0 + i)*NBPG, store, pageth);
	  }
	  frm_tab[i].fr_status = FRM_UNMAPPED;
	  frm_tab[i].fr_pid = -1;
	  frm_tab[i].fr_vpno = -1;
	  frm_tab[i].fr_refcnt = 0;
	  frm_tab[i].fr_type = -1;
	  frm_tab[i].fr_dirty = 0;
	  frm_tab[i].fr_sc = 1;

	  pt->pt_pres = 0;
	  frm_tab[pd->pd_base - FRAME0].fr_refcnt--;
	  if(frm_tab[pd->pd_base - FRAME0].fr_refcnt == 0)
	  {
		  frm_tab[pd->pd_base - FRAME0].fr_status = FRM_UNMAPPED;
		  frm_tab[pd->pd_base - FRAME0].fr_pid = -1;
		  frm_tab[pd->pd_base - FRAME0].fr_vpno = -1;
		  frm_tab[pd->pd_base - FRAME0].fr_type = -1;
		  frm_tab[pd->pd_base - FRAME0].fr_dirty = 0;
		  frm_tab[pd->pd_base - FRAME0].fr_sc = 1;
		  pd->pd_pres = 0;
	  }
  }
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * free_prvt_frm - free a frame mapped to private page
 *-------------------------------------------------------------------------
 */
SYSCALL free_prvt_frm(int i)
{
	STATWORD ps;
	  disable(ps);
	  struct pentry * fptr = &proctab[frm_tab[i].fr_pid];
	  if(frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_type == FR_PAGE)
	  {
		  virt_addr_t * vf = (virt_addr_t *)&frm_tab[i].fr_vpno;
		  pd_t * pd =  fptr->pdbr + sizeof(pd_t) * vf->pd_offset ;
		  pt_t * pt = pd->pd_base * NBPG + sizeof(pt_t) * vf->pt_offset;
		  frm_tab[i].fr_status = FRM_UNMAPPED;
		  frm_tab[i].fr_pid = -1;
		  frm_tab[i].fr_vpno = -1;
		  frm_tab[i].fr_refcnt = 0;
		  frm_tab[i].fr_type = -1;
		  frm_tab[i].fr_dirty = 0;
		  frm_tab[i].fr_sc = 1;

		  pt->pt_pres = 0;
		  frm_tab[pd->pd_base - FRAME0].fr_refcnt--;
		  if(frm_tab[pd->pd_base - FRAME0].fr_refcnt == 0)
		  {
			  frm_tab[pd->pd_base - FRAME0].fr_status = FRM_UNMAPPED;
			  frm_tab[pd->pd_base - FRAME0].fr_pid = -1;
			  frm_tab[pd->pd_base - FRAME0].fr_vpno = -1;
			  frm_tab[pd->pd_base - FRAME0].fr_type = -1;
			  frm_tab[pd->pd_base - FRAME0].fr_dirty = 0;
			  frm_tab[pd->pd_base - FRAME0].fr_sc = 1;
			  pd->pd_pres = 0;
		  }
	  }
	  restore(ps);
	  return OK;
}
