mod release_mode;
pub use release_mode::*;

// For automatic pointer-based generic array release
mod auto_elements;
pub use auto_elements::*;

// For automatic pointer-based primitive array release
mod auto_elements_critical;
pub use auto_elements_critical::*;
