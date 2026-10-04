#![forbid(unsafe_code)]
#![doc = include_str!("../README.md")]

#[cfg(feature = "wasm")]
mod wasm;

use binfarce::Format;

/// Extracts the Zlib-compressed dependency info from an executable.
///
/// This function does not allocate any memory on the heap and can be safely given untrusted input.
pub fn raw_auditable_data(data: &[u8]) -> Result<&[u8], Error> {
    match binfarce::detect_format(data) {
        Format::Elf32 { byte_order } => {
            let section = binfarce::elf32::parse(data, byte_order)?
                .section_with_name(".dep-v0")?
                .ok_or(Error::NoAuditData)?;
            Ok(data.get(section.range()?).ok_or(Error::UnexpectedEof)?)
        }
        Format::Elf64 { byte_order } => {
            let section = binfarce::elf64::parse(data, byte_order)?
                .section_with_name(".dep-v0")?
                .ok_or(Error::NoAuditData)?;
            Ok(data.get(section.range()?).ok_or(Error::UnexpectedEof)?)
        }
        Format::Macho => {
            let parsed = binfarce::macho::parse(data)?;
            let section = parsed.section_with_name("__DATA", ".dep-v0")?;
            let section = section.ok_or(Error::NoAuditData)?;
            Ok(data.get(section.range()?).ok_or(Error::UnexpectedEof)?)
        }
        Format::PE => {
            let parsed = binfarce::pe::parse(data)?;
            let section = parsed
                .section_with_name(".dep-v0")?
                .ok_or(Error::NoAuditData)?;
            Ok(data.get(section.range()?).ok_or(Error::UnexpectedEof)?)
        }
        Format::Unknown => {
            #[cfg(feature = "wasm")]
            if data.starts_with(b"\0asm") {
                return wasm::raw_auditable_data_wasm(data);
            }

            Err(Error::NotAnExecutable)
        }
    }
}

#[cfg(all(fuzzing, feature = "wasm"))]
pub fn raw_auditable_data_wasm_for_fuzz(input: &[u8]) -> Result<&[u8], Error> {
    wasm::raw_auditable_data_wasm(input)
}

#[derive(Debug, Copy, Clone)]
pub enum Error {
    NoAuditData,
    NotAnExecutable,
    UnexpectedEof,
    MalformedFile,
    SymbolsSectionIsMissing,
    SectionIsMissing,
    UnexpectedSectionType,
}

impl std::error::Error for Error {}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let message = match self {
            Error::NoAuditData => "No audit data found in the executable",
            Error::NotAnExecutable => "Not an executable file",
            Error::UnexpectedEof => "Unexpected end of file",
            Error::MalformedFile => "Malformed executable file",
            Error::SymbolsSectionIsMissing => "Symbols section missing from executable",
            Error::SectionIsMissing => "Section is missing from executable",
            Error::UnexpectedSectionType => "Unexpected executable section type",
        };
        write!(f, "{message}")
    }
}

impl From<binfarce::ParseError> for Error {
    fn from(e: binfarce::ParseError) -> Self {
        match e {
            binfarce::ParseError::MalformedInput => Error::MalformedFile,
            binfarce::ParseError::UnexpectedEof => Error::UnexpectedEof,
            binfarce::ParseError::SymbolsSectionIsMissing => Error::SymbolsSectionIsMissing,
            binfarce::ParseError::SectionIsMissing(_) => Error::SectionIsMissing,
            binfarce::ParseError::UnexpectedSectionType { .. } => Error::UnexpectedSectionType,
        }
    }
}
