pub enum FMT {}

#[allow(non_camel_case_types)]
pub type fmt_available_callback = extern fn(
        parser: *mut u8, cmd: i16, fmt: *mut FMT, userdata: *mut u8
    );

#[link(name="fmt", kind="static")]
#[allow(dead_code)]
extern {
    // creates FMT* primitive value objects.
    // and there are functions to create advanced FMT objects, e.g.
    // fmt_new_string, fmt_new_object, fmt_new_array, ... see bellow.
    pub fn fmt_new_boolean(v: i32) -> *mut FMT;
    pub fn fmt_new_byte(v: u8) -> *mut FMT;
    pub fn fmt_new_short (v: i16) -> *mut FMT;
    pub fn fmt_new_ushort(v: u16) -> *mut FMT;
    pub fn fmt_new_integer (v: i32) -> *mut FMT;
    pub fn fmt_new_uinteger(v: u32) -> *mut FMT;
    pub fn fmt_new_long (v: i64)  -> *mut FMT;
    pub fn fmt_new_ulong(v: u64)  -> *mut FMT;
    pub fn fmt_new_double(v: f64) -> *mut FMT;
    pub fn fmt_new_datetime(v: f64) -> *mut FMT;
    pub fn fmt_new_null() -> *mut FMT;

    // get primitive value from FMT* objects
    pub fn fmt_get_boolean(fmt: *mut FMT) -> i32;
    pub fn fmt_get_byte(fmt: *mut FMT) -> u8;
    pub fn fmt_get_short (fmt: *mut FMT) -> i16;
    pub fn fmt_get_ushort(fmt: *mut FMT) -> u16;
    pub fn fmt_get_int (fmt: *mut FMT) -> i32;
    pub fn fmt_get_uint(fmt: *mut FMT) -> u32;
    pub fn fmt_get_long (fmt: *mut FMT) -> i64;
    pub fn fmt_get_ulong(fmt: *mut FMT) -> u64;
    pub fn fmt_get_double(fmt: *mut FMT) -> f64;
    pub fn fmt_get_datetime(fmt: *mut FMT) -> u64;

    // FMT strings
    pub fn fmt_new_string(data: *const u8, len: u32) -> *mut FMT;
    pub fn fmt_get_string(fmt: *mut FMT) -> *const u8;
    pub fn fmt_get_string_len(fmt: *mut FMT) -> u32;

    // FMT objects
    pub fn fmt_new_object() -> *mut FMT;
    pub fn fmt_object_total(fmt: *mut FMT) -> i32;
    pub fn fmt_object_add(fmt: *mut FMT, key: *const u8, val: *mut FMT);
    pub fn fmt_object_remove(fmt: *mut FMT, key: *const u8);
    pub fn fmt_object_lookup(fmt: *mut FMT, key: *const u8) -> *mut FMT;

    // FMT arrays
    pub fn fmt_new_array() -> *mut FMT;
    pub fn fmt_array_length(fmt: *mut FMT) -> u32;
    pub fn fmt_array_append(fmt: *mut FMT, item: *mut FMT);
    pub fn fmt_array_remove(fmt: *mut FMT, index: u32);
    pub fn fmt_array_get_idx(fmt: *mut FMT, index: u32) -> *mut FMT;

    // manage references count of FMT objects
    pub fn fmt_object_get(fmt: *mut FMT) -> *mut FMT;
    pub fn fmt_object_put(fmt: *mut FMT);

    // type and packet
    pub fn fmt_get_type(fmt: *mut FMT) -> u8;
    pub fn fmt_packet(fmt: *mut FMT, cmd: i16, key: *const u8, data: *mut *const u8, size: *mut u32) -> *const u8;
    pub fn fmt_freemem(freefn: *const u8, mem: *const u8);

    // parser
    pub fn buffered_fmt_parser_new() -> *mut u8;
    pub fn buffered_fmt_parser_delete(parser: *mut u8);
    pub fn buffered_fmt_parser_push(parser: *mut u8, data: *const u8, len: u32,
                                    cb: fmt_available_callback, userdata: *mut u8);
    pub fn buffered_fmt_parser_reset(parser: *mut u8);
    pub fn buffered_fmt_parser_set_key(parser: *mut u8, key: *const u8);
}
