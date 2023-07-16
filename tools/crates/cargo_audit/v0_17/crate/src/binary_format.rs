/// A shim around `binfarce::Format` so that `binfarce` crate could be an optional dependency
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum BinaryFormat {
    Elf32,
    Elf64,
    Macho,
    PE,
    Unknown,
}

#[cfg(feature = "binary-scanning")]
impl From<binfarce::Format> for BinaryFormat {
    fn from(value: binfarce::Format) -> Self {
        match value {
            binfarce::Format::Elf32 { byte_order: _ } => BinaryFormat::Elf32,
            binfarce::Format::Elf64 { byte_order: _ } => BinaryFormat::Elf64,
            binfarce::Format::Macho => BinaryFormat::Macho,
            binfarce::Format::PE => BinaryFormat::PE,
            binfarce::Format::Unknown => BinaryFormat::Unknown,
        }
    }
}
