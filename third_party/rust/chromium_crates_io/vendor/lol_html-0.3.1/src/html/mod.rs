#[macro_use]
mod tag;

mod local_name;
mod namespace;
mod text_type;

pub use self::local_name::{LocalName, LocalNameHash};
pub use self::namespace::Namespace;
pub use self::tag::*;
pub use self::text_type::TextType;
