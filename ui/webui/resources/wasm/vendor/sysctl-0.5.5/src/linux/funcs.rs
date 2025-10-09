// linux/funcs.rs

use ctl_error::*;
use ctl_value::*;

use std::io::{Read, Write};

pub fn path_to_name(name: &str) -> String {
    name.to_owned()
        .replace("/proc/sys/", "")
        .replace("..", ".")
        .replace("/", ".")
}

pub fn value(name: &str) -> Result<CtlValue, SysctlError> {
    let file_res = std::fs::OpenOptions::new()
        .read(true)
        .write(false)
        .open(&name);

    file_res
        .map(|mut file| {
            let mut v = String::new();
            file.read_to_string(&mut v)?;
            Ok(CtlValue::String(v.trim().to_owned()))
        })
        .map_err(|e| {
            if e.kind() == std::io::ErrorKind::NotFound {
                SysctlError::NotFound(name.into())
            } else {
                e.into()
            }
        })?
}

pub fn set_value(name: &str, v: CtlValue) -> Result<CtlValue, SysctlError> {
    let file_res = std::fs::OpenOptions::new()
        .read(false)
        .write(true)
        .open(&name);

    file_res
        .map(|mut file| match v {
            CtlValue::String(v) => {
                file.write_all(&v.as_bytes())?;
                value(&name)
            }
            _ => Err(std::io::Error::from(std::io::ErrorKind::InvalidData).into()),
        })
        .map_err(|e| {
            if e.kind() == std::io::ErrorKind::NotFound {
                SysctlError::NotFound(name.into())
            } else {
                e.into()
            }
        })?
}
