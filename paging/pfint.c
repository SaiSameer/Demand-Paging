/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{

  STATWORD ps;
  disable(ps);
  unsigned long a = read_cr2();
  virt_addr_t * virt_addr_a =(virt_addr_t *)&a;


  restore(ps);
  return OK;
}


