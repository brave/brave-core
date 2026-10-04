# unicode-bom

[![Build status](https://gitlab.com/philbooth/unicode-bom/badges/master/pipeline.svg)](https://gitlab.com/philbooth/unicode-bom/pipelines)
[![Crate status](https://img.shields.io/crates/v/unicode-bom.svg)](https://crates.io/crates/unicode_bom)
[![Downloads](https://img.shields.io/crates/d/unicode-bom.svg)](https://crates.io/crates/unicode_bom)
[![License](https://img.shields.io/crates/l/unicode-bom.svg)](https://www.apache.org/licenses/LICENSE-2.0)

[Unicode byte-order mark](https://en.wikipedia.org/wiki/Byte_order_mark) detection
for Rust projects.

* [What does it do?](#what-does-it-do)
* [What doesn't it do?](#what-doesnt-it-do)
* [How do I install it?](#how-do-i-install-it)
* [How do I use it?](#how-do-i-use-it)
* [How do I set up the build environment?](#how-do-i-set-up-the-build-environment)
* [Is there API documentation?](#is-there-api-documentation)
* [Is there a change log?](#is-there-a-change-log)
* [What license is it published under?](#what-license-is-it-published-under)

## What does it do?

`unicode-bom` will read
the first few bytes from
an array or a file on disk,
then determine whether
a byte-order mark is present.

## What doesn't it do?

It won't check the rest of the data
to determine whether it's actually valid
according to the indicated encoding.

## How do I install it?

Add it to your dependencies
in `Cargo.toml`:

```toml
[dependencies]
unicode-bom = "2"
```

## How do I use it?

For more detailed information
see the [API docs](https://philbooth.gitlab.io/unicode-bom/unicode_bom/),
but the general gist
is as follows:

```rust
use unicode_bom::Bom;

// The BOM can be parsed from a file on disk via the `FromStr` trait...
let bom: Bom = "foo.txt".parse().unwrap();
match bom {
    Bom::Null => {
        // No BOM was detected
    }
    Bom::Bocu1 => {
        // BOCU-1 BOM was detected
    }
    Bom::Gb18030 => {
        // GB 18030 BOM was detected
    }
    Bom::Scsu => {
        // SCSU BOM was detected
    }
    Bom::UtfEbcdic => {
        // UTF-EBCDIC BOM was detected
    }
    Bom::Utf1 => {
        // UTF-1 BOM was detected
    }
    Bom::Utf7 => {
        // UTF-7 BOM was detected
    }
    Bom::Utf8 => {
        // UTF-8 BOM was detected
    }
    Bom::Utf16Be => {
        // UTF-16 (big-endian) BOM was detected
    }
    Bom::Utf16Le => {
        // UTF-16 (little-endian) BOM was detected
    }
    Bom::Utf32Be => {
        // UTF-32 (big-endian) BOM was detected
    }
    Bom::Utf32Le => {
        // UTF-32 (little-endian) BOM was detected
    }
}

// ...or you can detect the BOM in a byte array
let bytes = [0u8, 0u8, 0xfeu8, 0xffu8];
let bom = Bom::from(&bytes[0..]);
assert_eq!(bom, Bom::Utf32Be);
assert_eq(bom.len(), 4);
```

## How do I set up the build environment?

If you don't already have Rust installed,
get that first using [`rustup`](https://rustup.rs/):

```
curl https://sh.rustup.rs -sSf | sh
```

Then you can build the project:

```
cargo b
```

And run the tests:

```
cargo t
```

## Is there API documentation?

[Yes](https://philbooth.gitlab.io/unicode-bom/unicode_bom/).

## Is there a change log?

[Yes](HISTORY.md).

## What license is it published under?

[Apache-2.0](https://www.apache.org/licenses/LICENSE-2.0).

