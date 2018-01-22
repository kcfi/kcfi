#ifndef _ASM_X86_CONTEXT_TRACKING_H
#define _ASM_X86_CONTEXT_TRACKING_H

#ifdef CONFIG_KCFI
#include <kcfi/kcfi_tags.h>
#ifdef CONFIG_CONTEXT_TRACKING
# define SCHEDULE_USER  call schedule_user; \
			nopl KCFIr_schedule_user;
#else
# define SCHEDULE_USER  call schedule; \
			nopl KCFIr_schedule;
#endif /* CONFIG_CONTEXT_TRACKING */
#else
#ifdef CONFIG_CONTEXT_TRACKING
# define SCHEDULE_USER  call schedule_user;
#else
# define SCHEDULE_USER  call schedule;
#endif /* CONFIG_CONTEXT_TRACKING */
#endif /* CONFIG_KCFI */

#endif /* _ASM_X86_CONTEXT_TRACKING_H */
