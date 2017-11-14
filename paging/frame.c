/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int sc_start_frm = 0;
int qhead =-1;
rqueue rq[NFRAMES];
extern int page_replace_policy;
extern int policy_print;


/*-------------------------------------------------------------------------
 * init_queue - initialize queue
 *-------------------------------------------------------------------------
 */
void init_queue()
{
  int i = 0;
  for(i ; i< NFRAMES; i++)
  {
	  rq[i].next = -1;
	  rq[i].prev = -1;
  }
  return;
}


/*-------------------------------------------------------------------------
 * insert_qframe - insert a frame into queue
 *-------------------------------------------------------------------------
 */
void insert_qframe(i)
{
  int prev = rq[qhead].prev;
  if(qhead == -1)
  {
	  rq[i].next = rq[i].prev = i;
	  qhead = i;
	  return;
  }
  rq[prev].next = i;
  rq[qhead].prev = i;
  return;
}

/*-------------------------------------------------------------------------
 * remove_qframe - remove a frame from queue
 *-------------------------------------------------------------------------
 */
void remove_qframe(i)
{
  int qprev = rq[i].prev;
  int qnext = rq[i].next;
  if(qprev == qnext)
  {
	  qhead = -1;
	  return;
  }
  if(i == qhead)
  {
	  qhead = qnext;
  }
  rq[qprev].next = rq[i].next;
  rq[qnext].prev = rq[i].prev;
  return;
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
  int i =0;

  //kprintf("PFINTR\n");
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
	  kprintf("SC frame counter %d\n",i);
	  while(1)
	  {
		  struct pentry *pptr = &proctab[frm_tab[i].fr_pid];
		  virt_addr_t * vf = (virt_addr_t *)(frm_tab[i].fr_vpno<<12);
		  pd_t * pd =  pptr->pdbr + (sizeof(pd_t) * vf->pd_offset) ;
		  pt_t * pt = pd->pd_base * NBPG + (sizeof(pt_t) * vf->pt_offset);
		  if(pt->pt_acc == 1)
		  {
			  pt->pt_acc = 0;
		  }
		  else
		  {
			  free_frm(i);
			  if(policy_print == 1)
			  {
				  kprintf("Frame %d is being replaced through SC\n", i);
			  }
			  *avail = i;
			  qhead = rq[i].next;
			  remove_qframe(i);
			  restore(ps);
			  return OK;
		  }
		  i = rq[i].next;
	  }
  }
  if(page_replace_policy == AGING)
  {
	  kprintf("Aging");
	  i = qhead;
	  int young_age = 256;
	  int young_f = i;
	  while(1){
		  struct pentry *pptr = &proctab[frm_tab[i].fr_pid];
		  virt_addr_t * vf = (virt_addr_t *)(frm_tab[i].fr_vpno<<12);
		  pd_t * pd =  pptr->pdbr + (sizeof(pd_t) * (vf->pd_offset)) ;
		  pt_t * pt = pd->pd_base * NBPG + (sizeof(pt_t) * (vf->pt_offset));
		  if(pt->pt_acc == 1)
		  {
			  pt->pt_acc = 0;
			  frm_tab[i].fr_sc = frm_tab[i].fr_sc << 1;
			  if(frm_tab[i].fr_sc > 255)
			  {
				  frm_tab[i].fr_sc = 255;
			  }
		  }
		  if(young_age > frm_tab[i].fr_sc )
		  {
			  young_age = frm_tab[i].fr_sc;
			  young_f = i;
		  }
		  i = rq[i].next;
		  if(i == qhead)
		  {
			  free_frm(young_f);
			  if(policy_print == 1){
				  kprintf("Frame %d is being replaced through Aging\n", i);
			  }
			  *avail = young_f;
			  qhead = rq[i].next;
			  remove_qframe(i);
			  restore(ps);
			  return OK;
		  }
	  }

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
	  int pd_offset = frm_tab[i].fr_vpno >> 10;
	  int pt_offset = frm_tab[i].fr_vpno & 1023;
	  pd_t * pd = proctab[frm_tab[i].fr_pid].pdbr
	  				+ (pd_offset * sizeof(pd_t));
	  pt_t * pt	= (pd->pd_base * NBPG
	  				+ pt_offset * sizeof(pt_t));
	  /*virt_addr_t * vf = (virt_addr_t *)(frm_tab[i].fr_vpno<<12);
	  pd_t * pd =  fptr->pdbr + sizeof(pd_t) * vf->pd_offset ;
	  pt_t * pt = pd->pd_base * NBPG + sizeof(pt_t) * vf->pt_offset;
	 */
	  int store,pageth;
	  if(bsm_lookup(frm_tab[i].fr_pid,frm_tab[i].fr_vpno/NBPG,&store,&pageth))
	  {
		  restore(ps);
		  return SYSERR;
	  }
	  write_bs((FRAME0 + i)*NBPG, store, pageth);
	  frm_tab[i].fr_status = FRM_UNMAPPED;
	  frm_tab[i].fr_pid = -1;
	  frm_tab[i].fr_vpno = -1;
	  frm_tab[i].fr_refcnt = 0;
	  frm_tab[i].fr_type = -1;
	  frm_tab[i].fr_dirty = 0;
	  frm_tab[i].fr_sc = 0;

	  pt->pt_pres = 0;
	  pt->pt_acc =0;
	  frm_tab[pd->pd_base - FRAME0].fr_refcnt--;
	  if(frm_tab[pd->pd_base - FRAME0].fr_refcnt == 0)
	  {
		  frm_tab[pd->pd_base - FRAME0].fr_status = FRM_UNMAPPED;
		  frm_tab[pd->pd_base - FRAME0].fr_pid = -1;
		  frm_tab[pd->pd_base - FRAME0].fr_vpno = -1;
		  frm_tab[pd->pd_base - FRAME0].fr_type = -1;
		  frm_tab[pd->pd_base - FRAME0].fr_dirty = 0;
		  frm_tab[pd->pd_base - FRAME0].fr_sc = 0;
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
