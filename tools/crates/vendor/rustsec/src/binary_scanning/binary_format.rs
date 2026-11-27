//! A shim around `binfarce::Format` so that `binfarce` crate could be an optional dependency

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
/// Formats of compiled executables that can be scanned
pub enum BinaryFormat {
    /// Executable and Linkable Format, 32-bit. Used on Unix systems.
    Elf32,
    /// Executable and Linkable Format, 64-bit. Used on Unix systems.
    Elf64,
    /// Mach object file format. Used on Apple systems.
    Macho,
    /// Portable Executable (PE) format. Used on Windows.
    PE,
    /// WebAssembly
    Wasm,
    /// The format is not known
    Unknown,
}

#[cfg(feature = "binary-scanning")]
impl From<binfarce::Format> for BinaryFormat {
    fn from(value: binfarce::Format) -> Self {
        match value {
            binfarce::Format::Elf32 { byte_order: _ } => Self::Elf32,
            binfarce::Format::Elf64 { byte_order: _ } => Self::Elf64,
            binfarce::Format::Macho => Self::Macho,
            binfarce::Format::PE => Self::PE,
            binfarce::Format::Unknown => Self::Unknown,
        }
    }
}
