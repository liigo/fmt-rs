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
    fn fmt_object_add(fmt: *mut FMT, key: *const i8, val: *mut FMT);
    fn fmt_object_remove(fmt: *mut FMT, key: *const i8);
    fn fmt_object_lookup(fmt: *mut FMT, key: *const i8) -> *mut FMT;

    // FMT arrays
    fn fmt_new_array() -> *mut FMT;
    fn fmt_get_array_len(fmt: *mut FMT) -> i32;
    fn fmt_array_apped(fmt: *mut FMT, item: *mut FMT);
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
            println!("fmt_object_put()");
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
    pub fn with_boolean(v: i32) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_boolean(v)) }
        }
    }

    pub fn with_byte(v: u8) -> Fmt {
    unsafe {
            Fmt { fmt: getfmt(fmt_new_byte(v)) }
        }
    }

    pub fn with_short(v: i16) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_short(v)) }
        }
    }

    pub fn with_ushort(v: u16) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_ushort(v)) }
        }
    }

    pub fn with_int(v: i32) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_integer(v)) }
        }
    }
    
    pub fn with_uint(v: u32) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_uinteger(v)) }
        }
    }

    pub fn with_long(v: i64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_long(v)) }
        }
    }

    pub fn with_ulong(v: u64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_ulong(v)) }
        }
    }

    pub fn with_double(v: f64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_double(v)) }
        }
    }

    pub fn with_datetime(v: f64) -> Fmt {
        unsafe {
            Fmt { fmt: getfmt(fmt_new_datetime(v)) }
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


}

#[cfg(test)]
mod tests {
    use Fmt;
    #[test]
    fn test_fmt_byte() {
        let fmt = Fmt::with_byte(16u8);
        assert_eq!(fmt.get_byte(), 16u8)
        let fmt = Fmt::with_int(6i32);
        assert_eq!(fmt.get_int(), 6i32);
    }

}

