/// This module is a fork of https://github.com/alexcrichton/rustc-demangle
///
/// Changes:
///
/// - Added `SymbolName` struct.
/// - `v0` demangler prints into a String and not into fmt::Formatter.
/// - `v0` demangler can omit disambiguator hashes.
/// - `v0` demangler stores a crate name.
/// - Updated to Rust 2018.
/// - Ignore LLVM suffixes.

mod legacy;
mod v0;

#[derive(Clone, Copy, PartialEq)]
pub enum Kind {
    Unknown,
    Legacy,
    V0,
}

pub struct SymbolData {
    pub name: SymbolName,
    pub address: u64,
    pub size: u64,
}

pub struct SymbolName {
    pub complete: String,
    pub trimmed: String,
    pub crate_name: Option<String>,
    pub kind: Kind,
}

impl SymbolName {
    pub fn demangle(name: &str) -> Self {
        let d = demangle(name);
        match d.style {
            Some(DemangleStyle::Legacy(ref d)) => {
                let complete = d.to_string();
                let mut trimmed = complete.clone();

                // crate::mod::fn::h5fbe0f2f0b5c7342 -> crate::mod::fn
                if let Some(pos) = trimmed.bytes().rposition(|b| b == b':') {
                    trimmed.drain((pos - 1)..);
                }

                SymbolName {
                    complete,
                    trimmed,
                    crate_name: None, // We will parse a crate name later.
                    kind: Kind::Legacy,
                }
            }
            Some(DemangleStyle::V0(ref d)) => {
                d.demangle()
            }
            None => {
                SymbolName {
                    complete: name.to_string(),
                    trimmed: name.to_string(),
                    crate_name: None, // Unknown.
                    kind: Kind::Unknown,
                }
            }
        }
    }
}


/// Representation of a demangled symbol name.
pub struct Demangle<'a> {
    style: Option<DemangleStyle<'a>>,
}

enum DemangleStyle<'a> {
    Legacy(legacy::Demangle<'a>),
    V0(v0::Demangle<'a>),
}

/// De-mangles a Rust symbol into a more readable version
///
/// This function will take a **mangled** symbol and return a value. When printed,
/// the de-mangled version will be written. If the symbol does not look like
/// a mangled symbol, the original value will be written instead.
///
/// # Examples
///
/// ```ignore
/// use rustc_demangle::demangle;
///
/// assert_eq!(demangle("_ZN4testE").to_string(), "test");
/// assert_eq!(demangle("_ZN3foo3barE").to_string(), "foo::bar");
/// assert_eq!(demangle("foo").to_string(), "foo");
/// ```
pub fn demangle(mut s: &str) -> Demangle {
    // During ThinLTO LLVM may import and rename internal symbols, so strip out
    // those endings first as they're one of the last manglings applied to symbol
    // names.
    let llvm = ".llvm.";
    if let Some(i) = s.find(llvm) {
        let candidate = &s[i + llvm.len()..];
        let all_hex = candidate.chars().all(|c| {
            match c {
                'A' ..= 'F' | '0' ..= '9' | '@' => true,
                _ => false,
            }
        });

        if all_hex {
            s = &s[..i];
        }
    }

    let mut suffix = "";
    let mut style = match legacy::demangle(s) {
        Ok((d, s)) => {
            suffix = s;
            Some(DemangleStyle::Legacy(d))
        }
        Err(()) => match v0::demangle(s) {
            Ok((d, s)) => {
                suffix = s;
                Some(DemangleStyle::V0(d))
            }
            Err(v0::Invalid) => None,
        },
    };

    // Output like LLVM IR adds extra period-delimited words. See if
    // we are in that case and save the trailing words if so.
    if !suffix.is_empty() {
        if suffix.starts_with(".") && is_symbol_like(suffix) {
            // Keep the suffix.
        } else {
            // Reset the suffix and invalidate the demangling.
            style = None;
        }
    }

    Demangle { style }
}

fn is_symbol_like(s: &str) -> bool {
    s.chars().all(|c| {
        // Once `char::is_ascii_punctuation` and `char::is_ascii_alphanumeric`
        // have been stable for long enough, use those instead for clarity
        is_ascii_alphanumeric(c) || is_ascii_punctuation(c)
    })
}

// Copied from the documentation of `char::is_ascii_alphanumeric`
fn is_ascii_alphanumeric(c: char) -> bool {
    match c {
        '\u{0041}' ..= '\u{005A}' |
        '\u{0061}' ..= '\u{007A}' |
        '\u{0030}' ..= '\u{0039}' => true,
        _ => false,
    }
}

// Copied from the documentation of `char::is_ascii_punctuation`
fn is_ascii_punctuation(c: char) -> bool {
    match c {
        '\u{0021}' ..= '\u{002F}' |
        '\u{003A}' ..= '\u{0040}' |
        '\u{005B}' ..= '\u{0060}' |
        '\u{007B}' ..= '\u{007E}' => true,
        _ => false,
    }
}
