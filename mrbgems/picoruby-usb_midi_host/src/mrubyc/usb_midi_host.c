/*
 * PicoRuby USB-MIDI - mrubyc bindings
 */

#include <mrubyc.h>
#include <alloc.h>

#include "../../include/usb_midi_host.h"

/*
 * USB_MIDI_HOST._init
 */
static void
c_usb_midi_init(mrbc_vm *vm, mrbc_value v[], int argc)
{
    int ret = USB_MIDI_HOST_init();
    SET_INT_RETURN(ret);
}

/*
 * USB_MIDI_HOST._get_status
 */
static void
c_usb_midi_get_status(mrbc_vm *vm, mrbc_value v[], int argc)
{
    usb_midi_host_status_t status = USB_MIDI_HOST_get_status();
    SET_INT_RETURN((int)status);
}

/*
 * USB_MIDI_HOST._get_device_info
 */
static void
c_usb_midi_get_device_info(mrbc_vm *vm, mrbc_value v[], int argc)
{
    usb_midi_host_device_info_t info;

    if (!USB_MIDI_HOST_get_device_info(&info)) {
        SET_NIL_RETURN();
        return;
    }

    mrbc_value hash = mrbc_hash_new(vm, 6);

    mrbc_value key_vendor = mrbc_symbol_value(mrbc_str_to_symid("vendor_id"));
    mrbc_value val_vendor = mrbc_integer_value(info.vendor_id);
    mrbc_hash_set(&hash, &key_vendor, &val_vendor);

    mrbc_value key_product_id = mrbc_symbol_value(mrbc_str_to_symid("product_id"));
    mrbc_value val_product_id = mrbc_integer_value(info.product_id);
    mrbc_hash_set(&hash, &key_product_id, &val_product_id);

    mrbc_value key_manufacturer = mrbc_symbol_value(mrbc_str_to_symid("manufacturer"));
    mrbc_value val_manufacturer = mrbc_string_new_cstr(vm, info.manufacturer);
    mrbc_hash_set(&hash, &key_manufacturer, &val_manufacturer);

    mrbc_value key_product = mrbc_symbol_value(mrbc_str_to_symid("product"));
    mrbc_value val_product = mrbc_string_new_cstr(vm, info.product);
    mrbc_hash_set(&hash, &key_product, &val_product);

    mrbc_value key_midi_in = mrbc_symbol_value(mrbc_str_to_symid("midi_in_ep"));
    mrbc_value val_midi_in = mrbc_integer_value(info.midi_in_ep);
    mrbc_hash_set(&hash, &key_midi_in, &val_midi_in);

    mrbc_value key_midi_out = mrbc_symbol_value(mrbc_str_to_symid("midi_out_ep"));
    mrbc_value val_midi_out = mrbc_integer_value(info.midi_out_ep);
    mrbc_hash_set(&hash, &key_midi_out, &val_midi_out);

    SET_RETURN(hash);
}

/*
 * USB_MIDI_HOST._send_packet(cable, cin, midi1, midi2, midi3)
 */
static void
c_usb_midi_send_packet(mrbc_vm *vm, mrbc_value v[], int argc)
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

    int ret = USB_MIDI_HOST_send_packet(cable, cin, midi1, midi2, midi3);
    SET_INT_RETURN(ret);
}

/*
 * USB_MIDI_HOST._bytes_available
 */
static void
c_usb_midi_bytes_available(mrbc_vm *vm, mrbc_value v[], int argc)
{
    int available = USB_MIDI_HOST_bytes_available();
    SET_INT_RETURN(available);
}

/*
 * USB_MIDI_HOST._read_available
 */
static void
c_usb_midi_read_available(mrbc_vm *vm, mrbc_value v[], int argc)
{
    int available = USB_MIDI_HOST_bytes_available();

    if (available < 4) {
        SET_NIL_RETURN();
        return;
    }

    uint8_t buffer[64];
    size_t max_read = (available > 64) ? 64 : available;

    int read_len = USB_MIDI_HOST_read_packet(buffer, max_read);

    if (read_len <= 0) {
        SET_NIL_RETURN();
        return;
    }

    mrbc_value str = mrbc_string_new(vm, (const char *)buffer, read_len);
    SET_RETURN(str);
}

/*
 * Gem initialization
 */
void
mrbc_usb_midi_host_init(mrbc_vm *vm)
{
    mrbc_class *class_USB_MIDI_HOST = mrbc_define_class(vm, "USB_MIDI_HOST", mrbc_class_object);

    /* Constants */
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("DISCONNECTED"),
                         &mrbc_integer_value(USB_MIDI_HOST_DISCONNECTED));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CONNECTED"),
                         &mrbc_integer_value(USB_MIDI_HOST_CONNECTED));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("INITIALIZING"),
                         &mrbc_integer_value(USB_MIDI_HOST_INITIALIZING));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("ERROR"),
                         &mrbc_integer_value(USB_MIDI_HOST_ERROR));

    /* CIN constants */
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_NOTE_OFF"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_NOTE_OFF));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_NOTE_ON"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_NOTE_ON));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_CONTROL_CHANGE"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_CONTROL_CHANGE));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_PROGRAM_CHANGE"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_PROGRAM_CHANGE));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_PITCH_BEND"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_PITCH_BEND));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_SINGLE_BYTE"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_SINGLE_BYTE));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_SYSEX_START"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_SYSEX_START));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_SYSEX_END_2"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_SYSEX_END_2));
    mrbc_set_class_const(class_USB_MIDI_HOST,
                         mrbc_str_to_symid("CIN_SYSEX_END_3"),
                         &mrbc_integer_value(USB_MIDI_HOST_CIN_SYSEX_END_3));

    /* Methods */
    mrbc_define_method(vm, class_USB_MIDI_HOST, "_init", c_usb_midi_init);
    mrbc_define_method(vm, class_USB_MIDI_HOST, "_get_status", c_usb_midi_get_status);
    mrbc_define_method(vm, class_USB_MIDI_HOST, "_get_device_info", c_usb_midi_get_device_info);
    mrbc_define_method(vm, class_USB_MIDI_HOST, "_send_packet", c_usb_midi_send_packet);
    mrbc_define_method(vm, class_USB_MIDI_HOST, "_bytes_available", c_usb_midi_bytes_available);
    mrbc_define_method(vm, class_USB_MIDI_HOST, "_read_available", c_usb_midi_read_available);
}
