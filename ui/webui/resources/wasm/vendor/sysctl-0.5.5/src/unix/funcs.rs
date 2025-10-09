// unix/funcs.rs

use byteorder::{ByteOrder, WriteBytesExt};
use consts::*;
use ctl_error::*;
use ctl_info::*;
use ctl_type::*;
use ctl_value::*;
#[cfg(any(target_os = "macos", target_os = "ios"))]
use std::ffi::CString;

#[cfg(target_os = "freebsd")]
use temperature::*;

pub fn name2oid(name: &str) -> Result<Vec<libc::c_int>, SysctlError> {
    // We get results in this vector
    let mut len: usize = CTL_MAXNAME as usize;
    let mut res = Vec::<libc::c_int>::with_capacity(len);
    let rcname = std::ffi::CString::new(name);
    if rcname.is_err() {
        return Err(SysctlError::NotFound(name.to_owned()));
    }
    let cname = rcname.unwrap();

    let ret = unsafe {
        libc::sysctlnametomib(
            cname.as_ptr() as *const libc::c_char,
            res.as_mut_ptr(),
            &mut len
        )
    };
    if ret < 0 {
        let e = std::io::Error::last_os_error();
        return Err(match e.kind() {
            std::io::ErrorKind::NotFound => SysctlError::NotFound(name.into()),
            _ => SysctlError::IoError(e),
        });
    } else {
        unsafe { res.set_len(len); }
    }

    Ok(res)
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn oidfmt(oid: &[libc::c_int]) -> Result<CtlInfo, SysctlError> {
    // Request command for type info
    let mut qoid: Vec<libc::c_int> = vec![0, 4];
    qoid.extend(oid);

    // Store results here
    let mut buf: [libc::c_uchar; libc::BUFSIZ as usize] = [0; libc::BUFSIZ as usize];
    let mut buf_len = std::mem::size_of_val(&buf);
    let ret = unsafe {
        libc::sysctl(
            qoid.as_ptr(),
            qoid.len() as u32,
            buf.as_mut_ptr() as *mut libc::c_void,
            &mut buf_len,
            std::ptr::null(),
            0,
        )
    };
    if ret != 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // 'Kind' is the first 32 bits of result buffer
    let kind = byteorder::LittleEndian::read_u32(&buf);

    // 'Type' is the first 4 bits of 'Kind'
    let ctltype_val = kind & CTLTYPE as u32;

    // 'fmt' is after 'Kind' in result buffer
    let fmt: String =
        match std::ffi::CStr::from_bytes_with_nul(&buf[std::mem::size_of::<u32>()..buf_len]) {
            Ok(x) => x.to_string_lossy().into(),
            Err(e) => return Err(SysctlError::InvalidCStr(e)),
        };

    #[cfg_attr(feature = "cargo-clippy", allow(clippy::redundant_field_names))]
    let s = CtlInfo {
        ctl_type: CtlType::from(ctltype_val),
        fmt: fmt,
        flags: kind,
    };
    Ok(s)
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn oidfmt(oid: &[libc::c_int]) -> Result<CtlInfo, SysctlError> {
    // Request command for type info
    let mut qoid: Vec<libc::c_int> = vec![0, 4];
    qoid.extend(oid);

    // Store results here
    let mut buf: [libc::c_uchar; libc::BUFSIZ as usize] = [0; libc::BUFSIZ as usize];
    let mut buf_len = std::mem::size_of_val(&buf);
    let ret = unsafe {
        libc::sysctl(
            qoid.as_mut_ptr(),
            qoid.len() as u32,
            buf.as_mut_ptr() as *mut libc::c_void,
            &mut buf_len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret != 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // 'Kind' is the first 32 bits of result buffer
    let kind = byteorder::LittleEndian::read_u32(&buf);

    // 'Type' is the first 4 bits of 'Kind'
    let ctltype_val = kind & CTLTYPE as u32;

    // 'fmt' is after 'Kind' in result buffer
    let bytes = &buf[std::mem::size_of::<u32>()..buf_len];

    // Ensure we drop the '\0' byte if there are any.
    let len = match bytes.iter().position(|c| *c == 0) {
        Some(index) => index,
        _ => bytes.len(),
    };

    let fmt: String = match std::str::from_utf8(&bytes[..len]) {
        Ok(x) => x.to_owned(),
        Err(e) => return Err(SysctlError::Utf8Error(e)),
    };

    let s = CtlInfo {
        ctl_type: CtlType::from(ctltype_val),
        fmt: fmt,
        flags: kind,
    };
    Ok(s)
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn value_oid(oid: &[i32]) -> Result<CtlValue, SysctlError> {
    let info: CtlInfo = oidfmt(&oid)?;

    // Check if the value is readable
    if info.flags & CTLFLAG_RD != CTLFLAG_RD {
        return Err(SysctlError::NoReadAccess);
    }

    // First get size of value in bytes
    let mut val_len = 0;
    let ret = unsafe {
        libc::sysctl(
            oid.as_ptr(),
            oid.len() as u32,
            std::ptr::null_mut(),
            &mut val_len,
            std::ptr::null(),
            0,
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // If the length reported is shorter than the type we will convert it into,
    // byteorder::LittleEndian::read_* will panic. Therefore, expand the value length to at
    // Least the size of the value.
    let val_minsize = std::cmp::max(val_len, info.ctl_type.min_type_size());

    // Then get value
    let mut val: Vec<libc::c_uchar> = vec![0; val_minsize];
    let mut new_val_len = val_len;
    let ret = unsafe {
        libc::sysctl(
            oid.as_ptr(),
            oid.len() as u32,
            val.as_mut_ptr() as *mut libc::c_void,
            &mut new_val_len,
            std::ptr::null(),
            0,
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Confirm that we did not read out of bounds
    assert!(new_val_len <= val_len);
    // Confirm that we got the bytes we requested
    if new_val_len < val_len {
        return Err(SysctlError::ShortRead {
            read: new_val_len,
            reported: val_len,
        });
    }

    // Special treatment for temperature ctls.
    if info.is_temperature() {
        return temperature(&info, &val);
    }

    // Wrap in Enum and return
    match info.ctl_type {
        CtlType::None => Ok(CtlValue::None),
        CtlType::Node => Ok(CtlValue::Node(val)),
        CtlType::Int => Ok(CtlValue::Int(byteorder::LittleEndian::read_i32(&val))),
        CtlType::String => match val.len() {
            0 => Ok(CtlValue::String("".to_string())),
            l => std::str::from_utf8(&val[..l - 1])
                .map_err(SysctlError::Utf8Error)
                .map(|s| CtlValue::String(s.into())),
        },
        CtlType::S64 => Ok(CtlValue::S64(byteorder::LittleEndian::read_i64(&val))),
        CtlType::Struct => Ok(CtlValue::Struct(val)),
        CtlType::Uint => Ok(CtlValue::Uint(byteorder::LittleEndian::read_u32(&val))),
        CtlType::Long => Ok(CtlValue::Long(byteorder::LittleEndian::read_i64(&val))),
        CtlType::Ulong => Ok(CtlValue::Ulong(byteorder::LittleEndian::read_u64(&val))),
        CtlType::U64 => Ok(CtlValue::U64(byteorder::LittleEndian::read_u64(&val))),
        CtlType::U8 => Ok(CtlValue::U8(val[0])),
        CtlType::U16 => Ok(CtlValue::U16(byteorder::LittleEndian::read_u16(&val))),
        CtlType::S8 => Ok(CtlValue::S8(val[0] as i8)),
        CtlType::S16 => Ok(CtlValue::S16(byteorder::LittleEndian::read_i16(&val))),
        CtlType::S32 => Ok(CtlValue::S32(byteorder::LittleEndian::read_i32(&val))),
        CtlType::U32 => Ok(CtlValue::U32(byteorder::LittleEndian::read_u32(&val))),
        _ => Err(SysctlError::UnknownType),
    }
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn value_oid(oid: &mut Vec<i32>) -> Result<CtlValue, SysctlError> {
    let info: CtlInfo = oidfmt(&oid)?;

    // Check if the value is readable
    if !(info.flags & CTLFLAG_RD == CTLFLAG_RD) {
        return Err(SysctlError::NoReadAccess);
    }

    // First get size of value in bytes
    let mut val_len = 0;
    let ret = unsafe {
        libc::sysctl(
            oid.as_mut_ptr(),
            oid.len() as u32,
            std::ptr::null_mut(),
            &mut val_len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // If the length reported is shorter than the type we will convert it into,
    // byteorder::LittleEndian::read_* will panic. Therefore, expand the value length to at
    // Least the size of the value.
    let val_minsize = std::cmp::max(val_len, info.ctl_type.min_type_size());

    // Then get value
    let mut val: Vec<libc::c_uchar> = vec![0; val_minsize];
    let mut new_val_len = val_len;
    let ret = unsafe {
        libc::sysctl(
            oid.as_mut_ptr(),
            oid.len() as u32,
            val.as_mut_ptr() as *mut libc::c_void,
            &mut new_val_len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Confirm that we did not read out of bounds
    assert!(new_val_len <= val_len);
    // The call can sometimes return bytes that are smaller than initially indicated, so it should
    // be safe to truncate it.  See a similar approach in golang module:
    // https://github.com/golang/sys/blob/43e60d72a8e2bd92ee98319ba9a384a0e9837c08/unix/syscall_bsd.go#L545-L548
    if new_val_len < val_len {
        val.truncate(new_val_len);
    }

    // Wrap in Enum and return
    match info.ctl_type {
        CtlType::None => Ok(CtlValue::None),
        CtlType::Node => Ok(CtlValue::Node(val)),
        CtlType::Int => match info.fmt.as_str() {
            "I" => Ok(CtlValue::Int(byteorder::LittleEndian::read_i32(&val))),
            "IU" => Ok(CtlValue::Uint(byteorder::LittleEndian::read_u32(&val))),
            "L" => Ok(CtlValue::Long(byteorder::LittleEndian::read_i64(&val))),
            "LU" => Ok(CtlValue::Ulong(byteorder::LittleEndian::read_u64(&val))),
            _ => Ok(CtlValue::None),
        }
        CtlType::String => match val.len() {
            0 => Ok(CtlValue::String("".to_string())),
            l => std::str::from_utf8(&val[..l - 1])
                .map_err(SysctlError::Utf8Error)
                .map(|s| CtlValue::String(s.into())),
        },
        CtlType::S64 => Ok(CtlValue::S64(byteorder::LittleEndian::read_i64(&val))),
        CtlType::Struct => Ok(CtlValue::Struct(val)),
        CtlType::Uint => Ok(CtlValue::Uint(byteorder::LittleEndian::read_u32(&val))),
        CtlType::Long => Ok(CtlValue::Long(byteorder::LittleEndian::read_i64(&val))),
        CtlType::Ulong => Ok(CtlValue::Ulong(byteorder::LittleEndian::read_u64(&val))),
        CtlType::U64 => Ok(CtlValue::U64(byteorder::LittleEndian::read_u64(&val))),
        CtlType::U8 => Ok(CtlValue::U8(val[0])),
        CtlType::U16 => Ok(CtlValue::U16(byteorder::LittleEndian::read_u16(&val))),
        CtlType::S8 => Ok(CtlValue::S8(val[0] as i8)),
        CtlType::S16 => Ok(CtlValue::S16(byteorder::LittleEndian::read_i16(&val))),
        CtlType::S32 => Ok(CtlValue::S32(byteorder::LittleEndian::read_i32(&val))),
        CtlType::U32 => Ok(CtlValue::U32(byteorder::LittleEndian::read_u32(&val))),
    }
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn value_oid_as<T>(oid: &[i32]) -> Result<Box<T>, SysctlError> {
    let val_enum = value_oid(oid)?;

    // Some structs are apparently reported as Node so this check is invalid..
    // let ctl_type = CtlType::from(&val_enum);
    // assert_eq!(CtlType::Struct, ctl_type, "Error type is not struct/opaque");

    // TODO: refactor this when we have better clue to what's going on
    if let CtlValue::Struct(val) = val_enum {
        // Make sure we got correct data size
        assert_eq!(
            std::mem::size_of::<T>(),
            val.len(),
            "Error memory size mismatch. Size of struct {}, size of data retrieved {}.",
            std::mem::size_of::<T>(),
            val.len()
        );

        // val is Vec<u8>
        let val_array: Box<[u8]> = val.into_boxed_slice();
        let val_raw: *mut T = Box::into_raw(val_array) as *mut T;
        let val_box: Box<T> = unsafe { Box::from_raw(val_raw) };
        Ok(val_box)
    } else if let CtlValue::Node(val) = val_enum {
        // Make sure we got correct data size
        assert_eq!(
            std::mem::size_of::<T>(),
            val.len(),
            "Error memory size mismatch. Size of struct {}, size of data retrieved {}.",
            std::mem::size_of::<T>(),
            val.len()
        );

        // val is Vec<u8>
        let val_array: Box<[u8]> = val.into_boxed_slice();
        let val_raw: *mut T = Box::into_raw(val_array) as *mut T;
        let val_box: Box<T> = unsafe { Box::from_raw(val_raw) };
        Ok(val_box)
    } else {
        Err(SysctlError::ExtractionError)
    }
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn value_oid_as<T>(oid: &mut Vec<i32>) -> Result<Box<T>, SysctlError> {
    let val_enum = value_oid(oid)?;

    // Some structs are apparently reported as Node so this check is invalid..
    // let ctl_type = CtlType::from(&val_enum);
    // assert_eq!(CtlType::Struct, ctl_type, "Error type is not struct/opaque");

    // TODO: refactor this when we have better clue to what's going on
    if let CtlValue::Struct(val) = val_enum {
        // Make sure we got correct data size
        assert_eq!(
            std::mem::size_of::<T>(),
            val.len(),
            "Error memory size mismatch. Size of struct {}, size of data retrieved {}.",
            std::mem::size_of::<T>(),
            val.len()
        );

        // val is Vec<u8>
        let val_array: Box<[u8]> = val.into_boxed_slice();
        let val_raw: *mut T = Box::into_raw(val_array) as *mut T;
        let val_box: Box<T> = unsafe { Box::from_raw(val_raw) };
        Ok(val_box)
    } else if let CtlValue::Node(val) = val_enum {
        // Make sure we got correct data size
        assert_eq!(
            std::mem::size_of::<T>(),
            val.len(),
            "Error memory size mismatch. Size of struct {}, size of data retrieved {}.",
            std::mem::size_of::<T>(),
            val.len()
        );

        // val is Vec<u8>
        let val_array: Box<[u8]> = val.into_boxed_slice();
        let val_raw: *mut T = Box::into_raw(val_array) as *mut T;
        let val_box: Box<T> = unsafe { Box::from_raw(val_raw) };
        Ok(val_box)
    } else {
        Err(SysctlError::ExtractionError)
    }
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn value_name(name: &str, ctl_type: CtlType, fmt: &str) -> Result<CtlValue, SysctlError> {
    let name = CString::new(name)?;

    // First get size of value in bytes
    let mut val_len = 0;
    let ret = unsafe {
        libc::sysctlbyname(
            name.as_ptr(),
            std::ptr::null_mut(),
            &mut val_len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // If the length reported is shorter than the type we will convert it into,
    // byteorder::LittleEndian::read_* will panic. Therefore, expand the value length to at
    // Least the size of the value.
    let val_minsize = std::cmp::max(val_len, ctl_type.min_type_size());

    // Then get value
    let mut val: Vec<libc::c_uchar> = vec![0; val_minsize];
    let mut new_val_len = val_len;
    let ret = unsafe {
        libc::sysctlbyname(
            name.as_ptr(),
            val.as_mut_ptr() as *mut libc::c_void,
            &mut new_val_len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Confirm that we did not read out of bounds
    assert!(new_val_len <= val_len);
    // The call can sometimes return bytes that are smaller than initially indicated, so it should
    // be safe to truncate it.  See a similar approach in golang module:
    // https://github.com/golang/sys/blob/43e60d72a8e2bd92ee98319ba9a384a0e9837c08/unix/syscall_bsd.go#L545-L548
    if new_val_len < val_len {
        val.truncate(new_val_len);
    }

    // Wrap in Enum and return
    match ctl_type {
        CtlType::None => Ok(CtlValue::None),
        CtlType::Node => Ok(CtlValue::Node(val)),
        CtlType::Int => match fmt {
            "I" => Ok(CtlValue::Int(byteorder::LittleEndian::read_i32(&val))),
            "IU" => Ok(CtlValue::Uint(byteorder::LittleEndian::read_u32(&val))),
            "L" => Ok(CtlValue::Long(byteorder::LittleEndian::read_i64(&val))),
            "LU" => Ok(CtlValue::Ulong(byteorder::LittleEndian::read_u64(&val))),
            _ => Ok(CtlValue::None),
        }
        CtlType::String => match val.len() {
            0 => Ok(CtlValue::String("".to_string())),
            l => std::str::from_utf8(&val[..l - 1])
                .map_err(SysctlError::Utf8Error)
                .map(|s| CtlValue::String(s.into())),
        },
        CtlType::S64 => Ok(CtlValue::S64(byteorder::LittleEndian::read_i64(&val))),
        CtlType::Struct => Ok(CtlValue::Struct(val)),
        CtlType::Uint => Ok(CtlValue::Uint(byteorder::LittleEndian::read_u32(&val))),
        CtlType::Long => Ok(CtlValue::Long(byteorder::LittleEndian::read_i64(&val))),
        CtlType::Ulong => Ok(CtlValue::Ulong(byteorder::LittleEndian::read_u64(&val))),
        CtlType::U64 => Ok(CtlValue::U64(byteorder::LittleEndian::read_u64(&val))),
        CtlType::U8 => Ok(CtlValue::U8(val[0])),
        CtlType::U16 => Ok(CtlValue::U16(byteorder::LittleEndian::read_u16(&val))),
        CtlType::S8 => Ok(CtlValue::S8(val[0] as i8)),
        CtlType::S16 => Ok(CtlValue::S16(byteorder::LittleEndian::read_i16(&val))),
        CtlType::S32 => Ok(CtlValue::S32(byteorder::LittleEndian::read_i32(&val))),
        CtlType::U32 => Ok(CtlValue::U32(byteorder::LittleEndian::read_u32(&val))),
    }
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn value_name_as<T>(name: &str, ctl_type: CtlType, fmt: &str) -> Result<Box<T>, SysctlError> {
    let val_enum = value_name(name, ctl_type, fmt)?;

    // Some structs are apparently reported as Node so this check is invalid..
    // let ctl_type = CtlType::from(&val_enum);
    // assert_eq!(CtlType::Struct, ctl_type, "Error type is not struct/opaque");

    // TODO: refactor this when we have better clue to what's going on
    if let CtlValue::Struct(val) = val_enum {
        // Make sure we got correct data size
        assert_eq!(
            std::mem::size_of::<T>(),
            val.len(),
            "Error memory size mismatch. Size of struct {}, size of data retrieved {}.",
            std::mem::size_of::<T>(),
            val.len()
        );

        // val is Vec<u8>
        let val_array: Box<[u8]> = val.into_boxed_slice();
        let val_raw: *mut T = Box::into_raw(val_array) as *mut T;
        let val_box: Box<T> = unsafe { Box::from_raw(val_raw) };
        Ok(val_box)
    } else if let CtlValue::Node(val) = val_enum {
        // Make sure we got correct data size
        assert_eq!(
            std::mem::size_of::<T>(),
            val.len(),
            "Error memory size mismatch. Size of struct {}, size of data retrieved {}.",
            std::mem::size_of::<T>(),
            val.len()
        );

        // val is Vec<u8>
        let val_array: Box<[u8]> = val.into_boxed_slice();
        let val_raw: *mut T = Box::into_raw(val_array) as *mut T;
        let val_box: Box<T> = unsafe { Box::from_raw(val_raw) };
        Ok(val_box)
    } else {
        Err(SysctlError::ExtractionError)
    }
}

fn value_to_bytes(value: CtlValue) -> Result<Vec<u8>, SysctlError> {
    // TODO rest of the types
    match value {
        CtlValue::String(v) => Ok(v.as_bytes().to_owned()),
        CtlValue::Ulong(v) => {
            let mut bytes = vec![];
            bytes.write_u64::<byteorder::LittleEndian>(v)?;
            Ok(bytes)
        }
        CtlValue::Uint(v) => {
            let mut bytes = vec![];
            bytes.write_u32::<byteorder::LittleEndian>(v)?;
            Ok(bytes)
        }
        CtlValue::Int(v) => {
            let mut bytes = vec![];
            bytes.write_i32::<byteorder::LittleEndian>(v)?;
            Ok(bytes)
        }
        CtlValue::U8(v) => {
            let mut bytes = vec![];
            bytes.write_u8(v)?;
            Ok(bytes)
        }
        _ => Err(SysctlError::MissingImplementation),
    }
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn set_oid_value(oid: &[libc::c_int], value: CtlValue) -> Result<CtlValue, SysctlError> {
    let info: CtlInfo = oidfmt(&oid)?;

    // Check if the value is writeable
    if info.flags & CTLFLAG_WR != CTLFLAG_WR {
        return Err(SysctlError::NoWriteAccess);
    }

    let ctl_type = CtlType::from(&value);
    assert_eq!(
        info.ctl_type, ctl_type,
        "Error type mismatch. Type given {:?}, sysctl type: {:?}",
        ctl_type, info.ctl_type
    );

    let bytes = value_to_bytes(value)?;

    let ret = unsafe {
        libc::sysctl(
            oid.as_ptr(),
            oid.len() as u32,
            std::ptr::null_mut(),
            std::ptr::null_mut(),
            bytes.as_ptr() as *const libc::c_void,
            bytes.len(),
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Get the new value and return for confirmation
    self::value_oid(oid)
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn set_oid_value(oid: &mut Vec<libc::c_int>, value: CtlValue) -> Result<CtlValue, SysctlError> {
    let info: CtlInfo = oidfmt(&oid)?;

    // Check if the value is writeable
    if !(info.flags & CTLFLAG_WR == CTLFLAG_WR) {
        return Err(SysctlError::NoWriteAccess);
    }

    let ctl_type = CtlType::from(&value);

    // Get the correct ctl type based on the format string
    let info_ctl_type = match info.ctl_type {
        CtlType::Int => match info.fmt.as_str() {
            "I" => CtlType::Int,
            "IU" => CtlType::Uint,
            "L" => CtlType::Long,
            "LU" => CtlType::Ulong,
            _ => return Err(SysctlError::MissingImplementation),
        }
        ctl_type => ctl_type,
    };

    assert_eq!(
        info_ctl_type, ctl_type,
        "Error type mismatch. Type given {:?}, sysctl type: {:?}",
        ctl_type, info_ctl_type
    );

    let bytes = value_to_bytes(value)?;

    // Set value
    let ret = unsafe {
        libc::sysctl(
            oid.as_mut_ptr(),
            oid.len() as u32,
            std::ptr::null_mut(),
            std::ptr::null_mut(),
            bytes.as_ptr() as *mut libc::c_void,
            bytes.len(),
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Get the new value and return for confirmation
    self::value_oid(oid)
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn set_name_value(
    name: &str,
    info_ctl_type: CtlType,
    fmt: &str,
    value: CtlValue,
) -> Result<CtlValue, SysctlError> {
    let c_name = CString::new(name)?;
    let ctl_type = CtlType::from(&value);

    // Get the correct ctl type based on the format string
    let info_ctl_type = match info_ctl_type {
        CtlType::Int => match fmt {
            "I" => CtlType::Int,
            "IU" => CtlType::Uint,
            "L" => CtlType::Long,
            "LU" => CtlType::Ulong,
            _ => return Err(SysctlError::MissingImplementation),
        }
        ctl_type => ctl_type,
    };

    assert_eq!(
        info_ctl_type, ctl_type,
        "Error type mismatch. Type given {:?}, sysctl type: {:?}",
        ctl_type, info_ctl_type
    );

    let bytes = value_to_bytes(value)?;

    // Set value
    let ret = unsafe {
        libc::sysctlbyname(
            c_name.as_ptr(),
            std::ptr::null_mut(),
            std::ptr::null_mut(),
            bytes.as_ptr() as *mut libc::c_void,
            bytes.len(),
        )
    };
    if ret < 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Get the new value and return for confirmation
    self::value_name(name, ctl_type, fmt)
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn oid2description(oid: &[libc::c_int]) -> Result<String, SysctlError> {
    // Request command for description
    let mut qoid: Vec<libc::c_int> = vec![0, 5];
    qoid.extend(oid);

    // Store results in u8 array
    let mut buf: [libc::c_uchar; libc::BUFSIZ as usize] = [0; libc::BUFSIZ as usize];
    let mut buf_len = std::mem::size_of_val(&buf);
    let ret = unsafe {
        libc::sysctl(
            qoid.as_ptr(),
            qoid.len() as u32,
            buf.as_mut_ptr() as *mut libc::c_void,
            &mut buf_len,
            std::ptr::null(),
            0,
        )
    };
    if ret != 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Use buf_len - 1 so that we remove the trailing NULL
    match std::str::from_utf8(&buf[..buf_len - 1]) {
        Ok(s) => Ok(s.to_owned()),
        Err(e) => Err(SysctlError::Utf8Error(e)),
    }
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn oid2name(oid: &[libc::c_int]) -> Result<String, SysctlError> {
    // Request command for name
    let mut qoid: Vec<libc::c_int> = vec![0, 1];
    qoid.extend(oid);

    // Store results in u8 array
    let mut buf: [libc::c_uchar; libc::BUFSIZ as usize] = [0; libc::BUFSIZ as usize];
    let mut buf_len = std::mem::size_of_val(&buf);
    let ret = unsafe {
        libc::sysctl(
            qoid.as_ptr(),
            qoid.len() as u32,
            buf.as_mut_ptr() as *mut libc::c_void,
            &mut buf_len,
            std::ptr::null(),
            0,
        )
    };
    if ret != 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Use buf_len - 1 so that we remove the trailing NULL
    match std::str::from_utf8(&buf[..buf_len - 1]) {
        Ok(s) => Ok(s.to_owned()),
        Err(e) => Err(SysctlError::Utf8Error(e)),
    }
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn oid2name(oid: &Vec<libc::c_int>) -> Result<String, SysctlError> {
    // Request command for name
    let mut qoid: Vec<libc::c_int> = vec![0, 1];
    qoid.extend(oid);

    // Store results in u8 array
    let mut buf: [libc::c_uchar; libc::BUFSIZ as usize] = [0; libc::BUFSIZ as usize];
    let mut buf_len = std::mem::size_of_val(&buf);
    let ret = unsafe {
        libc::sysctl(
            qoid.as_mut_ptr(),
            qoid.len() as u32,
            buf.as_mut_ptr() as *mut libc::c_void,
            &mut buf_len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret != 0 {
        return Err(SysctlError::IoError(std::io::Error::last_os_error()));
    }

    // Use buf_len - 1 so that we remove the trailing NULL
    match std::str::from_utf8(&buf[..buf_len - 1]) {
        Ok(s) => Ok(s.to_owned()),
        Err(e) => Err(SysctlError::Utf8Error(e)),
    }
}

#[cfg(not(any(target_os = "macos", target_os = "ios")))]
pub fn next_oid(oid: &[libc::c_int]) -> Result<Option<Vec<libc::c_int>>, SysctlError> {
    // Request command for next oid
    let mut qoid: Vec<libc::c_int> = vec![0, 2];
    qoid.extend(oid);

    let mut len: usize = CTL_MAXNAME as usize * std::mem::size_of::<libc::c_int>();

    // We get results in this vector
    let mut res: Vec<libc::c_int> = vec![0; CTL_MAXNAME as usize];

    let ret = unsafe {
        libc::sysctl(
            qoid.as_ptr(),
            qoid.len() as u32,
            res.as_mut_ptr() as *mut libc::c_void,
            &mut len,
            std::ptr::null(),
            0,
        )
    };
    if ret != 0 {
        let e = std::io::Error::last_os_error();

        if e.raw_os_error() == Some(libc::ENOENT) {
            return Ok(None);
        }
        return Err(SysctlError::IoError(e));
    }

    // len is in bytes, convert to number of libc::c_ints
    len /= std::mem::size_of::<libc::c_int>();

    // Trim result vector
    res.truncate(len);

    Ok(Some(res))
}

#[cfg(any(target_os = "macos", target_os = "ios"))]
pub fn next_oid(oid: &Vec<libc::c_int>) -> Result<Option<Vec<libc::c_int>>, SysctlError> {
    // Request command for next oid
    let mut qoid: Vec<libc::c_int> = vec![0, 2];
    qoid.extend(oid);

    let mut len: usize = CTL_MAXNAME as usize * std::mem::size_of::<libc::c_int>();

    // We get results in this vector
    let mut res: Vec<libc::c_int> = vec![0; CTL_MAXNAME as usize];

    let ret = unsafe {
        libc::sysctl(
            qoid.as_mut_ptr(),
            qoid.len() as u32,
            res.as_mut_ptr() as *mut libc::c_void,
            &mut len,
            std::ptr::null_mut(),
            0,
        )
    };
    if ret != 0 {
        let e = std::io::Error::last_os_error();

        if e.raw_os_error() == Some(libc::ENOENT) {
            return Ok(None);
        }
        return Err(SysctlError::IoError(e));
    }

    // len is in bytes, convert to number of libc::c_ints
    len /= std::mem::size_of::<libc::c_int>();

    // Trim result vector
    res.truncate(len);

    Ok(Some(res))
}

#[cfg(test)]
mod tests {
    use crate::Sysctl;

    #[test]
    fn ctl_name() {
        let oid = vec![libc::CTL_KERN, libc::KERN_OSREV];
        let name = super::oid2name(&oid).expect("Could not get name of kern.osrevision sysctl.");

        assert_eq!(name, "kern.osrevision");

        let ctl = crate::Ctl::Oid(oid);
        let name = ctl
            .name()
            .expect("Could not get name of kern.osrevision sysctl.");
        assert_eq!(name, "kern.osrevision");
    }

    #[test]
    fn ctl_type() {
        let oid = super::name2oid("kern").unwrap();
        let fmt = super::oidfmt(&oid).unwrap();
        assert_eq!(fmt.ctl_type, crate::CtlType::Node);
        let kern = crate::Ctl::new("kern").expect("Could not get kern node");
        let value_type = kern.value_type().expect("Could not get kern value type");
        assert_eq!(value_type, crate::CtlType::Node);

        let oid = super::name2oid("kern.osrelease").unwrap();
        let fmt = super::oidfmt(&oid).unwrap();
        assert_eq!(fmt.ctl_type, crate::CtlType::String);
        let osrelease =
            crate::Ctl::new("kern.osrelease").expect("Could not get kern.osrelease sysctl");
        let value_type = osrelease
            .value_type()
            .expect("Could not get kern.osrelease value type");
        assert_eq!(value_type, crate::CtlType::String);

        let oid = super::name2oid("kern.osrevision").unwrap();
        let fmt = super::oidfmt(&oid).unwrap();
        assert_eq!(fmt.ctl_type, crate::CtlType::Int);
        let osrevision =
            crate::Ctl::new("kern.osrevision").expect("Could not get kern.osrevision sysctl");
        let value_type = osrevision
            .value_type()
            .expect("Could notget kern.osrevision value type");
        assert_eq!(value_type, crate::CtlType::Int);
    }

    /// The name must be respresentable as a C String
    #[test]
    fn name2oid_invalid_name() {
        let r = super::name2oid("kern.\0.invalid.utf-8");
        assert!(matches!(r, Err(super::SysctlError::NotFound(_))));
    }
}

#[cfg(all(test, target_os = "freebsd"))]
mod tests_freebsd {
    #[test]
    fn ctl_mib() {
        let oid = super::name2oid("kern.proc.pid").unwrap();
        assert_eq!(oid.len(), 3);
        assert_eq!(oid[0], libc::CTL_KERN);
        assert_eq!(oid[1], libc::KERN_PROC);
        assert_eq!(oid[2], libc::KERN_PROC_PID);
    }
}

#[cfg(all(test, any(target_os = "macos", target_os = "ios")))]
mod tests_macos {
    #[test]
    fn ctl_mib() {
        let oid = super::name2oid("kern.proc.pid").unwrap();
        assert_eq!(oid.len(), 3);
        assert_eq!(oid[0], libc::CTL_KERN);
        assert_eq!(oid[1], libc::KERN_PROC);
        assert_eq!(oid[2], libc::KERN_PROC_PID);
    }
}
