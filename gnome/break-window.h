#ifndef _break_window_h_
#define _break_window_h_

/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 */

#include "app.h"

void handle_mini_break_start(gpointer data);
void handle_work_break_start(gpointer data);
void handle_break_end(gpointer data);
void handle_break_update(gpointer data);

void break_window_create(app_context * app);

#endif
