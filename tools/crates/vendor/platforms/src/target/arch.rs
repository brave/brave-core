//! Rust architectures

use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

use crate::error::Error;

/// `target_arch`: Target CPU architecture
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Arch {
    /// `aarch64`: ARMv8 64-bit architecture
    AArch64,

    /// `amdgpu`
    Amdgpu,

    /// `arm`: 32-bit ARM architecture
    Arm,

    /// `arm64ec`
    Arm64ec,

    /// `avr`
    Avr,

    /// `bpf`
    Bpf,

    /// `csky`
    Csky,

    /// `hexagon`
    Hexagon,

    /// `loongarch32`
    Loongarch32,

    /// `loongarch64`
    Loongarch64,

    /// `m68k`
    M68k,

    /// `mips`: 32-bit MIPS CPU architecture
    Mips,

    /// `mips32r6`
    Mips32r6,

    /// `mips64`: 64-bit MIPS CPU architecture
    Mips64,

    /// `mips64r6`
    Mips64r6,

    /// `msp430`: 16-bit MSP430 microcontrollers
    Msp430,

    /// `nvptx64`: 64-bit NVIDIA PTX
    Nvptx64,

    /// `powerpc`: 32-bit POWERPC platform
    PowerPc,

    /// `powerpc64`: 64-bit POWERPC platform
    PowerPc64,

    /// `riscv32`
    Riscv32,

    /// `riscv64`
    Riscv64,

    /// `s390x`: 64-bit IBM z/Architecture
    S390X,

    /// `sparc`: 32-bit SPARC CPU architecture
    Sparc,

    /// `sparc64`: 64-bit SPARC CPU architecture
    Sparc64,

    /// `wasm32`: Web Assembly (32-bit)
    Wasm32,

    /// `wasm64`
    Wasm64,

    /// `x86`: Generic x86 CPU architecture
    X86,

    /// `x86_64`: 'AMD64' CPU architecture
    X86_64,

    /// `xtensa`
    Xtensa,
}

impl Arch {
    /// String representing this `Arch` which matches `#[cfg(target_arch)]`
    pub fn as_str(self) -> &'static str {
        match self {
            Self::AArch64 => "aarch64",
            Self::Amdgpu => "amdgpu",
            Self::Arm => "arm",
            Self::Arm64ec => "arm64ec",
            Self::Avr => "avr",
            Self::Bpf => "bpf",
            Self::Csky => "csky",
            Self::Hexagon => "hexagon",
            Self::Loongarch32 => "loongarch32",
            Self::Loongarch64 => "loongarch64",
            Self::M68k => "m68k",
            Self::Mips => "mips",
            Self::Mips32r6 => "mips32r6",
            Self::Mips64 => "mips64",
            Self::Mips64r6 => "mips64r6",
            Self::Msp430 => "msp430",
            Self::Nvptx64 => "nvptx64",
            Self::PowerPc => "powerpc",
            Self::PowerPc64 => "powerpc64",
            Self::Riscv32 => "riscv32",
            Self::Riscv64 => "riscv64",
            Self::S390X => "s390x",
            Self::Sparc => "sparc",
            Self::Sparc64 => "sparc64",
            Self::Wasm32 => "wasm32",
            Self::Wasm64 => "wasm64",
            Self::X86 => "x86",
            Self::X86_64 => "x86_64",
            Self::Xtensa => "xtensa",
        }
    }
}

impl FromStr for Arch {
    type Err = Error;

    /// Create a new `Arch` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "aarch64" => Self::AArch64,
            "amdgpu" => Self::Amdgpu,
            "arm" => Self::Arm,
            "arm64ec" => Self::Arm64ec,
            "avr" => Self::Avr,
            "bpf" => Self::Bpf,
            "csky" => Self::Csky,
            "hexagon" => Self::Hexagon,
            "loongarch32" => Self::Loongarch32,
            "loongarch64" => Self::Loongarch64,
            "m68k" => Self::M68k,
            "mips" => Self::Mips,
            "mips32r6" => Self::Mips32r6,
            "mips64" => Self::Mips64,
            "mips64r6" => Self::Mips64r6,
            "msp430" => Self::Msp430,
            "nvptx64" => Self::Nvptx64,
            "powerpc" => Self::PowerPc,
            "powerpc64" => Self::PowerPc64,
            "riscv32" => Self::Riscv32,
            "riscv64" => Self::Riscv64,
            "s390x" => Self::S390X,
            "sparc" => Self::Sparc,
            "sparc64" => Self::Sparc64,
            "wasm32" => Self::Wasm32,
            "wasm64" => Self::Wasm64,
            "x86" => Self::X86,
            "x86_64" => Self::X86_64,
            "xtensa" => Self::Xtensa,
            _ => return Err(Error),
        };

        Ok(result)
    }
}

impl fmt::Display for Arch {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for Arch {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for Arch {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let string = std::string::String::deserialize(deserializer)?;
        string.parse().map_err(|_| {
            D::Error::custom(std::format!(
                "Unrecognized value '{}' for target_arch",
                string
            ))
        })
    }
}
