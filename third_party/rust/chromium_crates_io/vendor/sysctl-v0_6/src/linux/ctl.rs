// linux/ctl.rs

use super::funcs::{path_to_name, set_value, value};
use consts::*;
use ctl_error::SysctlError;
use ctl_flags::CtlFlags;
use ctl_info::CtlInfo;
use ctl_type::CtlType;
use ctl_value::CtlValue;
use std::str::FromStr;
use traits::Sysctl;

/// This struct represents a system control.
#[derive(Debug, Clone, PartialEq)]
pub struct Ctl {
    name: String,
}

impl FromStr for Ctl {
    type Err = SysctlError;

    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let ctl = Ctl {
            name: path_to_name(name),
        };
        let _ =
            std::fs::File::open(ctl.path()).map_err(|_| SysctlError::NotFound(name.to_owned()))?;
        Ok(ctl)
    }
}

impl Ctl {
    pub fn path(&self) -> String {
        format!("/proc/sys/{}", self.name.replace(".", "/"))
    }
}

impl Sysctl for Ctl {
    fn new(name: &str) -> Result<Self, SysctlError> {
        Ctl::from_str(name)
    }

    fn new_with_type(name: &str, _ctl_type: CtlType, _fmt: &str) -> Result<Self, SysctlError> {
        Ctl::from_str(name)
    }

    fn name(&self) -> Result<String, SysctlError> {
        Ok(self.name.clone())
    }

    fn value_type(&self) -> Result<CtlType, SysctlError> {
        let md = std::fs::metadata(&self.path()).map_err(SysctlError::IoError)?;
        if md.is_dir() {
            Ok(CtlType::Node)
        } else {
            Ok(CtlType::String)
        }
    }

    fn description(&self) -> Result<String, SysctlError> {
        Ok("[N/A]".to_owned())
    }

    fn value(&self) -> Result<CtlValue, SysctlError> {
        value(&self.path())
    }

    fn value_string(&self) -> Result<String, SysctlError> {
        self.value().map(|v| format!("{}", v))
    }

    fn value_as<T>(&self) -> Result<Box<T>, SysctlError> {
        Err(SysctlError::NotSupported)
    }

    fn set_value(&self, value: CtlValue) -> Result<CtlValue, SysctlError> {
        set_value(&self.path(), value)
    }

    fn set_value_string(&self, value: &str) -> Result<String, SysctlError> {
        self.set_value(CtlValue::String(value.to_owned()))?;
        self.value_string()
    }

    fn flags(&self) -> Result<CtlFlags, SysctlError> {
        Ok(self.info()?.flags())
    }

    fn info(&self) -> Result<CtlInfo, SysctlError> {
        let md = std::fs::metadata(&self.path()).map_err(SysctlError::IoError)?;
        let mut flags = 0;
        if md.permissions().readonly() {
            flags |= CTLFLAG_RD;
        } else {
            flags |= CTLFLAG_RW;
        }
        let s = CtlInfo {
            ctl_type: CtlType::String,
            fmt: "".to_owned(),
            flags: flags,
        };
        Ok(s)
    }
}

#[cfg(test)]
mod tests {
    use crate::Sysctl;

    #[test]
    fn ctl_new() {
        let _ = super::Ctl::new("kernel.ostype").expect("Ctl::new");
    }
}
