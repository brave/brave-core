This crate provides a safe interface for reading and writing information to the kernel using the sysctl interface.

[![Build Status](https://api.cirrus-ci.com/github/johalun/sysctl-rs.svg)](https://cirrus-ci.com/github/johalun/sysctl-rs/master)

[![Current Version](https://img.shields.io/crates/v/sysctl.svg)](https://crates.io/crates/sysctl)


*FreeBSD, Linux, macOS and iOS are supported.*
*Contributions for improvements and other platforms are welcome.*

### Documentation

Documentation is available on [docs.rs](https://docs.rs/sysctl)

### Usage

Add to `Cargo.toml`

```toml
[dependencies]
sysctl = "*"
```

### macOS/iOS

* Due to limitations in the sysctl(3) API, many of the methods of
  the `Ctl` take a mutable reference to `self` on macOS/iOS.
* Sysctl descriptions are not available on macOS/iOS and Linux.
* Some tests failures are ignored, as the respective sysctls do not
  exist on macos.

### Example

sysctl comes with several examples, see the examples folder:

* `value.rs`: shows how to get a sysctl value
* `value_as.rs`: parsing values as structures
* `value_string.rs`: parsing values as string. Use this for cross platform compatibility since all sysctls are strings on Linux.
* `value_oid_as.rs`: getting a sysctl from OID constants from the `libc` crate.
* `set_value.rs`: shows how to set a sysctl value
* `struct.rs`: reading data into a struct
* `temperature.rs`: parsing temperatures
* `iterate.rs`: showcases iteration over the sysctl tree

Run with:

```sh
$ cargo run --example iterate
```

Or to use in your program:

```rust
extern crate sysctl;
use sysctl::Sysctl;

fn main() {
    let ctl = sysctl::Ctl::new("kern.osrevision").unwrap();
    println!("Description: {}", ctl.description().unwrap());
    println!("Value: {}", ctl.value_string().unwrap());
}
```

