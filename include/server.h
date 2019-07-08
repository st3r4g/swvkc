#ifndef MYSERVER_H
#define MYSERVER_H

#include <extensions/xdg_shell/xdg_surface.h>
struct server;

void server_request_redraw(struct server *server);
void server_window_create(struct server *server, struct xdg_surface0
*xdg_surface);
void server_window_destroy(struct server *server, struct xdg_surface0
*xdg_surface);
struct screen *server_get_screen(struct server *server);

#endif
