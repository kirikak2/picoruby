/*
 * PicoRuby UART_MIDI - VM selection wrapper
 */

#include "../include/uart_midi.h"

#if defined(PICORB_VM_MRUBY)
  #include "mruby/uart_midi.c"
#elif defined(PICORB_VM_MRUBYC)
  #include "mrubyc/uart_midi.c"
#endif
