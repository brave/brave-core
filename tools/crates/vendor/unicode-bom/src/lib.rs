// Copyright © 2018 Phil Booth
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain
// a copy of the License at:
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

//! Detects and classifies
//! [Unicode byte-order marks](https://en.wikipedia.org/wiki/Byte_order_mark).
//!
//! ## Usage
//!
//! ```
//! use unicode_bom::Bom;
//!
//! // Detect the UTF-32 (little-endian) BOM in a file on disk
//! let bom: Bom = "fixtures/utf32-le.txt".parse().unwrap();
//! assert_eq!(bom, Bom::Utf32Le);
//! assert_eq!(bom.len(), 4);
//!
//! // Detect the UTF-16 (little-endian) BOM in a file on disk
//! let bom: Bom = "fixtures/utf16-le.txt".parse().unwrap();
//! assert_eq!(bom, Bom::Utf16Le);
//! assert_eq!(bom.len(), 2);
//!
//! // Detect no BOM in a file on disk
//! let bom: Bom = "fixtures/ascii.txt".parse().unwrap();
//! assert_eq!(bom, Bom::Null);
//! assert_eq!(bom.len(), 0);
//!
//! // Detect the BOM in a byte array
//! let bytes = [0u8, 0u8, 0xfeu8, 0xffu8];
//! assert_eq!(Bom::from(&bytes[0..]), Bom::Utf32Be);
//! ```

use std::fmt::{self, Display, Formatter};
use std::fs::File;
use std::io::{Error, ErrorKind, Read};
use std::str::FromStr;

#[cfg(test)]
mod test;

/// Unicode byte-order mark (BOM) abstraction.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Bom {
    /// Indicates no BOM was detected.
    Null,

    /// Indicates [BOCU-1](https://www.unicode.org/notes/tn6/) BOM was detected.
    Bocu1,

    /// Indicates [GB 18030](https://en.wikipedia.org/wiki/GB_18030) BOM was detected.
    Gb18030,

    /// Indicates [SCSU](https://www.unicode.org/reports/tr6/) BOM was detected.
    Scsu,

    /// Indicates [UTF-EBCIDC](https://www.unicode.org/reports/tr16/) BOM was detected.
    UtfEbcdic,

    /// Indicates [UTF-1](https://en.wikipedia.org/wiki/UTF-1) BOM was detected.
    Utf1,

    /// Indicates [UTF-7](https://tools.ietf.org/html/rfc2152) BOM was detected.
    Utf7,

    /// Indicates [UTF-8](https://tools.ietf.org/html/rfc3629) BOM was detected.
    Utf8,

    /// Indicates [UTF-16](https://tools.ietf.org/html/rfc2781) (big-endian) BOM was detected.
    Utf16Be,

    /// Indicates [UTF-16](https://tools.ietf.org/html/rfc2781) (little-endian) BOM was detected.
    Utf16Le,

    /// Indicates [UTF-32](https://www.unicode.org/reports/tr19/) (big-endian) BOM was detected.
    Utf32Be,

    /// Indicates [UTF-32](https://www.unicode.org/reports/tr19/) (little-endian) BOM was detected.
    Utf32Le,
}

impl Bom {
    /// Returns the size in bytes of the BOM.
    pub fn len(&self) -> usize {
        match *self {
            Bom::Null => 0,
            Bom::Bocu1 => 3,
            Bom::Gb18030 => 4,
            Bom::Scsu => 3,
            Bom::UtfEbcdic => 4,
            Bom::Utf1 => 3,
            Bom::Utf7 => 4,
            Bom::Utf8 => 3,
            Bom::Utf16Be => 2,
            Bom::Utf16Le => 2,
            Bom::Utf32Be => 4,
            Bom::Utf32Le => 4,
        }
    }
}

impl AsRef<str> for Bom {
    /// Returns a `&str` representation of the BOM type.
    fn as_ref(&self) -> &str {
        match *self {
            Bom::Null => "[not set]",
            Bom::Bocu1 => "BOCU-1",
            Bom::Gb18030 => "GB 18030",
            Bom::Scsu => "SCSU",
            Bom::UtfEbcdic => "UTF-EBCDIC",
            Bom::Utf1 => "UTF-1",
            Bom::Utf7 => "UTF-7",
            Bom::Utf8 => "UTF-8",
            Bom::Utf16Be => "UTF-16 (big-endian)",
            Bom::Utf16Le => "UTF-16 (little-endian)",
            Bom::Utf32Be => "UTF-32 (big-endian)",
            Bom::Utf32Le => "UTF-32 (little-endian)",
        }
    }
}

impl AsRef<[u8]> for Bom {
    /// Returns the BOM byte-array literal.
    ///
    /// Note that for UTF-7,
    /// only the first three bytes of the BOM are returned.
    /// That's because the last two bits of the fourth byte
    /// belong to the following character,
    /// so it's impossible to return the fourth byte
    /// without further context.
    /// Possible values for the missing fourth byte
    /// are `0x38`, `0x39`, `0x2a` and `0x2b`.
    fn as_ref(&self) -> &[u8] {
        match *self {
            Bom::Null => &[],
            Bom::Bocu1 => &[0xfb, 0xee, 0x28],
            Bom::Gb18030 => &[0x84, 0x31, 0x95, 0x33],
            Bom::Scsu => &[0x0e, 0xfe, 0xff],
            Bom::UtfEbcdic => &[0xdd, 0x73, 0x66, 0x73],
            Bom::Utf1 => &[0xf7, 0x64, 0x4c],
            Bom::Utf7 => &[0x2b, 0x2f, 0x76],
            Bom::Utf8 => &[0xef, 0xbb, 0xbf],
            Bom::Utf16Be => &[0xfe, 0xff],
            Bom::Utf16Le => &[0xff, 0xfe],
            Bom::Utf32Be => &[0, 0, 0xfe, 0xff],
            Bom::Utf32Le => &[0xff, 0xfe, 0, 0],
        }
    }
}

impl Default for Bom {
    /// Returns the default/empty BOM type, `Bom::Null`.
    fn default() -> Self {
        Bom::Null
    }
}

impl Display for Bom {
    /// Formats the BOM type as a `String`.
    fn fmt(&self, formatter: &mut Formatter) -> fmt::Result {
        write!(formatter, "{}", AsRef::<str>::as_ref(self))
    }
}

impl Eq for Bom {}

macro_rules! compare_tail {
    ($slice:ident, $bytes:expr) => {
        compare_tail!($slice, $bytes, 1)
    };

    ($slice:ident, $bytes:expr, $from:expr) => {
        compare_tail!($slice, $bytes.len() + $from, $bytes, $from)
    };

    ($slice:ident, $len:expr, $bytes:expr, $from:expr) => {
        $slice.len() >= $len && $slice[$from..$from + $bytes.len()] == $bytes
    };
}

impl From<&[u8]> for Bom {
    /// Detect the BOM type from a byte array.
    fn from(slice: &[u8]) -> Self {
        if slice.len() >= 2 {
            match slice[0] {
                0 => {
                    if compare_tail!(slice, [0, 0xfe, 0xff]) {
                        return Bom::Utf32Be;
                    }
                }
                0x0e => {
                    if compare_tail!(slice, [0xfe, 0xff]) {
                        return Bom::Scsu;
                    }
                }
                0x2b => {
                    if compare_tail!(slice, 4, [0x2f, 0x76], 1)
                        && (slice[3] == 0x38
                            || slice[3] == 0x39
                            || slice[3] == 0x2b
                            || slice[3] == 0x2f)
                    {
                        return Bom::Utf7;
                    }
                }
                0x84 => {
                    if compare_tail!(slice, [0x31, 0x95, 0x33]) {
                        return Bom::Gb18030;
                    }
                }
                0xdd => {
                    if compare_tail!(slice, [0x73, 0x66, 0x73]) {
                        return Bom::UtfEbcdic;
                    }
                }
                0xef => {
                    if compare_tail!(slice, [0xbb, 0xbf]) {
                        return Bom::Utf8;
                    }
                }
                0xf7 => {
                    if compare_tail!(slice, [0x64, 0x4c]) {
                        return Bom::Utf1;
                    }
                }
                0xfb => {
                    if compare_tail!(slice, [0xee, 0x28]) {
                        return Bom::Bocu1;
                    }
                }
                0xfe => {
                    if slice[1] == 0xff {
                        return Bom::Utf16Be;
                    }
                }
                0xff => {
                    if slice[1] == 0xfe {
                        if compare_tail!(slice, [0, 0], 2) {
                            return Bom::Utf32Le;
                        }

                        return Bom::Utf16Le;
                    }
                }
                _ => {}
            }
        }

        Bom::Null
    }
}

impl From<&mut File> for Bom {
    /// Detect the BOM type from a `File` instance.
    ///
    /// Note that I/O errors are swallowed by this method.
    /// Instead the default type, `Bom::Null`,
    /// will be returned.
    fn from(file: &mut File) -> Self {
        let mut data = [0u8; 4];
        let mut result = file.read_exact(&mut data);

        if let Err(ref error) = result {
            if error.kind() == ErrorKind::UnexpectedEof {
                let short_data = [0u8; 3];
                result = file.read_exact(&mut data);

                if let Err(ref error) = result {
                    if error.kind() == ErrorKind::UnexpectedEof {
                        let short_data = [0u8; 2];
                        result = file.read_exact(&mut data);
                        data[0] = short_data[0];
                        data[1] = short_data[1];
                    }
                } else {
                    data[0] = short_data[0];
                    data[1] = short_data[1];
                    data[2] = short_data[2];
                }
            }
        }

        if result.is_ok() {
            Bom::from(&data[0..])
        } else {
            Bom::Null
        }
    }
}

impl FromStr for Bom {
    /// A `std::io::Error` instance returned by `std::fs::File::open`.
    type Err = Error;

    /// Parse the BOM type from the file located at `path`.
    fn from_str(path: &str) -> Result<Self, Self::Err> {
        let mut file = File::open(path)?;
        Ok(Bom::from(&mut file))
    }
}
