use std::str::FromStr;

/// All display hints
#[derive(Clone, Debug, Eq, PartialEq)]
#[non_exhaustive]
pub enum DisplayHint {
    NoHint {
        zero_pad: usize,
    },
    /// `:x` OR `:X`
    Hexadecimal {
        alternate: bool,
        uppercase: bool,
        zero_pad: usize,
    },
    /// `:o`
    Octal {
        alternate: bool,
        zero_pad: usize,
    },
    /// `:b`
    Binary {
        alternate: bool,
        zero_pad: usize,
    },
    /// `:a`
    Ascii,
    /// `:?`
    Debug,
    /// `:us` `:ms`, formats integers as timestamps in seconds
    Seconds(TimePrecision),
    /// `:tus` `:tms` `:ts`, formats integers as human-readable time
    Time(TimePrecision),
    /// `:iso8601{ms,s}`, formats integers as timestamp in ISO8601 date time format
    ISO8601(TimePrecision),
    /// `__internal_bitflags_NAME` instructs the decoder to print the flags that are set, instead of
    /// the raw value.
    Bitflags {
        name: String,
        package: String,
        disambiguator: String,
        crate_name: Option<String>,
    },
    /// `:cbor`: There is CBOR data encoded in those bytes, to be shown in diagnostic notation.
    ///
    /// Technically, the byte string interpreted as a CBOR sequence, and shown in the diagnostic
    /// notation of a sequence. That is identical to processing a single CBOR item if there is just
    /// one present, but also allows reporting multiple items from consecutive memory; diagnostic
    /// notation turns those into comma separated items.
    // Should we have a flag to say "do a more display style stringification" (like, if you
    // recognize `54([64,h'20010db8'])`, don't show "IP'2001:db8::/64'" but just "2001:db8::64")?
    // Should we allow additional params that give a CDDL that further guides processing (like,
    // when data is not tagged but the shape is known for processing anyway)?
    Cbor,
    /// Display hints currently not supported / understood
    Unknown(String),
}

impl DisplayHint {
    /// Parses the display hint (e.g. the `#x` in `{=u8:#x}`)
    pub(crate) fn parse(mut s: &str) -> Option<Self> {
        const BITFLAGS_HINT_START: &str = "__internal_bitflags_";

        // The `#` comes before any padding hints (I think this matches core::fmt).
        // It is ignored for types that don't have an alternate representation.
        let alternate = if let Some(rest) = s.strip_prefix('#') {
            s = rest;
            true
        } else {
            false
        };

        let zero_pad = if let Some(rest) = s.strip_prefix('0') {
            let (rest, columns) = parse_integer::<usize>(rest)?;
            s = rest;
            columns
        } else {
            0 // default behavior is the same as no zero-padding.
        };

        if let Some(stripped) = s.strip_prefix(BITFLAGS_HINT_START) {
            let parts = stripped.split('@').collect::<Vec<_>>();
            if parts.len() < 3 || parts.len() > 4 {
                return Some(DisplayHint::Unknown(s.into()));
            }
            return Some(DisplayHint::Bitflags {
                name: parts[0].into(),
                package: parts[1].into(),
                disambiguator: parts[2].into(),
                // crate_name was added in wire format version 4
                crate_name: parts.get(3).map(|&s| s.to_string()),
            });
        }

        Some(match s {
            "" => DisplayHint::NoHint { zero_pad },
            "us" => DisplayHint::Seconds(TimePrecision::Micros),
            "ms" => DisplayHint::Seconds(TimePrecision::Millis),
            "tus" => DisplayHint::Time(TimePrecision::Micros),
            "tms" => DisplayHint::Time(TimePrecision::Millis),
            "ts" => DisplayHint::Time(TimePrecision::Seconds),
            "a" => DisplayHint::Ascii,
            "b" => DisplayHint::Binary {
                alternate,
                zero_pad,
            },
            "o" => DisplayHint::Octal {
                alternate,
                zero_pad,
            },
            "x" => DisplayHint::Hexadecimal {
                alternate,
                uppercase: false,
                zero_pad,
            },
            "X" => DisplayHint::Hexadecimal {
                alternate,
                uppercase: true,
                zero_pad,
            },
            "iso8601ms" => DisplayHint::ISO8601(TimePrecision::Millis),
            "iso8601s" => DisplayHint::ISO8601(TimePrecision::Seconds),
            "cbor" => DisplayHint::Cbor,
            "?" => DisplayHint::Debug,
            _ => return None,
        })
    }
}

/// Precision of timestamp
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum TimePrecision {
    Micros,
    Millis,
    Seconds,
}

/// Parses an integer at the beginning of `s`.
///
/// Returns the integer and remaining text, if `s` started with an integer. Any errors parsing the
/// number (which we already know only contains digits) are silently ignored.
fn parse_integer<T: FromStr>(s: &str) -> Option<(&str, T)> {
    let start_digits = s
        .as_bytes()
        .iter()
        .copied()
        .take_while(|b| b.is_ascii_digit())
        .count();
    let num = s[..start_digits].parse().ok()?;
    Some((&s[start_digits..], num))
}
