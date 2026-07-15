/*----------------------------------------------------------------------------*/
// COPYRIGHT FCNT LIMITED 2021
/*----------------------------------------------------------------------------*/
// SPDX-License-Identifier: GPL-2.0

/* FCNT LIMITED:2021-06-01 H21200293 add start */
//==============================================================================
// include file
//==============================================================================
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/suspend.h>

//==============================================================================
// define
//==============================================================================

//==============================================================================
// const data
//==============================================================================


//==============================================================================
// private valuable
//==============================================================================
static struct timespec64 sleep_time_ts;

//==============================================================================
// static functions prototype
//==============================================================================

//==============================================================================
// functions
//==============================================================================

static int sleep_time_log_notification(struct notifier_block *notifier,
			   unsigned long event, void *ptr)
{

	struct rtc_time tm;
	struct timespec64 post_suspend_ts;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		ktime_get_real_ts64(&sleep_time_ts);
		rtc_time64_to_tm(sleep_time_ts.tv_sec, &tm);

		pr_info("PM: suspend entry time %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
			tm.tm_year + 1900, tm.tm_mon + 1,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sleep_time_ts.tv_nsec);
		break;
	case PM_POST_SUSPEND:
		ktime_get_real_ts64(&post_suspend_ts);
		rtc_time64_to_tm(post_suspend_ts.tv_sec, &tm);
		if (sleep_time_ts.tv_sec > post_suspend_ts.tv_sec) {
			sleep_time_ts.tv_sec = 0;
		} else {
			sleep_time_ts.tv_sec = post_suspend_ts.tv_sec - sleep_time_ts.tv_sec;
		}
		pr_info("Suspend entry -> exit %lld sec\n",sleep_time_ts.tv_sec);
		pr_info("PM: suspend exit time %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, post_suspend_ts.tv_nsec);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block sleep_time_log_notifier = {
	.notifier_call = sleep_time_log_notification,
};

static int speep_time_log_init(void)
{
	register_pm_notifier(&sleep_time_log_notifier);
	return 0;
}
module_init(speep_time_log_init);

static void sleep_time_log_exit(void)
{
	unregister_pm_notifier(&sleep_time_log_notifier);
}
module_exit(sleep_time_log_exit);

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:sleep_time_log");
/* FCNT LIMITED:2021-06-01 H21200293 add end */
