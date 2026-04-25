use std::char;
use std::fmt::{self, Display, Write};

/// Representation of a demangled symbol name.
pub struct Demangle<'a> {
    inner: &'a str,
}

/// De-mangles a Rust symbol into a more readable version
///
/// This function will take a **mangled** symbol and return a value. When printed,
/// the de-mangled version will be written. If the symbol does not look like
/// a mangled symbol, the original value will be written instead.
pub fn demangle(s: &str) -> Result<(Demangle, &str), Invalid> {
    // First validate the symbol. If it doesn't look like anything we're
    // expecting, we just print it literally. Note that we must handle non-Rust
    // symbols because we could have any function in the backtrace.
    let inner;
    if s.len() > 2 && s.starts_with("_R") {
        inner = &s[2..];
    } else if s.len() > 1 && s.starts_with("R") {
        // On Windows, dbghelp strips leading underscores, so we accept "R..."
        // form too.
        inner = &s[1..];
    } else if s.len() > 3 && s.starts_with("__R") {
        // On OSX, symbols are prefixed with an extra _
        inner = &s[3..];
    } else {
        return Err(Invalid);
    }

    // Paths always start with uppercase characters.
    match inner.as_bytes()[0] {
        b'A'..=b'Z' => {}
        _ => return Err(Invalid),
    }

    // only work with ascii text
    if inner.bytes().any(|c| c & 0x80 != 0) {
        return Err(Invalid);
    }

    // Verify that the symbol is indeed a valid path.
    let mut parser = Parser {
        sym: inner,
        next: 0,
    };
    parser.skip_path()?;

    // Instantiating crate (paths always start with uppercase characters).
    match parser.sym.as_bytes().get(parser.next) {
        Some(&(b'A'..=b'Z')) => {
            parser.skip_path()?;
        }
        _ => {}
    }

    Ok((Demangle {
        inner,
    }, &parser.sym[parser.next..]))
}

impl Demangle<'_> {
    pub fn demangle(&self) -> super::SymbolName {
        let mut complete = String::new();
        let crate_name = {
            let mut printer = Printer {
                parser: Ok(Parser {
                    sym: self.inner,
                    next: 0,
                }),
                out: &mut complete,
                bound_lifetime_depth: 0,
                print_disambiguator: true,
                crate_name: None,
            };
            printer.print_path(true).unwrap();
            printer.crate_name
        };

        // Parse again, but without printing disambiguator.
        let mut trimmed = String::new();
        {
            let mut printer = Printer {
                parser: Ok(Parser {
                    sym: self.inner,
                    next: 0,
                }),
                out: &mut trimmed,
                bound_lifetime_depth: 0,
                print_disambiguator: false,
                crate_name: None,
            };
            printer.print_path(true).unwrap();
        }

        super::SymbolName {
            complete,
            trimmed,
            crate_name,
            kind: super::Kind::V0,
        }
    }
}

#[derive(PartialEq, Eq)]
pub struct Invalid;

struct Ident<'s> {
    /// ASCII part of the identifier.
    ascii: &'s str,
    /// Punycode insertion codes for Unicode codepoints, if any.
    punycode: &'s str,
}

const SMALL_PUNYCODE_LEN: usize = 128;

impl<'s> Ident<'s> {
    /// Attempt to decode punycode on the stack (allocation-free),
    /// and pass the char slice to the closure, if successful.
    /// This supports up to `SMALL_PUNYCODE_LEN` characters.
    fn try_small_punycode_decode<F: FnOnce(&[char]) -> R, R>(
        &self,
        f: F,
    ) -> Option<R> {
        let mut out = ['\0'; SMALL_PUNYCODE_LEN];
        let mut out_len = 0;
        let r = self.punycode_decode(|i, c| {
            // Check there's space left for another character.
            out.get(out_len).ok_or(())?;

            // Move the characters after the insert position.
            let mut j = out_len;
            out_len += 1;

            while j > i {
                out[j] = out[j - 1];
                j -= 1;
            }

            // Insert the new character.
            out[i] = c;

            Ok(())
        });
        if r.is_ok() {
            Some(f(&out[..out_len]))
        } else {
            None
        }
    }

    /// Decode punycode as insertion positions and characters
    /// and pass them to the closure, which can return `Err(())`
    /// to stop the decoding process.
    fn punycode_decode<F: FnMut(usize, char) -> Result<(), ()>>(
        &self,
        mut insert: F,
    ) -> Result<(), ()> {
        let mut punycode_bytes = self.punycode.bytes().peekable();
        if punycode_bytes.peek().is_none() {
            return Err(());
        }

        let mut len = 0;

        // Populate initial output from ASCII fragment.
        for c in self.ascii.chars() {
            insert(len, c)?;
            len += 1;
        }

        // Punycode parameters and initial state.
        let base = 36;
        let t_min = 1;
        let t_max = 26;
        let skew = 38;
        let mut damp = 700;
        let mut bias = 72;
        let mut i: usize = 0;
        let mut n: usize = 0x80;

        loop {
            // Read one delta value.
            let mut delta: usize = 0;
            let mut w = 1;
            let mut k: usize = 0;
            loop {
                use core::cmp::{min, max};

                k += base;
                let t = min(max(k.saturating_sub(bias), t_min), t_max);

                let d = match punycode_bytes.next() {
                    Some(d @ b'a'..=b'z') => d - b'a',
                    Some(d @ b'0'..=b'9') => 26 + (d - b'0'),
                    _ => return Err(()),
                };
                let d = d as usize;
                delta = delta.checked_add(
                    d.checked_mul(w).ok_or(())?
                ).ok_or(())?;
                if d < t {
                    break;
                }
                w = w.checked_mul(base - t).ok_or(())?;
            }

            // Compute the new insert position and character.
            len += 1;
            i = i.checked_add(delta).ok_or(())?;
            n = n.checked_add(i / len).ok_or(())?;
            i %= len;

            let n_u32 = n as u32;
            let c = if n_u32 as usize == n {
                char::from_u32(n_u32).ok_or(())?
            } else {
                return Err(());
            };

            // Insert the new character and increment the insert position.
            insert(i, c)?;
            i += 1;

            // If there are no more deltas, decoding is complete.
            if punycode_bytes.peek().is_none() {
                return Ok(());
            }

            // Perform bias adaptation.
            delta /= damp;
            damp = 2;

            delta += delta / len;
            let mut k = 0;
            while delta > ((base - t_min) * t_max) / 2 {
                delta /= base - t_min;
                k += base;
            }
            bias = k + ((base - t_min + 1) * delta) / (delta + skew);
        }
    }
}

impl<'s> Display for Ident<'s> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        self.try_small_punycode_decode(|chars| {
            for &c in chars {
                c.fmt(f)?;
            }
            Ok(())
        }).unwrap_or_else(|| {
            if !self.punycode.is_empty() {
                f.write_str("punycode{")?;

                // Reconstruct a standard Punycode encoding,
                // by using `-` as the separator.
                if !self.ascii.is_empty() {
                    f.write_str(self.ascii)?;
                    f.write_str("-")?;
                }
                f.write_str(self.punycode)?;

                f.write_str("}")
            } else {
                f.write_str(self.ascii)
            }
        })
    }
}

fn basic_type(tag: u8) -> Option<&'static str> {
    Some(match tag {
        b'b' => "bool",
        b'c' => "char",
        b'e' => "str",
        b'u' => "()",
        b'a' => "i8",
        b's' => "i16",
        b'l' => "i32",
        b'x' => "i64",
        b'n' => "i128",
        b'i' => "isize",
        b'h' => "u8",
        b't' => "u16",
        b'm' => "u32",
        b'y' => "u64",
        b'o' => "u128",
        b'j' => "usize",
        b'f' => "f32",
        b'd' => "f64",
        b'z' => "!",
        b'p' => "_",
        b'v' => "...",

        _ => return None,
    })
}

struct Parser<'s> {
    sym: &'s str,
    next: usize,
}

impl<'s> Parser<'s> {
    fn peek(&self) -> Option<u8> {
        self.sym.as_bytes().get(self.next).cloned()
    }

    fn eat(&mut self, b: u8) -> bool {
        if self.peek() == Some(b) {
            self.next += 1;
            true
        } else {
            false
        }
    }

    fn next(&mut self) -> Result<u8, Invalid> {
        let b = self.peek().ok_or(Invalid)?;
        self.next += 1;
        Ok(b)
    }

    fn hex_nibbles(&mut self) -> Result<&'s str, Invalid> {
        let start = self.next;
        loop {
            match self.next()? {
                b'0'..=b'9' | b'a'..=b'f' => {}
                b'_' => break,
                _ => return Err(Invalid),
            }
        }
        Ok(&self.sym[start..self.next - 1])
    }

    fn digit_10(&mut self) -> Result<u8, Invalid> {
        let d = match self.peek() {
            Some(d @ b'0'..=b'9') => d - b'0',
            _ => return Err(Invalid),
        };
        self.next += 1;
        Ok(d)
    }

    fn digit_62(&mut self) -> Result<u8, Invalid> {
        let d = match self.peek() {
            Some(d @ b'0'..=b'9') => d - b'0',
            Some(d @ b'a'..=b'z') => 10 + (d - b'a'),
            Some(d @ b'A'..=b'Z') => 10 + 26 + (d - b'A'),
            _ => return Err(Invalid),
        };
        self.next += 1;
        Ok(d)
    }

    fn integer_62(&mut self) -> Result<u64, Invalid> {
        if self.eat(b'_') {
            return Ok(0);
        }

        let mut x: u64 = 0;
        while !self.eat(b'_') {
            let d = self.digit_62()? as u64;
            x = x.checked_mul(62).ok_or(Invalid)?;
            x = x.checked_add(d).ok_or(Invalid)?;
        }
        x.checked_add(1).ok_or(Invalid)
    }

    fn opt_integer_62(&mut self, tag: u8) -> Result<u64, Invalid> {
        if !self.eat(tag) {
            return Ok(0);
        }
        self.integer_62()?.checked_add(1).ok_or(Invalid)
    }

    fn disambiguator(&mut self) -> Result<u64, Invalid> {
        self.opt_integer_62(b's')
    }

    fn namespace(&mut self) -> Result<Option<char>, Invalid> {
        match self.next()? {
            // Special namespaces, like closures and shims.
            ns @ b'A'..=b'Z' => Ok(Some(ns as char)),

            // Implementation-specific/unspecified namespaces.
            b'a'..=b'z' => Ok(None),

            _ => Err(Invalid),
        }
    }

    fn backref(&mut self) -> Result<Parser<'s>, Invalid> {
        let s_start = self.next - 1;
        let i = self.integer_62()?;
        if i >= s_start as u64 {
            return Err(Invalid);
        }
        Ok(Parser {
            sym: self.sym,
            next: i as usize,
        })
    }

    fn ident(&mut self) -> Result<Ident<'s>, Invalid> {
        let is_punycode = self.eat(b'u');
        let mut len = self.digit_10()? as usize;
        if len != 0 {
            loop {
                match self.digit_10() {
                    Ok(d) => {
                        len = len.checked_mul(10).ok_or(Invalid)?;
                        len = len.checked_add(d as usize).ok_or(Invalid)?;
                    }
                    Err(Invalid) => break,
                }
            }
        }

        // Skip past the optional `_` separator.
        self.eat(b'_');

        let start = self.next;
        self.next = self.next.checked_add(len).ok_or(Invalid)?;
        if self.next > self.sym.len() {
            return Err(Invalid);
        }

        let ident = &self.sym[start..self.next];

        if is_punycode {
            let ident = match ident.bytes().rposition(|b| b == b'_') {
                Some(i) => Ident {
                    ascii: &ident[..i],
                    punycode: &ident[i + 1..],
                },
                None => Ident {
                    ascii: "",
                    punycode: ident,
                },
            };
            if ident.punycode.is_empty() {
                return Err(Invalid);
            }
            Ok(ident)
        } else {
            Ok(Ident {
                ascii: ident,
                punycode: "",
            })
        }
    }

    fn skip_path(&mut self) -> Result<(), Invalid> {
        match self.next()? {
            b'C' => {
                self.disambiguator()?;
                self.ident()?;
            }
            b'N' => {
                self.namespace()?;
                self.skip_path()?;
                self.disambiguator()?;
                self.ident()?;
            }
            b'M' => {
                self.disambiguator()?;
                self.skip_path()?;
                self.skip_type()?;
            }
            b'X' => {
                self.disambiguator()?;
                self.skip_path()?;
                self.skip_type()?;
                self.skip_path()?;
            }
            b'Y' => {
                self.skip_type()?;
                self.skip_path()?;
            }
            b'I' => {
                self.skip_path()?;
                while !self.eat(b'E') {
                    self.skip_generic_arg()?;
                }
            }
            b'B' => {
                self.backref()?;
            }
            _ => return Err(Invalid),
        }
        Ok(())
    }

    fn skip_generic_arg(&mut self) -> Result<(), Invalid> {
        if self.eat(b'L') {
            self.integer_62()?;
            Ok(())
        } else if self.eat(b'K') {
            self.skip_const()
        } else {
            self.skip_type()
        }
    }

    fn skip_type(&mut self) -> Result<(), Invalid> {
        match self.next()? {
            tag if basic_type(tag).is_some() => {}

            b'R' | b'Q' => {
                if self.eat(b'L') {
                    self.integer_62()?;
                }
                self.skip_type()?;
            }
            b'P' | b'O' | b'S' => self.skip_type()?,
            b'A' => {
                self.skip_type()?;
                self.skip_const()?;
            }
            b'T' => while !self.eat(b'E') {
                self.skip_type()?;
            },
            b'F' => {
                let _binder = self.opt_integer_62(b'G')?;
                let _is_unsafe = self.eat(b'U');
                if self.eat(b'K') {
                    let c_abi = self.eat(b'C');
                    if !c_abi {
                        let abi = self.ident()?;
                        if abi.ascii.is_empty() || !abi.punycode.is_empty() {
                            return Err(Invalid);
                        }
                    }
                }
                while !self.eat(b'E') {
                    self.skip_type()?;
                }
                self.skip_type()?;
            }
            b'D' => {
                let _binder = self.opt_integer_62(b'G')?;
                while !self.eat(b'E') {
                    self.skip_path()?;
                    while self.eat(b'p') {
                        self.ident()?;
                        self.skip_type()?;
                    }
                }
                if !self.eat(b'L') {
                    return Err(Invalid);
                }
                self.integer_62()?;
            }
            b'B' => {
                self.backref()?;
            }
            _ => {
                // Go back to the tag, so `skip_path` also sees it.
                self.next -= 1;
                self.skip_path()?;
            }
        }
        Ok(())
    }

    fn skip_const(&mut self) -> Result<(), Invalid> {
        if self.eat(b'B') {
            self.backref()?;
            return Ok(());
        }

        match self.next()? {
            // Unsigned integer types.
            b'h' | b't' | b'm' | b'y' | b'o' | b'j' => {}

            _ => return Err(Invalid),
        }

        if self.eat(b'p') {
            return Ok(());
        }
        self.hex_nibbles()?;
        Ok(())
    }
}

struct Printer<'a, 's> {
    parser: Result<Parser<'s>, Invalid>,
    out: &'a mut String,
    bound_lifetime_depth: u32,
    print_disambiguator: bool,
    crate_name: Option<String>,
}

/// Mark the parser as errored, print `?` and return early.
/// This allows callers to keep printing the approximate
/// syntax of the path/type/const, despite having errors.
/// E.g. `Vec<[(A, ?); ?]>` instead of `Vec<[(A, ?`.
macro_rules! invalid {
    ($printer:ident) => {{
        $printer.parser = Err(Invalid);
        $printer.out.push('?');
        return Ok(());
    }}
}

/// Call a parser method (if the parser hasn't errored yet),
/// and mark the parser as errored if it returns `Err(Invalid)`.
///
/// If the parser errored, before or now, prints `?`, and
/// returns early the current function (see `invalid!` above).
macro_rules! parse {
    ($printer:ident, $method:ident $(($($arg:expr),*))*) => {
        match $printer.parser_mut().and_then(|p| p.$method($($($arg),*)*)) {
            Ok(x) => x,
            Err(Invalid) => invalid!($printer),
        }
    };
}

impl<'a, 's> Printer<'a, 's> {
    fn parser_mut<'c>(&'c mut self) -> Result<&'c mut Parser<'s>, Invalid> {
        self.parser.as_mut().map_err(|_| Invalid)
    }

    /// Eat the given character from the parser,
    /// returning `false` if the parser errored.
    fn eat(&mut self, b: u8) -> bool {
        self.parser_mut().map(|p| p.eat(b)) == Ok(true)
    }

    /// Return a nested parser for a backref.
    fn backref_printer<'c>(&'c mut self) -> Printer<'c, 's> {
        Printer {
            parser: self.parser_mut().and_then(|p| p.backref()),
            out: self.out,
            bound_lifetime_depth: self.bound_lifetime_depth,
            print_disambiguator: self.print_disambiguator,
            crate_name: self.crate_name.clone(),
        }
    }

    /// Print the lifetime according to the previously decoded index.
    /// An index of `0` always refers to `'_`, but starting with `1`,
    /// indices refer to late-bound lifetimes introduced by a binder.
    fn print_lifetime_from_index(&mut self, lt: u64) -> fmt::Result {
        self.out.write_str("'")?;
        if lt == 0 {
            return self.out.write_str("_");
        }
        match (self.bound_lifetime_depth as u64).checked_sub(lt) {
            Some(depth) => {
                // Try to print lifetimes alphabetically first.
                if depth < 26 {
                    let c = (b'a' + depth as u8) as char;
                    self.out.write_fmt(format_args!("{}", c))
                } else {
                    // Use `'_123` after running out of letters.
                    self.out.write_str("_")?;
                    self.out.write_fmt(format_args!("{}", depth))
                }
            }
            None => invalid!(self),
        }
    }

    /// Optionally enter a binder ('G') for late-bound lifetimes,
    /// printing e.g. `for<'a, 'b> ` before calling the closure,
    /// and make those lifetimes visible to it (via depth level).
    fn in_binder<F>(&mut self, f: F) -> fmt::Result
        where F: FnOnce(&mut Self) -> fmt::Result,
    {
        let bound_lifetimes = parse!(self, opt_integer_62(b'G'));

        if bound_lifetimes > 0 {
            self.out.push_str("for<");
            for i in 0..bound_lifetimes {
                if i > 0 {
                    self.out.push_str(", ");
                }
                self.bound_lifetime_depth += 1;
                self.print_lifetime_from_index(1)?;
            }
            self.out.push_str("> ");
        }

        let r = f(self);

        // Restore `bound_lifetime_depth` to the previous value.
        self.bound_lifetime_depth -= bound_lifetimes as u32;

        r
    }

    /// Print list elements using the given closure and separator,
    /// until the end of the list ('E') is found, or the parser errors.
    /// Returns the number of elements printed.
    fn print_sep_list<F>(&mut self, f: F, sep: &str) -> Result<usize, fmt::Error>
        where F: Fn(&mut Self) -> fmt::Result,
    {
        let mut i = 0;
        while self.parser.is_ok() && !self.eat(b'E') {
            if i > 0 {
                self.out.push_str(sep);
            }
            f(self)?;
            i += 1;
        }
        Ok(i)
    }

    fn print_path(&mut self, in_value: bool) -> fmt::Result {
        let tag = parse!(self, next);
        match tag {
            b'C' => {
                let dis = parse!(self, disambiguator);
                let name = parse!(self, ident);

                if self.crate_name.is_none() {
                    self.crate_name = Some(name.to_string());
                }

                self.out.write_fmt(format_args!("{}", name))?;
                if self.print_disambiguator {
                    self.out.push_str("[");
                    self.out.write_fmt(format_args!("{:x}", dis))?;
                    self.out.push_str("]");
                }
            }
            b'N' => {
                let ns = parse!(self, namespace);

                self.print_path(in_value)?;

                let dis = parse!(self, disambiguator);
                let name = parse!(self, ident);

                match ns {
                    // Special namespaces, like closures and shims.
                    Some(ns) => {
                        self.out.push_str("::{");
                        match ns {
                            'C' => self.out.push_str("closure"),
                            'S' => self.out.push_str("shim"),
                            _ => self.out.write_fmt(format_args!("{}", ns))?,
                        }
                        if !name.ascii.is_empty() || !name.punycode.is_empty() {
                            self.out.push_str(":");
                            self.out.write_fmt(format_args!("{}", name))?;
                        }
                        self.out.push_str("#");
                        self.out.write_fmt(format_args!("{}", dis))?;
                        self.out.push_str("}");
                    }

                    // Implementation-specific/unspecified namespaces.
                    None => {
                        if !name.ascii.is_empty() || !name.punycode.is_empty() {
                            self.out.push_str("::");
                            self.out.write_fmt(format_args!("{}", name))?;
                        }
                    }
                }
            }
            b'M' | b'X' | b'Y' => {
                if tag != b'Y' {
                    // Ignore the `impl`'s own path.
                    parse!(self, disambiguator);

                    // `rustc-demangle` ignores this path since it makes
                    // a symbol name too verbose, but we need it to extract a crate name.
                    // So we will print it as usual and then remove it from an output buffer.
                    let start = self.out.len();
                    self.print_path(in_value)?;
                    self.out.drain(start..);
                }

                self.out.push_str("<");
                self.print_type()?;
                if tag != b'M' {
                    self.out.push_str(" as ");
                    self.print_path(false)?;
                }
                self.out.push_str(">");
            }
            b'I' => {
                self.print_path(in_value)?;
                if in_value {
                    self.out.push_str("::");
                }
                self.out.push_str("<");
                self.print_sep_list(Self::print_generic_arg, ", ")?;
                self.out.push_str(">");
            }
            b'B' => {
                self.backref_printer().print_path(in_value)?;
            }
            _ => invalid!(self),
        }
        Ok(())
    }

    fn print_generic_arg(&mut self) -> fmt::Result {
        if self.eat(b'L') {
            let lt = parse!(self, integer_62);
            self.print_lifetime_from_index(lt)
        } else if self.eat(b'K') {
            self.print_const()
        } else {
            self.print_type()
        }
    }

    fn print_type(&mut self) -> fmt::Result {
        let tag = parse!(self, next);

        match basic_type(tag) {
            Some(ty) => return self.out.write_str(ty),
            None => {}
        }

        match tag {
            b'R' | b'Q' => {
                self.out.write_str("&")?;
                if self.eat(b'L') {
                    let lt = parse!(self, integer_62);
                    if lt != 0 {
                        self.print_lifetime_from_index(lt)?;
                        self.out.write_str(" ")?;
                    }
                }
                if tag != b'R' {
                    self.out.write_str("mut ")?;
                }
                self.print_type()?;
            }

            b'P' | b'O' => {
                self.out.write_str("*")?;
                if tag != b'P' {
                    self.out.write_str("mut ")?;
                } else {
                    self.out.write_str("const ")?;
                }
                self.print_type()?;
            }

            b'A' | b'S' => {
                self.out.write_str("[")?;
                self.print_type()?;
                if tag == b'A' {
                    self.out.write_str("; ")?;
                    self.print_const()?;
                }
                self.out.write_str("]")?;
            }
            b'T' => {
                self.out.write_str("(")?;
                let count = self.print_sep_list(Self::print_type, ", ")?;
                if count == 1 {
                    self.out.write_str(",")?;
                }
                self.out.write_str(")")?;
            }
            b'F' => self.in_binder(|this| {
                let is_unsafe = this.eat(b'U');
                let abi = if this.eat(b'K') {
                    if this.eat(b'C') {
                        Some("C")
                    } else {
                        let abi = parse!(this, ident);
                        if abi.ascii.is_empty() || !abi.punycode.is_empty() {
                            invalid!(this);
                        }
                        Some(abi.ascii)
                    }
                } else {
                    None
                };

                if is_unsafe {
                    this.out.write_str("unsafe ")?;
                }

                match abi {
                    Some(abi) => {
                        this.out.write_str("extern \"")?;

                        // If the ABI had any `-`, they were replaced with `_`,
                        // so the parts between `_` have to be re-joined with `-`.
                        let mut parts = abi.split('_');
                        this.out.write_str(parts.next().unwrap())?;
                        for part in parts {
                            this.out.write_str("-")?;
                            this.out.write_str(part)?;
                        }

                        this.out.write_str("\" ")?;
                    }
                    None => {}
                }

                this.out.write_str("fn(")?;
                this.print_sep_list(Self::print_type, ", ")?;
                this.out.write_str(")")?;

                if this.eat(b'u') {
                    // Skip printing the return type if it's 'u', i.e. `()`.
                } else {
                    this.out.write_str(" -> ")?;
                    this.print_type()?;
                }

                Ok(())
            })?,
            b'D' => {
                self.out.write_str("dyn ")?;
                self.in_binder(|this| {
                    this.print_sep_list(Self::print_dyn_trait, " + ")?;
                    Ok(())
                })?;

                if !self.eat(b'L') {
                    invalid!(self);
                }
                let lt = parse!(self, integer_62);
                if lt != 0 {
                    self.out.write_str(" + ")?;
                    self.print_lifetime_from_index(lt)?;
                }
            }
            b'B' => {
                self.backref_printer().print_type()?;
            }
            _ => {
                // Go back to the tag, so `print_path` also sees it.
                let _ = self.parser_mut().map(|p| p.next -= 1);
                self.print_path(false)?;
            }
        }
        Ok(())
    }

    /// A trait in a trait object may have some "existential projections"
    /// (i.e. associated type bindings) after it, which should be printed
    /// in the `<...>` of the trait, e.g. `dyn Trait<T, U, Assoc=X>`.
    /// To this end, this method will keep the `<...>` of an 'I' path
    /// open, by omitting the `>`, and return `Ok(true)` in that case.
    fn print_path_maybe_open_generics(&mut self) -> Result<bool, fmt::Error> {
        if self.eat(b'B') {
            self.backref_printer().print_path_maybe_open_generics()
        } else if self.eat(b'I') {
            self.print_path(false)?;
            self.out.write_str("<")?;
            self.print_sep_list(Self::print_generic_arg, ", ")?;
            Ok(true)
        } else {
            self.print_path(false)?;
            Ok(false)
        }
    }

    fn print_dyn_trait(&mut self) -> fmt::Result {
        let mut open = self.print_path_maybe_open_generics()?;

        while self.eat(b'p') {
            if !open {
                self.out.write_str("<")?;
                open = true;
            } else {
                self.out.write_str(", ")?;
            }

            let name = parse!(self, ident);
            self.out.write_fmt(format_args!("{}", name))?;
            self.out.write_str(" = ")?;
            self.print_type()?;
        }

        if open {
            self.out.write_str(">")?;
        }

        Ok(())
    }

    fn print_const(&mut self) -> fmt::Result {
        if self.eat(b'B') {
            return self.backref_printer().print_const();
        }

        let ty_tag = parse!(self, next);
        let ty = match ty_tag {
            // Unsigned integer types.
            b'h' | b't' | b'm' | b'y' | b'o' | b'j' => {
                basic_type(ty_tag).unwrap()
            }

            _ => invalid!(self),
        };


        if self.eat(b'p') {
            self.out.write_str("_")?;
        } else {
            self.print_const_uint()?;
        }

        self.out.write_str(": ")?;
        self.out.write_str(ty)?;

        Ok(())
    }

    fn print_const_uint(&mut self) -> fmt::Result {
        let hex = parse!(self, hex_nibbles);

        // Print anything that doesn't fit in `u64` verbatim.
        if hex.len() > 16 {
            self.out.push_str("0x");
            self.out.push_str(hex);
            return Ok(());
        }

        let mut v = 0;
        for c in hex.chars() {
            v = (v << 4) | (c.to_digit(16).unwrap() as u64);
        }
        self.out.write_fmt(format_args!("{}", v))
    }
}

#[cfg(test)]
mod tests {
    use crate::demangle::SymbolName;

    macro_rules! t_nohash {
        ($a:expr, $b:expr) => ({
            assert_eq!(SymbolName::demangle($a).trimmed, $b);
        })
    }
    macro_rules! t_nohash_type {
        ($a:expr, $b:expr) => (
            t_nohash!(concat!("_RMC0", $a), concat!("<", $b, ">"))
        )
    }

    #[test]
    fn demangle_crate_with_leading_digit() {
        t_nohash!(
            "_RNvC6_123foo3bar",
            "123foo::bar"
        );
    }

    #[test]
    fn demangle_utf8_idents() {
        t_nohash!(
            "_RNqCs4fqI2P2rA04_11utf8_identsu30____7hkackfecea1cbdathfdh9hlq6y",
            "utf8_idents::საჭმელად_გემრიელი_სადილი"
        );
    }

    #[test]
    fn demangle_closure() {
        t_nohash!(
            "_RNCNCNgCs6DXkGYLi8lr_2cc5spawn00B5_",
            "cc::spawn::{closure#0}::{closure#0}"
        );
        t_nohash!(
            "_RNCINkXs25_NgCsbmNqQUJIY6D_4core5sliceINyB9_4IterhENuNgNoBb_4iter8iterator8Iterator9rpositionNCNgNpB9_6memchr7memrchrs_0E0Bb_",
            "<core::slice::Iter<u8> as core::iter::iterator::Iterator>::rposition::<core::slice::memchr::memrchr::{closure#1}>::{closure#0}"
        );
    }

    #[test]
    fn demangle_dyn_trait() {
        t_nohash!(
            "_RINbNbCskIICzLVDPPb_5alloc5alloc8box_freeDINbNiB4_5boxed5FnBoxuEp6OutputuEL_ECs1iopQbuBiw2_3std",
            "alloc::alloc::box_free::<dyn alloc::boxed::FnBox<(), Output = ()>>"
        );
    }

    #[test]
    fn demangle_const_generics() {
        // NOTE(eddyb) this was hand-written, before rustc had working
        // const generics support (but the mangling format did include them).
        t_nohash_type!(
            "INtC8arrayvec8ArrayVechKj7b_E",
            "arrayvec::ArrayVec<u8, 123: usize>"
        );
    }

    #[test]
    fn demangle_exponential_explosion() {
        // NOTE(eddyb) because of the prefix added by `t_nohash_type!` is
        // 3 bytes long, `B2_` refers to the start of the type, not `B_`.
        // 6 backrefs (`B8_E` through `B3_E`) result in 2^6 = 64 copies of `_`.
        // Also, because the `p` (`_`) type is after all of the starts of the
        // backrefs, it can be replaced with any other type, independently.
        t_nohash_type!(
            concat!("TTTTTT", "p", "B8_E", "B7_E", "B6_E", "B5_E", "B4_E", "B3_E"),
            "((((((_, _), (_, _)), ((_, _), (_, _))), (((_, _), (_, _)), ((_, _), (_, _)))), \
               ((((_, _), (_, _)), ((_, _), (_, _))), (((_, _), (_, _)), ((_, _), (_, _))))), \
              (((((_, _), (_, _)), ((_, _), (_, _))), (((_, _), (_, _)), ((_, _), (_, _)))), \
               ((((_, _), (_, _)), ((_, _), (_, _))), (((_, _), (_, _)), ((_, _), (_, _))))))"
        );
    }

    #[test]
    fn demangle_thinlto() {
        t_nohash!("_RC3foo.llvm.9D1C9369", "foo");
        t_nohash!("_RC3foo.llvm.9D1C9369@@16", "foo");
        t_nohash!("_RNvC9backtrace3foo.llvm.A5310EB9", "backtrace::foo");
    }
}
