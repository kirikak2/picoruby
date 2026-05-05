/*
 * PicoRuby UART_MIDI - mrubyc bindings
 */

#include <mrubyc.h>
#include <alloc.h>

#include "../../include/uart_midi.h"

/*
 * UART_MIDI._init(tx_pin, rx_pin, baud)
 */
static void
c_uart_midi_init(mrbc_vm *vm, mrbc_value v[], int argc)
{
    if (argc < 2) {
        SET_INT_RETURN(-1);
        return;
    }

    int tx_pin   = (int)GET_INT_ARG(1);
    int rx_pin   = (int)GET_INT_ARG(2);
    uint32_t baud = 0;
    if (argc >= 3 && v[3].tt == MRBC_TT_INTEGER) {
        baud = (uint32_t)v[3].i;
    }

    int ret = UART_MIDI_init(tx_pin, rx_pin, baud);
    SET_INT_RETURN(ret);
}

/*
 * UART_MIDI._get_status
 */
static void
c_uart_midi_get_status(mrbc_vm *vm, mrbc_value v[], int argc)
{
    uart_midi_status_t status = UART_MIDI_get_status();
    SET_INT_RETURN((int)status);
}

/*
 * UART_MIDI._get_device_info
 */
static void
c_uart_midi_get_device_info(mrbc_vm *vm, mrbc_value v[], int argc)
{
    uart_midi_device_info_t info;

    if (!UART_MIDI_get_device_info(&info)) {
        SET_NIL_RETURN();
        return;
    }

    mrbc_value hash = mrbc_hash_new(vm, 4);

    mrbc_value key_baud = mrbc_symbol_value(mrbc_str_to_symid("baud_rate"));
    mrbc_value val_baud = mrbc_integer_value(info.baud_rate);
    mrbc_hash_set(&hash, &key_baud, &val_baud);

    mrbc_value key_uart = mrbc_symbol_value(mrbc_str_to_symid("uart_num"));
    mrbc_value val_uart = mrbc_integer_value(info.uart_num);
    mrbc_hash_set(&hash, &key_uart, &val_uart);

    mrbc_value key_tx = mrbc_symbol_value(mrbc_str_to_symid("tx_pin"));
    mrbc_value val_tx = mrbc_integer_value(info.tx_pin);
    mrbc_hash_set(&hash, &key_tx, &val_tx);

    mrbc_value key_rx = mrbc_symbol_value(mrbc_str_to_symid("rx_pin"));
    mrbc_value val_rx = mrbc_integer_value(info.rx_pin);
    mrbc_hash_set(&hash, &key_rx, &val_rx);

    SET_RETURN(hash);
}

/*
 * UART_MIDI._send_packet(cable, cin, midi1, midi2, midi3)
 */
static void
c_uart_midi_send_packet(mrbc_vm *vm, mrbc_value v[], int argc)
{
    if (argc != 5) {
        SET_INT_RETURN(-1);
        return;
    }

    uint8_t cable = (uint8_t)GET_INT_ARG(1);
    uint8_t cin   = (uint8_t)GET_INT_ARG(2);
    uint8_t midi1 = (uint8_t)GET_INT_ARG(3);
    uint8_t midi2 = (uint8_t)GET_INT_ARG(4);
    uint8_t midi3 = (uint8_t)GET_INT_ARG(5);

    int ret = UART_MIDI_send_packet(cable, cin, midi1, midi2, midi3);
    SET_INT_RETURN(ret);
}

/*
 * UART_MIDI._input_start
 */
static void
c_uart_midi_input_start(mrbc_vm *vm, mrbc_value v[], int argc)
{
    int ret = UART_MIDI_Input_start();
    SET_INT_RETURN(ret);
}

/*
 * UART_MIDI._input_stop
 */
static void
c_uart_midi_input_stop(mrbc_vm *vm, mrbc_value v[], int argc)
{
    UART_MIDI_Input_stop();
    SET_NIL_RETURN();
}

/*
 * UART_MIDI._input_is_running
 */
static void
c_uart_midi_input_is_running(mrbc_vm *vm, mrbc_value v[], int argc)
{
    bool running = UART_MIDI_Input_is_running();
    SET_BOOL_RETURN(running);
}

/*
 * Gem initialization
 */
void
mrbc_uart_midi_init(mrbc_vm *vm)
{
    mrbc_class *class_UART_MIDI =
        mrbc_define_class(vm, "UART_MIDI", mrbc_class_object);

    /* Status constants */
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("NOT_INITIALIZED"),
                         &mrbc_integer_value(UART_MIDI_NOT_INITIALIZED));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("READY"),
                         &mrbc_integer_value(UART_MIDI_READY));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("ERROR"),
                         &mrbc_integer_value(UART_MIDI_ERROR));

    /* Default baud */
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("DEFAULT_BAUD_RATE"),
                         &mrbc_integer_value(UART_MIDI_DEFAULT_BAUD_RATE));

    /* CIN constants */
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_NOTE_OFF"),
                         &mrbc_integer_value(UART_MIDI_CIN_NOTE_OFF));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_NOTE_ON"),
                         &mrbc_integer_value(UART_MIDI_CIN_NOTE_ON));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_CONTROL_CHANGE"),
                         &mrbc_integer_value(UART_MIDI_CIN_CONTROL_CHANGE));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_PROGRAM_CHANGE"),
                         &mrbc_integer_value(UART_MIDI_CIN_PROGRAM_CHANGE));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_PITCH_BEND"),
                         &mrbc_integer_value(UART_MIDI_CIN_PITCH_BEND));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_SINGLE_BYTE"),
                         &mrbc_integer_value(UART_MIDI_CIN_SINGLE_BYTE));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_SYSEX_START"),
                         &mrbc_integer_value(UART_MIDI_CIN_SYSEX_START));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_SYSEX_END_2"),
                         &mrbc_integer_value(UART_MIDI_CIN_SYSEX_END_2));
    mrbc_set_class_const(class_UART_MIDI,
                         mrbc_str_to_symid("CIN_SYSEX_END_3"),
                         &mrbc_integer_value(UART_MIDI_CIN_SYSEX_END_3));

    /* Methods */
    mrbc_define_method(vm, class_UART_MIDI, "_init",             c_uart_midi_init);
    mrbc_define_method(vm, class_UART_MIDI, "_get_status",       c_uart_midi_get_status);
    mrbc_define_method(vm, class_UART_MIDI, "_get_device_info",  c_uart_midi_get_device_info);
    mrbc_define_method(vm, class_UART_MIDI, "_send_packet",      c_uart_midi_send_packet);
    mrbc_define_method(vm, class_UART_MIDI, "_input_start",      c_uart_midi_input_start);
    mrbc_define_method(vm, class_UART_MIDI, "_input_stop",       c_uart_midi_input_stop);
    mrbc_define_method(vm, class_UART_MIDI, "_input_is_running", c_uart_midi_input_is_running);
}
