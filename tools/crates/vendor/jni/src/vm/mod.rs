#[cfg(feature = "invocation")]
mod init_args;
#[cfg(feature = "invocation")]
pub use self::init_args::*;

mod java_vm;
pub use self::java_vm::*;

#[cfg(use_fls_attach_guard)]
mod fls_attach_guard;
#[cfg(not(use_fls_attach_guard))]
mod tls_attach_guard;
