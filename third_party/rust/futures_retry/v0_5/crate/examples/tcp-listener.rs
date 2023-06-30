use futures::TryStreamExt;
use futures_retry::{RetryPolicy, StreamRetryExt};
use std::time::Duration;
use tokio::io;
use tokio::net::{TcpListener, TcpStream};

async fn process_connection(mut socket: TcpStream) -> io::Result<()> {
    // Copy the data back to the client
    let conn = move || async move {
        let (mut reader, mut writer) = socket.split();
        match io::copy(&mut reader, &mut writer).await {
            Ok(n) => println!("Wrote {} bytes", n),
            Err(err) => println!("Can't copy data: IO error {:?}", err),
        }
    };

    // Spawn the future as a concurrent task
    tokio::spawn(conn());
    Ok(())
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let addr = "127.0.0.1:12345";
    let mut server = TcpListener::bind(addr).await.unwrap();
    println!("Listening at {}", server.local_addr().unwrap());

    server
        .incoming()
        .retry(|e: io::Error| match e.kind() {
            io::ErrorKind::Interrupted
            | io::ErrorKind::ConnectionRefused
            | io::ErrorKind::ConnectionReset
            | io::ErrorKind::ConnectionAborted
            | io::ErrorKind::NotConnected
            | io::ErrorKind::BrokenPipe => RetryPolicy::Repeat,
            io::ErrorKind::PermissionDenied => RetryPolicy::ForwardError(e),
            _ => RetryPolicy::WaitRetry(Duration::from_millis(5)),
        })
        .map_ok(|(x, _)| x)
        .map_err(|(x, _)| x)
        .try_for_each(process_connection)
        .await
}
