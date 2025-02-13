use maybe_async::maybe_async;

#[maybe_async]
async fn async_fn() -> bool {
    true
}

#[maybe_async::test(
feature = "is_sync",
async(all(not(feature="is_sync"), feature = "async_std"), async_std::test),
async(all(not(feature="is_sync"), feature = "tokio"), tokio::test)
)]
async fn test_async_fn() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

#[maybe_async::test(feature = "is_sync", async(not(feature = "is_sync"), async_std::test))]
async fn test_async_fn2() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

#[maybe_async::test("feature=\"is_sync\"", async(not(feature = "is_sync"), async_std::test))]
async fn test_async_fn3() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

#[maybe_async::test(feature = "is_sync", async("not(feature = \"is_sync\")", "async_std::test"))]
async fn test_async_fn4() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

fn main() {

}
