// This file is part of ICU4X. For terms of use, please see the file
// called LICENSE at the top level of the ICU4X source tree
// (online at: https://github.com/unicode-org/icu4x/blob/main/LICENSE ).

use crate::*;

#[derive(Debug)]
#[allow(clippy::exhaustive_structs)] // newtype
pub struct CoreWriteAsPartsWrite<W: fmt::Write + ?Sized>(pub W);

impl<W: fmt::Write + ?Sized> fmt::Write for CoreWriteAsPartsWrite<W> {
    #[inline]
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.0.write_str(s)
    }

    #[inline]
    fn write_char(&mut self, c: char) -> fmt::Result {
        self.0.write_char(c)
    }
}

impl<W: fmt::Write + ?Sized> PartsWrite for CoreWriteAsPartsWrite<W> {
    type SubPartsWrite = CoreWriteAsPartsWrite<W>;

    #[inline]
    fn with_part(
        &mut self,
        _part: Part,
        mut f: impl FnMut(&mut Self::SubPartsWrite) -> fmt::Result,
    ) -> fmt::Result {
        f(self)
    }
}
