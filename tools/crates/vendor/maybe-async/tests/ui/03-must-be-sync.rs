#![allow(dead_code)]

#[maybe_async::maybe_async]
trait Trait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async::maybe_async]
pub trait PubTrait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async::maybe_async]
pub(crate) trait PubCrateTrait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async::maybe_async]
async fn async_fn() {}

#[maybe_async::maybe_async]
pub async fn pub_async_fn() {}

#[maybe_async::maybe_async]
pub(crate) async fn pub_crate_async_fn() {}

#[maybe_async::maybe_async]
unsafe fn unsafe_fn() {}

struct Struct;

#[cfg(feature = "is_sync")]
#[maybe_async::must_be_sync]
impl Struct {
    fn sync_fn_inherent() {}

    async fn declare_async_inherent(&self) {}

    async fn async_fn_inherent(&self) {
        async { self.declare_async_inherent().await }.await
    }
}

#[cfg(feature = "is_sync")]
#[maybe_async::must_be_sync]
impl Trait for Struct {
    fn sync_fn() {}

    async fn declare_async(&self) {}

    async fn async_fn(&self) {
        async { self.declare_async().await }.await
    }
}

#[cfg(feature = "is_sync")]
fn main() -> std::result::Result<(), ()> {
    let s = Struct;
    s.declare_async_inherent();
    s.async_fn_inherent();
    s.declare_async();
    s.async_fn();
    async_fn();
    pub_async_fn();
    Ok(())
}

#[cfg(not(feature = "is_sync"))]
#[async_std::main]
async fn main() {}
