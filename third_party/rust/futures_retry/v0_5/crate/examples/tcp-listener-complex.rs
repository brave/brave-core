use futures::TryStreamExt;
use futures_retry::{ErrorHandler, RetryPolicy, StreamRetryExt};
use std::time::Duration;
use tokio::io::{self};
use tokio::net::{TcpListener, TcpStream};

/// An I/O errors handler that counts consecutive error attempts.
struct IoHandler<D> {
    max_attempts: usize,
    current_attempt: usize,
    display_name: D,
}

impl<D> IoHandler<D> {
    fn new(max_attempts: usize, display_name: D) -> Self {
        IoHandler {
            max_attempts,
            current_attempt: 0,
            display_name,
        }
    }

    /// Calculates a duration to wait before a retry based on the current attempt number.
    ///
    /// I'm using the `atan` function to increase the duration from 5 to 1000 milliseconds (or
    /// actually close to 1000) rather fast, but never actually exceed the upper value.
    ///
    /// On the first attempt the duration will be 5 msec, on the second — 299 ms, then 503 ms,
    /// 628 ms, 706 ms and by the tenth attempt it will be about 861 ms.
    fn calculate_wait_duration(&self) -> Duration {
        const MIN_WAIT_MSEC: f32 = 5_f32;
        const MAX_WAIT_MSEC: f32 = 1000_f32;
        const FRAC_DIFF: f32 = 1_f32 / (MAX_WAIT_MSEC - MIN_WAIT_MSEC);
        let duration_msec = MIN_WAIT_MSEC
            + (self.current_attempt as f32 - 1_f32).atan()
                * ::std::f32::consts::FRAC_2_PI
                * FRAC_DIFF;
        Duration::from_millis(duration_msec.round() as u64)
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
                "[{}] All attempts ({}) have been used up",
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
            | io::ErrorKind::ConnectionRefused
            | io::ErrorKind::ConnectionReset
            | io::ErrorKind::ConnectionAborted
            | io::ErrorKind::NotConnected
            | io::ErrorKind::BrokenPipe => RetryPolicy::Repeat,
            io::ErrorKind::PermissionDenied => RetryPolicy::ForwardError(e),
            _ => RetryPolicy::WaitRetry(self.calculate_wait_duration()),
        }
    }
}

async fn process_connection((mut socket, _attempt): (TcpStream, usize)) -> io::Result<()> {
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
    let mut tcp = TcpListener::bind(addr).await.unwrap();

    tcp.incoming()
        .retry(IoHandler::new(3, "Accepting connections"))
        .map_err(|(x, _)| x)
        .try_for_each(process_connection)
        .await
}
