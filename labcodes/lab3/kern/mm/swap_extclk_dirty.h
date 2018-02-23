#ifndef __KERN_MM_SWAP_EXTCLK_DIRTY_H__
#define __KERN_MM_SWAP_EXTCLK_DIRTY_H__

#include <swap.h>
extern struct swap_manager swap_manager_extclk_dirty;

typedef struct extclk_dirty_priv {
    list_entry_t list_head;
    list_entry_t *clk_ptr;
    bool all_pages_in_list;
} extclk_dirty_priv_t;

#endif
