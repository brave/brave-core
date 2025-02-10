// Prohibit dangerous things we definitely don't want
#![deny(clippy::integer_arithmetic)]
#![deny(clippy::cast_possible_truncation)]
#![deny(clippy::indexing_slicing)]
// Style lints
#![warn(clippy::cast_lossless)]

use std::{convert::TryInto, ops::Range, mem::size_of};

use crate::ByteOrder;
use crate::demangle::SymbolData;
use crate::parser::*;
use crate::ParseError;

mod elf {
    pub type Address = u64;
    pub type Offset = u64;
    pub type Half = u16;
    pub type Word = u32;
    pub type XWord = u64;
}

mod section_type {
    pub const SYMBOL_TABLE: super::elf::Word = 2;
    pub const STRING_TABLE: super::elf::Word = 3;
}

const RAW_ELF_HEADER_SIZE: usize = size_of::<Elf64Header>();
const RAW_SECTION_HEADER_SIZE: usize = size_of::<RawSection>();

#[derive(Debug, Clone, Copy)]
pub struct Elf64Header {
    pub elf_type: elf::Half,
    pub machine: elf::Half,
    pub version: elf::Word,
    pub entry: elf::Address,
    pub phoff: elf::Offset,
    pub shoff: elf::Offset,
    pub flags: elf::Word,
    pub ehsize: elf::Half,
    pub phentsize: elf::Half,
    pub phnum: elf::Half,
    pub shentsize: elf::Half,
    pub shnum: elf::Half,
    pub shstrndx: elf::Half,
}

fn parse_elf_header(data: &[u8], byte_order: ByteOrder) -> Result<Elf64Header, UnexpectedEof> {
    let mut s = Stream::new(&data.get(16..).ok_or(UnexpectedEof{})?, byte_order);
    if s.remaining() >= RAW_ELF_HEADER_SIZE {
        Ok(Elf64Header {
            elf_type: s.read()?,
            machine: s.read()?,
            version: s.read()?,
            entry: s.read()?,
            phoff: s.read()?,
            shoff: s.read()?,
            flags: s.read()?,
            ehsize: s.read()?,
            phentsize: s.read()?,
            phnum: s.read()?,
            shentsize: s.read()?,
            shnum: s.read()?,
            shstrndx: s.read()?,
        })
    } else {
        Err(UnexpectedEof {})
    }

}
#[derive(Debug, Clone, Copy)]
pub struct Section {
    index: u16,
    name_offset: u32,
    kind: u32,
    link: u32,
    offset: u64,
    size: u64,
    entry_size: u64,
}

impl Section {
    pub fn range(&self) -> Result<Range<usize>, ParseError> {
        let start: usize = self.offset.try_into()?;
        let end: usize = start.checked_add(self.size.try_into()?).ok_or(ParseError::MalformedInput)?;
        Ok(start..end)
    }

    pub fn entries(&self) -> u64 {
        self.size.checked_div(self.entry_size).unwrap_or(0)
    }

    fn from_raw(rs: RawSection, index: u16) -> Section {
        Section {
            index,
            name_offset: rs.name,
            kind: rs.kind,
            link: rs.link,
            offset: rs.offset,
            size: rs.size,
            entry_size: rs.entry_size,
        }
    }

    pub fn name<'a>(&self, parent: &Elf64<'a>) -> Option<&'a str> {
        self.__name(parent.data, parent.header, parent.byte_order).unwrap_or(None)
    }

    fn __name<'a>(&self, data: &'a [u8], header: Elf64Header, byte_order: ByteOrder) -> Result<Option<&'a str>, ParseError> {
        let section_offset: usize = header.shoff.try_into()?;
        let mut s = Stream::new_at(data, section_offset, byte_order)?;

        let number_of_section_with_section_names = header.shstrndx;
        s.skip_len(RAW_SECTION_HEADER_SIZE.checked_mul(number_of_section_with_section_names.into())
            .ok_or(ParseError::MalformedInput)?)?;
        let section_with_section_names = Section::from_raw(read_section(&mut s)?, number_of_section_with_section_names);
        let section_name_strings = &data.get(section_with_section_names.range()?)
            .ok_or(UnexpectedEof{})?;
        Ok(parse_null_string(section_name_strings, self.name_offset as usize))
    }
}

pub struct Elf64<'a> {
    data: &'a [u8],
    byte_order: ByteOrder,
    header: Elf64Header,
}
#[derive(Debug, Clone, Copy)]
struct RawSection {
    name: elf::Word,
    kind: elf::Word,
    flags: elf::XWord,
    addr: elf::Address,
    offset: elf::Offset,
    size: elf::XWord,
    link: elf::Word,
    info: elf::Word,
    addralign: elf::XWord,
    entry_size: elf::XWord,
}

pub fn parse(data: &[u8], byte_order: ByteOrder) -> Result<Elf64, ParseError> {
    let header = parse_elf_header(data, byte_order)?;
    Ok(Elf64 { data, byte_order, header})
}

impl<'a> Elf64<'a> {
    pub fn header(&self) -> Elf64Header {
        self.header
    }

    pub fn section_with_name(&self, name: &str) -> Result<Option<Section>, ParseError> {
        let callback = |section: Section| {
            section.name(self) == Some(name)
        };
        self.find_section(callback)
    }

    pub fn find_section<F: Fn(Section) -> bool>(&self, callback: F) -> Result<Option<Section>, ParseError> {
        let section_count = self.header.shnum;
        let section_offset: usize = self.header.shoff.try_into()?;

        let mut s = Stream::new_at(self.data, section_offset, self.byte_order)?;
        for i in 0..section_count {
            let rs = read_section(&mut s)?;
            let section = Section::from_raw(rs, i);
            if callback(section) {
                return Ok(Some(section));
            }
        }
        Ok(None)
    }

    pub fn symbols(&self, section_name: &str) -> Result<(Vec<SymbolData>, u64), ParseError> {
        let text_section = self.section_with_name(section_name)?
            .ok_or(ParseError::SymbolsSectionIsMissing)?;

        let symbols_section = self.find_section(|v| v.kind == section_type::SYMBOL_TABLE)?
            .ok_or(ParseError::SectionIsMissing(".symtab"))?;

        let linked_section = self.find_section(|v| u32::from(v.index) == symbols_section.link)?
            .ok_or(ParseError::SectionIsMissing(".strtab"))?;

        if linked_section.kind != section_type::STRING_TABLE {
            return Err(ParseError::UnexpectedSectionType {
                expected: section_type::STRING_TABLE,
                actual: linked_section.kind,
            });
        }

        let strings = self.data.get(linked_section.range()?)
            .ok_or(ParseError::UnexpectedEof)?;
        let symbols_data_range = &self.data.get(symbols_section.range()?)
            .ok_or(ParseError::UnexpectedEof)?;
        let s = Stream::new(symbols_data_range, self.byte_order);
        let symbols_count: usize = symbols_section.entries().try_into()?;
        let symbols = parse_symbols(s, symbols_count, strings, text_section)?;
        Ok((symbols, text_section.size))
    }
}

fn read_section(s: &mut Stream) -> Result<RawSection, UnexpectedEof> {
    Ok(RawSection {
        name: s.read()?,
        kind: s.read()?,
        flags: s.read()?,
        addr: s.read()?,
        offset: s.read()?,
        size: s.read()?,
        link: s.read()?,
        info: s.read()?,
        addralign: s.read()?,
        entry_size: s.read()?,
    })
}

fn parse_symbols(
    mut s: Stream,
    count: usize,
    strings: &[u8],
    text_section: Section,
) -> Result<Vec<SymbolData>, UnexpectedEof> {
    let mut symbols = Vec::with_capacity(count);
    while !s.at_end() {
        // Note: the order of fields in 32 and 64 bit ELF is different.
        let name_offset = s.read::<elf::Word>()? as usize;
        let info: u8 = s.read()?;
        s.skip::<u8>()?; // other
        let shndx: elf::Half = s.read()?;
        let value: elf::Address = s.read()?;
        let size: elf::XWord = s.read()?;

        if shndx != text_section.index {
            continue;
        }

        // Ignore symbols with zero size.
        if size == 0 {
            continue;
        }

        // Ignore symbols without a name.
        if name_offset == 0 {
            continue;
        }

        // Ignore symbols that aren't functions.
        const STT_FUNC: u8 = 2;
        let kind = info & 0xf;
        if kind != STT_FUNC {
            continue;
        }

        if let Some(s) = parse_null_string(strings, name_offset) {
            symbols.push(SymbolData {
                name: crate::demangle::SymbolName::demangle(s),
                address: value,
                size,
            });
        }
    }

    Ok(symbols)
}
