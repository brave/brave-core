// unix/ctl.rs

use super::funcs::*;
use ctl_error::SysctlError;
use ctl_flags::CtlFlags;
use ctl_info::CtlInfo;
use ctl_type::CtlType;
use ctl_value::CtlValue;
use std::str::FromStr;
use traits::Sysctl;

/// This struct represents a system control.
#[derive(Debug, Clone, PartialEq)]
pub enum Ctl {
    Oid(Vec<libc::c_int>),
    Name(String, CtlType, String),
}

impl Ctl {
    pub fn oid(&self) -> Option<&Vec<libc::c_int>> {
        match self {
            Ctl::Oid(oid) => Some(oid),
            _ => None,
        }
    }
}

impl std::str::FromStr for Ctl {
    type Err = SysctlError;

    #[cfg_attr(feature = "cargo-clippy", allow(clippy::redundant_field_names))]
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let oid = name2oid(s)?;

        Ok(Ctl::Oid(oid))
    }
}

impl Sysctl for Ctl {
    fn new(name: &str) -> Result<Self, SysctlError> {
        Ctl::from_str(name)
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn new_with_type(name: &str, _ctl_type: CtlType, _fmt: &str) -> Result<Self, SysctlError> {
        Ctl::from_str(name)
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn new_with_type(name: &str, ctl_type: CtlType, fmt: &str) -> Result<Self, SysctlError> {
        let _ = name2oid(name)?;

        Ok(Ctl::Name(name.to_string(), ctl_type, fmt.to_string()))
    }

    fn name(&self) -> Result<String, SysctlError> {
        match self {
            Ctl::Oid(oid) => oid2name(&oid),
            Ctl::Name(name, ..) => Ok(name.clone()),
        }
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn value_type(&self) -> Result<CtlType, SysctlError> {
        match self {
            Ctl::Oid(oid) => {
                let info = oidfmt(oid)?;
                Ok(info.ctl_type)
            }
            Ctl::Name(_, ctl_type, _) => {
                Ok(*ctl_type)
            }
        }
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn value_type(&self) -> Result<CtlType, SysctlError> {
        match self {
            Ctl::Oid(oid) => {
                let info = oidfmt(oid)?;

                Ok(match info.ctl_type {
                    CtlType::Int => match info.fmt.as_str() {
                        "I" => CtlType::Int,
                        "IU" => CtlType::Uint,
                        "L" => CtlType::Long,
                        "LU" => CtlType::Ulong,
                        _ => return Err(SysctlError::MissingImplementation),
                    },
                    ctl_type => ctl_type,
                })
            }
            Ctl::Name(_, ctl_type, fmt) => {
                Ok(match ctl_type {
                    CtlType::Int => match fmt.as_str() {
                        "I" => CtlType::Int,
                        "IU" => CtlType::Uint,
                        "L" => CtlType::Long,
                        "LU" => CtlType::Ulong,
                        _ => return Err(SysctlError::MissingImplementation),
                    },
                    ctl_type => *ctl_type,
                })
            }
        }
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn description(&self) -> Result<String, SysctlError> {
        let oid = self.oid().ok_or(SysctlError::MissingImplementation)?;
        oid2description(oid)
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn description(&self) -> Result<String, SysctlError> {
        Ok("[N/A]".to_string())
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn value(&self) -> Result<CtlValue, SysctlError> {
        let oid = self.oid().ok_or(SysctlError::MissingImplementation)?;
        value_oid(oid)
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn value(&self) -> Result<CtlValue, SysctlError> {
        match self {
            Ctl::Oid(oid) => {
                let mut oid = oid.clone();
                value_oid(&mut oid)
            }
            Ctl::Name(name, ctl_type, fmt) => {
                value_name(name.as_str(), *ctl_type, fmt.as_str())
            }
        }
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn value_as<T>(&self) -> Result<Box<T>, SysctlError> {
        let oid = self.oid().ok_or(SysctlError::MissingImplementation)?;
        value_oid_as::<T>(oid)
    }

    fn value_string(&self) -> Result<String, SysctlError> {
        self.value().map(|v| format!("{}", v))
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn value_as<T>(&self) -> Result<Box<T>, SysctlError> {
        match self {
            Ctl::Oid(oid) => {
                let mut oid = oid.clone();
                value_oid_as::<T>(&mut oid)
            }
            Ctl::Name(name, ctl_type, fmt) => {
                value_name_as::<T>(name.as_str(), *ctl_type, fmt.as_str())
            }
        }
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn set_value(&self, value: CtlValue) -> Result<CtlValue, SysctlError> {
        let oid = self.oid().ok_or(SysctlError::MissingImplementation)?;
        set_oid_value(&oid, value)
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn set_value(&self, value: CtlValue) -> Result<CtlValue, SysctlError> {
        match self {
            Ctl::Oid(oid) => {
                let mut oid = oid.clone();
                set_oid_value(&mut oid, value)
            }
            Ctl::Name(name, ctl_type, fmt) => {
                set_name_value(name.as_str(), *ctl_type, fmt.as_str(), value)
            }
        }
    }

    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    fn set_value_string(&self, value: &str) -> Result<String, SysctlError> {
        let oid = self.oid().ok_or(SysctlError::MissingImplementation)?;
        let ctl_type = self.value_type()?;
        let _ = match ctl_type {
            CtlType::String => set_oid_value(oid, CtlValue::String(value.to_owned())),
            CtlType::Uint => set_oid_value(
                oid,
                CtlValue::Uint(value.parse::<u32>().map_err(|_| SysctlError::ParseError)?),
            ),
            CtlType::Int => set_oid_value(
                oid,
                CtlValue::Int(value.parse::<i32>().map_err(|_| SysctlError::ParseError)?),
            ),
            CtlType::Ulong => set_oid_value(
                oid,
                CtlValue::Ulong(value.parse::<u64>().map_err(|_| SysctlError::ParseError)?),
            ),
            CtlType::U8 => set_oid_value(
                oid,
                CtlValue::U8(value.parse::<u8>().map_err(|_| SysctlError::ParseError)?),
            ),
            _ => Err(SysctlError::MissingImplementation),
        }?;
        self.value_string()
    }

    #[cfg(any(target_os = "macos", target_os = "ios"))]
    fn set_value_string(&self, value: &str) -> Result<String, SysctlError> {
        let ctl_type = self.value_type()?;

        match self {
            Ctl::Oid(oid) => {
                let mut oid = oid.clone();
                let _ = match ctl_type {
                    CtlType::String => set_oid_value(&mut oid, CtlValue::String(value.to_owned())),
                    CtlType::Uint => set_oid_value(
                        &mut oid,
                        CtlValue::Uint(value.parse::<u32>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    CtlType::Int => set_oid_value(
                        &mut oid,
                        CtlValue::Int(value.parse::<i32>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    CtlType::Ulong => set_oid_value(
                        &mut oid,
                        CtlValue::Ulong(value.parse::<u64>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    CtlType::U8 => set_oid_value(
                        &mut oid,
                        CtlValue::U8(value.parse::<u8>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    _ => Err(SysctlError::MissingImplementation),
                }?;
            }
            Ctl::Name(name, ctl_type, fmt) => {
                let _ = match ctl_type {
                    CtlType::String => set_name_value(
                        name.as_str(),
                        *ctl_type,
                        fmt.as_str(),
                        CtlValue::String(value.to_owned()),
                    ),
                    CtlType::Uint => set_name_value(
                        name.as_str(),
                        *ctl_type,
                        fmt.as_str(),
                        CtlValue::Uint(value.parse::<u32>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    CtlType::Int => set_name_value(
                        name.as_str(),
                        *ctl_type,
                        fmt.as_str(),
                        CtlValue::Int(value.parse::<i32>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    CtlType::Ulong => set_name_value(
                        name.as_str(),
                        *ctl_type,
                        fmt.as_str(),
                        CtlValue::Ulong(value.parse::<u64>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    CtlType::U8 => set_name_value(
                        name.as_str(),
                        *ctl_type,
                        fmt.as_str(),
                        CtlValue::U8(value.parse::<u8>().map_err(|_| SysctlError::ParseError)?),
                    ),
                    _ => Err(SysctlError::MissingImplementation),
                }?;
            }
        }

        self.value_string()
    }

    fn flags(&self) -> Result<CtlFlags, SysctlError> {
        Ok(self.info()?.flags())
    }

    fn info(&self) -> Result<CtlInfo, SysctlError> {
        let oid = self.oid().ok_or(SysctlError::MissingImplementation)?;
        oidfmt(oid)
    }
}

#[cfg(test)]
mod tests {
    use crate::Sysctl;

    #[test]
    fn ctl_new() {
        let _ = super::Ctl::new("kern.ostype").expect("Ctl::new");
    }

    #[test]
    fn ctl_description() {
        let ctl = super::Ctl::new("kern.ostype").expect("Ctl::new");

        let descp = ctl.description();
        assert!(descp.is_ok());

        let descp = descp.unwrap();

        #[cfg(target_os = "freebsd")]
        assert_eq!(descp, "Operating system type");

        #[cfg(any(target_os = "macos", target_os = "ios", target_os = "linux"))]
        assert_eq!(descp, "[N/A]");
    }
}

#[cfg(all(test, target_os = "freebsd"))]
mod tests_freebsd {}
