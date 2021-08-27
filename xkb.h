#ifndef xkb_h_INCLUDED
#define xkb_h_INCLUDED

void xkb_init();
int32_t xkb_get_keymap_fd();
uint32_t xkb_get_keymap_size();
void xkb_update(uint32_t key, int state_);
void xkb_get_modifiers(unsigned int *mods_depressed, unsigned int *mods_latched, unsigned int *mods_locked, unsigned int *group);
int xkb_test_ctrlalt();

#endif // xkb_h_INCLUDED

