#![allow(unused_imports)]
use std::future::Future;

#[maybe_async::maybe_async]
pub async fn with_fn<T, F: Sync + std::future::Future<Output = Result<(), ()>>>(
    test: T,
) -> Result<(), ()>
    where
        T: FnOnce() -> F,
{
    test().await
}

#[maybe_async::maybe_async]
pub async fn with_fn_where<T, F>(test: T) -> Result<(), ()>
    where
        T: FnOnce() -> F,
        F: Sync + Future<Output = Result<(), ()>>,
{
    test().await
}

#[maybe_async::sync_impl]
fn main() {
    with_fn(|| Ok(())).unwrap();
    with_fn_where(|| Ok(())).unwrap();
}

#[maybe_async::async_impl]
#[tokio::main]
async fn main() {
    with_fn(|| async { Ok(()) }).await.unwrap();
    with_fn_where(|| async { Ok(()) }).await.unwrap();
}
