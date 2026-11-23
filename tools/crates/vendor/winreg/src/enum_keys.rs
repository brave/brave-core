// Copyright 2026, Igor Shaula
// Licensed under the MIT License <LICENSE or
// http://opensource.org/licenses/MIT>. This file
// may not be copied, modified, or distributed
// except according to those terms.
use crate::RegKey;
use std::io;
use windows_sys::Win32::Foundation;

/// Iterator over subkeys names
pub struct EnumKeys<'key> {
    pub(crate) key: &'key RegKey,
    pub(crate) index: u32,
}

impl Iterator for EnumKeys<'_> {
    type Item = io::Result<String>;

    fn next(&mut self) -> Option<io::Result<String>> {
        match self.key.enum_key(self.index) {
            None => None,
            Some(Err(err)) => {
                self.index += 1;
                Some(Err(err))
            }
            Some(Ok(name_os_string)) => {
                self.index += 1;
                match name_os_string.into_string() {
                    Ok(name_string) => Some(Ok(name_string)),
                    Err(_) => Some(werr!(Foundation::ERROR_INVALID_DATA)),
                }
            }
        }
    }

    fn nth(&mut self, n: usize) -> Option<Self::Item> {
        self.index += n as u32;
        self.next()
    }
}
