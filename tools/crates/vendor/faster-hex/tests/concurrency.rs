//! A separate test binary ensures dispatch has not run before the barrier.
#![cfg(not(miri))]

use faster_hex::{hex_check, hex_decode, hex_encode};
use std::sync::{Arc, Barrier};

#[test]
fn concurrent_first_calls_and_reused_dispatch_preserve_results() {
    let barrier = Arc::new(Barrier::new(12));
    let workers: Vec<_> = (0..12)
        .map(|worker| {
            let barrier = Arc::clone(&barrier);
            std::thread::spawn(move || {
                let bytes: Vec<_> = (0..129).map(|i| (i * 73 + worker) as u8).collect();
                let expected = hex::encode(&bytes);
                let mut encoded = vec![0xa5; bytes.len() * 2 + 7];
                let mut decoded = vec![0xa5; bytes.len() + 7];
                barrier.wait();
                // Different entry points contend for the same initial CPU probe.
                for round in 0..64 {
                    match (worker + round) % 3 {
                        0 => assert_eq!(&*hex_encode(&bytes, &mut encoded).unwrap(), expected),
                        1 => assert_eq!(
                            hex_decode(expected.as_bytes(), &mut decoded).unwrap(),
                            bytes
                        ),
                        _ => assert!(hex_check(expected.as_bytes())),
                    }
                    assert_eq!(&encoded[bytes.len() * 2..], &[0xa5; 7]);
                    assert_eq!(&decoded[bytes.len()..], &[0xa5; 7]);
                }
            })
        })
        .collect();
    for worker in workers {
        worker.join().unwrap();
    }
}
