#ifndef MAILBOX_H
#define MAILBOX_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/spinlock_types.h>
#include<linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>

#define ERR_EMPTY -1
#define ERR_FULL -2

struct mailbox_head_t {
	int count;
	spinlock_t sherlock;
	struct list_head head;
};

struct mailbox_entry_t {
	char name[10];
	char data[1024];
	struct list_head entry;
};

#endif
