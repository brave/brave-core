//! Target `cfg` attributes. Documented in the "Conditional compilation" section
//! of the Rust reference:
//!
//! <https://doc.rust-lang.org/reference/attributes.html#conditional-compilation>

mod arch;
mod endian;
mod env;
mod os;
mod pointerwidth;

pub use self::{arch::Arch, endian::Endian, env::Env, os::OS, pointerwidth::PointerWidth};
