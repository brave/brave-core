//! Operating systems

use crate::error::Error;
use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

/// `target_os`: Operating system of the target.
///
/// This value is closely related to the second and third element
/// of the platform target triple, though it is not identical.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum OS {
    /// `aix`
    Aix,

    /// `android`: Google's Android mobile operating system
    Android,

    /// `cuda`: CUDA parallel computing platform
    Cuda,

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

    /// `hermit`: HermitCore is a novel unikernel operating system targeting a scalable and predictable runtime behavior for HPC and cloud environments
    Hermit,

    /// `horizon`
    Horizon,

    /// `hurd`
    Hurd,

    /// `illumos`: illumos is a partly free and open-source Unix operating system based on OpenSolaris
    IllumOS,

    /// `ios`: Apple's iOS mobile operating system
    #[allow(non_camel_case_types)]
    iOS,

    /// `l4re`
    L4re,

    /// `linux`: Linux
    Linux,

    /// `macos`: Apple's Mac OS X
    MacOS,

    /// `netbsd`: The NetBSD operating system
    NetBSD,

    /// `none`
    None,

    /// `nto`
    Nto,

    /// `openbsd`: The OpenBSD operating system
    OpenBSD,

    /// `psp`
    Psp,

    /// `redox`: Redox, a Unix-like OS written in Rust
    Redox,

    /// `solaris`: Oracle's (formerly Sun) Solaris operating system
    Solaris,

    /// `solid_asp3`
    SolidAsp3,

    /// `teeos`
    TeeOS,

    /// `tvos`
    TvOS,

    /// `uefi`
    Uefi,

    /// `unknown`
    Unknown,

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
}

impl OS {
    /// String representing this `OS` which matches `#[cfg(target_os)]`
    pub fn as_str(self) -> &'static str {
        match self {
            OS::Aix => "aix",
            OS::Android => "android",
            OS::Cuda => "cuda",
            OS::Dragonfly => "dragonfly",
            OS::Emscripten => "emscripten",
            OS::Espidf => "espidf",
            OS::FreeBSD => "freebsd",
            OS::Fuchsia => "fuchsia",
            OS::Haiku => "haiku",
            OS::Hermit => "hermit",
            OS::Horizon => "horizon",
            OS::Hurd => "hurd",
            OS::IllumOS => "illumos",
            OS::iOS => "ios",
            OS::L4re => "l4re",
            OS::Linux => "linux",
            OS::MacOS => "macos",
            OS::NetBSD => "netbsd",
            OS::None => "none",
            OS::Nto => "nto",
            OS::OpenBSD => "openbsd",
            OS::Psp => "psp",
            OS::Redox => "redox",
            OS::Solaris => "solaris",
            OS::SolidAsp3 => "solid_asp3",
            OS::TeeOS => "teeos",
            OS::TvOS => "tvos",
            OS::Uefi => "uefi",
            OS::Unknown => "unknown",
            OS::Vita => "vita",
            OS::VxWorks => "vxworks",
            OS::Wasi => "wasi",
            OS::WatchOS => "watchos",
            OS::Windows => "windows",
            OS::Xous => "xous",
        }
    }
}

impl FromStr for OS {
    type Err = Error;

    /// Create a new `OS` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "aix" => OS::Aix,
            "android" => OS::Android,
            "cuda" => OS::Cuda,
            "dragonfly" => OS::Dragonfly,
            "emscripten" => OS::Emscripten,
            "espidf" => OS::Espidf,
            "freebsd" => OS::FreeBSD,
            "fuchsia" => OS::Fuchsia,
            "haiku" => OS::Haiku,
            "hermit" => OS::Hermit,
            "horizon" => OS::Horizon,
            "hurd" => OS::Hurd,
            "illumos" => OS::IllumOS,
            "ios" => OS::iOS,
            "l4re" => OS::L4re,
            "linux" => OS::Linux,
            "macos" => OS::MacOS,
            "netbsd" => OS::NetBSD,
            "none" => OS::None,
            "nto" => OS::Nto,
            "openbsd" => OS::OpenBSD,
            "psp" => OS::Psp,
            "redox" => OS::Redox,
            "solaris" => OS::Solaris,
            "solid_asp3" => OS::SolidAsp3,
            "teeos" => OS::TeeOS,
            "tvos" => OS::TvOS,
            "uefi" => OS::Uefi,
            "unknown" => OS::Unknown,
            "vita" => OS::Vita,
            "vxworks" => OS::VxWorks,
            "wasi" => OS::Wasi,
            "watchos" => OS::WatchOS,
            "windows" => OS::Windows,
            "xous" => OS::Xous,
            _ => return Err(Error),
        };

        Ok(result)
    }
}

impl fmt::Display for OS {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for OS {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for OS {
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
