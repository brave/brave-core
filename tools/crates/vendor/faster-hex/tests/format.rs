use core::fmt::{self, Write};
use faster_hex::Hex;

// Use the standard formatter's integer padding as an independent flag oracle.
struct Digits<'a>(&'a str);

impl fmt::Display for Digits<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.pad_integral(true, "0x", self.0)
    }
}

impl fmt::LowerHex for Digits<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.pad_integral(true, "0x", self.0)
    }
}

impl fmt::UpperHex for Digits<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.pad_integral(true, "0x", self.0)
    }
}

#[test]
fn bytes_keep_their_order_and_leading_zeroes() {
    for len in [
        0, 1, 7, 8, 16, 32, 63, 64, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1024, 4097,
    ] {
        let bytes: Vec<_> = (0..len).map(|i| i as u8).collect();
        let view = Hex::new(&bytes);
        assert_eq!(format!("{view}"), hex::encode(&bytes));
        assert_eq!(format!("{view:x}"), hex::encode(&bytes));
        assert_eq!(format!("{view:X}"), hex::encode_upper(&bytes));
    }
    assert_eq!(format!("{}", Hex::new(&[0, 0, 0xab, 0])), "0000ab00");
}

#[test]
fn flags_match_standard_integer_padding() {
    for bytes in [b"".as_slice(), &[0], &[0, 0xab, 0xcd], &[255; 33]] {
        let view = Hex::new(bytes);
        let lower = hex::encode(bytes);
        let upper = hex::encode_upper(bytes);
        let lower = Digits(&lower);
        let upper = Digits(&upper);
        macro_rules! compare {
            ($expected:expr; $($format:literal),+ $(,)?) => {
                $(assert_eq!(format!($format, view), format!($format, $expected), "format {}", $format);)+
            };
        }
        compare!(lower;
            "{}", "{:#}", "{:+}", "{:+#}", "{:20}", "{:020}", "{:#020}",
            "{:+#020}", "{:<20}", "{:>20}", "{:^21}", "{:*>20}", "{:💠^21}",
            "{:0>20}", "{:0>#20}", "{:*<+#020}", "{:.0}", "{:.1}", "{:-#20.1}",
            "{:x}", "{:#x}", "{:+#020x}", "{:💠^#21x}", "{:.1x}", "{:*<+#020.1x}",
        );
        compare!(upper;
            "{:X}", "{:#X}", "{:+#020X}", "{:💠^#21X}", "{:.1X}", "{:*<+#020.1X}",
        );
        for width in [0, 1, 2, 3, 7, 10, 19, 22, 67, 72] {
            for precision in [0, 1, 2, 7, 100] {
                assert_eq!(
                    format!("{view:💠^+#width$.precision$X}"),
                    format!("{upper:💠^+#width$.precision$X}"),
                );
            }
        }
    }
}

#[test]
fn empty_sign_prefix_and_zero_padding_are_explicit() {
    let empty = Hex::new(&[]);
    assert_eq!(format!("{empty}"), "");
    assert_eq!(format!("{empty:#X}"), "0x");
    assert_eq!(format!("{empty:+#}"), "+0x");
    let hex = Hex::new(&[0xab]);
    assert_eq!(format!("{hex:+#08X}"), "+0x000AB");
    assert_eq!(format!("{hex:0>#8X}"), "00000xAB");
    assert_eq!(format!("{hex:💠^#7X}"), "💠0xAB💠💠");
    assert_eq!(format!("{hex:.0}"), "ab");
}

struct FixedWriter {
    bytes: [u8; 1100],
    len: usize,
    limit: usize,
    failed: bool,
    called_after_error: bool,
    accept_partial: bool,
}

impl FixedWriter {
    fn new(limit: usize) -> Self {
        Self {
            bytes: [0; 1100],
            len: 0,
            limit,
            failed: false,
            called_after_error: false,
            accept_partial: false,
        }
    }

    fn as_str(&self) -> &str {
        core::str::from_utf8(&self.bytes[..self.len]).unwrap()
    }
}

impl Write for FixedWriter {
    fn write_str(&mut self, text: &str) -> fmt::Result {
        self.called_after_error |= self.failed;
        if text.len() > self.limit - self.len {
            if self.accept_partial {
                let mut accepted = self.limit - self.len;
                while !text.is_char_boundary(accepted) {
                    accepted -= 1;
                }
                self.bytes[self.len..self.len + accepted]
                    .copy_from_slice(&text.as_bytes()[..accepted]);
                self.len += accepted;
            }
            self.failed = true;
            return Err(fmt::Error);
        }
        self.bytes[self.len..self.len + text.len()].copy_from_slice(text.as_bytes());
        self.len += text.len();
        Ok(())
    }
}

#[test]
fn partial_writer_errors_preserve_the_exact_accepted_prefix() {
    let bytes = [0xab; 513];
    let view = Hex::new(&bytes);
    for style in 0..3 {
        let expected = match style {
            0 => format!("{view:💠^+#1040X}"),
            1 => format!("{view:💠>+#1040x}"),
            _ => format!("{view:+#01040X}"),
        };
        for limit in (0..12).chain([
            511,
            512,
            513,
            1023,
            1024,
            1025,
            expected.len() - 1,
            expected.len(),
        ]) {
            let mut writer = FixedWriter::new(limit);
            writer.accept_partial = true;
            let result = match style {
                0 => write!(writer, "{view:💠^+#1040X}"),
                1 => write!(writer, "{view:💠>+#1040x}"),
                _ => write!(writer, "{view:+#01040X}"),
            };
            assert_eq!(result.is_ok(), limit == expected.len());
            let mut accepted = limit;
            while !expected.is_char_boundary(accepted) {
                accepted -= 1;
            }
            assert_eq!(writer.as_str(), &expected[..accepted]);
            assert!(!writer.called_after_error);
        }
    }
}

#[test]
fn writes_into_fixed_storage_without_alloc() {
    const VIEW: Hex<'static> = Hex::new(&[0, 0xab, 0xcd]);
    let mut writer = FixedWriter::new(1100);
    write!(writer, "hash={VIEW:#X}, again={VIEW}").unwrap();
    assert_eq!(writer.as_str(), "hash=0x00ABCD, again=00abcd");
    assert!(!writer.failed);
}

#[test]
fn writer_errors_stop_immediately_and_preserve_accepted_text() {
    let bytes = [0xab; 513];
    for len in [0, 1, 10, 32, 128, 129, 513] {
        let view = Hex::new(&bytes[..len]);
        let width = len * 2 + 14;
        for padded in [false, true] {
            let expected = if padded {
                format!("{view:💠^+#width$X}")
            } else {
                format!("{view:+#X}")
            };
            for limit in [0, 1, 4, 7, 8, 9, 10, 100, 511, 512, 513, 1024, 1040, 1100] {
                let mut writer = FixedWriter::new(limit);
                let result = if padded {
                    write!(writer, "{view:💠^+#width$X}")
                } else {
                    write!(writer, "{view:+#X}")
                };
                if limit >= expected.len() {
                    assert!(result.is_ok());
                    assert_eq!(writer.as_str(), expected);
                } else {
                    assert!(result.is_err());
                    assert!(writer.failed);
                    assert!(expected.starts_with(writer.as_str()));
                }
                assert!(!writer.called_after_error);
            }
        }
    }
}
