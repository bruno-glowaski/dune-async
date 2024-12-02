#include "asm/syscall.h"
#include "dune.h"

#include <asm/atomic.h>
#include <linux/highmem.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>

int dune_proccess_cmd_channel(void *data) {
  struct dune_cmd_channel_config *config = data;
  struct dune_cmd_channel *channel = config->kernel_addr;
  struct dune_cmd_channel_call_slot current_call;
  int i, j;
  long syscall_ret;

  /* TODO: Implement processing interruption */
  while (1) {
    for (i = 0; i < DUNE_CMD_CHANNEL_CALL_SLOTS; i++) {
      if (atomic_read_acquire(&channel->calls[i].state) !=
          DUNE_CMD_CHANNEL_SLOT_STATE_READY) {
        continue;
      }

      current_call = channel->calls[i];
      atomic_set_release(&channel->calls[i].state,
                         DUNE_CMD_CHANNEL_SLOT_STATE_FREE);
      syscall_ret = sys_call_table[current_call.id](
          current_call.arg1, current_call.arg2, current_call.arg3,
          current_call.arg4, current_call.arg5, current_call.arg6);

      while (1) {
        for (j = 0; j < DUNE_CMD_CHANNEL_EVENT_SLOTS; j++) {
          if (atomic_read_acquire(&channel->events[i].state) !=
              DUNE_CMD_CHANNEL_SLOT_STATE_FREE) {
            continue;
          }
        }
        if (j < DUNE_CMD_CHANNEL_EVENT_SLOTS) {
          break;
        }
        schedule();
      }

      channel->events[i].tag = current_call.tag;
      channel->events[i].ret = syscall_ret;
      atomic_set_release(&channel->events[i].state,
                         DUNE_CMD_CHANNEL_SLOT_STATE_READY);
    }
  }

  kunmap(config->page);
  kfree(config);
  return 0;
}

long dune_register_cmd_channel(unsigned long arg) {
  long r;
  struct dune_cmd_channel_params params;
  struct dune_cmd_channel_config *config;

  if ((r = copy_from_user(&params, (void __user *)arg,
                          sizeof(struct dune_cmd_channel_params))) < 0) {
    return r;
  }

  if ((unsigned long)params.addr % PAGE_SIZE != 0) {
    return -EINVAL;
  }

  if (!access_ok(VERIFY_WRITE, params.addr, PAGE_SIZE)) {
    return -EACCES;
  }

  if ((config = kmalloc(sizeof(struct dune_cmd_channel_config), GFP_KERNEL)) ==
      NULL) {
    return -ENOMEM;
  }

  if ((r = get_user_pages(current, current->mm, (unsigned long)params.addr, 1,
                          1, 0, &config->page, &config->vma)) < 1) {
    goto error_with_alloc;
  }

  if ((config->kernel_addr = kmap(config->page)) == NULL) {
    return -EIO;
    goto error_with_alloc;
  }

  kthread_run(dune_proccess_cmd_channel, config, "dune_cmd_channel_dispatch");

  return 0;

error_with_alloc:
  kfree(config);
  return r;
}
