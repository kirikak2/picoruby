/*
 * PicoRuby USB-MIDI - VM selection wrapper
 */

#include "../include/usb_midi_host.h"

#if defined(PICORB_VM_MRUBY)
  #include "mruby/usb_midi_host.c"
#elif defined(PICORB_VM_MRUBYC)
  #include "mrubyc/usb_midi_host.c"
#endif
