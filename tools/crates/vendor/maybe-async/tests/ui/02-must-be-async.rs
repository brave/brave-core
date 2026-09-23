#![allow(dead_code)]

#[maybe_async::maybe_async]
trait Trait {
    fn sync_fn() {}

    async fn declare_async(&self);

    async fn async_fn(&self) {
        self.declare_async().await
    }
}

#[maybe_async::maybe_async(?Send)]
trait NotSendTrait {
    async fn declare_async_not_send(&self);

    async fn async_fn_not_send(&self) {
        self.declare_async_not_send().await
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

#[maybe_async::maybe_async(AFIT)]
trait AfitTrait {
    fn sync_fn_afit() {}

    async fn declare_async_afit(&self);

    async fn async_fn_afit(&self) {
        self.declare_async_afit().await
    }
}

#[cfg(not(feature = "is_sync"))]
#[maybe_async::must_be_async]
async fn async_fn() {}

#[cfg(not(feature = "is_sync"))]
#[maybe_async::must_be_async]
pub async fn pub_async_fn() {}

#[cfg(not(feature = "is_sync"))]
#[maybe_async::maybe_async]
pub(crate) async fn pub_crate_async_fn() {}

#[cfg(not(feature = "is_sync"))]
#[maybe_async::maybe_async]
unsafe fn unsafe_fn() {}

struct Struct;

#[cfg(not(feature = "is_sync"))]
#[maybe_async::must_be_async]
impl Struct {
    fn sync_fn_inherent() {}

    async fn declare_async_inherent(&self) {}

    async fn async_fn_inherent(&self) {
        async { self.declare_async_inherent().await }.await
    }
}

#[cfg(not(feature = "is_sync"))]
#[maybe_async::must_be_async]
impl Trait for Struct {
    fn sync_fn() {}

    async fn declare_async(&self) {}

    async fn async_fn(&self) {
        async { self.declare_async().await }.await
    }
}

#[cfg(not(feature = "is_sync"))]
#[maybe_async::must_be_async(?Send)]
impl NotSendTrait for Struct {
    async fn declare_async_not_send(&self) {}

    async fn async_fn_not_send(&self) {
        async { self.declare_async_not_send().await }.await
    }
}
#[cfg(not(feature = "is_sync"))]
#[maybe_async::must_be_async(AFIT)]
impl AfitTrait for Struct {
    fn sync_fn_afit() {}

    async fn declare_async_afit(&self) {}

    async fn async_fn_afit(&self) {
        async { self.declare_async_afit().await }.await
    }
}

#[cfg(feature = "is_sync")]
fn main() {}

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
