/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  if(virtpage < 4096 || source <0 || source >= BS_COUNT || npages <= 0 || npages > 256)
  {
  	restore(ps);
	return SYSERR;
  }
  if(bsm_map(currpid, virtpage, source, npages) == SYSERR)
  {
	  restore(ps);
	  return SYSERR;
  }
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);
  if(virtpage < VIRTUAL_BASE_ADDR){
	  restore(ps);
	  return SYSERR;
  }
  if(bsm_unmap(currpid,virtpage,0) == SYSERR)
  {
	  restore(ps);
	  return SYSERR;
  }
  restore(ps);
  return OK;
}
