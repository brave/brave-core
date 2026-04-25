# Żmij

[<img alt="github" src="https://img.shields.io/badge/github-dtolnay/zmij-8da0cb?style=for-the-badge&labelColor=555555&logo=github" height="20">](https://github.com/dtolnay/zmij)
[<img alt="crates.io" src="https://img.shields.io/crates/v/zmij.svg?style=for-the-badge&color=fc8d62&logo=rust" height="20">](https://crates.io/crates/zmij)
[<img alt="docs.rs" src="https://img.shields.io/badge/docs.rs-zmij-66c2a5?style=for-the-badge&labelColor=555555&logo=docs.rs" height="20">](https://docs.rs/zmij)
[<img alt="build status" src="https://img.shields.io/github/actions/workflow/status/dtolnay/zmij/ci.yml?branch=master&style=for-the-badge" height="20">](https://github.com/dtolnay/zmij/actions?query=branch%3Amaster)

Pure Rust implementation of Żmij, an algorithm to quickly convert floating point
numbers to decimal strings.

This Rust implementation is a line-by-line port of Victor Zverovich's
implementation in C++, [https://github.com/vitaut/zmij][upstream].

[upstream]: https://github.com/vitaut/zmij/tree/d22904d82206a5812e8dc7e26cc1ced90135fb2a

## Example

```rust
fn main() {
    let mut buffer = zmij::Buffer::new();
    let printed = buffer.format(1.234);
    assert_eq!(printed, "1.234");
}
```

## Performance (lower is better)

![performance](https://raw.githubusercontent.com/dtolnay/zmij/master/performance.png)

<br>

#### License

<a href="LICENSE-MIT">MIT license</a>.
