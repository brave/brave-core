// traits.rs

use ctl_error::SysctlError;
use ctl_flags::CtlFlags;
use ctl_info::CtlInfo;
use ctl_type::CtlType;
use ctl_value::CtlValue;

pub trait Sysctl {
    /// Construct a Ctl from the name.
    ///
    /// Returns a result containing the struct Ctl on success or a SysctlError
    /// on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// #
    /// let ctl = sysctl::Ctl::new("kern.ostype");
    /// ```
    ///
    /// If the sysctl does not exist, `Err(SysctlError::NotFound)` is returned.
    /// ```
    /// # use sysctl::Sysctl;
    /// #
    /// let ctl = sysctl::Ctl::new("this.sysctl.does.not.exist");
    /// match ctl {
    ///     Err(sysctl::SysctlError::NotFound(_)) => (),
    ///     Err(e) => panic!(format!("Wrong error type returned: {:?}", e)),
    ///     Ok(_) => panic!("Nonexistent sysctl seems to exist"),
    /// }
    /// ```
    fn new(name: &str) -> Result<Self, SysctlError>
    where
        Self: std::marker::Sized;

    /// Construct a Ctl from the name, type and format.
    ///
    /// Returns a result containing the struct Ctl on success or a SysctlError
    /// on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::{CtlType, Sysctl};
    /// #
    /// let ctl = sysctl::Ctl::new_with_type("kern.ostype", CtlType::String, "");
    /// ```
    ///
    /// If the sysctl does not exist, `Err(SysctlError::NotFound)` is returned.
    /// ```
    /// # use sysctl::{CtlType, Sysctl};
    /// #
    /// let ctl = sysctl::Ctl::new_with_type("this.sysctl.does.not.exist", CtlType::String, "");
    /// match ctl {
    ///     Err(sysctl::SysctlError::NotFound(_)) => (),
    ///     Err(e) => panic!("Wrong error type returned: {:?}", e),
    ///     Ok(_) => panic!("Nonexistent sysctl seems to exist"),
    /// }
    /// ```
    fn new_with_type(name: &str, ctl_type: CtlType, fmt: &str) -> Result<Self, SysctlError>
    where
        Self: std::marker::Sized;

    /// Returns a result containing the sysctl name on success, or a
    /// SysctlError on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.ostype") {
    ///     assert_eq!(ctl.name().unwrap(), "kern.ostype");
    /// }
    /// ```
    fn name(&self) -> Result<String, SysctlError>;

    /// Returns a result containing the sysctl value type on success,
    /// or a Sysctl Error on failure.
    ///
    /// # Example
    ///
    /// ```
    /// # use sysctl::Sysctl;
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.ostype") {
    ///     let value_type = ctl.value_type().unwrap();
    ///     assert_eq!(value_type, sysctl::CtlType::String);
    /// }
    /// ```
    fn value_type(&self) -> Result<CtlType, SysctlError>;

    /// Returns a result containing the sysctl description if success, or an
    /// Error on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.ostype") {
    ///     println!("Description: {:?}", ctl.description())
    /// }
    /// ```
    fn description(&self) -> Result<String, SysctlError>;

    /// Returns a result containing the sysctl value on success, or a
    /// SysctlError on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.ostype") {
    ///     println!("Value: {:?}", ctl.value());
    /// }
    /// ```
    fn value(&self) -> Result<CtlValue, SysctlError>;

    /// A generic method that takes returns a result containing the sysctl
    /// value if success, or a SysctlError on failure.
    ///
    /// May only be called for sysctls of type Opaque or Struct.
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// #[derive(Debug)]
    /// #[repr(C)]
    /// struct ClockInfo {
    ///     hz: libc::c_int, /* clock frequency */
    ///     tick: libc::c_int, /* micro-seconds per hz tick */
    ///     spare: libc::c_int,
    ///     stathz: libc::c_int, /* statistics clock frequency */
    ///     profhz: libc::c_int, /* profiling clock frequency */
    /// }
    ///
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.clockrate") {
    ///     println!("{:?}", ctl.value_as::<ClockInfo>());
    /// }
    /// ```
    fn value_as<T>(&self) -> Result<Box<T>, SysctlError>;

    /// Returns a result containing the sysctl value as String on
    /// success, or a SysctlError on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.osrevision") {
    ///     println!("Value: {:?}", ctl.value_string());
    /// }
    /// ```
    fn value_string(&self) -> Result<String, SysctlError>;

    #[cfg_attr(feature = "cargo-clippy", allow(clippy::needless_doctest_main))]
    /// Sets the value of a sysctl.
    /// Fetches and returns the new value if successful, or returns a
    /// SysctlError on failure.
    /// # Example
    /// ```
    /// use sysctl::Sysctl;
    ///
    /// fn main() {
    ///     if unsafe { libc::getuid() } == 0 {
    ///         if let Ok(ctl) = sysctl::Ctl::new("hw.usb.debug") {
    ///             let org = ctl.value().unwrap();
    ///             let set = ctl.set_value(sysctl::CtlValue::Int(1)).unwrap();
    ///             assert_eq!(set, sysctl::CtlValue::Int(1));
    ///             ctl.set_value(org).unwrap();
    ///         }
    ///     }
    /// }
    fn set_value(&self, value: CtlValue) -> Result<CtlValue, SysctlError>;

    #[cfg_attr(feature = "cargo-clippy", allow(clippy::needless_doctest_main))]
    /// Sets the value of a sysctl with input as string.
    /// Fetches and returns the new value if successful, or returns a
    /// SysctlError on failure.
    /// # Example
    /// ```
    /// use sysctl::Sysctl;
    ///
    /// fn main() {
    ///     if unsafe { libc::getuid() } == 0 {
    ///         if let Ok(ctl) = sysctl::Ctl::new("hw.usb.debug") {
    ///             let org = ctl.value_string().unwrap();
    ///             let set = ctl.set_value_string("1");
    ///             println!("hw.usb.debug: -> {:?}", set);
    ///             ctl.set_value_string(&org).unwrap();
    ///         }
    ///     }
    /// }
    fn set_value_string(&self, value: &str) -> Result<String, SysctlError>;

    /// Get the flags for a sysctl.
    ///
    /// Returns a Result containing the flags on success,
    /// or a SysctlError on failure.
    ///
    /// # Example
    /// ```
    /// # use sysctl::Sysctl;
    /// if let Ok(ctl) = sysctl::Ctl::new("kern.ostype") {
    ///     let readable = ctl.flags().unwrap().contains(sysctl::CtlFlags::RD);
    ///     assert!(readable);
    /// }
    /// ```
    fn flags(&self) -> Result<CtlFlags, SysctlError>;

    #[cfg_attr(feature = "cargo-clippy", allow(clippy::needless_doctest_main))]
    /// Returns a Result containing the control metadata for a sysctl.
    ///
    /// Returns a Result containing the CtlInfo struct on success,
    /// or a SysctlError on failure.
    ///
    /// # Example
    /// ```
    /// use sysctl::Sysctl;
    ///
    /// fn main() {
    ///     if let Ok(ctl) = sysctl::Ctl::new("kern.osrevision") {
    ///         let info = ctl.info().unwrap();
    ///
    ///         // kern.osrevision is not a structure.
    ///         assert_eq!(info.struct_type(), None);
    ///     }
    /// }
    /// ```
    fn info(&self) -> Result<CtlInfo, SysctlError>;
}
