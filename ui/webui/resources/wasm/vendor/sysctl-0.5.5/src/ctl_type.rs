// ctl_type.rs

use ctl_value::*;

/// An Enum that represents a sysctl's type information.
///
/// # Example
///
/// ```
/// # use sysctl::Sysctl;
/// if let Ok(ctl) = sysctl::Ctl::new("kern.osrevision") {
///     if let Ok(value) = ctl.value() {
///         let val_type: sysctl::CtlType = value.into();
///         assert_eq!(val_type, sysctl::CtlType::Int);
///     }
/// }
/// ```
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(u32)]
pub enum CtlType {
    Node = 1,
    Int = 2,
    String = 3,
    S64 = 4,
    Struct = 5,
    Uint = 6,
    Long = 7,
    Ulong = 8,
    U64 = 9,
    U8 = 10,
    U16 = 11,
    S8 = 12,
    S16 = 13,
    S32 = 14,
    U32 = 15,
    // Added custom types below
    None = 0,
    #[cfg(target_os = "freebsd")]
    Temperature = 16,
}
impl std::convert::From<u32> for CtlType {
    fn from(t: u32) -> Self {
        assert!(t <= 16);
        unsafe { std::mem::transmute(t) }
    }
}
impl std::convert::From<&CtlValue> for CtlType {
    fn from(t: &CtlValue) -> Self {
        match t {
            CtlValue::None => CtlType::None,
            CtlValue::Node(_) => CtlType::Node,
            CtlValue::Int(_) => CtlType::Int,
            CtlValue::String(_) => CtlType::String,
            CtlValue::S64(_) => CtlType::S64,
            CtlValue::Struct(_) => CtlType::Struct,
            CtlValue::Uint(_) => CtlType::Uint,
            CtlValue::Long(_) => CtlType::Long,
            CtlValue::Ulong(_) => CtlType::Ulong,
            CtlValue::U64(_) => CtlType::U64,
            CtlValue::U8(_) => CtlType::U8,
            CtlValue::U16(_) => CtlType::U16,
            CtlValue::S8(_) => CtlType::S8,
            CtlValue::S16(_) => CtlType::S16,
            CtlValue::S32(_) => CtlType::S32,
            CtlValue::U32(_) => CtlType::U32,
            #[cfg(target_os = "freebsd")]
            CtlValue::Temperature(_) => CtlType::Temperature,
        }
    }
}

impl std::convert::From<CtlValue> for CtlType {
    fn from(t: CtlValue) -> Self {
        Self::from(&t)
    }
}

impl CtlType {
    pub fn min_type_size(&self) -> usize {
        match self {
            CtlType::None => 0,
            CtlType::Node => 0,
            CtlType::Int => std::mem::size_of::<libc::c_int>(),
            CtlType::String => 0,
            CtlType::S64 => std::mem::size_of::<i64>(),
            CtlType::Struct => 0,
            CtlType::Uint => std::mem::size_of::<libc::c_uint>(),
            CtlType::Long => std::mem::size_of::<libc::c_long>(),
            CtlType::Ulong => std::mem::size_of::<libc::c_ulong>(),
            CtlType::U64 => std::mem::size_of::<u64>(),
            CtlType::U8 => std::mem::size_of::<u8>(),
            CtlType::U16 => std::mem::size_of::<u16>(),
            CtlType::S8 => std::mem::size_of::<i8>(),
            CtlType::S16 => std::mem::size_of::<i16>(),
            CtlType::S32 => std::mem::size_of::<i32>(),
            CtlType::U32 => std::mem::size_of::<u32>(),
            // Added custom types below
            #[cfg(target_os = "freebsd")]
            CtlType::Temperature => 0,
        }
    }
}
