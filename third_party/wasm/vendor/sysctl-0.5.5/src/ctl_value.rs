// ctl_value.rs

#[cfg(target_os = "freebsd")]
use temperature::Temperature;
use enum_as_inner::EnumAsInner;

/// An Enum that holds all values returned by sysctl calls.
/// Extract inner value with accessors like `as_int()`.
///
/// # Example
///
/// ```
/// # use sysctl::Sysctl;
/// if let Ok(ctl) = sysctl::Ctl::new("kern.osrevision") {
///     if let Ok(r) = ctl.value() {
///         if let Some(val) = r.as_int() {
///             println!("Value: {}", val);
///         }
///     }
/// }
/// ```
#[derive(Debug, EnumAsInner, PartialEq, PartialOrd)]
pub enum CtlValue {
    None,
    Node(Vec<u8>),
    Int(i32),
    String(String),
    S64(i64),
    Struct(Vec<u8>),
    Uint(u32),
    Long(i64),
    Ulong(u64),
    U64(u64),
    U8(u8),
    U16(u16),
    S8(i8),
    S16(i16),
    S32(i32),
    U32(u32),
    #[cfg(target_os = "freebsd")]
    Temperature(Temperature),
}

impl std::fmt::Display for CtlValue {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let s = match self {
            CtlValue::None => "[None]".to_owned(),
            CtlValue::Int(i) => format!("{}", i),
            CtlValue::Uint(i) => format!("{}", i),
            CtlValue::Long(i) => format!("{}", i),
            CtlValue::Ulong(i) => format!("{}", i),
            CtlValue::U8(i) => format!("{}", i),
            CtlValue::U16(i) => format!("{}", i),
            CtlValue::U32(i) => format!("{}", i),
            CtlValue::U64(i) => format!("{}", i),
            CtlValue::S8(i) => format!("{}", i),
            CtlValue::S16(i) => format!("{}", i),
            CtlValue::S32(i) => format!("{}", i),
            CtlValue::S64(i) => format!("{}", i),
            CtlValue::Struct(_) => "[Opaque Struct]".to_owned(),
            CtlValue::Node(_) => "[Node]".to_owned(),
            CtlValue::String(s) => s.to_owned(),
            #[cfg(target_os = "freebsd")]
            CtlValue::Temperature(t) => format!("{}", t.kelvin()),
        };
        write!(f, "{}", s)
    }
}

#[cfg(all(test, any(target_os = "linux", target_os = "android")))]
mod tests_linux {
    use crate::sys;
    use crate::Sysctl;

    #[test]
    fn ctl_value_string() {
        // NOTE: Some linux distributions require Root permissions
        //        e.g Debian.
        let output = std::process::Command::new("sysctl")
            .arg("-n")
            .arg("kernel.version")
            .output()
            .expect("failed to execute process");
        let ver = String::from_utf8_lossy(&output.stdout);
        let s = match sys::funcs::value("/proc/sys/kernel/version") {
            Ok(crate::CtlValue::String(s)) => s,
            _ => panic!("crate::value() returned Error"),
        };
        assert_eq!(s.trim(), ver.trim());

        let kernversion = crate::Ctl::new("kernel.version").unwrap();
        let s = match kernversion.value() {
            Ok(crate::CtlValue::String(s)) => s,
            _ => "...".into(),
        };
        assert_eq!(s.trim(), ver.trim());
    }
}

#[cfg(all(test, any(target_os = "freebsd", target_os = "macos", target_os = "ios")))]
mod tests_unix {
    use crate::sys;
    use crate::Sysctl;

    #[test]
    fn ctl_value_string() {
        let output = std::process::Command::new("sysctl")
            .arg("-n")
            .arg("kern.version")
            .output()
            .expect("failed to execute process");
        let ver = String::from_utf8_lossy(&output.stdout);
        let ctl = crate::Ctl::new("kern.version").expect("Ctl::new");
        let s = match ctl.value() {
            Ok(crate::CtlValue::String(s)) => s,
            _ => "...".into(),
        };
        assert_eq!(s.trim(), ver.trim());

        let kernversion = crate::Ctl::new("kern.version").unwrap();
        let s = match kernversion.value() {
            Ok(crate::CtlValue::String(s)) => s,
            _ => "...".into(),
        };
        assert_eq!(s.trim(), ver.trim());
    }

    #[test]
    fn ctl_value_int() {
        let output = std::process::Command::new("sysctl")
            .arg("-n")
            .arg("kern.osrevision")
            .output()
            .expect("failed to execute process");
        let rev_str = String::from_utf8_lossy(&output.stdout);
        let rev = rev_str.trim().parse::<i32>().unwrap();

        let ctl =
            crate::Ctl::new("kern.osrevision").expect("Could not get kern.osrevision sysctl.");
        let n = match ctl.value() {
            Ok(crate::CtlValue::Int(n)) => n,
            Ok(_) => 0,
            Err(_) => 0,
        };
        assert_eq!(n, rev);
    }

    #[test]
    fn ctl_value_oid_int() {
        let output = std::process::Command::new("sysctl")
            .arg("-n")
            .arg("kern.osrevision")
            .output()
            .expect("failed to execute process");
        let rev_str = String::from_utf8_lossy(&output.stdout);
        let rev = rev_str.trim().parse::<i32>().unwrap();
        let n = match sys::funcs::value_oid(&mut vec![libc::CTL_KERN, libc::KERN_OSREV]) {
            Ok(crate::CtlValue::Int(n)) => n,
            Ok(_) => 0,
            Err(_) => 0,
        };
        assert_eq!(n, rev);
    }

    #[test]
    fn ctl_struct_type() {
        let info = crate::CtlInfo {
            ctl_type: crate::CtlType::Int,
            fmt: "S,TYPE".into(),
            flags: 0,
        };

        assert_eq!(info.struct_type(), Some("TYPE".into()));

        let info = crate::CtlInfo {
            ctl_type: crate::CtlType::Int,
            fmt: "I".into(),
            flags: 0,
        };
        assert_eq!(info.struct_type(), None);
    }
}
