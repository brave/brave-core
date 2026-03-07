# unicode-normalization-alignments

[![Build Status](https://travis-ci.org/n1t0/unicode-normalization.svg)](https://travis-ci.org/n1t0/unicode-normalization)
[![Docs](https://docs.rs/unicode-normalization-alignments/badge.svg)](https://docs.rs/unicode-normalization-alignments/)

This is a forked version of [unicode-normalization](https://github.com/unicode-rs/unicode-normalization)
wich provides alignment information during normalization.

Unicode character composition and decomposition utilities as described in
[Unicode Standard Annex #15](http://www.unicode.org/reports/tr15/).

This crate requires Rust 1.36+.

```rust
extern crate unicode_normalization_alignments;

use unicode_normalization_alignments::char::compose;
use unicode_normalization_alignments::UnicodeNormalization;

fn main() {
	assert_eq!(compose('A','\u{30a}'), Some('Å'));

	let s = "ÅΩ";
	let c = s.nfc().map(|(c, diff)| {
		match diff {
			0 => println!("Nothing changed here"),
			1 => println!("New character"),
			_ => println!("{} characters were removed", diff),
		}

		c
	}).collect::<String>();
	assert_eq!(c, "ÅΩ");
}
```

## crates.io

You can use this package in your project by adding the following to your `Cargo.toml`:

```toml
[dependencies]
unicode-normalization-alignments = "0.1.12"
```
