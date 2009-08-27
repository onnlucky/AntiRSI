/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-core
 * license: GPL
 *
 * TODO
 * mini or work break should both be allowed to be disabled using extra boolean ...
 *
 * emit lock focus and have setting for it
 *
 * implement a smarter algorithm for setting and predicting breaks,
 * maybe include scheduling static events, like tee or food and end of work day ...
 */

#include "antirsi-core.h"

int
ai_seconds_until_next_work_break(ai_core * c)
{
  if (c->state == S_IN_WORK) {
    return c->work_duration + c->work_interval - c->work_taking_t;
  }

  return c->work_interval - c->work_t;
}

int
ai_break_time_left(ai_core * c)
{
  switch (c->state) {
    case S_IN_MINI:
      return c->mini_duration - c->mini_taking_t;
    case S_IN_WORK:
      return c->work_duration - c->work_taking_t;
    default:
      return 0;
  }
}

double
ai_break_progress(ai_core * c)
{
  switch (c->state) {
    case S_IN_MINI:
      return 1.0 * c->mini_taking_t / c->mini_duration;
    case S_IN_WORK:
      return 1.0 * c->work_taking_t / c->work_duration;
    default:
      return 0.0;
  }
}

void
ai_work_break_postpone(ai_core *c)
{
  c->mini_t = 0;
  c->mini_taking_t = 0;

  c->work_t = c->work_interval - c->postpone_time;
  if (c->work_t < 0) c->work_t = 0;
  c->work_taking_t = 0;

  c->state = S_NORMAL;

  if (c->emit_break_end) c->emit_break_end(c->user_data);
}

void
ai_work_break_now(ai_core *c)
{
  // implicit natural work break, shouldn't use it, but for older clients
  if (c->last_work_taking_t_countdown > 0 && c->work_taking_t < c->last_work_taking_t) {
    c->work_taking_t = c->last_work_taking_t;
  }

  c->state = S_IN_WORK;

  if (c->emit_work_break_start) c->emit_work_break_start(c->user_data);
}

int
ai_can_continue_natural_break(ai_core *c)
{
    return c->last_work_taking_t_countdown > 0;
}

void
ai_continue_natural_work_break(ai_core *c)
{
  if (c->work_taking_t < c->last_work_taking_t) {
    c->work_taking_t = c->last_work_taking_t;
  }
  c->state = S_IN_WORK;

  if (c->emit_work_break_start) c->emit_work_break_start(c->user_data);
}

void
ai_tick(ai_core * c, double idle_time)
{
  double new_time;
  double delta;

  struct timeval tv;
  int slack;

  if (gettimeofday(&tv, 0) != 0) {
    fprintf(stderr, "no gettimeofday\n");
    return;
  }

  new_time = tv.tv_sec + (tv.tv_usec / 1000000.0);
  delta = new_time - c->time;

  // too fast, or initial run
  if (delta < 0.0001) {
    fprintf(stderr, "going too fast! %f - %f = %f\n", new_time, c->time, delta);
    return;
  }

  c->time = new_time;

  //fprintf(stderr, "fps: %0.2f\n", 1.0/delta);

  // if there was a long sleep, handle that first
  if (delta > c->work_duration) {
    c->mini_t = 0;
    c->mini_taking_t = c->mini_duration;

    c->work_t = 0;
    c->work_taking_t = c->work_duration;

    if (c->state != S_NORMAL) {
      if (c->emit_break_end) c->emit_break_end(c->user_data);
    }

    // do stuff on next tick
    return;

  } else if (delta > c->mini_duration) {
    c->mini_t = 0;
    c->mini_taking_t = 0;

    if (c->state != S_NORMAL) {
      if (c->emit_break_end) c->emit_break_end(c->user_data);
    }

    // do stuff on next tick
    return;
  }

  // process idle time with a 4 history filter; at least four discrete events within 15 seconds
  // prevents media players and such to activate antirsi (does not prevent accidental mouse moving or similar!)
  if (c->ith[0] >= idle_time) {
    // new event
    c->ith[3] = c->ith[2];
    c->ith[2] = c->ith[1];
    c->ith[1] = c->ith[0];
  }
  c->ith[0] = idle_time;

  slack = (c->ith[3] + c->ith[2] + c->ith[1] + c->ith[0]) > 15;

  switch (c->state) {
    case S_NORMAL:
        // count down the natural work break validity
        if (c->last_work_taking_t_countdown > 0) {
          c->last_work_taking_t_countdown -= delta;
        }

        if (idle_time <= c->mini_duration * 0.3 && !slack) {
          // the normal case, no slack, and not enough idle time
          c->mini_t += delta;
          c->mini_taking_t = 0;

          c->work_t += delta;
          c->work_taking_t = 0;
        } else {
          // there is either slack or idle time, so we are taking some natural break

          if (c->work_taking_t >= c->work_duration) {

            // idle time has passed work break
            c->mini_t = 0;

            c->work_t = 0;
            c->work_taking_t = c->work_duration;

            // natural work break no longer valid
            c->last_work_taking_t_countdown = 0;

          } else if (c->mini_taking_t >= c->mini_duration && c->work_t > 0) {

            // idle time has passed mini break
            c->mini_t = 0;
            c->mini_taking_t = c->mini_duration;
            c->work_taking_t += delta;

            // 30 second window for user to activate natural break continuation
            c->last_work_taking_t_countdown = 30;
            c->last_work_taking_t = c->work_taking_t;

          } else {
            // idle time has just kicked in
            c->mini_taking_t += delta;

            c->work_t += delta;
            c->work_taking_t = 0;
          }
        }

        if (c->mini_t >= c->mini_interval) {
          // need to take a mini break
          if (c->work_t > c->work_interval - (c->mini_interval / 2)) {

            c->state = S_IN_WORK;
            c->work_t = c->work_interval;

            if (c->emit_work_break_start) c->emit_work_break_start(c->user_data);

          } else {

            c->state = S_IN_MINI;

            if (c->emit_mini_break_start) c->emit_mini_break_start(c->user_data);

          }
        }

        if (c->work_t >= c->work_interval) {
          // need to take a work break
          c->mini_t = 0;
          c->mini_taking_t = c->mini_duration;
          c->state = S_IN_WORK;

          if (c->emit_work_break_start) c->emit_work_break_start(c->user_data);
        }

        break;

    case S_IN_MINI:
        // natural work break no longer valid
        c->last_work_taking_t_countdown = 0;

        c->mini_taking_t += delta;
        c->work_t += delta;

        if (idle_time < 1 && !slack) { // TODO remove the idle_time ?
          // we weren't breaking, so reset the break; 
          c->mini_taking_t = 0;
        }

        // TODO emit redraws
        // TODO if (c->lock_focus)

        if (c->mini_taking_t > c->mini_duration) {
          // mini break is over
          c->mini_t = 0;
          c->mini_taking_t = c->mini_duration;

          if (c->emit_break_end) c->emit_break_end(c->user_data);

          c->state = S_NORMAL;
        }

        if (c->work_t >= c->work_interval) {
          // work break should start
          c->mini_t = 0;
          c->mini_taking_t = c->mini_duration;

          if (c->emit_work_break_start) c->emit_work_break_start(c->user_data);

          c->state = S_IN_WORK;
        }

        if (c->emit_break_update) c->emit_break_update(c->user_data);

      break;

    case S_IN_WORK:
        // natural work break no longer valid
        c->last_work_taking_t_countdown = 0;

        if (idle_time >= 4 || slack) {
          // only when idle for 4 seconds, or there is some slack
          c->work_taking_t += delta;
        }

        // TODO emit redraws
        // TODO if (c->lock_focus)

        if (c->work_taking_t > c->work_duration) {
          // work break is over
          c->mini_t = 0;
          c->mini_taking_t = c->mini_duration;

          c->work_t = 0;
          c->work_taking_t = c->work_duration;

          if (c->emit_break_end) c->emit_break_end(c->user_data);

          c->state = S_NORMAL;
        }

        if (c->emit_break_update) c->emit_break_update(c->user_data);

      break;
  }

  if (c->emit_status_update) c->emit_status_update(c->user_data);
}

ai_core *
antirsi_init(void * data)
{
  ai_core * c = malloc(sizeof(ai_core));
  memset(c, 0, sizeof(ai_core));

  c->user_data = data;

  c->time = time(0);

  // default settings
  c->mini_duration = 14;
  c->mini_interval = 4*60;
  c->work_duration = 8*60;
  c->work_interval = 50*60;
  c->postpone_time = 10*60;

  c->state = S_NORMAL;

  return c;
}

