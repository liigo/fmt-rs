use std::vec::Vec;
use self::raw::FMT;

mod raw;

#[deriving(PartialEq, Eq, Show)]
pub enum FmtType {
    Bool,
    Byte,
    Short,
    UShort,
    Int,
    UInt,
    Long,
    ULong,
    Double,
    Datetime,
    String,
    Array,
    Object,
    Null,
    EndTag,
    Invalid,
}

pub struct Fmt {
    fmt: *mut FMT,
}

impl Drop for Fmt {
    fn drop(&mut self) {
        unsafe {
            raw::fmt_object_put(self.fmt);
        }
    }
}

impl Fmt {
    pub unsafe fn from_raw(raw_fmt: *mut FMT) -> Fmt {
        Fmt { fmt: raw::fmt_object_get(raw_fmt) }
    }

    pub fn new_boolean(v: bool) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_boolean(if v {1} else {0}))
        }
    }

    pub fn new_byte(v: u8) -> Fmt {
    unsafe {
            Fmt::from_raw(raw::fmt_new_byte(v))
        }
    }

    pub fn new_short(v: i16) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_short(v))
        }
    }

    pub fn new_ushort(v: u16) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_ushort(v))
        }
    }

    pub fn new_int(v: i32) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_integer(v))
        }
    }

    pub fn new_uint(v: u32) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_uinteger(v))
        }
    }

    pub fn new_long(v: i64) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_long(v))
        }
    }

    pub fn new_ulong(v: u64) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_ulong(v))
        }
    }

    pub fn new_double(v: f64) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_double(v))
        }
    }

    pub fn new_datetime(v: f64) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_datetime(v))
        }
    }

    pub fn is_valid(&self) -> bool {
        self.get_type() == FmtType::Invalid
    }

    /// returns `FmtType::Invalid` if `is_valid()` returns false.
    pub fn get_type(&self) -> FmtType {
        if self.fmt.is_null() {
            return FmtType::Invalid;
        }
        let ty = unsafe {
            raw::fmt_get_type(self.fmt)
        };
        match ty {
            1 => FmtType::Byte,
            2 => FmtType::Short,
            3 => FmtType::UShort,
            4 => FmtType::Int,
            5 => FmtType::UInt,
            6 => FmtType::Long,
            7 => FmtType::ULong,
            8 => FmtType::Double,
            9 => FmtType::String,
           10 => FmtType::Datetime,
           11 => FmtType::Array,
           12 => FmtType::Object,
           13 => FmtType::Null,
           14 => FmtType::Bool,
          255 => FmtType::EndTag, // internal use
            _ => FmtType::Invalid,
        }
    }

    pub fn get_bool(&self) -> bool {
        unsafe {
            raw::fmt_get_boolean(self.fmt) != 0
        }
    }

    pub fn get_byte(&self) -> u8 {
        unsafe {
            raw::fmt_get_byte(self.fmt)
        }
    }

    pub fn get_short(&self) -> i16 {
        unsafe {
            raw::fmt_get_short(self.fmt)
        }
    }

    pub fn get_ushort(&self) -> u16 {
        unsafe {
            raw::fmt_get_ushort(self.fmt)
        }
    }

    pub fn get_int(&self) -> i32 {
        unsafe {
            raw::fmt_get_int(self.fmt)
        }
    }

    pub fn get_uint(&self) -> u32 {
        unsafe {
            raw::fmt_get_uint(self.fmt)
        }
    }

    pub fn get_long(&self) -> i64 {
        unsafe {
            raw::fmt_get_long(self.fmt)
        }
    }

    pub fn get_ulong(&self) -> u64 {
        unsafe {
            raw::fmt_get_ulong(self.fmt)
        }
    }

    pub fn get_double(&self) -> f64 {
        unsafe {
            raw::fmt_get_double(self.fmt)
        }
    }

    pub fn get_datetime(&self) -> u64 {
        unsafe {
            raw::fmt_get_datetime(self.fmt)
        }
    }

    /// creates an string/binary fmt object
    pub fn new_string(data: &[u8]) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_string(data.as_ptr(), data.len() as u32))
        }
    }

    pub fn new_empty_string() -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_string(0 as *const u8, 0 as u32))
        }
    }

    pub fn get_string(&self) -> &[u8] {
        use std::raw::Slice;
        use std::intrinsics::transmute;
        unsafe {
            let data = raw::fmt_get_string(self.fmt);
            let len  = raw::fmt_get_string_len(self.fmt) as uint;
            transmute(Slice { data: data, len: len })
        }
    }

    /// creates an array fmt object
    pub fn new_array() -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_array())
        }
    }

    pub fn array_length(&self) -> u32 {
        unsafe {
            raw::fmt_array_length(self.fmt)
        }
    }

    pub fn array_append(&mut self, fmt: Fmt) {
        unsafe {
            raw::fmt_array_append(self.fmt, fmt.fmt);
        }
    }

    pub fn array_remove(&mut self, index: u32) {
        unsafe {
            raw::fmt_array_remove(self.fmt, index);
        }
    }

    pub fn array_index(&self, index: u32) -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_array_get_idx(self.fmt, index))
        }
    }

    /// create an object fmt object
    pub fn new_object() -> Fmt {
        unsafe {
            Fmt::from_raw(raw::fmt_new_object())
        }
    }

    pub fn object_total(&self) -> i32 {
        unsafe {
            raw::fmt_object_total(self.fmt)
        }
    }

    /// please ensure that `key` end with `\0`, e.g. `"liigo\0"`.
    pub fn object_lookup(&self, key: &str) -> Option<Fmt> {
        unsafe {
            let fmt = raw::fmt_object_lookup(self.fmt, key.as_ptr());
            if fmt.is_null() {
                None
            } else {
                Some(Fmt::from_raw(fmt))
            }
        }
    }

    /// please ensure that `key` end with `\0`, e.g. `"liigo\0"`.
    pub fn object_add(&mut self, key: &str, val: Fmt) {
        unsafe {
            raw::fmt_object_add(self.fmt, key.as_ptr(), val.fmt);
        }
    }

    /// please ensure that `key` end with `\0`, e.g. `"liigo\0"`.
    pub fn object_remove(&mut self, key: &str) {
        unsafe {
            raw::fmt_object_remove(self.fmt, key.as_ptr());
        }
    }

    pub fn packet(&self, cmd: i16) -> Vec<u8> {
        unsafe {
            let mut data = 0 as *const u8;
            let mut size: u32 = 0;
            let freefn = raw::fmt_packet(self.fmt, cmd, 0 as *const u8, &mut data, &mut size);
            let vec = Vec::from_raw_buf(data, size as uint);
            raw::fmt_freemem(freefn, data);
            vec
        }
    }

    pub fn packet_v2(&self, cmd: i16, key: &[u8, ..16]) -> Vec<u8> {
        unsafe {
            let mut data = 0 as *const u8;
            let mut size: u32 = 0;
            let freefn = raw::fmt_packet(self.fmt, cmd, key.as_ptr(), &mut data, &mut size);
            let vec = Vec::from_raw_buf(data, size as uint);
            raw::fmt_freemem(freefn, data);
            vec
        }
    }
}

extern fn raw_fmt_callback(_parser: *mut u8, cmd: i16, raw_fmt: *mut FMT, userdata: *mut u8) {
    unsafe {
        // userdata is a closure, see `FmtParser::push()`
        let closure = userdata as *mut |cmd: i16, fmt: &Fmt|;
        let fmt = Fmt::from_raw(raw_fmt);
        (*closure)(cmd, &fmt);
    }
}

pub struct FmtParser {
    parser: *mut u8,
}

impl Drop for FmtParser {
    fn drop(&mut self) {
        unsafe {
            raw::buffered_fmt_parser_delete(self.parser);
        }
    }
}

impl FmtParser {
    pub fn new() -> FmtParser {
        unsafe {
            FmtParser { parser: raw::buffered_fmt_parser_new() }
        }
    }

    /// `fmt` passed in closure maybe invalid, if `cmd` has no corresponding fmt.
    /// please check `fmt.is_valid()` before reading from it.
    pub fn push(&mut self, data: &[u8], cb: |cmd: i16, fmt: &Fmt|) {
        use std::intrinsics::transmute;
        unsafe {
            let closure: *mut u8 = transmute(&cb);
            raw::buffered_fmt_parser_push(self.parser, data.as_ptr(), data.len() as u32, raw_fmt_callback, closure);
        }
    }

    pub fn set_key(&mut self, key: &[u8, ..16]) {
        unsafe {
            raw::buffered_fmt_parser_set_key(self.parser, key.as_ptr());
        }
    }

    pub fn reset(&mut self) {
        unsafe {
            raw::buffered_fmt_parser_reset(self.parser);
        }
    }
}

#[cfg(test)]
mod tests {
    use {Fmt, FmtType, FmtParser};
    #[test]
    fn test_fmt_primitives() {
        let fmt = Fmt::new_byte(16);
        assert_eq!(fmt.get_type(), FmtType::Byte);
        assert_eq!(fmt.get_byte(), 16)
        let fmt = Fmt::new_int(8);
        assert_eq!(fmt.get_type(), FmtType::Int);
        assert_eq!(fmt.get_int(), 8);
    }

    #[test]
    fn test_fmt_string() {
        let s1 = Fmt::new_string("liigo".as_bytes());
        assert_eq!(s1.get_string(), "liigo".as_bytes());
        let s2 = Fmt::new_string(&[]);
        assert_eq!(s2.get_string(), [].as_slice());
        let s3 = Fmt::new_empty_string();
        assert_eq!(s3.get_string(), [].as_slice());
    }

    #[test]
    fn test_fmt_array() {
        let mut a = Fmt::new_array();
        assert_eq!(a.get_type(), FmtType::Array);
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
    fn test_fmt_object() {
        let mut o = Fmt::new_object();
        assert_eq!(o.get_type(), FmtType::Object);
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

    #[test]
    fn test_fmt_packet() {
        let mut o = Fmt::new_object();
        // NOTE: string must end with \0 !
        o.object_add("abc\0", Fmt::new_int(16));

        let datav1: Vec<u8> = o.packet(1);
        println!("v1: {}", datav1);
        assert_eq!(datav1.len(), 18);
        assert_eq!(datav1, vec![3,1,0,0,1,12,12,9,3,97,98,99,4,0,0,0,16,255]);

        let datav2: Vec<u8> = o.packet_v2(1, &[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]);
        println!("v2: {}", datav2);
        assert_eq!(datav2.len(), 24);
        assert_eq!(datav2, vec![0x03, 0x02, 0x00, 0x14, 0x8B, 0x00, 0x38, 0xD3, 0x99, 0x7A, 0xE4, 0xFB, 0xCB, 0x7B, 0xC4, 0x37, 0xD2, 0x60, 0x52, 0xE8, 0xFD, 0x4E, 0x5F, 0x76]);
    }

    #[test]
    fn test_fmt_parser() {
        let mut parser = FmtParser::new();
        let datav1 = &[3,1,0,0,1,12,12,9,3,97,98,99,4,0,0,0,16,255];
        parser.push(datav1, |cmd,fmt|{
            assert_eq!(cmd, 1);
            assert_eq!(fmt.get_type(), FmtType::Object);
            assert_eq!(fmt.object_total(), 1);
            assert!(fmt.object_lookup("abc\0").is_some());
            assert_eq!(fmt.object_lookup("abc\0").unwrap().get_type(), FmtType::Int);
            assert_eq!(fmt.object_lookup("abc\0").unwrap().get_int(), 16);
        });
        let datav2 = &[0x03, 0x02, 0x00, 0x14, 0x8B, 0x00, 0x38, 0xD3, 0x99, 0x7A, 0xE4, 0xFB, 0xCB, 0x7B, 0xC4, 0x37, 0xD2, 0x60, 0x52, 0xE8, 0xFD, 0x4E, 0x5F, 0x76];
        parser.set_key(&[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]);
        parser.push(datav2, |cmd,fmt|{
            assert_eq!(cmd, 1);
            assert_eq!(fmt.get_type(), FmtType::Object);
            assert_eq!(fmt.object_total(), 1);
            assert!(fmt.object_lookup("abc\0").is_some());
            assert_eq!(fmt.object_lookup("abc\0").unwrap().get_type(), FmtType::Int);
            assert_eq!(fmt.object_lookup("abc\0").unwrap().get_int(), 16);
        });

        parser.delete();
    }
}
