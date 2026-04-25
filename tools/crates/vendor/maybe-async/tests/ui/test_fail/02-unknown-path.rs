use maybe_async::maybe_async;

#[maybe_async]
async fn async_fn() -> bool {
    true
}

// should only accept `async`
#[maybe_async::test(feature="is_sync", unknown(not(feature="is_sync"), async_std::test))]
async fn test_async_fn() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

fn main() {

}
