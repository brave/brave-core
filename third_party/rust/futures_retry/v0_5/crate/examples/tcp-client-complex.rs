use futures_retry::{ErrorHandler, FutureRetry, RetryPolicy};
use std::net::SocketAddr;
use std::time::Duration;
use tokio::io;
use tokio::net::TcpStream;
use tokio::prelude::*;

/// An I/O handler that counts attempts.
struct IoHandler<D> {
    max_attempts: usize,
    display_name: D,
}

impl<D> IoHandler<D> {
    fn new(max_attempts: usize, display_name: D) -> Self {
        IoHandler {
            max_attempts,
            display_name,
        }
    }
}

impl<D> ErrorHandler<io::Error> for IoHandler<D>
where
    D: ::std::fmt::Display,
{
    type OutError = io::Error;

    fn handle(&mut self, current_attempt: usize, e: io::Error) -> RetryPolicy<io::Error> {
        if current_attempt > self.max_attempts {
            eprintln!(
                "[{}] All attempts ({}) have been used",
                self.display_name, self.max_attempts
            );
            return RetryPolicy::ForwardError(e);
        }
        eprintln!(
            "[{}] Attempt {}/{} has failed",
            self.display_name, current_attempt, self.max_attempts
        );
        match e.kind() {
            io::ErrorKind::Interrupted
            | io::ErrorKind::ConnectionReset
            | io::ErrorKind::ConnectionAborted
            | io::ErrorKind::NotConnected
            | io::ErrorKind::BrokenPipe => RetryPolicy::Repeat,
            io::ErrorKind::ConnectionRefused => RetryPolicy::WaitRetry(Duration::from_secs(5)),
            _ => RetryPolicy::ForwardError(e),
        }
    }
}

/// In this function we try to establish a connection to a given address for 3 times, and then try
/// to send some data exactly once.
async fn connect_and_send(addr: SocketAddr) -> io::Result<()> {
    // Try to connect until we succeed or until an unrecoverable error is encountered.
    let connection = FutureRetry::new(
        move || {
            println!("Trying to connect to {}", addr);
            TcpStream::connect(addr)
        },
        IoHandler::new(3, "Establishing a connection"),
    );
    let (mut socket, _) = connection.await.map_err(|(e, _attempt)| e)?;
    let (_, mut writer) = socket.split();
    writer.write_all(b"Yo!").await
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let addr = "127.0.0.1:12345".parse().unwrap();
    // Try to connect and send data 2 times.
    FutureRetry::new(
        move || {
            println!("Trying to execute a client");
            connect_and_send(addr)
        },
        IoHandler::new(2, "Running a client"),
    )
    .await
    .map_err(|(e, _attempt)| {
        eprintln!("Connect and send has failed: {}", e);
        e
    })?;
    println!("Done");
    // To check out that attempts logic works as expected, launch a listener within 30-seconds time
    // period after launching the example, e.g. on linux:
    //
    // % nc -l 127.0.0.1 12345
    Ok(())
}
