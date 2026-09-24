use maybe_async::maybe_async;

#[maybe_async]
async fn async_fn() -> bool {
    true
}

// at least one sync condition should be specified
#[maybe_async::test()]
async fn test_async_fn() {
    let res = async_fn().await;
    assert_eq!(res, true);
}

fn main() {

}
