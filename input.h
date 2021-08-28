#ifndef input_h_INCLUDED
#define input_h_INCLUDED

#include <libinput.h>

struct libinput *input_init();
void input_fini(struct libinput *li);

#endif // input_h_INCLUDED
