/*
 * dynamic_fsync - Disables file sync when active
 *
 * Copyright (c) 2015, Tyler Dunn <ireallylikeshrek@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/state_notifier.h>
#include <linux/spinlock.h>

static struct notifier_block notif;

extern void sync_filesystems(int wait);

bool fsync_sleep = true;
module_param(fsync_sleep, bool, 0644);

/*
 * fsync_lock protects sync during suspend, spinlock is used over
 * mutex as we do not want to sleep on a sync and use a multi-core system
 */
static DEFINE_SPINLOCK(fsync_lock);

static void dfsync_suspend(void)
{
	unsigned long flags;

	pr_info("[dynamic_fsync] Syncing filesystem!\n");

	spin_lock_irqsave(&fsync_lock, flags);
	sync_filesystems(0);
	sync_filesystems(1);
	spin_unlock_irqrestore(&fsync_lock, flags);
}

static int state_notifier_callback(struct notifier_block *this,
				unsigned long event, void *data)
{
	switch (event) {
		case STATE_NOTIFIER_SUSPEND:
			dfsync_suspend();
			break;
		default:
			break;
	}

	return NOTIFY_OK;
}

static int __init dfsync_init(void)
{
 	notif.notifier_call = state_notifier_callback;
 	if (state_register_client(&notif))
 		pr_err("Cannot register state notifier callback for fsync.\n");
 		
 	return 0;
}

static void __exit dfsync_exit(void)
{
	state_unregister_client(&notif);
}

module_init(dfsync_init);
module_exit(dfsync_exit);

MODULE_AUTHOR("Tyler Dunn <ireallylikeshrek@gmail.com>");
MODULE_DESCRIPTION("dynamic_fsync v2 - Simplified fsync driver!");
MODULE_LICENSE("GPL v2");
