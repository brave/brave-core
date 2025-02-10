// Prohibit dangerous things we definitely don't want
#![deny(clippy::integer_arithmetic)]
#![deny(clippy::cast_possible_truncation)]
#![deny(clippy::indexing_slicing)]
// Style lints
#![warn(clippy::cast_lossless)]

use crate::ByteOrder;
use crate::demangle::SymbolData;
use crate::parser::*;
use crate::ParseError;

use std::ops::Range;
use std::convert::TryInto;

const LC_SYMTAB: u32 = 0x2;
const LC_SEGMENT_64: u32 = 0x19;

#[derive(Debug, Clone, Copy)]
struct Cmd {
    kind: u32,
    offset: usize,
}

#[derive(Debug, Clone, Copy)]
pub struct Section <'a> {
    segment_name: &'a str,
    section_name: &'a str,
    address: u64,
    offset: u32,
    size: u64,
}

impl Section <'_> {
    pub fn range(&self) -> Result<Range<usize>, ParseError> {
        let start: usize = self.offset.try_into()?;
        let end: usize = start.checked_add(self.size.try_into()?).ok_or(ParseError::MalformedInput)?;
        Ok(start..end)
    }
}

#[derive(Debug, Clone, Copy)]
pub struct MachoHeader {
    cputype: u32,
    cpusubtype: u32,
    /// type of file - exec, dylib, ...
    filetype: u32,
    /// number of load commands
    ncmds: u32,
    /// size of load command region
    sizeofcmds: u32,
    flags: u32,
}

#[derive(Debug, Clone)]
pub struct Macho <'a> {
    data: &'a [u8],
    header: MachoHeader,
}

fn parse_macho_header(s: &mut Stream) -> Result<MachoHeader, UnexpectedEof> {
    s.skip::<u32>()?; // magic
    let header = MachoHeader {
        cputype: s.read()?,
        cpusubtype: s.read()?,
        filetype: s.read()?,
        ncmds: s.read()?,
        sizeofcmds: s.read()?,
        flags: s.read()?,
    };
    s.skip::<u32>()?; // reserved
    Ok(header)
}

struct MachoCommandsIterator<'a> {
    stream: Stream<'a>,
    number_of_commands: u32,
    commands_already_read: u32,
    result: Result<(), ParseError>,
}

impl Iterator for MachoCommandsIterator<'_> {
    type Item = Result<Cmd, ParseError>;
    fn next(&mut self) -> Option<Self::Item> {
        if self.commands_already_read < self.number_of_commands && self.result.is_ok() {
            let s = &mut self.stream;
            let cmd_kind: u32 = s.read().ok()?;
            let cmd_size: u32 = s.read().ok()?;
            let item = Cmd {kind: cmd_kind, offset: s.offset()};
            self.commands_already_read = self.commands_already_read.checked_add(1)?;

            // cmd_size is a size of a whole command data,
            // so we have to remove the header size first.
            let to_skip = (cmd_size as usize).checked_sub(8);
            // Skip the rest of the command to get to the start of the next one.
            // If we encounter EOF or if the command size makes no sense,
            // make the iterator return None from now on.
            self.result = match to_skip {
                None => Err(ParseError::MalformedInput),
                Some(len) => s.skip_len(len).map_err(|_| ParseError::UnexpectedEof),
            };
            match self.result {
                Ok(()) => Some(Ok(item)),
                Err(err_val) => Some(Err(err_val)),
            }
        } else {
            None
        }
    }
}

pub fn parse(data: &[u8]) -> Result<Macho, ParseError> {
    let mut s = Stream::new(&data, ByteOrder::LittleEndian);
    let header = parse_macho_header(&mut s)?;
    Ok(Macho{
        data,
        header,
    })
}

impl <'a> Macho<'a> {
    pub fn header(&self) -> MachoHeader {
        self.header
    }

    pub fn find_section<F: Fn(Section) -> bool>(&self, callback: F) -> Result<Option<Section>, ParseError> {
        for cmd in self.commands() {
            let cmd = cmd?;
            if cmd.kind == LC_SEGMENT_64 {
                let mut s = Stream::new_at(self.data, cmd.offset, ByteOrder::LittleEndian)?;
                s.skip_len(16)?; // segname
                s.skip::<u64>()?; // vmaddr
                s.skip::<u64>()?; // vmsize
                s.skip::<u64>()?; // fileoff
                s.skip::<u64>()?; // filesize
                s.skip::<u32>()?; // maxprot
                s.skip::<u32>()?; // initprot
                let sections_count: u32 = s.read()?;
                s.skip::<u32>()?; // flags

                for _ in 0..sections_count {
                    let section_name = parse_null_string(s.read_bytes(16)?, 0);
                    let segment_name = parse_null_string(s.read_bytes(16)?, 0);
                    let address: u64 = s.read()?;
                    let size: u64 = s.read()?;
                    let offset: u32 = s.read()?;
                    s.skip::<u32>()?; // align
                    s.skip::<u32>()?; // reloff
                    s.skip::<u32>()?; // nreloc
                    s.skip::<u32>()?; // flags
                    s.skip_len(12)?; // padding

                    if let (Some(segment), Some(section)) = (segment_name, section_name) {
                        let section = Section {
                            segment_name: segment,
                            section_name: section,
                            address,
                            offset,
                            size,
                        };
                        if callback(section) {
                            return Ok(Some(section));
                        }
                    }
                }
            }
        }
        Ok(None)
    }

    pub fn section_with_name(&self, segment_name: &str, section_name: &str) -> Result<Option<Section>, ParseError> {
        let callback = |section: Section| {
            section.segment_name == segment_name && section.section_name == section_name
        };
        self.find_section(callback)
    }

    fn commands(&self) -> MachoCommandsIterator {
        let mut s = Stream::new(&self.data, ByteOrder::LittleEndian);
        let _ = parse_macho_header(&mut s); // skip the header
        MachoCommandsIterator {
            stream: s,
            number_of_commands: self.header.ncmds,
            commands_already_read: 0,
            result: Ok(())
        }
    }

    #[allow(clippy::indexing_slicing)]
    pub fn symbols(&self) -> Result<(Vec<SymbolData>, u64), ParseError> {
        let text_section = self.section_with_name("__TEXT", "__text")?
            .ok_or(ParseError::SymbolsSectionIsMissing)?;
        assert_ne!(text_section.size, 0);

        if let Some(cmd) = self.commands().find(|v| v.unwrap().kind == LC_SYMTAB) {
            let mut s = Stream::new(&self.data[cmd.unwrap().offset..], ByteOrder::LittleEndian);
            let symbols_offset: u32 = s.read()?;
            let number_of_symbols: u32 = s.read()?;
            let strings_offset: u32 = s.read()?;
            let strings_size: u32 = s.read()?;

            let strings = {
                let start = strings_offset as usize;
                let end = start.checked_add(strings_size as usize).ok_or(ParseError::MalformedInput)?;
                &self.data[start..end]
            };

            let symbols_data = &self.data[symbols_offset as usize..];
            return Ok((
                parse_symbols(symbols_data, number_of_symbols, strings, text_section)?,
                text_section.size,
            ));
        }

        Ok((Vec::new(), 0))
    }
}

#[derive(Clone, Copy, Debug)]
struct RawSymbol {
    string_index: u32,
    kind: u8,
    section: u8,
    address: u64,
}

// only used by cargo-bloat which operates on trusted data,
// so it's not hardened against malicious inputs
#[allow(clippy::integer_arithmetic)]
#[allow(clippy::indexing_slicing)]
fn parse_symbols(
    data: &[u8],
    count: u32,
    strings: &[u8],
    text_section: Section,
) -> Result<Vec<SymbolData>, UnexpectedEof> {
    let mut raw_symbols = Vec::with_capacity(count as usize);
    let mut s = Stream::new(data, ByteOrder::LittleEndian);
    for _ in 0..count {
        let string_index: u32 = s.read()?;
        let kind: u8 = s.read()?;
        let section: u8 = s.read()?;
        s.skip::<u16>()?; // description
        let value: u64 = s.read()?;

        if value == 0 {
            continue;
        }

        raw_symbols.push(RawSymbol {
            string_index,
            kind,
            section,
            address: value,
        });
    }

    // To find symbol sizes, we have to sort them by address.
    raw_symbols.sort_by_key(|v| v.address);

    // Add the __TEXT section end address, which will be used
    // to calculate the size of the last symbol.
    raw_symbols.push(RawSymbol {
        string_index: 0,
        kind: 0,
        section: 0,
        address: text_section.address + text_section.size,
    });

    let mut symbols = Vec::with_capacity(count as usize);
    for i in 0..raw_symbols.len() - 1 {
        let sym = &raw_symbols[i];

        if sym.string_index == 0 {
            continue;
        }

        const N_TYPE: u8   = 0x0E;
        const INDIRECT: u8 = 0xA;
        const SECTION: u8  = 0xE;

        let sub_type = sym.kind & N_TYPE;

        // Ignore indirect symbols.
        if sub_type & INDIRECT == 0 {
            continue;
        }

        // Ignore symbols without a section.
        if sub_type & SECTION == 0 {
            continue;
        }

        // Ignore symbols that aren't in the first section.
        // The first section is usually __TEXT,__text.
        if sym.section != 1 {
            continue;
        }

        // Mach-O format doesn't store the symbols size,
        // so we have to calculate it by subtracting an address of the next symbol
        // from the current.
        // Next symbol can have the same address as the current one,
        // so we have to find the one that has a different address.
        let next_sym = raw_symbols[i..].iter().skip_while(|s| s.address == sym.address).next();
        let size = match next_sym {
            Some(next) => next.address - sym.address,
            None => continue,
        };

        if let Some(s) = parse_null_string(strings, sym.string_index as usize) {
            symbols.push(SymbolData {
                name: crate::demangle::SymbolName::demangle(s),
                address: sym.address,
                size,
            });
        }
    }

    Ok(symbols)
}
