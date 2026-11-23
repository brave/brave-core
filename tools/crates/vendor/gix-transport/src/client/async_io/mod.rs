#![allow(clippy::double_must_use)] // `async_trait` generates `#[must_use]` on boxed futures.

mod bufread_ext;
pub use bufread_ext::{ExtendedBufRead, HandleProgress, ReadlineBufRead};

mod request;
pub use request::RequestWriter;

mod traits;
pub use traits::{SetServiceResponse, Transport, TransportV2Ext};

///
pub mod connect;
#[cfg(feature = "async-std")]
pub use connect::function::connect;
