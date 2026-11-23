//! Operating systems

use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

use crate::error::Error;

/// `target_os`: Operating system of the target.
///
/// This value is closely related to the second and third element
/// of the platform target triple, though it is not identical.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Os {
    /// `aix`
    Aix,

    /// `amdhsa`
    Amdhsa,

    /// `android`: Google's Android mobile operating system
    Android,

    /// `cuda`: CUDA parallel computing platform
    Cuda,

    /// `cygwin`
    Cygwin,

    /// `dragonfly`: DragonflyBSD
    Dragonfly,

    /// `emscripten`: The emscripten JavaScript transpiler
    Emscripten,

    /// `espidf`
    Espidf,

    /// `freebsd`: The FreeBSD operating system
    FreeBSD,

    /// `fuchsia`: Google's next-gen Rust OS
    Fuchsia,

    /// `haiku`: Haiku, an open source BeOS clone
    Haiku,

    /// `helenos`
    HelenOS,

    /// `hermit`: HermitCore is a novel unikernel operating system targeting a scalable and predictable runtime behavior for HPC and cloud environments
    Hermit,

    /// `horizon`
    Horizon,

    /// `hurd`
    Hurd,

    /// `hyperlight`
    Hyperlight,

    /// `illumos`: illumos is a partly free and open-source Unix operating system based on OpenSolaris
    IllumOS,

    /// `ios`: Apple's iOS mobile operating system
    #[allow(non_camel_case_types)]
    iOS,

    /// `l4re`
    L4re,

    /// `linux`: Linux
    Linux,

    /// `lynxos178`
    Lynxos178,

    /// `macos`: Apple's Mac OS X
    MacOS,

    /// `managarm`
    Managarm,

    /// `motor`
    Motor,

    /// `netbsd`: The NetBSD operating system
    NetBSD,

    /// `none`
    None,

    /// `nto`
    Nto,

    /// `nuttx`
    Nuttx,

    /// `openbsd`: The OpenBSD operating system
    OpenBSD,

    /// `ps3`
    Ps3,

    /// `psp`
    Psp,

    /// `psx`
    Psx,

    /// `qnx`
    Qnx,

    /// `qtee`
    Qtee,

    /// `qurt`
    Qurt,

    /// `redox`: Redox, a Unix-like OS written in Rust
    Redox,

    /// `rtems`
    Rtems,

    /// `serenity`
    Serenity,

    /// `solaris`: Oracle's (formerly Sun) Solaris operating system
    Solaris,

    /// `solid_asp3`
    SolidAsp3,

    /// `teeos`
    TeeOS,

    /// `trusty`
    Trusty,

    /// `tvos`
    TvOS,

    /// `uefi`
    Uefi,

    /// `unknown`
    Unknown,

    /// `vexos`
    VexOS,

    /// `visionos`
    VisionOS,

    /// `vita`
    Vita,

    /// `vxworks`: VxWorks is a deterministic, priority-based preemptive RTOS with low latency and minimal jitter
    VxWorks,

    /// `wasi`: The WebAssembly System Interface
    Wasi,

    /// `watchos`
    WatchOS,

    /// `windows`: Microsoft's Windows operating system
    Windows,

    /// `xous`
    Xous,

    /// `zkvm`
    Zkvm,
}

impl Os {
    /// String representing this `Os` which matches `#[cfg(target_os)]`
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Aix => "aix",
            Self::Amdhsa => "amdhsa",
            Self::Android => "android",
            Self::Cuda => "cuda",
            Self::Cygwin => "cygwin",
            Self::Dragonfly => "dragonfly",
            Self::Emscripten => "emscripten",
            Self::Espidf => "espidf",
            Self::FreeBSD => "freebsd",
            Self::Fuchsia => "fuchsia",
            Self::Haiku => "haiku",
            Self::HelenOS => "helenos",
            Self::Hermit => "hermit",
            Self::Horizon => "horizon",
            Self::Hurd => "hurd",
            Self::Hyperlight => "hyperlight",
            Self::IllumOS => "illumos",
            Self::iOS => "ios",
            Self::L4re => "l4re",
            Self::Linux => "linux",
            Self::Lynxos178 => "lynxos178",
            Self::MacOS => "macos",
            Self::Managarm => "managarm",
            Self::Motor => "motor",
            Self::NetBSD => "netbsd",
            Self::None => "none",
            Self::Nto => "nto",
            Self::Nuttx => "nuttx",
            Self::OpenBSD => "openbsd",
            Self::Ps3 => "ps3",
            Self::Psp => "psp",
            Self::Psx => "psx",
            Self::Qnx => "qnx",
            Self::Qtee => "qtee",
            Self::Qurt => "qurt",
            Self::Redox => "redox",
            Self::Rtems => "rtems",
            Self::Serenity => "serenity",
            Self::Solaris => "solaris",
            Self::SolidAsp3 => "solid_asp3",
            Self::TeeOS => "teeos",
            Self::Trusty => "trusty",
            Self::TvOS => "tvos",
            Self::Uefi => "uefi",
            Self::Unknown => "unknown",
            Self::VexOS => "vexos",
            Self::VisionOS => "visionos",
            Self::Vita => "vita",
            Self::VxWorks => "vxworks",
            Self::Wasi => "wasi",
            Self::WatchOS => "watchos",
            Self::Windows => "windows",
            Self::Xous => "xous",
            Self::Zkvm => "zkvm",
        }
    }
}

impl FromStr for Os {
    type Err = Error;

    /// Create a new `Os` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "aix" => Self::Aix,
            "amdhsa" => Self::Amdhsa,
            "android" => Self::Android,
            "cuda" => Self::Cuda,
            "cygwin" => Self::Cygwin,
            "dragonfly" => Self::Dragonfly,
            "emscripten" => Self::Emscripten,
            "espidf" => Self::Espidf,
            "freebsd" => Self::FreeBSD,
            "fuchsia" => Self::Fuchsia,
            "haiku" => Self::Haiku,
            "helenos" => Self::HelenOS,
            "hermit" => Self::Hermit,
            "horizon" => Self::Horizon,
            "hurd" => Self::Hurd,
            "hyperlight" => Self::Hyperlight,
            "illumos" => Self::IllumOS,
            "ios" => Self::iOS,
            "l4re" => Self::L4re,
            "linux" => Self::Linux,
            "lynxos178" => Self::Lynxos178,
            "macos" => Self::MacOS,
            "managarm" => Self::Managarm,
            "motor" => Self::Motor,
            "netbsd" => Self::NetBSD,
            "none" => Self::None,
            "nto" => Self::Nto,
            "nuttx" => Self::Nuttx,
            "openbsd" => Self::OpenBSD,
            "ps3" => Self::Ps3,
            "psp" => Self::Psp,
            "psx" => Self::Psx,
            "qnx" => Self::Qnx,
            "qtee" => Self::Qtee,
            "qurt" => Self::Qurt,
            "redox" => Self::Redox,
            "rtems" => Self::Rtems,
            "serenity" => Self::Serenity,
            "solaris" => Self::Solaris,
            "solid_asp3" => Self::SolidAsp3,
            "teeos" => Self::TeeOS,
            "trusty" => Self::Trusty,
            "tvos" => Self::TvOS,
            "uefi" => Self::Uefi,
            "unknown" => Self::Unknown,
            "vexos" => Self::VexOS,
            "visionos" => Self::VisionOS,
            "vita" => Self::Vita,
            "vxworks" => Self::VxWorks,
            "wasi" => Self::Wasi,
            "watchos" => Self::WatchOS,
            "windows" => Self::Windows,
            "xous" => Self::Xous,
            "zkvm" => Self::Zkvm,
            _ => return Err(Error),
        };

        Ok(result)
    }
}

impl fmt::Display for Os {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for Os {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for Os {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let string = std::string::String::deserialize(deserializer)?;
        string.parse().map_err(|_| {
            D::Error::custom(std::format!(
                "Unrecognized value '{}' for target_os",
                string
            ))
        })
    }
}
