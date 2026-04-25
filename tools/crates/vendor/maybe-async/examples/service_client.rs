#![allow(dead_code, unused_variables)]
/// To use `maybe-async`, we must know which block of codes is only used on
/// blocking implementation, and which on async. These two implementation should
/// share the same API except for async/await keywords, and use `sync_impl` and
/// `async_impl` to mark these implementation.
type Response = String;
type Url = &'static str;
type Method = String;

/// InnerClient are used to actually send request,
/// which differ a lot between sync and async.
///
/// Use native async function in trait
#[maybe_async::maybe_async(AFIT)]
trait InnerClient {
    async fn request(method: Method, url: Url, data: String) -> Response;
    #[inline]
    async fn post(url: Url, data: String) -> Response {
        Self::request(String::from("post"), url, data).await
    }
    #[inline]
    async fn delete(url: Url, data: String) -> Response {
        Self::request(String::from("delete"), url, data).await
    }
}

/// The higher level API for end user.
pub struct ServiceClient;

/// Synchronous  implementation, only compiles when `is_sync` feature is off.
/// Else the compiler will complain that *request is defined multiple times* and
/// blabla.
#[maybe_async::sync_impl]
impl InnerClient for ServiceClient {
    fn request(method: Method, url: Url, data: String) -> Response {
        // your implementation for sync, like use
        // `reqwest::blocking` to send request
        String::from("pretend we have a response")
    }
}

/// Asynchronous implementation, only compiles when `is_sync` feature is off.
#[maybe_async::async_impl(AFIT)]
impl InnerClient for ServiceClient {
    async fn request(method: Method, url: Url, data: String) -> Response {
        // your implementation for async, like use `reqwest::client`
        // or `async_std` to send request
        String::from("pretend we have a response")
    }
}

/// Code of upstream API are almost the same for sync and async,
/// except for async/await keyword.
impl ServiceClient {
    #[maybe_async::maybe_async]
    async fn create_bucket(name: String) -> Response {
        Self::post("http://correct_url4create", String::from("my_bucket")).await
        // When `is_sync` is toggle on, this block will compiles to:
        // Self::post("http://correct_url4create", String::from("my_bucket"))
    }
    #[maybe_async::maybe_async]
    async fn delete_bucket(name: String) -> Response {
        Self::delete("http://correct_url4delete", String::from("my_bucket")).await
    }
    // and another thousands of functions that interact with service side
}

#[maybe_async::sync_impl]
fn main() {
    let _ = ServiceClient::create_bucket("bucket".to_owned());
}

#[maybe_async::async_impl]
#[tokio::main]
async fn main() {
    let _ = ServiceClient::create_bucket("bucket".to_owned()).await;
}
