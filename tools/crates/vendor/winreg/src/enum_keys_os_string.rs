// Copyright 2026, Igor Shaula
// Licensed under the MIT License <LICENSE or
// http://opensource.org/licenses/MIT>. This file
// may not be copied, modified, or distributed
// except according to those terms.
use crate::RegKey;
use std::{ffi::OsString, io};

/// Iterator over subkeys names that uses `OsString`
pub struct EnumKeysOsString<'key> {
    pub(crate) key: &'key RegKey,
    pub(crate) index: u32,
}

impl Iterator for EnumKeysOsString<'_> {
    type Item = io::Result<OsString>;

    fn next(&mut self) -> Option<io::Result<OsString>> {
        match self.key.enum_key(self.index) {
            v @ Some(_) => {
                self.index += 1;
                v
            }
            e @ None => e,
        }
    }

    fn nth(&mut self, n: usize) -> Option<Self::Item> {
        self.index += n as u32;
        self.next()
    }
}
