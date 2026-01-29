// ctl_info.rs

use ctl_flags::*;
use ctl_type::*;

#[derive(Debug, PartialEq)]
/// A structure representing control metadata
pub struct CtlInfo {
    /// The control type.
    pub ctl_type: CtlType,

    /// A string which specifies the format of the OID in
    /// a symbolic way.
    ///
    /// This format is used as a hint by sysctl(8) to
    /// apply proper data formatting for display purposes.
    ///
    /// Formats defined in sysctl(9):
    /// * `N`       node
    /// * `A`       char *
    /// * `I`       int
    /// * `IK[n]`   temperature in Kelvin, multiplied by an optional single
    ///    digit power of ten scaling factor: 1 (default) gives deciKelvin,
    ///    0 gives Kelvin, 3 gives milliKelvin
    /// * `IU`      unsigned int
    /// * `L`       long
    /// * `LU`      unsigned long
    /// * `Q`       quad_t
    /// * `QU`      u_quad_t
    /// * `S,TYPE`  struct TYPE structures
    pub fmt: String,

    pub flags: u32,
}

#[cfg(target_os = "freebsd")]
impl CtlInfo {
    /// Is this sysctl a temperature?
    pub fn is_temperature(&self) -> bool {
        self.fmt.starts_with("IK")
    }
}

impl CtlInfo {
    /// Return the flags for this sysctl.
    pub fn flags(&self) -> CtlFlags {
        CtlFlags::from_bits_truncate(self.flags)
    }

    /// If the sysctl is a structure, return the structure type string.
    ///
    /// Checks whether the format string starts with `S,` and returns the rest
    /// of the format string or None if the format String does not have a struct
    /// hint.
    pub fn struct_type(&self) -> Option<String> {
        if !self.fmt.starts_with("S,") {
            return None;
        }

        Some(self.fmt[2..].into())
    }
}
