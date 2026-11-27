//! \[Experimental\] Deadlock detection
//!
//! This feature is optional and can be enabled with the `deadlock_detection`
//! Cargo feature.
//!
//! Reporting is destructive: detected threads are removed from their parking
//! queues, capture their backtraces, and then remain blocked permanently.
//! Waits with a deadline are not reported because they can resolve by timing
//! out.
//!
//! # Example
//!
//! ```
//! #[cfg(feature = "deadlock_detection")]
//! { // only for #[cfg]
//! use std::thread;
//! use std::time::Duration;
//! use parking_lot::deadlock;
//!
//! // Create a background thread which checks for deadlocks every 10s
//! thread::spawn(move || {
//!     loop {
//!         thread::sleep(Duration::from_secs(10));
//!         let deadlocks = deadlock::check_deadlock();
//!         if deadlocks.is_empty() {
//!             continue;
//!         }
//!
//!         println!("{} deadlocks detected", deadlocks.len());
//!         for (i, threads) in deadlocks.iter().enumerate() {
//!             println!("Deadlock #{}", i);
//!             for t in threads {
//!                 println!("Thread ID {:#?}", t.thread_id());
//!                 println!("{:#?}", t.backtrace());
//!             }
//!         }
//!     }
//! });
//! } // only for #[cfg]
//! ```

#[cfg(feature = "deadlock_detection")]
pub use parking_lot_core::deadlock::check_deadlock;

// Gate these on our deadlock_detection feature instead of the one in
// parking_lot_core.
//
// This is necessary to enforce the incompatibility of deadlock detection with
// the send_guard feature.
#[inline]
pub(crate) unsafe fn acquire_resource(key: usize) {
    if cfg!(feature = "deadlock_detection") {
        unsafe {
            parking_lot_core::deadlock::acquire_resource(key);
        }
    }
}
#[inline]
pub(crate) unsafe fn release_resource(key: usize) {
    if cfg!(feature = "deadlock_detection") {
        unsafe {
            parking_lot_core::deadlock::release_resource(key);
        }
    }
}

#[cfg(test)]
#[cfg(feature = "deadlock_detection")]
mod tests {
    use crate::{Mutex, Once, ReentrantMutex, RwLock};
    use std::sync::{Arc, Barrier, mpsc};
    use std::thread::{self, sleep};
    use std::time::Duration;

    // Serialize these tests with each other because deadlock detection scans
    // and mutates process-global parking queues.
    static DEADLOCK_DETECTION_LOCK: Mutex<()> = Mutex::new(());

    fn check_deadlock() -> bool {
        use parking_lot_core::deadlock::check_deadlock;
        !check_deadlock().is_empty()
    }

    #[track_caller]
    fn assert_deadlock(thread_count: usize) {
        let deadlocks = parking_lot_core::deadlock::check_deadlock();
        assert_eq!(deadlocks.len(), 1);
        assert_eq!(deadlocks[0].len(), thread_count);
    }

    #[test]
    fn test_mutex_deadlock() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let m1: Arc<Mutex<()>> = Default::default();
        let m2: Arc<Mutex<()>> = Default::default();
        let m3: Arc<Mutex<()>> = Default::default();
        let b = Arc::new(Barrier::new(4));

        let m1_ = m1.clone();
        let m2_ = m2.clone();
        let m3_ = m3.clone();
        let b1 = b.clone();
        let b2 = b.clone();
        let b3 = b.clone();

        assert!(!check_deadlock());

        let _t1 = thread::spawn(move || {
            let _g = m1.lock();
            b1.wait();
            let _blocked = m2_.lock();
        });

        let _t2 = thread::spawn(move || {
            let _g = m2.lock();
            b2.wait();
            let _blocked = m3_.lock();
        });

        let _t3 = thread::spawn(move || {
            let _g = m3.lock();
            b3.wait();
            let _blocked = m1_.lock();
        });

        assert!(!check_deadlock());

        b.wait();
        sleep(Duration::from_millis(50));
        assert_deadlock(3);

        assert!(!check_deadlock());
    }

    #[test]
    fn test_mutex_deadlock_reentrant() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let m1: Arc<Mutex<()>> = Default::default();

        assert!(!check_deadlock());

        let _t1 = thread::spawn(move || {
            let _g = m1.lock();
            let _blocked = m1.lock();
        });

        sleep(Duration::from_millis(50));
        assert_deadlock(1);

        assert!(!check_deadlock());
    }

    #[test]
    fn test_once_deadlock() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let once = Arc::new(Once::new());
        let mutex = Arc::new(Mutex::new(()));
        let barrier = Arc::new(Barrier::new(2));

        let once1 = Arc::clone(&once);
        let mutex1 = Arc::clone(&mutex);
        let barrier1 = Arc::clone(&barrier);
        let _t1 = thread::spawn(move || {
            once1.call_once(|| {
                barrier1.wait();
                let _blocked = mutex1.lock();
            });
        });

        let barrier2 = Arc::clone(&barrier);
        let _t2 = thread::spawn(move || {
            let _guard = mutex.lock();
            barrier2.wait();
            once.call_once(|| unreachable!());
        });

        sleep(Duration::from_millis(50));
        assert_deadlock(2);
        assert!(!check_deadlock());
    }

    #[test]
    fn test_remutex_deadlock() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let m1: Arc<ReentrantMutex<()>> = Default::default();
        let m2: Arc<ReentrantMutex<()>> = Default::default();
        let m3: Arc<ReentrantMutex<()>> = Default::default();
        let b = Arc::new(Barrier::new(4));

        let m1_ = m1.clone();
        let m2_ = m2.clone();
        let m3_ = m3.clone();
        let b1 = b.clone();
        let b2 = b.clone();
        let b3 = b.clone();

        assert!(!check_deadlock());

        let _t1 = thread::spawn(move || {
            let _g = m1.lock();
            let _g = m1.lock();
            b1.wait();
            let _ = m2_.lock();
        });

        let _t2 = thread::spawn(move || {
            let _g = m2.lock();
            let _g = m2.lock();
            b2.wait();
            let _ = m3_.lock();
        });

        let _t3 = thread::spawn(move || {
            let _g = m3.lock();
            let _g = m3.lock();
            b3.wait();
            let _ = m1_.lock();
        });

        assert!(!check_deadlock());

        b.wait();
        sleep(Duration::from_millis(50));
        assert_deadlock(3);

        assert!(!check_deadlock());
    }

    #[test]
    fn test_rwlock_deadlock() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let m1: Arc<RwLock<()>> = Default::default();
        let m2: Arc<RwLock<()>> = Default::default();
        let m3: Arc<RwLock<()>> = Default::default();
        let b = Arc::new(Barrier::new(4));

        let m1_ = m1.clone();
        let m2_ = m2.clone();
        let m3_ = m3.clone();
        let b1 = b.clone();
        let b2 = b.clone();
        let b3 = b.clone();

        assert!(!check_deadlock());

        let _t1 = thread::spawn(move || {
            let _g = m1.read();
            b1.wait();
            let _g = m2_.write();
        });

        let _t2 = thread::spawn(move || {
            let _g = m2.read();
            b2.wait();
            let _g = m3_.write();
        });

        let _t3 = thread::spawn(move || {
            let _g = m3.read();
            b3.wait();
            let _blocked = m1_.write();
        });

        assert!(!check_deadlock());

        b.wait();
        sleep(Duration::from_millis(50));
        assert_deadlock(3);

        assert!(!check_deadlock());
    }

    #[test]
    fn test_rwlock_deadlock_reentrant() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let m1: Arc<RwLock<()>> = Default::default();

        assert!(!check_deadlock());

        let _t1 = thread::spawn(move || {
            let _g = m1.read();
            let _ = m1.write();
        });

        sleep(Duration::from_millis(50));
        assert_deadlock(1);

        assert!(!check_deadlock());
    }

    #[test]
    fn test_rwlock_shared_reader_does_not_own_primary_key() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let rwlock: Arc<RwLock<()>> = Default::default();
        let mutex: Arc<Mutex<()>> = Default::default();
        let (upgradable_ready_tx, upgradable_ready_rx) = mpsc::channel();
        let (release_upgradable_tx, release_upgradable_rx) = mpsc::channel();
        let (reader_ready_tx, reader_ready_rx) = mpsc::channel();

        let rwlock1 = Arc::clone(&rwlock);
        let upgradable = thread::spawn(move || {
            let _guard = rwlock1.upgradable_read();
            upgradable_ready_tx.send(()).unwrap();
            release_upgradable_rx.recv().unwrap();
        });
        upgradable_ready_rx.recv().unwrap();

        let rwlock2 = Arc::clone(&rwlock);
        let mutex1 = Arc::clone(&mutex);
        let blocked_upgradable = thread::spawn(move || {
            let _guard = mutex1.lock();
            let _blocked = rwlock2.upgradable_read();
        });
        while !unsafe { rwlock.raw() }.has_parked_threads() {
            thread::yield_now();
        }

        let rwlock3 = Arc::clone(&rwlock);
        let mutex2 = Arc::clone(&mutex);
        let reader = thread::spawn(move || {
            let _guard = rwlock3.read();
            reader_ready_tx.send(()).unwrap();
            let _blocked = mutex2.lock();
        });
        reader_ready_rx.recv().unwrap();

        sleep(Duration::from_millis(50));
        assert!(!check_deadlock());

        release_upgradable_tx.send(()).unwrap();
        upgradable.join().unwrap();
        blocked_upgradable.join().unwrap();
        reader.join().unwrap();
    }

    #[test]
    fn test_rwlock_pending_writer_owns_primary_key() {
        let _guard = DEADLOCK_DETECTION_LOCK.lock();

        let rwlock: Arc<RwLock<()>> = Default::default();
        let mutex: Arc<Mutex<()>> = Default::default();
        let (reader_ready_tx, reader_ready_rx) = mpsc::channel();
        let (block_reader_tx, block_reader_rx) = mpsc::channel();
        let (mutex_ready_tx, mutex_ready_rx) = mpsc::channel();

        let rwlock1 = Arc::clone(&rwlock);
        let mutex1 = Arc::clone(&mutex);
        let _reader = thread::spawn(move || {
            let _guard = rwlock1.read();
            reader_ready_tx.send(()).unwrap();
            block_reader_rx.recv().unwrap();
            let _blocked = mutex1.lock();
        });
        reader_ready_rx.recv().unwrap();

        let rwlock2 = Arc::clone(&rwlock);
        let _writer = thread::spawn(move || {
            let _blocked = rwlock2.write();
        });
        while !unsafe { rwlock.raw() }.has_parked_threads() {
            thread::yield_now();
        }

        let rwlock3 = Arc::clone(&rwlock);
        let _blocked_reader = thread::spawn(move || {
            let _guard = mutex.lock();
            mutex_ready_tx.send(()).unwrap();
            let _blocked = rwlock3.read();
        });
        mutex_ready_rx.recv().unwrap();
        block_reader_tx.send(()).unwrap();

        sleep(Duration::from_millis(50));
        assert_deadlock(3);
        assert!(!check_deadlock());
    }
}
