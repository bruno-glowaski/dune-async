#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/types.h>

#define DUNE_CMD_CHANNEL_SLOT_SIZE 64
#define DUNE_CMD_CHANNEL_TOTAL_SLOTS (PAGE_SIZE / DUNE_CMD_CHANNEL_SLOT_SIZE)

struct dune_cmd_channel_call_slot {
  atomic_t state_seq;
  uint64_t id;
  uint64_t arg1;
  uint64_t arg2;
  uint64_t arg3;
  uint64_t arg4;
  uint64_t arg5;
  uint64_t arg6;
};

struct dune_cmd_channel_event_slot {
  atomic_t state_seq;
  uint64_t ret1;
  uint64_t ret2;
} __aligned(DUNE_CMD_CHANNEL_SLOT_SIZE);

struct dune_cmd_channel {
  struct dune_cmd_channel_call_slot calls[DUNE_CMD_CHANNEL_TOTAL_SLOTS / 2];
  struct dune_cmd_channel_event_slot events[DUNE_CMD_CHANNEL_TOTAL_SLOTS / 2];
} __aligned(PAGE_SIZE);

struct dune_cmd_channel_params {
  void __user *addr;
};

struct dune_cmd_channel_config {
  struct vm_area_struct *vma;
  struct page *page;
  void *kernel_addr;
};

long dune_register_cmd_channel(unsigned long arg);
