//! Implements WASM parsing

use crate::Error;

use wasmparser::{self, Payload};

pub(crate) fn raw_auditable_data_wasm(input: &[u8]) -> Result<&[u8], Error> {
    for payload in wasmparser::Parser::new(0).parse_all(input) {
        match payload.map_err(|_| Error::MalformedFile)? {
            Payload::CustomSection(reader) => {
                if reader.name() == ".dep-v0" {
                    return Ok(reader.data());
                }
            }
            // We reached the end without seeing ".dep-v0" custom section
            Payload::End(_) => return Err(Error::NoAuditData),
            // ignore everything that's not a custom section
            _ => {}
        }
    }
    Err(Error::MalformedFile)
}
