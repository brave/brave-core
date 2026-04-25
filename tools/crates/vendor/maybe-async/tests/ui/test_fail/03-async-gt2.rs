use maybe_async::maybe_async;

#[maybe_async]
async fn async_fn() -> bool {
    true
}

#[maybe_async::test(feature="is_sync", async(feature="async", async_std::test, added))]
async fn test_async_fn() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

fn main() {

}
