/*
const PDT_BYTE    : u8 = 0x01;
const PDT_SHORT   : u8 = 0x02;
const PDT_USHORT  : u8 = 0x03;
const PDT_INTEGER : u8 = 0x04;
const PDT_UINTEGER: u8 = 0x05;
const PDT_LONG    : u8 = 0x06;
const PDT_ULONG   : u8 = 0x07;
const PDT_DOUBLE  : u8 = 0x08;
const PDT_STRING  : u8 = 0x09;

const PDT_DATETIME: u8 = 0x0A;
const PDT_ARRAY   : u8 = 0x0B;
const PDT_OBJECT  : u8 = 0x0C;
const PDT_NULL	: u8 = 0x0D;
const PDT_BOOL	: u8 = 0x0E;

const _PDT_END: u8 = 0xFF;
*/

enum FMT {}

#[link(name="fmt", kind="static")]
#[allow(dead_code)]
extern {
    // creates FMT* primitive value objects.
    // and there are functions to create advanced FMT objects, e.g.
    // fmt_new_string, fmt_new_object, fmt_new_array, ... see bellow.
    fn fmt_new_boolean(v: i32) -> *mut FMT;
    fn fmt_new_byte(v: u8) -> *mut FMT;
    fn fmt_new_short (v: i16) -> *mut FMT;
    fn fmt_new_ushort(v: u16) -> *mut FMT;
    fn fmt_new_integer (v: i32) -> *mut FMT;
    fn fmt_new_uinteger(v: u32) -> *mut FMT;
    fn fmt_new_long (v: i64)  -> *mut FMT;
    fn fmt_new_ulong(v: u64)  -> *mut FMT;
    fn fmt_new_double(v: f64) -> *mut FMT;
    fn fmt_new_datetime(v: f64) -> *mut FMT;
    fn fmt_new_null() -> *mut FMT;

    // get primitive value from FMT* objects
    fn fmt_get_boolean(fmt: *mut FMT) -> i32;
    fn fmt_get_byte(fmt: *mut FMT) -> u8;
    fn fmt_get_short (fmt: *mut FMT) -> i16;
    fn fmt_get_ushort(fmt: *mut FMT) -> u16;
    fn fmt_get_int (fmt: *mut FMT) -> i32;
    fn fmt_get_uint(fmt: *mut FMT) -> u32;
    fn fmt_get_long (fmt: *mut FMT) -> i64;
    fn fmt_get_ulong(fmt: *mut FMT) -> u64;
    fn fmt_get_double(fmt: *mut FMT) -> f64;
    fn fmt_get_datetime(fmt: *mut FMT) -> f64;

    // FMT strings
    fn fmt_new_string(data: *const u8, len: i32) -> *mut FMT;
    fn fmt_get_str(fmt: *mut FMT) -> *const u8;
    fn fmt_get_str_len(fmt: *mut FMT) -> i32;

    // FMT objects
    fn fmt_new_object() -> *mut FMT;
    fn fmt_object_total(fmt: *mut FMT) -> i32;
    fn fmt_object_add(fmt: *mut FMT, key: *const u8, val: *mut FMT);
    fn fmt_object_remove(fmt: *mut FMT, key: *const u8);
    fn fmt_object_lookup(fmt: *mut FMT, key: *const u8) -> *mut FMT;

    // FMT arrays
    fn fmt_new_array() -> *mut FMT;
    fn fmt_array_length(fmt: *mut FMT) -> u32;
    fn fmt_array_append(fmt: *mut FMT, item: *mut FMT);
    fn fmt_array_remove(fmt: *mut FMT, index: u32);
    fn fmt_array_get_idx(fmt: *mut FMT, index: u32) -> *mut FMT;

    // manage references count of FMT objects
    fn fmt_object_get(fmt: *mut FMT) -> *mut FMT;
    fn fmt_object_put(fmt: *mut FMT);

    // other
    fn fmt_get_type(fmt: *mut FMT) -> u8;
}

pub struct Fmt {
    fmt: *mut FMT,
}

impl Drop for Fmt {
    fn drop(&mut self) {
        unsafe {
            fmt_object_put(self.fmt);
        }
    }
}

#[inline]
fn getfmt(fmt: *mut FMT) -> *mut FMT {
    unsafe {
        fmt_object_get(fmt)
    }
}

impl Fmt {
    pub fn new_boolean(v: bool) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_boolean(if v {1} else {0})) }
        }
    }

    pub fn new_byte(v: u8) -> Fmt {
    unsafe {
            Fmt { fmt: getfmt(fmt_new_byte(v)) }
        }
    }

    pub fn new_short(v: i16) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_short(v)) }
        }
    }

    pub fn new_ushort(v: u16) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_ushort(v)) }
        }
    }

    pub fn new_int(v: i32) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_integer(v)) }
        }
    }
    
    pub fn new_uint(v: u32) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_uinteger(v)) }
        }
    }

    pub fn new_long(v: i64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_long(v)) }
        }
    }

    pub fn new_ulong(v: u64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_ulong(v)) }
        }
    }

    pub fn new_double(v: f64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_double(v)) }
        }
    }

    pub fn new_datetime(v: f64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_datetime(v)) }
        }
    }

    pub fn get_boolean(&self) -> bool {
        unsafe {
            fmt_get_boolean(self.fmt) != 0
        }
    }

    pub fn get_byte(&self) -> u8 {
        unsafe {
            fmt_get_byte(self.fmt)
        }
    }

    pub fn get_short(&self) -> i16 {
        unsafe {
            fmt_get_short(self.fmt)
        }
    }

    pub fn get_ushort(&self) -> u16 {
        unsafe {
            fmt_get_ushort(self.fmt)
        }
    }

    pub fn get_int(&self) -> i32 {
        unsafe {
            fmt_get_int(self.fmt)
        }
    }

    pub fn get_uint(&self) -> u32 {
        unsafe {
            fmt_get_uint(self.fmt)
        }
    }

    pub fn get_long(&self) -> i64 {
        unsafe {
            fmt_get_long(self.fmt)
        }
    }

    pub fn get_ulong(&self) -> u64 {
        unsafe {
            fmt_get_ulong(self.fmt)
        }
    }

    pub fn get_double(&self) -> f64 {
        unsafe {
            fmt_get_double(self.fmt)
        }
    }

    pub fn get_datetime(&self) -> f64 {
        unsafe {
            fmt_get_datetime(self.fmt)
        }
    }

    /// creates an array fmt object
    pub fn new_array() -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_array()) }
        }
    }

    pub fn array_length(&self) -> u32 {
        unsafe {
            fmt_array_length(self.fmt)
        }
    }

    pub fn array_append(&mut self, fmt: Fmt) {
        unsafe {
            fmt_array_append(self.fmt, fmt.fmt);
        }
    }

    pub fn array_remove(&mut self, index: u32) {
        unsafe {
            fmt_array_remove(self.fmt, index);
        }
    }

    pub fn array_index(&self, index: u32) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_array_get_idx(self.fmt, index)) }
        }
    }

    pub fn new_object() -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_object()) }
        }
    }

    pub fn object_total(&self) -> i32 {
        unsafe {
            fmt_object_total(self.fmt)
        }
    }

    pub fn object_lookup(&self, key: &str) -> Option<Fmt> {
        unsafe {
            let fmt = fmt_object_lookup(self.fmt, key.as_ptr());
            if fmt.is_null() {
                None
            } else {
                Some(Fmt {fmt: getfmt(fmt)})
            }
        }
    }

    pub fn object_add(&mut self, key: &str, val: Fmt) {
        unsafe {
            fmt_object_add(self.fmt, key.as_ptr(), val.fmt);
        }
    }

    pub fn object_remove(&mut self, key: &str) {
        unsafe {
            fmt_object_remove(self.fmt, key.as_ptr());
        }
    }
}

#[cfg(test)]
mod tests {
    use Fmt;
    #[test]
    fn test_fmt_primitives() {
        let fmt = Fmt::new_byte(16);
        assert_eq!(fmt.get_byte(), 16)
        let fmt = Fmt::new_int(8);
        assert_eq!(fmt.get_int(), 8);
    }

    #[test]
    fn test_fmt_array() {
        let mut a = Fmt::new_array();
        a.array_append(Fmt::new_int(1));
        a.array_append(Fmt::new_int(2));
        a.array_append(Fmt::new_int(3));
        assert_eq!(a.array_length(), 3);
        let a1 = a.array_index(0);
        assert_eq!(a1.get_int(), 1);
        assert_eq!(a.array_index(1).get_int(), 2);
        assert_eq!(a.array_index(2).get_int(), 3);
        a.array_remove(1);
        assert_eq!(a.array_index(1).get_int(), 3);
    }

    #[test]
    #[no_mangle]
    fn test_fmt_object() {
        let mut o = Fmt::new_object();
        assert_eq!(o.object_total(), 0);
        o.object_add("a", Fmt::new_int(1));
        o.object_add("b", Fmt::new_byte(2));
        assert_eq!(o.object_total(), 2);

        let a = o.object_lookup("a").unwrap();
        assert_eq!(a.get_int(), 1);
        let b = o.object_lookup("b").unwrap();
        assert_eq!(b.get_byte(), 2);
        o.object_remove("b");
        let b = o.object_lookup("b");
        assert!(b.is_none());
    }
}
