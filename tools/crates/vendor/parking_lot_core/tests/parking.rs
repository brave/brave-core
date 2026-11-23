//! Small parking regressions suitable for WASI as well as native targets.
//! On threadless WASI, run only `single_thread` with `--test-threads=1`.

use parking_lot_core::{FilterOp, ParkResult, ParkToken, UnparkToken};
use std::sync::mpsc;
use std::thread;
use std::time::{Duration, Instant};

const TOKEN: UnparkToken = UnparkToken(42);

#[test]
fn single_thread_timeout() {
    let key = 0u8;
    let key = (&key as *const u8).addr();
    let deadline = Instant::now() + Duration::from_millis(20);
    let mut timed_out = None;
    // SAFETY: This test owns the key, and the callbacks do not panic or park.
    let result = unsafe {
        parking_lot_core::park(
            key,
            || true,
            || {},
            |key, last| timed_out = Some((key, last)),
            ParkToken(0),
            Some(deadline),
        )
    };
    assert_eq!(result, ParkResult::TimedOut);
    assert!(Instant::now() >= deadline);
    assert_eq!(timed_out, Some((key, true)));
    assert_eq!(unsafe { parking_lot_core::unpark_all(key, TOKEN) }, 0);
}

#[test]
fn single_thread_unpark_before_sleep() {
    let key = 0u8;
    let key = (&key as *const u8).addr();
    // Exercise both park and park_until after a wake committed before sleeping.
    for timeout in [None, Some(Instant::now() + Duration::from_secs(5))] {
        let mut unparked = 0;
        // SAFETY: This test owns the key. before_sleep may unpark this thread;
        // all callbacks return normally and never call park.
        let result = unsafe {
            parking_lot_core::park(
                key,
                || true,
                || {
                    unparked = parking_lot_core::unpark_one(key, |_| TOKEN).unparked_threads;
                },
                |_, _| {},
                ParkToken(0),
                timeout,
            )
        };
        assert_eq!(unparked, 1);
        assert_eq!(result, ParkResult::Unparked(TOKEN));
    }
}

fn wake_threads(wake: impl FnOnce(usize)) {
    let key = 0u8;
    let key = (&key as *const u8).addr();
    let (ready, receiver) = mpsc::channel();
    thread::scope(|scope| {
        let mut workers = Vec::new();
        for index in 0..2 {
            let ready = ready.clone();
            workers.push(scope.spawn(move || {
                // SAFETY: The key remains owned by this test until all workers
                // join. Callbacks cannot panic, and the deadline bounds failure.
                unsafe {
                    parking_lot_core::park(
                        key,
                        || true,
                        || {
                            let _ = ready.send(());
                        },
                        |_, _| {},
                        ParkToken(index),
                        Some(Instant::now() + Duration::from_secs(5)),
                    )
                }
            }));
        }
        for _ in 0..2 {
            receiver.recv_timeout(Duration::from_secs(5)).unwrap();
        }
        // Give both workers an opportunity to enter the platform timed wait.
        thread::sleep(Duration::from_millis(20));
        wake(key);
        for worker in workers {
            assert_eq!(worker.join().unwrap(), ParkResult::Unparked(TOKEN));
        }
    });
}

#[test]
fn wake_one_then_all() {
    wake_threads(|key| unsafe {
        assert_eq!(
            parking_lot_core::unpark_one(key, |_| TOKEN).unparked_threads,
            1
        );
        assert_eq!(parking_lot_core::unpark_all(key, TOKEN), 1);
    });
}

#[test]
fn wake_filter_then_all() {
    wake_threads(|key| unsafe {
        let result = parking_lot_core::unpark_filter(
            key,
            |token| {
                if token == ParkToken(0) {
                    FilterOp::Unpark
                } else {
                    FilterOp::Skip
                }
            },
            |_| TOKEN,
        );
        assert_eq!(result.unparked_threads, 1);
        assert_eq!(parking_lot_core::unpark_all(key, TOKEN), 1);
    });
}
