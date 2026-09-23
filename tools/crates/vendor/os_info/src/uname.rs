use std::process::Command;

use log::error;
use nix::sys::utsname::uname as nix_uname;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[allow(dead_code)]
pub enum UnameField {
    Sysname,
    Release,
    Version,
    Machine,
    Nodename,
    OperatingSystem,
}

impl UnameField {
    fn cli_arg_name(&self) -> &'static str {
        match self {
            UnameField::Sysname => "-s",
            UnameField::Release => "-r",
            UnameField::Version => "-v",
            UnameField::Machine => "-m",
            UnameField::Nodename => "-n",
            UnameField::OperatingSystem => "-o",
        }
    }

    fn supports_uname_syscall(&self) -> bool {
        self != &UnameField::OperatingSystem
    }

    fn get_from_syscall(&self) -> Option<String> {
        if !self.supports_uname_syscall() {
            return None;
        }

        let utsname = match nix_uname() {
            Ok(utsname) => utsname,
            Err(e) => {
                log::error!("Failed to invoke native uname: {e:?}");
                return None;
            }
        };

        let val_os = match self {
            UnameField::Sysname => utsname.sysname(),
            UnameField::Release => utsname.release(),
            UnameField::Version => utsname.version(),
            UnameField::Machine => utsname.machine(),
            UnameField::Nodename => utsname.nodename(),
            UnameField::OperatingSystem => return None,
        };

        let val = val_os.to_str();
        if val.is_none() {
            error!("Failed to convert uname value to string: {val_os:?}");
            return None;
        }

        val.map(String::from)
    }
}

pub fn uname(field: UnameField) -> Option<String> {
    field
        .get_from_syscall()
        .or_else(|| uname_cli(field.cli_arg_name()))
}

fn uname_cli(arg: &str) -> Option<String> {
    Command::new("uname")
        .arg(arg)
        .output()
        .map_err(|e| {
            error!("Failed to invoke 'uname {}': {:?}", arg, e);
        })
        .ok()
        .and_then(|out| {
            if out.status.success() {
                Some(String::from_utf8_lossy(&out.stdout).trim_end().to_owned())
            } else {
                error!("'uname {}' failed with status: {:?}", arg, out.status);
                None
            }
        })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn uname_nonempty() {
        let val = uname(UnameField::Sysname).expect("uname failed");
        assert!(!val.is_empty());
    }
}
