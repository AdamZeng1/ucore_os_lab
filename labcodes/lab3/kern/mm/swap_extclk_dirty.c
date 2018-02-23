#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_extclk_dirty.h>
#include <list.h>

/*LAB3 EXERCISE 2: 2015011278*/

static extclk_dirty_priv_t pra_priv;

static int
__init_mm(struct mm_struct *mm)
{     
     list_init(&pra_priv.list_head);
     pra_priv.clk_ptr = &pra_priv.list_head;
     pra_priv.all_pages_in_list = 0;
     mm->sm_priv = &pra_priv;
     return 0;
}

static int
__map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    if (((extclk_dirty_priv_t *)mm->sm_priv)->all_pages_in_list) return 0;

    list_entry_t *head = &((extclk_dirty_priv_t *)mm->sm_priv)->list_head;
    list_entry_t *entry = &(page->pra_page_link);
 
    assert(entry != NULL && head != NULL);
    list_add_before(head, entry);
    return 0;
}

static int
__swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
    ((extclk_dirty_priv_t *)mm->sm_priv)->all_pages_in_list = 1; // once page fault occurred, all pages are in list.

    list_entry_t *head = &((extclk_dirty_priv_t *)mm->sm_priv)->list_head;
    list_entry_t **clk_ptr_ptr = &((extclk_dirty_priv_t *)mm->sm_priv)->clk_ptr;
    assert(list_next(head) != head);
    for (; ; *clk_ptr_ptr = list_next(*clk_ptr_ptr)) {
        if (*clk_ptr_ptr == head) continue;

        struct Page *p = le2page(*clk_ptr_ptr, pra_page_link);
        uintptr_t va = p->pra_vaddr;
        pte_t *ptep = get_pte(mm->pgdir, va, 0);
        assert(*ptep & PTE_P);
        cprintf("visit virt: 0x%08x pte flags: %c%c\n",
                p->pra_vaddr, *ptep & PTE_A ? 'A' : '-', *ptep & PTE_D ? 'D' : '-');
        if (!(*ptep & PTE_A)) {
            if (!(*ptep & PTE_D)) { // 00
                *ptr_page = p;
                *clk_ptr_ptr = list_next(*clk_ptr_ptr);
                break;
            } else { // 01
                if (swapfs_write((va / PGSIZE + 1) << 8, p) != 0) {
                    cprintf("extclk dirty: failed to save\n");
                } else {
                    cprintf("extclk dirty: store page in vaddr 0x%x to disk swap entry %d\n", va, va / PGSIZE + 1);
                    *ptep &= ~PTE_D;
                    tlb_invalidate(mm->pgdir, va);
                }
            }
        } else { // 10, 11
            *ptep &= ~PTE_A;
            tlb_invalidate(mm->pgdir, va);
        }
    }
    return 0;
}

static int
__check_swap(void) {
    // init
    pde_t *pgdir = KADDR((pde_t *)rcr3());
    for (int i = 0; i < 4; ++i) {
        pte_t *ptep = get_pte(pgdir, (i + 1) * 0x1000, 0);
        assert(*ptep & PTE_P);
        assert(swapfs_write(((i + 1) * 0x1000 / PGSIZE + 1) << 8, pte2page(*ptep)) == 0);
        *ptep &= ~(PTE_A | PTE_D);
        tlb_invalidate(pgdir, (i + 1) * 0x1000);
    }
    assert(pgfault_num == 4);
    cprintf("read Virt Page c in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x3000 == 0x0c);
    assert(pgfault_num == 4);
    cprintf("write Virt Page a in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 4);
    cprintf("read Virt Page d in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x4000 == 0x0d);
    assert(pgfault_num == 4);
    cprintf("write Virt Page b in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num == 4);
    cprintf("read Virt Page e in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x5000 == 0x00);
    assert(pgfault_num == 5);
    cprintf("read Virt Page b in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    assert(pgfault_num == 5);
    cprintf("write Virt Page a in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 5);
    cprintf("read Virt Page b in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    assert(pgfault_num == 5);
    cprintf("read Virt Page c in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x3000 == 0x0c);
    assert(pgfault_num == 6);
    cprintf("read Virt Page d in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x4000 == 0x0d);
    assert(pgfault_num == 7);
    cprintf("write Virt Page e in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x5000 == 0x00);
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num == 7);
    cprintf("write Virt Page a in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 7);
    return 0;
}


static int
__init(void)
{
    return 0;
}

static int
__set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
__tick_event(struct mm_struct *mm)
{ return 0; }


struct swap_manager swap_manager_extclk_dirty =
{
     .name            = "extended clock swap manager",
     .init            = &__init,
     .init_mm         = &__init_mm,
     .tick_event      = &__tick_event,
     .map_swappable   = &__map_swappable,
     .set_unswappable = &__set_unswappable,
     .swap_out_victim = &__swap_out_victim,
     .check_swap      = &__check_swap,
};
