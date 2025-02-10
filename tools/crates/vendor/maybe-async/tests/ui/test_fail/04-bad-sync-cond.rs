use maybe_async::maybe_async;

#[maybe_async]
async fn async_fn() -> bool {
    true
}

// bad sync condition
#[maybe_async::test(unknown(feature="async", async_std::test))]
async fn test_async_fn() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

fn main() {

}