/*
 * PicoRuby UART_MIDI - mruby bindings
 */

#include <mruby.h>
#include <mruby/presym.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/string.h>
#include <mruby/hash.h>

#include "../../include/uart_midi.h"

/*
 * UART_MIDI._init(tx_pin, rx_pin, baud=0)
 */
static mrb_value
mrb_uart_midi_init(mrb_state *mrb, mrb_value self)
{
    mrb_int tx_pin, rx_pin;
    mrb_int baud = 0;
    mrb_get_args(mrb, "ii|i", &tx_pin, &rx_pin, &baud);

    int ret = UART_MIDI_init((int)tx_pin, (int)rx_pin, (uint32_t)baud);
    return mrb_fixnum_value(ret);
}

static mrb_value
mrb_uart_midi_get_status(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value((mrb_int)UART_MIDI_get_status());
}

static mrb_value
mrb_uart_midi_get_device_info(mrb_state *mrb, mrb_value self)
{
    uart_midi_device_info_t info;
    if (!UART_MIDI_get_device_info(&info)) return mrb_nil_value();

    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_symbol_value(MRB_SYM(baud_rate)),
                 mrb_fixnum_value(info.baud_rate));
    mrb_hash_set(mrb, hash, mrb_symbol_value(MRB_SYM(uart_num)),
                 mrb_fixnum_value(info.uart_num));
    mrb_hash_set(mrb, hash, mrb_symbol_value(MRB_SYM(tx_pin)),
                 mrb_fixnum_value(info.tx_pin));
    mrb_hash_set(mrb, hash, mrb_symbol_value(MRB_SYM(rx_pin)),
                 mrb_fixnum_value(info.rx_pin));
    return hash;
}

static mrb_value
mrb_uart_midi_send_packet(mrb_state *mrb, mrb_value self)
{
    mrb_int cable, cin, midi1, midi2, midi3;
    mrb_get_args(mrb, "iiiii", &cable, &cin, &midi1, &midi2, &midi3);

    int ret = UART_MIDI_send_packet(
        (uint8_t)cable, (uint8_t)cin,
        (uint8_t)midi1, (uint8_t)midi2, (uint8_t)midi3);
    return mrb_fixnum_value(ret);
}

static mrb_value
mrb_uart_midi_input_start(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value(UART_MIDI_Input_start());
}

static mrb_value
mrb_uart_midi_input_stop(mrb_state *mrb, mrb_value self)
{
    UART_MIDI_Input_stop();
    return mrb_nil_value();
}

static mrb_value
mrb_uart_midi_input_is_running(mrb_state *mrb, mrb_value self)
{
    return mrb_bool_value(UART_MIDI_Input_is_running());
}

void
mrb_picoruby_uart_midi_gem_init(mrb_state *mrb)
{
    struct RClass *class_UART_MIDI =
        mrb_define_class_id(mrb, MRB_SYM(UART_MIDI), mrb->object_class);

    /* Status constants */
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(NOT_INITIALIZED),
                        mrb_fixnum_value(UART_MIDI_NOT_INITIALIZED));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(READY),
                        mrb_fixnum_value(UART_MIDI_READY));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(ERROR),
                        mrb_fixnum_value(UART_MIDI_ERROR));

    /* Default baud */
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(DEFAULT_BAUD_RATE),
                        mrb_fixnum_value(UART_MIDI_DEFAULT_BAUD_RATE));

    /* CIN constants */
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_NOTE_OFF),
                        mrb_fixnum_value(UART_MIDI_CIN_NOTE_OFF));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_NOTE_ON),
                        mrb_fixnum_value(UART_MIDI_CIN_NOTE_ON));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_CONTROL_CHANGE),
                        mrb_fixnum_value(UART_MIDI_CIN_CONTROL_CHANGE));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_PROGRAM_CHANGE),
                        mrb_fixnum_value(UART_MIDI_CIN_PROGRAM_CHANGE));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_PITCH_BEND),
                        mrb_fixnum_value(UART_MIDI_CIN_PITCH_BEND));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_SINGLE_BYTE),
                        mrb_fixnum_value(UART_MIDI_CIN_SINGLE_BYTE));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_SYSEX_START),
                        mrb_fixnum_value(UART_MIDI_CIN_SYSEX_START));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_SYSEX_END_2),
                        mrb_fixnum_value(UART_MIDI_CIN_SYSEX_END_2));
    mrb_define_const_id(mrb, class_UART_MIDI, MRB_SYM(CIN_SYSEX_END_3),
                        mrb_fixnum_value(UART_MIDI_CIN_SYSEX_END_3));

    /* Methods */
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_init),
                         mrb_uart_midi_init, MRB_ARGS_ARG(2, 1));
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_get_status),
                         mrb_uart_midi_get_status, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_get_device_info),
                         mrb_uart_midi_get_device_info, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_send_packet),
                         mrb_uart_midi_send_packet, MRB_ARGS_REQ(5));
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_input_start),
                         mrb_uart_midi_input_start, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_input_stop),
                         mrb_uart_midi_input_stop, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_UART_MIDI, MRB_SYM(_input_is_running),
                         mrb_uart_midi_input_is_running, MRB_ARGS_NONE());
}

void
mrb_picoruby_uart_midi_gem_final(mrb_state *mrb)
{
    UART_MIDI_deinit();
}
