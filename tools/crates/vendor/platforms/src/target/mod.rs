//! Target `cfg` attributes. Documented in the "Conditional compilation" section
//! of the Rust reference:
//!
//! <https://doc.rust-lang.org/reference/attributes.html#conditional-compilation>

mod arch;
pub use arch::Arch;

mod endian;
pub use endian::Endian;

mod env;
pub use env::Env;

mod os;
pub use os::Os;

mod pointer_width;
pub use pointer_width::PointerWidth;
