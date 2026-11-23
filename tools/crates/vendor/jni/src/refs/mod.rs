mod cast;
pub use cast::*;

mod reference;
pub use reference::*;

mod loader_context;
pub use loader_context::*;

// For storing a reference to a java object
mod global;
pub use global::*;

mod weak;
pub use weak::*;

// For automatic local ref deletion
mod auto;
pub use auto::*;
