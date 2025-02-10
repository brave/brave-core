#![allow(dead_code, unused_variables)]

/// InnerClient differ a lot between sync and async.
#[maybe_async::maybe_async]
trait Trait {
    async fn maybe_async_fn();
}

/// The higher level API for end user.
pub struct Struct;

/// Synchronous  implementation, only compiles when `is_sync` feature is off.
/// Else the compiler will complain that *request is defined multiple times* and
/// blabla.
#[maybe_async::sync_impl]
impl Trait for Struct {
    fn maybe_async_fn() { }
}

/// Asynchronous implementation, only compiles when `is_sync` feature is off.
#[maybe_async::async_impl]
impl Trait for Struct {
    async fn maybe_async_fn() { }
}

impl Struct {
    #[maybe_async::maybe_async]
    async fn another_maybe_async_fn()  {
        Self::maybe_async_fn().await
        // When `is_sync` is toggle on, this block will compiles to:
        // Self::maybe_async_fn()
    }
}

#[maybe_async::sync_impl]
fn main() {
    let _ = Struct::another_maybe_async_fn();
}

#[maybe_async::async_impl]
#[tokio::main]
async fn main() {
    let _ = Struct::another_maybe_async_fn().await;
}
