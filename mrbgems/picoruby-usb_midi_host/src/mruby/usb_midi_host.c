/*
 * PicoRuby USB-MIDI - mruby bindings
 */

#include <mruby.h>
#include <mruby/presym.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/array.h>

#include "../../include/usb_midi_host.h"

/*
 * USB_MIDI_HOST._init
 * Initialize USB MIDI subsystem
 */
static mrb_value
mrb_usb_midi_init(mrb_state *mrb, mrb_value self)
{
    int ret = USB_MIDI_HOST_init();
    return mrb_fixnum_value(ret);
}

/*
 * USB_MIDI_HOST._get_status
 * Returns status as integer
 */
static mrb_value
mrb_usb_midi_get_status(mrb_state *mrb, mrb_value self)
{
    usb_midi_host_status_t status = USB_MIDI_HOST_get_status();
    return mrb_fixnum_value((mrb_int)status);
}

/*
 * USB_MIDI_HOST._get_device_info
 * Returns device info as Hash or nil
 */
static mrb_value
mrb_usb_midi_get_device_info(mrb_state *mrb, mrb_value self)
{
    usb_midi_host_device_info_t info;

    if (!USB_MIDI_HOST_get_device_info(&info)) {
        return mrb_nil_value();
    }

    mrb_value hash = mrb_hash_new(mrb);

    mrb_hash_set(mrb, hash,
                 mrb_symbol_value(MRB_SYM(vendor_id)),
                 mrb_fixnum_value(info.vendor_id));

    mrb_hash_set(mrb, hash,
                 mrb_symbol_value(MRB_SYM(product_id)),
                 mrb_fixnum_value(info.product_id));

    mrb_hash_set(mrb, hash,
                 mrb_symbol_value(MRB_SYM(manufacturer)),
                 mrb_str_new_cstr(mrb, info.manufacturer));

    mrb_hash_set(mrb, hash,
                 mrb_symbol_value(MRB_SYM(product)),
                 mrb_str_new_cstr(mrb, info.product));

    mrb_hash_set(mrb, hash,
                 mrb_symbol_value(MRB_SYM(midi_in_ep)),
                 mrb_fixnum_value(info.midi_in_ep));

    mrb_hash_set(mrb, hash,
                 mrb_symbol_value(MRB_SYM(midi_out_ep)),
                 mrb_fixnum_value(info.midi_out_ep));

    return hash;
}

/*
 * USB_MIDI_HOST._send_packet(cable, cin, midi1, midi2, midi3)
 * Send a single USB-MIDI packet
 */
static mrb_value
mrb_usb_midi_send_packet(mrb_state *mrb, mrb_value self)
{
    mrb_int cable, cin, midi1, midi2, midi3;

    mrb_get_args(mrb, "iiiii", &cable, &cin, &midi1, &midi2, &midi3);

    int ret = USB_MIDI_HOST_send_packet(
        (uint8_t)cable, (uint8_t)cin,
        (uint8_t)midi1, (uint8_t)midi2, (uint8_t)midi3
    );

    return mrb_fixnum_value(ret);
}

/*
 * USB_MIDI_HOST._bytes_available
 * Returns number of bytes available in RX buffer
 */
static mrb_value
mrb_usb_midi_bytes_available(mrb_state *mrb, mrb_value self)
{
    int available = USB_MIDI_HOST_bytes_available();
    return mrb_fixnum_value(available);
}

/*
 * USB_MIDI_HOST._read_available
 * Read available packets as String (binary)
 */
static mrb_value
mrb_usb_midi_read_available(mrb_state *mrb, mrb_value self)
{
    int available = USB_MIDI_HOST_bytes_available();

    if (available < 4) {
        return mrb_nil_value();
    }

    /* Allocate buffer for reading */
    uint8_t buffer[64];  /* Max 16 packets */
    size_t max_read = (available > 64) ? 64 : available;

    int read_len = USB_MIDI_HOST_read_packet(buffer, max_read);

    if (read_len <= 0) {
        return mrb_nil_value();
    }

    return mrb_str_new(mrb, (const char *)buffer, read_len);
}

/*
 * Gem initialization
 */
void
mrb_picoruby_usb_midi_host_gem_init(mrb_state *mrb)
{
    struct RClass *class_USB_MIDI_HOST;

    class_USB_MIDI_HOST = mrb_define_class_id(mrb, MRB_SYM(USB_MIDI_HOST), mrb->object_class);

    /* Constants */
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(DISCONNECTED),
                        mrb_fixnum_value(USB_MIDI_HOST_DISCONNECTED));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CONNECTED),
                        mrb_fixnum_value(USB_MIDI_HOST_CONNECTED));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(INITIALIZING),
                        mrb_fixnum_value(USB_MIDI_HOST_INITIALIZING));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(ERROR),
                        mrb_fixnum_value(USB_MIDI_HOST_ERROR));

    /* CIN constants */
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_NOTE_OFF),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_NOTE_OFF));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_NOTE_ON),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_NOTE_ON));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_CONTROL_CHANGE),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_CONTROL_CHANGE));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_PROGRAM_CHANGE),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_PROGRAM_CHANGE));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_PITCH_BEND),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_PITCH_BEND));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_SINGLE_BYTE),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_SINGLE_BYTE));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_SYSEX_START),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_SYSEX_START));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_SYSEX_END_2),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_SYSEX_END_2));
    mrb_define_const_id(mrb, class_USB_MIDI_HOST, MRB_SYM(CIN_SYSEX_END_3),
                        mrb_fixnum_value(USB_MIDI_HOST_CIN_SYSEX_END_3));

    /* Methods */
    mrb_define_method_id(mrb, class_USB_MIDI_HOST, MRB_SYM(_init),
                         mrb_usb_midi_init, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_USB_MIDI_HOST, MRB_SYM(_get_status),
                         mrb_usb_midi_get_status, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_USB_MIDI_HOST, MRB_SYM(_get_device_info),
                         mrb_usb_midi_get_device_info, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_USB_MIDI_HOST, MRB_SYM(_send_packet),
                         mrb_usb_midi_send_packet, MRB_ARGS_REQ(5));
    mrb_define_method_id(mrb, class_USB_MIDI_HOST, MRB_SYM(_bytes_available),
                         mrb_usb_midi_bytes_available, MRB_ARGS_NONE());
    mrb_define_method_id(mrb, class_USB_MIDI_HOST, MRB_SYM(_read_available),
                         mrb_usb_midi_read_available, MRB_ARGS_NONE());
}

void
mrb_picoruby_usb_midi_host_gem_final(mrb_state *mrb)
{
    USB_MIDI_HOST_deinit();
}
