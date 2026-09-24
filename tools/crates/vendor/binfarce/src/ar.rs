use std::str;

use crate::ByteOrder;
use crate::parser::*;
use crate::demangle::SymbolName;

pub fn parse(data: &[u8]) -> Result<Vec<String>, UnexpectedEof> {
    const MAGIC: &[u8] = b"!<arch>\x0A";

    if data.get(0..8) != Some(MAGIC) {
        return Ok(Vec::new());
    }

    let mut s = Stream::new(&data[8..], ByteOrder::BigEndian);
    while !s.at_end() {
        // Align the offset.
        if s.offset() & 1 == 1 {
            s.skip_len(1)?;
        }

        let identifier = str::from_utf8(s.read_bytes(16)?).unwrap();
        s.skip_len(12)?; // timestamp
        s.skip_len(6)?; // owner_id
        s.skip_len(6)?; // group_id
        s.skip_len(8)?; // mode
        let file_size = str::from_utf8(s.read_bytes(10)?).unwrap();
        let terminator = s.read_bytes(2)?;

        assert_eq!(terminator, &[0x60, 0x0A]);

        // Check for BSD name.
        let mut name = "";
        let mut raw_name_len: usize = 0;
        if identifier.starts_with("#1/") {
            raw_name_len = identifier[3..].trim().parse().unwrap();
            let raw_name = s.read_bytes(raw_name_len)?;
            name = str::from_utf8(raw_name).unwrap();
            name = name.trim_end_matches('\0');
        }

        let mut file_size: usize = file_size.trim().parse()
            .expect("invalid file size in member header");
        file_size -= raw_name_len;

        if name.is_empty() && identifier == "/               " {
            let index_data = s.read_bytes(file_size)?;
            return parse_sysv(index_data);
        } else if name == "__.SYMDEF" {
            let index_data = s.read_bytes(file_size)?;
            return parse_bsd(index_data);
        } else {
            s.skip_len(file_size)?;
        }
    }

    Ok(Vec::new())
}

fn parse_sysv(data: &[u8]) -> Result<Vec<String>, UnexpectedEof> {
    let mut symbols = Vec::new();

    let count = {
        let mut s = Stream::new(data, ByteOrder::BigEndian);
        s.read::<u32>()? as usize
    };

    // Skip offsets.
    let mut i = 4 + 4 * count; // u32 + u32 * size

    for _ in 0..count {
        if let Some(s) = parse_null_string(data, i) {
            symbols.push(SymbolName::demangle(s).complete);

            i += s.len() + 1;
        } else {
            i += 1;
        }

        if i >= data.len() {
            break;
        }
    }

    Ok(symbols)
}

fn parse_bsd(data: &[u8]) -> Result<Vec<String>, UnexpectedEof> {
    let mut symbols = Vec::new();

    let mut s = Stream::new(data, ByteOrder::LittleEndian);
    let entries_len = s.read::<u32>()? as usize;
    s.skip_len(entries_len)?;
    let strings_len = s.read::<u32>()? as usize;
    let strings = s.read_bytes(strings_len)?;

    let mut i = 0;
    while i < strings.len() {
        if let Some(s) = parse_null_string(strings, i) {
            symbols.push(SymbolName::demangle(s).complete);

            i += s.len() + 1;
        } else {
            i += 1;
        }
    }

    Ok(symbols)
}
