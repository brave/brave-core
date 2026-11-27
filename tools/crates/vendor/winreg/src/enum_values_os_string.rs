// Copyright 2026, Igor Shaula
// Licensed under the MIT License <LICENSE or
// http://opensource.org/licenses/MIT>. This file
// may not be copied, modified, or distributed
// except according to those terms.
use crate::{RegKey, RegValue};
use std::{ffi::OsString, io};

/// Iterator over values that uses `OsString` for names
pub struct EnumValuesOsString<'key> {
    pub(crate) key: &'key RegKey,
    pub(crate) index: u32,
}

impl<'a> Iterator for EnumValuesOsString<'a> {
    type Item = io::Result<(OsString, RegValue<'static>)>;

    fn next(&mut self) -> Option<io::Result<(OsString, RegValue<'static>)>> {
        match self.key.enum_value(self.index) {
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
