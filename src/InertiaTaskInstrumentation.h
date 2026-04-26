#ifndef _INERTIA_TASK_INSTRUMENTATION_INCLUDE_h
#define _INERTIA_TASK_INSTRUMENTATION_INCLUDE_h

// Required settings for TS::Task instrumentation to enable task IDs, debug, and status request features for profiling. Must be included before any TS::Task usage.
#define _TASK_WDT_IDS
#define _TASK_DEBUG
#define _TASK_STATUS_REQUEST

// OO Callback is required for Inertia Task components.
#if !defined(_TASK_OO_CALLBACKS)
#define _TASK_OO_CALLBACKS
#endif

// Sleep on idle run should not be enabled for profiling, as it can interfere with accurate task state measurement. Ensure it is disabled.
#if defined(_TASK_SLEEP_ON_IDLE_RUN)
#undef _TASK_SLEEP_ON_IDLE_RUN
#endif

#endif
