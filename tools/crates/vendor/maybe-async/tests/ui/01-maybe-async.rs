#![allow(dead_code)]

use maybe_async::maybe_async;

#[maybe_async(Send)]
trait Trait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async(?Send)]
pub trait PubTrait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async]
pub(crate) trait PubCrateTrait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async(AFIT)]
trait AfitTrait {
    fn sync_fn_afit() {}

    async fn declare_async_afit(&self);

    async fn async_fn_afit(&self) {
        self.declare_async_afit().await
    }
}

#[maybe_async]
async fn async_fn() {}

#[maybe_async]
pub async fn pub_async_fn() {}

#[maybe_async]
pub(crate) async fn pub_crate_async_fn() {}

#[maybe_async]
unsafe fn unsafe_fn() {}

struct Struct;

#[maybe_async]
impl Struct {
    fn sync_fn_inherent() {}

    async fn declare_async_inherent(&self) {}

    async fn async_fn_inherent(&self) {
        async { self.declare_async_inherent().await }.await
    }
}

#[maybe_async]
impl Trait for Struct {
    fn sync_fn() {}

    async fn declare_async(&self) {}

    async fn async_fn(&self) {
        async { self.declare_async().await }.await
    }
}

#[maybe_async(AFIT)]
impl AfitTrait for Struct {
    fn sync_fn_afit() {}

    async fn declare_async_afit(&self) {}

    async fn async_fn_afit(&self) {
        async { self.declare_async_afit().await }.await
    }
}

#[cfg(feature = "is_sync")]
fn main() -> std::result::Result<(), ()> {
    let s = Struct;
    s.declare_async_inherent();
    s.async_fn_inherent();
    s.declare_async();
    s.async_fn();
    s.declare_async_afit();
    s.async_fn_afit();
    async_fn();
    pub_async_fn();
    Ok(())
}

#[cfg(not(feature = "is_sync"))]
#[async_std::main]
async fn main() {
    let s = Struct;
    s.declare_async_inherent().await;
    s.async_fn_inherent().await;
    s.declare_async().await;
    s.async_fn().await;
    s.declare_async_afit().await;
    s.async_fn_afit().await;
    async_fn().await;
    pub_async_fn().await;
}
