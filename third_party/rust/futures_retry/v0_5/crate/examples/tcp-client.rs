use futures_retry::{FutureRetry, RetryPolicy};
use std::io;
use std::time::Duration;
use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;

fn handle_connection_error(e: io::Error) -> RetryPolicy<io::Error> {
    // This is kinda unrealistical error handling, don't use it as it is!
    match e.kind() {
        io::ErrorKind::Interrupted
        | io::ErrorKind::ConnectionRefused
        | io::ErrorKind::ConnectionReset
        | io::ErrorKind::ConnectionAborted
        | io::ErrorKind::NotConnected
        | io::ErrorKind::BrokenPipe => RetryPolicy::Repeat,
        io::ErrorKind::PermissionDenied => RetryPolicy::ForwardError(e),
        _ => RetryPolicy::WaitRetry(Duration::from_millis(5)),
    }
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let addr = "127.0.0.1:12345";
    // Try to connect until we succeed or until an unrecoverable error is encountered.
    let (mut socket, _attempt) =
        FutureRetry::new(move || TcpStream::connect(addr), handle_connection_error)
            .await
            .map_err(|(e, _attempt)| e)?;
    // .. and then try to write some data only once. If you want to retry on an error here as
    // well, wrap up the whole `let socket = ...` in a `FutureRetry`.
    let (_, mut writer) = socket.split();
    writer.write_all(b"Yo!").await?;
    Ok(())
}
