//! Rust architectures

use crate::error::Error;
use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

/// `target_arch`: Target CPU architecture
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Arch {
    /// `aarch64`: ARMv8 64-bit architecture
    AArch64,

    /// `arm`: 32-bit ARM architecture
    Arm,

    /// `avr`
    Avr,

    /// `bpf`
    Bpf,

    /// `csky`
    Csky,

    /// `hexagon`
    Hexagon,

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
}

impl Arch {
    /// String representing this `Arch` which matches `#[cfg(target_arch)]`
    pub fn as_str(self) -> &'static str {
        match self {
            Arch::AArch64 => "aarch64",
            Arch::Arm => "arm",
            Arch::Avr => "avr",
            Arch::Bpf => "bpf",
            Arch::Csky => "csky",
            Arch::Hexagon => "hexagon",
            Arch::Loongarch64 => "loongarch64",
            Arch::M68k => "m68k",
            Arch::Mips => "mips",
            Arch::Mips32r6 => "mips32r6",
            Arch::Mips64 => "mips64",
            Arch::Mips64r6 => "mips64r6",
            Arch::Msp430 => "msp430",
            Arch::Nvptx64 => "nvptx64",
            Arch::PowerPc => "powerpc",
            Arch::PowerPc64 => "powerpc64",
            Arch::Riscv32 => "riscv32",
            Arch::Riscv64 => "riscv64",
            Arch::S390X => "s390x",
            Arch::Sparc => "sparc",
            Arch::Sparc64 => "sparc64",
            Arch::Wasm32 => "wasm32",
            Arch::Wasm64 => "wasm64",
            Arch::X86 => "x86",
            Arch::X86_64 => "x86_64",
        }
    }
}

impl FromStr for Arch {
    type Err = Error;

    /// Create a new `Arch` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "aarch64" => Arch::AArch64,
            "arm" => Arch::Arm,
            "avr" => Arch::Avr,
            "bpf" => Arch::Bpf,
            "csky" => Arch::Csky,
            "hexagon" => Arch::Hexagon,
            "loongarch64" => Arch::Loongarch64,
            "m68k" => Arch::M68k,
            "mips" => Arch::Mips,
            "mips32r6" => Arch::Mips32r6,
            "mips64" => Arch::Mips64,
            "mips64r6" => Arch::Mips64r6,
            "msp430" => Arch::Msp430,
            "nvptx64" => Arch::Nvptx64,
            "powerpc" => Arch::PowerPc,
            "powerpc64" => Arch::PowerPc64,
            "riscv32" => Arch::Riscv32,
            "riscv64" => Arch::Riscv64,
            "s390x" => Arch::S390X,
            "sparc" => Arch::Sparc,
            "sparc64" => Arch::Sparc64,
            "wasm32" => Arch::Wasm32,
            "wasm64" => Arch::Wasm64,
            "x86" => Arch::X86,
            "x86_64" => Arch::X86_64,
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
