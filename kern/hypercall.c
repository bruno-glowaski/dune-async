#include "dune.h"

#include <linux/highmem.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>

int dune_proccess_cmd_channel(void *data) {
  struct dune_cmd_channel_config *config = data;

  while (1) {
    schedule();
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
