/*!\file sched-tp.c
 * \author Philippe Gerum
 * \brief Temporal partitioning (typical of IMA systems).
 *
 * Copyright (C) 2008 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * \ingroup sched
 */

#include <nucleus/pod.h>

static void tp_schedule_next(struct xnsched_tp *tp)
{
	struct xnsched_tp_window *w;
	struct xnsched *sched;
	int p_next, ret;
	xnticks_t t;

	for (;;) {
		/*
		 * Switch to the next partition. Time holes in a
		 * global time frame are defined as partition windows
		 * assigned to part# -1, in which case the (always
		 * empty) idle queue will be polled for runnable
		 * threads.  Therefore, we may assume that a window
		 * begins immediately after the previous one ends,
		 * which simplifies the implementation a lot.
		 */
		w = &tp->gps->pwins[tp->wnext];
		p_next = w->w_part;
		tp->tps = p_next < 0 ? &tp->idle : &tp->partitions[p_next];

		/* Schedule tick to advance to the next window. */
		tp->wnext = (tp->wnext + 1) % tp->gps->pwin_nr;
		w = &tp->gps->pwins[tp->wnext];
		t = tp->tf_start + w->w_offset;

		ret = xntimer_start(&tp->tf_timer, t, XN_INFINITE, XN_ABSOLUTE);
		if (ret != -ETIMEDOUT)
			break;
	}

	sched = container_of(tp, struct xnsched, tp);
	xnsched_set_resched(sched);
}

static void tp_tick_handler(struct xntimer *timer)
{
	struct xnsched_tp *tp = container_of(timer, struct xnsched_tp, tf_timer);
	/*
	 * Advance beginning date of time frame by a full period if we
	 * are processing the last window.
	 */
	if (tp->wnext + 1 == tp->gps->pwin_nr)
		tp->tf_start += tp->gps->tf_duration;

	tp_schedule_next(tp);
}

static void xnsched_tp_init(struct xnsched *sched)
{
	struct xnsched_tp *tp = &sched->tp;
	int n;

	/*
	 * Build the runqueues. Thread priorities for the TP policy
	 * are the same as RT priorities. TP is actually a superset of
	 * RT.
	 */
	for (n = 0; n < CONFIG_XENO_OPT_SCHED_TP_NRPART; n++)
		sched_initpq(&tp->partitions[n].runnable,
			     XNSCHED_RT_MIN_PRIO, XNSCHED_RT_MAX_PRIO);
	sched_initpq(&tp->idle.runnable,
		     XNSCHED_RT_MIN_PRIO, XNSCHED_RT_MAX_PRIO);

	tp->tps = NULL;
	tp->gps = NULL;
	initq(&tp->threads);
	xntimer_init_noblock(&tp->tf_timer, &nktbase, tp_tick_handler);
	xntimer_set_name(&tp->tf_timer, "tp-tick");
}

static void xnsched_tp_setparam(struct xnthread *thread,
				const union xnsched_policy_param *p)
{
	struct xnsched *sched = thread->sched;

	thread->tps = &sched->tp.partitions[p->tp.ptid];
	thread->cprio = p->tp.prio;
}

static void xnsched_tp_getparam(struct xnthread *thread,
				union xnsched_policy_param *p)
{
	p->tp.prio = thread->cprio;
	p->tp.ptid = thread->tps - thread->sched->tp.partitions;
}

static void xnsched_tp_trackprio(struct xnthread *thread,
				 const union xnsched_policy_param *p)
{
	/*
	 * The assigned partition never changes internally due to PIP
	 * (see xnsched_track_policy), since this would be pretty
	 * wrong with respect to TP scheduling: i.e. we may not allow
	 * a thread from another partition to consume CPU time from
	 * the current one, despite this would help enforcing PIP
	 * (*). In any case, introducing resource contention between
	 * threads that belong to different partitions is utterly
	 * wrong in the first place.  Only an explicit call to
	 * xnsched_set_policy() may change the partition assigned to a
	 * thread. For that reason, a policy reset action only boils
	 * down to reinstating the base priority.
	 *
	 * (*) However, we do allow threads from lower scheduling
	 * classes to consume CPU time from the current window as a
	 * result of a PIP boost, since this is aimed at speeding up
	 * the release of a synchronization object a TP thread needs.
	 */
	if (p) {
		/* We should never cross partition boundaries. */
		XENO_BUGON(NUCLEUS,
			   thread->base_class == &xnsched_class_tp &&
			   thread->tps - thread->sched->tp.partitions != p->tp.ptid);
		thread->cprio = p->tp.prio;
	} else
		thread->cprio = thread->bprio;
}

static int xnsched_tp_declare(struct xnthread *thread,
			      const union xnsched_policy_param *p)
{
	struct xnsched *sched = thread->sched;

	if (p->tp.prio < XNSCHED_RT_MIN_PRIO ||
	    p->tp.prio > XNSCHED_RT_MAX_PRIO)
		return -EINVAL;

	appendq(&sched->tp.threads, &thread->tp_link);
	/*
	 * RPI makes no sense with temporal partitioning, since
	 * resources obtained from Linux should have been
	 * pre-allocated by the application before entering real-time
	 * duties, so that timings remain accurate. As a consequence
	 * of this, the reason to have priority inheritance for the
	 * root thread disappears.
	 */
	xnthread_set_state(thread, XNRPIOFF);

	return 0;
}

static void xnsched_tp_forget(struct xnthread *thread)
{
	thread->tps = NULL;
	removeq(&thread->sched->tp.threads, &thread->tp_link);
}

static void xnsched_tp_enqueue(struct xnthread *thread)
{
	sched_insertpqf(&thread->tps->runnable,
			&thread->rlink, thread->cprio);
}

static void xnsched_tp_dequeue(struct xnthread *thread)
{
	sched_removepq(&thread->tps->runnable, &thread->rlink);
}

static void xnsched_tp_requeue(struct xnthread *thread)
{
	sched_insertpql(&thread->tps->runnable,
			&thread->rlink, thread->cprio);
}

static struct xnthread *xnsched_tp_pick(struct xnsched *sched)
{
	struct xnpholder *h;

	/* Never pick a thread if we don't schedule partitions. */
	if (!xntimer_running_p(&sched->tp.tf_timer))
		return NULL;

	h = sched_getpq(&sched->tp.tps->runnable);

	return h ? link2thread(h, rlink) : NULL;
}

static void xnsched_tp_migrate(struct xnthread *thread, struct xnsched *sched)
{
	union xnsched_policy_param param;
	/*
	 * Since our partition schedule is a per-scheduler property,
	 * it cannot apply to a thread that moves to another CPU
	 * anymore. So we downgrade that thread to the RT class when a
	 * CPU migration occurs. A subsequent call to
	 * xnsched_set_policy() may move it back to TP scheduling,
	 * with a partition assignment that fits the remote CPU's
	 * partition schedule.
	 */
	param.rt.prio = thread->cprio;
	xnsched_set_policy(thread, &xnsched_class_rt, &param);
}

void xnsched_tp_start_schedule(struct xnsched *sched)
{
	struct xnsched_tp *tp = &sched->tp;

	tp->wnext = 0;
	tp->tf_start = xnpod_get_cpu_time();
	tp_schedule_next(&sched->tp);
}
EXPORT_SYMBOL_GPL(xnsched_tp_start_schedule);

void xnsched_tp_stop_schedule(struct xnsched *sched)
{
	struct xnsched_tp *tp = &sched->tp;
	xntimer_stop(&tp->tf_timer);
}
EXPORT_SYMBOL_GPL(xnsched_tp_stop_schedule);

struct xnsched_tp_schedule *
xnsched_tp_set_schedule(struct xnsched *sched,
			struct xnsched_tp_schedule *gps)
{
	struct xnsched_tp_schedule *old_gps;
	struct xnsched_tp *tp = &sched->tp;
	union xnsched_policy_param param;
	struct xnthread *thread;
	struct xnholder *h;

	XENO_BUGON(NUCLEUS, gps != NULL &&
		    (gps->pwin_nr <= 0 || gps->pwins[0].w_offset != 0));

	xnsched_tp_stop_schedule(sched);

	/*
	 * Move all TP threads on this scheduler to the RT class,
	 * until we call xnsched_set_policy() for them again.
	 */
	while ((h = getq(&tp->threads)) != NULL) {
		thread = link2thread(h, tp_link);
		param.rt.prio = thread->cprio;
		xnsched_set_policy(thread, &xnsched_class_rt, &param);
	}

	old_gps = tp->gps;
	tp->gps = gps;

	return old_gps;
}
EXPORT_SYMBOL_GPL(xnsched_tp_set_schedule);

int xnsched_tp_get_partition(struct xnsched *sched)
{
	struct xnsched_tp *tp = &sched->tp;

	if (tp->tps == NULL || tp->tps == &tp->idle)
		return -1;

	return tp->tps - tp->partitions;
}
EXPORT_SYMBOL_GPL(xnsched_tp_get_partition);

#ifdef CONFIG_PROC_FS

#include <linux/seq_file.h>

struct xnsched_tp_seq_iterator {
	xnticks_t start_time;
	int nentries;
	struct xnsched_tp_info {
		int cpu;
		pid_t pid;
		char name[XNOBJECT_NAME_LEN];
		int prio;
		int ptid;
	} sched_info[1];
};

static void *xnsched_tp_seq_start(struct seq_file *seq, loff_t *pos)
{
	struct xnsched_tp_seq_iterator *iter = seq->private;

	if (*pos > iter->nentries)
		return NULL;

	if (*pos == 0)
		return SEQ_START_TOKEN;

	return iter->sched_info + *pos - 1;
}

static void *xnsched_tp_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct xnsched_tp_seq_iterator *iter = seq->private;

	++*pos;

	if (*pos > iter->nentries)
		return NULL;

	return iter->sched_info + *pos - 1;
}

static void xnsched_tp_seq_stop(struct seq_file *seq, void *v)
{
}

static int xnsched_tp_seq_show(struct seq_file *seq, void *v)
{
	struct xnsched_tp_info *p;

	if (v == SEQ_START_TOKEN)
		seq_printf(seq, "%-3s  %-6s %-4s %-4s  %s\n",
			   "CPU", "PID", "PTID", "PRI", "NAME");
	else {
		p = v;

		seq_printf(seq, "%3u  %-6d %-4d %-4d  %s\n",
			   p->cpu,
			   p->pid,
			   p->ptid,
			   p->prio,
			   p->name);
	}

	return 0;
}

static struct seq_operations xnsched_tp_seq_op = {
	.start = &xnsched_tp_seq_start,
	.next = &xnsched_tp_seq_next,
	.stop = &xnsched_tp_seq_stop,
	.show = &xnsched_tp_seq_show
};

static int xnsched_tp_seq_open(struct inode *inode, struct file *file)
{
	struct xnsched_tp_seq_iterator *iter = NULL;
	struct xnsched_tp_info *p;
	struct xnholder *holder;
	struct xnthread *thread;
	int ret, count, rev, n;
	struct seq_file *seq;
	spl_t s;

	if (!xnpod_active_p())
		return -ESRCH;

	xnlock_get_irqsave(&nklock, s);

      restart:
	rev = nkpod->threadq_rev;
	count = xnsched_class_tp.nthreads;
	holder = getheadq(&nkpod->threadq);

	xnlock_put_irqrestore(&nklock, s);

	if (iter)
		kfree(iter);

	if (count == 0)
		return -ESRCH;

	iter = kmalloc(sizeof(*iter)
		       + (count - 1) * sizeof(struct xnsched_tp_info),
		       GFP_KERNEL);
	if (iter == NULL)
		return -ENOMEM;

	ret = seq_open(file, &xnsched_tp_seq_op);
	if (ret) {
		kfree(iter);
		return ret;
	}

	iter->nentries = 0;
	iter->start_time = xntbase_get_jiffies(&nktbase);

	while (holder) {
		xnlock_get_irqsave(&nklock, s);

		if (nkpod->threadq_rev != rev)
			goto restart;

		rev = nkpod->threadq_rev;
		thread = link2thread(holder, glink);

		if (thread->base_class != &xnsched_class_tp)
			goto skip;

		n = iter->nentries++;
		p = iter->sched_info + n;
		p->cpu = xnsched_cpu(thread->sched);
		p->pid = xnthread_user_pid(thread);
		memcpy(p->name, thread->name, sizeof(p->name));
		p->ptid = thread->tps - thread->sched->tp.partitions;
		p->prio = thread->cprio;
	skip:
		holder = nextq(&nkpod->threadq, holder);
		xnlock_put_irqrestore(&nklock, s);
	}

	seq = file->private_data;
	seq->private = iter;

	return 0;
}

static struct file_operations xnsched_tp_seq_operations = {
	.owner = THIS_MODULE,
	.open = xnsched_tp_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release_private,
};

void xnsched_tp_init_proc(struct proc_dir_entry *root)
{
	rthal_add_proc_seq("threads", &xnsched_tp_seq_operations, 0, root);
}

void xnsched_tp_cleanup_proc(struct proc_dir_entry *root)
{
	remove_proc_entry("threads", root);
}

#endif /* CONFIG_PROC_FS */

struct xnsched_class xnsched_class_tp = {

	.sched_init		=	xnsched_tp_init,
	.sched_enqueue		=	xnsched_tp_enqueue,
	.sched_dequeue		=	xnsched_tp_dequeue,
	.sched_requeue		=	xnsched_tp_requeue,
	.sched_pick		=	xnsched_tp_pick,
	.sched_tick		=	NULL,
	.sched_rotate		=	NULL,
	.sched_migrate		=	xnsched_tp_migrate,
	.sched_setparam		=	xnsched_tp_setparam,
	.sched_getparam		=	xnsched_tp_getparam,
	.sched_trackprio	=	xnsched_tp_trackprio,
	.sched_declare		=	xnsched_tp_declare,
	.sched_forget		=	xnsched_tp_forget,
#ifdef CONFIG_PROC_FS
	.sched_init_proc	=	xnsched_tp_init_proc,
	.sched_cleanup_proc	=	xnsched_tp_cleanup_proc,
#endif
	.weight			=	XNSCHED_CLASS_WEIGHT(2),
	.name			=	"tp"
};
EXPORT_SYMBOL_GPL(xnsched_class_tp);
