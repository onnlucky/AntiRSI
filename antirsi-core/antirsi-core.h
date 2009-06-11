#ifndef _antirsi_core_h_
#define _antirsi_core_h_

/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-core
 * license: GPL
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/** the state antirsi is in */
typedef enum {
  S_NORMAL = 1,
  S_IN_MINI,
  S_IN_WORK
} ai_state;

/** a global context for the core */
typedef struct _ai_core {
    void * user_data;

    double time;

    // breaks
    /** time since last mini break */
    double mini_t;
    /** time the user has been taking a mini break */
    double mini_taking_t;
    /** time since last work break */
    double work_t;
    /** time the user has been taking a work break */
    double work_taking_t;

    // natural break continuation
    /** last #work_taking_t, for natural break continuation */
    double last_work_taking_t;
    /** time when last natural work break was stopped */
    double last_work_taking_t_countdown;

    // settings
    /** the time between mini breaks */
    int mini_interval;
    /** the time a mini break lasts */
    int mini_duration;
    /** the time between work breaks */
    int work_interval;
    /** the time a work break lasts */
    int work_duration;
    /** the time postpone will set #work_t back */
    int postpone_time;

    /** 1 if no mini breaks are to be issues, 0 otherwise */
    int mini_disabled;
    /** 1 if no work breaks are to be issues, 0 otherwise */
    int work_disabled;

    // state
    ai_state state;
    double ith[4]; // history filter

    // library exit functions
    /**
     * when a break is over, this function is called
     *
     * The function will only be called if the function pointer is not NULL (default)
     * the user data set with #antirsi_init is passed to the function
     */
    void (*emit_break_end)(void * data);
    /** when a mini break is starting; see #emit_break_end */
    void (*emit_mini_break_start)(void * data);
    /** when a work break is starting; see #emit_break_end */
    void (*emit_work_break_start)(void * data);
    /** everytime a tick has been processed, and we are in a break, this function is called; see #emit_break_end */
    void (*emit_break_update)(void * data);
    /** everytime a tick has been processed, this function is called; see #emit_break_end */
    void (*emit_status_update)(void * data);

} ai_core;

/** returns the seconds until the next work break */
int ai_seconds_until_next_work_break(ai_core * c);
/** returns the seconds the current break lasts */
int ai_break_time_left(ai_core * c);
/** returns double between 0 and 1, indicating the progress of the break; 0 is just starting, 1 is done */
double ai_break_progress(ai_core * c);
/** will postpone the current break #ai_core->postpone_time */
void ai_work_break_postpone(ai_core *c);
/** will initiate a work break right now */
void ai_work_break_now(ai_core *c);

// natural break continuation
/** 1 if a natural work break continuation is available, 0 otherwise */
int ai_can_continue_natural_break(ai_core *c);
/** continue a natural work break, use this instead of #ai_work_break_now */
void ai_continue_natural_work_break(ai_core *c);

/**
 * makes the core go one step, calculating times spend, idle time and then the new state/times
 * this function will call appropriate functions like #emit_break_end and such when necesairy.
 * Notice that these functions will only be called when the have been set:
 * @example
 * @code
 * static void handle_break_end(void * data){
 *   fprintf(stderr, "handle_break_end\n");
 * }
 *
 * main {
 * ai_core * core;
 * ...
 * core->emit_break_end = handle_break_end;
 * ...
 * }
 * @code
 *
 * provide the current idle time (#idle_time) and antirsi context #ai_core
 */
void ai_tick(ai_core * c, double idle_time);

/**
 * initialize antirsi core, #data will be used in all callbacks the library makes
 */
ai_core * antirsi_init(void * data);

#endif
