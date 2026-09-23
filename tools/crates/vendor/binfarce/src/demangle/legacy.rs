use std::char;
use std::fmt;

/// Representation of a demangled symbol name.
pub struct Demangle<'a> {
    inner: &'a str,
    /// The number of ::-separated elements in the original name.
    elements: usize,
}

/// De-mangles a Rust symbol into a more readable version
///
/// All Rust symbols by default are mangled as they contain characters that
/// cannot be represented in all object files. The mangling mechanism is similar
/// to C++'s, but Rust has a few specifics to handle items like lifetimes in
/// symbols.
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

// All Rust symbols are in theory lists of "::"-separated identifiers. Some
// assemblers, however, can't handle these characters in symbol names. To get
// around this, we use C++-style mangling. The mangling method is:
//
// 1. Prefix the symbol with "_ZN"
// 2. For each element of the path, emit the length plus the element
// 3. End the path with "E"
//
// For example, "_ZN4testE" => "test" and "_ZN3foo3barE" => "foo::bar".
//
// We're the ones printing our backtraces, so we can't rely on anything else to
// demangle our symbols. It's *much* nicer to look at demangled symbols, so
// this function is implemented to give us nice pretty output.
//
// Note that this demangler isn't quite as fancy as it could be. We have lots
// of other information in our symbols like hashes, version, type information,
// etc. Additionally, this doesn't handle glue symbols at all.
pub fn demangle(s: &str) -> Result<(Demangle, &str), ()> {
    // First validate the symbol. If it doesn't look like anything we're
    // expecting, we just print it literally. Note that we must handle non-Rust
    // symbols because we could have any function in the backtrace.
    let inner = if s.starts_with("_ZN") {
        &s[3..]
    } else if s.starts_with("ZN") {
        // On Windows, dbghelp strips leading underscores, so we accept "ZN...E"
        // form too.
        &s[2..]
    } else if s.starts_with("__ZN") {
        // On OSX, symbols are prefixed with an extra _
        &s[4..]
    } else {
        return Err(());
    };

    // only work with ascii text
    if inner.bytes().any(|c| c & 0x80 != 0) {
        return Err(());
    }

    let mut elements = 0;
    let mut chars = inner.chars();
    let mut c = chars.next().ok_or(())?;
    while c != 'E' {
        // Decode an identifier element's length.
        if !c.is_digit(10) {
            return Err(());
        }
        let mut len = 0usize;
        while let Some(d) = c.to_digit(10) {
            len = len.checked_mul(10)
                .and_then(|len| len.checked_add(d as usize))
                .ok_or(())?;
            c = chars.next().ok_or(())?;
        }

        // `c` already contains the first character of this identifier, skip it and
        // all the other characters of this identifier, to reach the next element.
        for _ in 0..len {
            c = chars.next().ok_or(())?;
        }

        elements += 1;
    }

    Ok((Demangle {
        inner,
        elements,
    }, chars.as_str()))
}

// Rust hashes are hex digits with an `h` prepended.
fn is_rust_hash(s: &str) -> bool {
    s.starts_with('h') && s[1..].chars().all(|c| c.is_digit(16))
}

impl<'a> fmt::Display for Demangle<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // Alright, let's do this.
        let mut inner = self.inner;
        for element in 0..self.elements {
            let mut rest = inner;
            while rest.chars().next().unwrap().is_digit(10) {
                rest = &rest[1..];
            }
            let i: usize = inner[..(inner.len() - rest.len())].parse().unwrap();
            inner = &rest[i..];
            rest = &rest[..i];
            // Skip printing the hash if alternate formatting
            // was requested.
            if f.alternate() && element+1 == self.elements && is_rust_hash(&rest) {
                break;
            }
            if element != 0 {
                f.write_str("::")?;
            }
            if rest.starts_with("_$") {
                rest = &rest[1..];
            }
            loop {
                if rest.starts_with('.') {
                    if let Some('.') = rest[1..].chars().next() {
                        f.write_str("::")?;
                        rest = &rest[2..];
                    } else {
                        f.write_str(".")?;
                        rest = &rest[1..];
                    }
                } else if rest.starts_with('$') {
                    let (escape, after_escape) = if let Some(end) = rest[1..].find('$') {
                        (&rest[1..end + 1], &rest[end + 2..])
                    } else {
                        break;
                    };

                    // see src/librustc_codegen_utils/symbol_names/legacy.rs for these mappings
                    let unescaped = match escape {
                        "SP" => "@",
                        "BP" => "*",
                        "RF" => "&",
                        "LT" => "<",
                        "GT" => ">",
                        "LP" => "(",
                        "RP" => ")",
                        "C" => ",",

                        _ => {
                            if escape.starts_with('u') {
                                let digits = &escape[1..];
                                let all_lower_hex = digits.chars().all(|c| match c {
                                    '0'..='9' | 'a'..='f' => true,
                                    _ => false,
                                });
                                let c = u32::from_str_radix(digits, 16).ok()
                                    .and_then(char::from_u32);
                                if let (true, Some(c)) = (all_lower_hex, c) {
                                    // FIXME(eddyb) do we need to filter out control codepoints?
                                    if !c.is_control() {
                                        c.fmt(f)?;
                                        rest = after_escape;
                                        continue;
                                    }
                                }
                            }
                            break;
                        }
                    };
                    f.write_str(unescaped)?;
                    rest = after_escape;
                } else if let Some(i) = rest.find(|c| c == '$' || c == '.') {
                    f.write_str(&rest[..i])?;
                    rest = &rest[i..];
                } else {
                    break;
                }
            }
            f.write_str(rest)?;
        }

        Ok(())
    }
}
