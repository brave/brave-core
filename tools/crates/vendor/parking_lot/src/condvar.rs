use crate::raw_condvar::RawCondvar;

pub use lock_api::WaitTimeoutResult;

/// A condition variable.
///
/// Condition variables represent the ability to block a thread such that it
/// consumes no CPU time while waiting for an event to occur. Condition
/// variables are typically associated with a boolean predicate (a condition)
/// and a mutex. The predicate is always verified inside of the mutex before
/// determining that thread must block.
///
/// Note that this module places one additional restriction over the system
/// condition variables: each condvar can be used with only one mutex at a
/// time. Any attempt to use multiple mutexes on the same condition variable
/// simultaneously will result in a runtime panic. However it is possible to
/// switch to a different mutex if there are no threads currently waiting on
/// the condition variable.
///
/// # Differences from the standard library `Condvar`
///
/// - No spurious wakeups: A wait will only return a non-timeout result if it
///   was woken up by `notify_one` or `notify_all`.
/// - `notify_one` returns exactly whether a waiting thread was notified, and
///   `notify_all` returns the exact number of waiting threads notified.
/// - `Condvar::notify_all` requeues waiters directly onto the associated
///   `Mutex` and wakes them as the lock becomes available, avoiding a
///   thundering herd.
///
/// # Examples
///
/// ```
/// use parking_lot::{Mutex, Condvar};
/// use std::sync::Arc;
/// use std::thread;
///
/// let pair = Arc::new((Mutex::new(false), Condvar::new()));
/// let pair2 = pair.clone();
///
/// // Inside of our lock, spawn a new thread, and then wait for it to start
/// thread::spawn(move|| {
///     let &(ref lock, ref cvar) = &*pair2;
///     let mut started = lock.lock();
///     *started = true;
///     cvar.notify_one();
/// });
///
/// // wait for the thread to start up
/// let &(ref lock, ref cvar) = &*pair;
/// let mut started = lock.lock();
/// if !*started {
///     cvar.wait(&mut started);
/// }
/// // In this single-waiter example an `if` is sufficient because
/// // parking_lot's Condvar never spuriously wakes: wait() only returns after
/// // notify_one or notify_all is called. Code with multiple waiters or multiple
/// // predicates may still need a loop to re-check the predicate.
/// ```
pub type Condvar = lock_api::Condvar<RawCondvar>;

#[cfg(test)]
mod tests {
    use crate::{Condvar, Mutex, MutexGuard};
    use std::sync::Arc;
    use std::sync::mpsc::channel;
    use std::thread;
    use std::thread::JoinHandle;
    use std::thread::sleep;
    use std::time::Duration;
    use std::time::Instant;

    #[test]
    fn smoke() {
        let c = Condvar::new();
        c.notify_one();
        c.notify_all();
    }

    #[test]
    fn notify_one() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();

        let mut g = m.lock();
        let _t = thread::spawn(move || {
            let _g = m2.lock();
            c2.notify_one();
        });
        c.wait(&mut g);
    }

    #[test]
    fn notify_all() {
        const N: usize = 10;

        let data = Arc::new((Mutex::new(0), Condvar::new()));
        let (tx, rx) = channel();
        for _ in 0..N {
            let data = data.clone();
            let tx = tx.clone();
            thread::spawn(move || {
                let (lock, cond) = &*data;
                let mut cnt = lock.lock();
                *cnt += 1;
                if *cnt == N {
                    tx.send(()).unwrap();
                }
                while *cnt != 0 {
                    cond.wait(&mut cnt);
                }
                tx.send(()).unwrap();
            });
        }
        drop(tx);

        let (lock, cond) = &*data;
        rx.recv().unwrap();
        let mut cnt = lock.lock();
        *cnt = 0;
        cond.notify_all();
        drop(cnt);

        for _ in 0..N {
            rx.recv().unwrap();
        }
    }

    #[test]
    fn notify_one_return_true() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();

        let mut g = m.lock();
        let t = thread::spawn(move || {
            let _g = m2.lock();
            assert!(c2.notify_one());
        });
        c.wait(&mut g);
        t.join().unwrap();
    }

    #[test]
    fn notify_one_return_false() {
        assert!(!Condvar::new().notify_one());
    }

    #[test]
    fn notify_all_return() {
        const N: usize = 10;

        let data = Arc::new((Mutex::new(0), Condvar::new()));
        let (tx, rx) = channel();
        for _ in 0..N {
            let data = data.clone();
            let tx = tx.clone();
            thread::spawn(move || {
                let (lock, cond) = &*data;
                let mut cnt = lock.lock();
                *cnt += 1;
                if *cnt == N {
                    tx.send(()).unwrap();
                }
                while *cnt != 0 {
                    cond.wait(&mut cnt);
                }
                tx.send(()).unwrap();
            });
        }
        drop(tx);

        let (lock, cond) = &*data;
        rx.recv().unwrap();
        let mut cnt = lock.lock();
        *cnt = 0;
        assert_eq!(cond.notify_all(), N);
        drop(cnt);

        for _ in 0..N {
            rx.recv().unwrap();
        }

        assert_eq!(cond.notify_all(), 0);
    }

    #[test]
    fn wait_for() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();

        let mut g = m.lock();
        let result = c.wait_for(&mut g, Duration::from_millis(1));
        assert!(result.timed_out());

        let t = thread::spawn(move || {
            let _g = m2.lock();
            c2.notify_one();
        });
        let result = c.wait_for(&mut g, Duration::from_millis(u32::MAX as u64));
        assert!(!result.timed_out());

        drop(g);
        t.join().unwrap();
    }

    #[test]
    fn wait_until() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();

        let mut g = m.lock();
        let result = c.wait_until(&mut g, Instant::now() + Duration::from_millis(1));
        assert!(result.timed_out());
        let t = thread::spawn(move || {
            let _g = m2.lock();
            c2.notify_one();
        });
        let result = c.wait_until(
            &mut g,
            Instant::now() + Duration::from_millis(u32::MAX as u64),
        );
        assert!(!result.timed_out());
        drop(g);
        t.join().unwrap();
    }

    fn spawn_wait_while_notifier(
        mutex: Arc<Mutex<u32>>,
        cv: Arc<Condvar>,
        num_iters: u32,
        timeout: Option<Instant>,
    ) -> JoinHandle<()> {
        thread::spawn(move || {
            for epoch in 1..=num_iters {
                // spin to wait for main test thread to block
                // before notifying it to wake back up and check
                // its condition.
                let mut sleep_backoff = Duration::from_millis(1);
                let _mutex_guard = loop {
                    let mutex_guard = mutex.lock();

                    if let Some(timeout) = timeout
                        && Instant::now() >= timeout
                    {
                        return;
                    }

                    if *mutex_guard == epoch {
                        break mutex_guard;
                    }

                    drop(mutex_guard);

                    // give main test thread a good chance to
                    // acquire the lock before this thread does.
                    sleep(sleep_backoff);
                    sleep_backoff *= 2;
                };

                cv.notify_one();
            }
        })
    }

    #[test]
    fn wait_while_does_not_wait_if_initially_false() {
        let mutex = Arc::new(Mutex::new(0));
        let cv = Arc::new(Condvar::new());

        let condition = |counter: &mut u32| {
            *counter += 1;
            false
        };

        let mut mutex_guard = mutex.lock();
        cv.wait_while(&mut mutex_guard, condition);
        assert!(*mutex_guard == 1);
    }

    #[test]
    fn wait_while_until_times_out_before_false() {
        let mutex = Arc::new(Mutex::new(0));
        let cv = Arc::new(Condvar::new());

        let num_iters = 3;
        let condition = |counter: &mut u32| {
            *counter += 1;
            true
        };

        let mut mutex_guard = mutex.lock();
        let timeout = Instant::now() + Duration::from_millis(500);
        let handle = spawn_wait_while_notifier(mutex.clone(), cv.clone(), num_iters, Some(timeout));

        let timeout_result = cv.wait_while_until(&mut mutex_guard, condition, timeout);

        assert!(timeout_result.timed_out());
        assert!(*mutex_guard == num_iters + 2);

        // prevent deadlock with notifier
        drop(mutex_guard);
        handle.join().unwrap();
    }

    #[test]
    fn wait_while_until_rechecks_condition_after_timeout() {
        let mutex = Mutex::new(());
        let cv = Condvar::new();
        let mut calls = 0;
        let condition = |_: &mut ()| {
            calls += 1;
            calls == 1
        };

        let mut mutex_guard = mutex.lock();
        let timeout_result = cv.wait_while_until(&mut mutex_guard, condition, Instant::now());

        assert!(!timeout_result.timed_out());
        assert_eq!(calls, 2);
    }

    #[test]
    fn wait_while() {
        let mutex = Arc::new(Mutex::new(0));
        let cv = Arc::new(Condvar::new());

        let num_iters = 4;

        let condition = |counter: &mut u32| {
            *counter += 1;
            *counter <= num_iters
        };

        let mut mutex_guard = mutex.lock();
        let handle = spawn_wait_while_notifier(mutex.clone(), cv.clone(), num_iters, None);

        cv.wait_while(&mut mutex_guard, condition);
        assert!(*mutex_guard == num_iters + 1);

        cv.wait_while(&mut mutex_guard, condition);
        handle.join().unwrap();

        assert!(*mutex_guard == num_iters + 2);
    }

    #[test]
    #[should_panic(expected = "attempted to use a condition variable with more than one mutex")]
    fn two_mutexes() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let m3 = Arc::new(Mutex::new(()));
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();

        // Make sure we don't leave the child thread dangling
        struct PanicGuard<'a>(&'a Condvar);
        impl<'a> Drop for PanicGuard<'a> {
            fn drop(&mut self) {
                self.0.notify_one();
            }
        }

        let (tx, rx) = channel();
        let g = m.lock();
        let _t = thread::spawn(move || {
            let mut g = m2.lock();
            tx.send(()).unwrap();
            c2.wait(&mut g);
        });
        drop(g);
        rx.recv().unwrap();
        let _g = m.lock();
        let _guard = PanicGuard(&c);
        c.wait(&mut m3.lock());
    }

    #[test]
    fn two_mutexes_disjoint() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let m3 = Arc::new(Mutex::new(()));
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();

        let mut g = m.lock();
        let _t = thread::spawn(move || {
            let _g = m2.lock();
            c2.notify_one();
        });
        c.wait(&mut g);
        drop(g);

        let _ = c.wait_for(&mut m3.lock(), Duration::from_millis(1));
    }

    #[test]
    fn test_debug_condvar() {
        let c = Condvar::new();
        assert_eq!(format!("{:?}", c), "Condvar { .. }");
    }

    #[test]
    fn test_condvar_requeue() {
        let m = Arc::new(Mutex::new(()));
        let m2 = m.clone();
        let c = Arc::new(Condvar::new());
        let c2 = c.clone();
        let t = thread::spawn(move || {
            let mut g = m2.lock();
            c2.wait(&mut g);
        });

        let mut g = m.lock();
        while !c.notify_one() {
            // Wait for the thread to get into wait()
            MutexGuard::bump(&mut g);
            // Yield, so the other thread gets a chance to do something.
            thread::yield_now();
        }
        // The thread should have been requeued to the mutex, which we wake up now.
        drop(g);
        t.join().unwrap();
    }

    #[test]
    fn test_issue_129() {
        let locks = Arc::new((Mutex::new(0), Condvar::new()));

        let (tx, rx) = channel();
        for _ in 0..4 {
            let locks = locks.clone();
            let tx = tx.clone();
            thread::spawn(move || {
                let mut guard = locks.0.lock();
                *guard += 1;
                locks.1.wait(&mut guard);
                locks.1.wait_for(&mut guard, Duration::from_millis(1));
                locks.1.notify_one();
                tx.send(()).unwrap();
            });
        }

        while *locks.0.lock() != 4 {
            thread::sleep(Duration::from_millis(100));
        }
        locks.1.notify_one();

        for _ in 0..4 {
            assert_eq!(rx.recv_timeout(Duration::from_millis(500)), Ok(()));
        }
    }
}

/// This module contains an integration test heavily inspired by WebKit's own
/// condition-variable tests.
#[cfg(test)]
#[cfg(not(miri))] // Miri is too slow
mod webkit_queue_test {
    use crate::{Condvar, Mutex, MutexGuard};
    use std::{collections::VecDeque, sync::Arc, thread, time::Duration};

    #[derive(Clone, Copy)]
    enum Timeout {
        Bounded(Duration),
        Forever,
    }

    #[derive(Clone, Copy)]
    enum NotifyStyle {
        One,
        All,
    }

    struct Queue {
        items: VecDeque<usize>,
        should_continue: bool,
    }

    impl Queue {
        fn new() -> Self {
            Self {
                items: VecDeque::new(),
                should_continue: true,
            }
        }
    }

    fn wait<T: ?Sized>(
        condition: &Condvar,
        lock: &mut MutexGuard<'_, T>,
        predicate: impl Fn(&mut MutexGuard<'_, T>) -> bool,
        timeout: &Timeout,
    ) {
        while !predicate(lock) {
            match timeout {
                Timeout::Forever => condition.wait(lock),
                Timeout::Bounded(bound) => {
                    condition.wait_for(lock, *bound);
                }
            }
        }
    }

    fn notify(style: NotifyStyle, condition: &Condvar, should_notify: bool) {
        match style {
            NotifyStyle::One => {
                condition.notify_one();
            }
            NotifyStyle::All => {
                if should_notify {
                    condition.notify_all();
                }
            }
        }
    }

    fn run_queue_test(
        num_producers: usize,
        num_consumers: usize,
        max_queue_size: usize,
        messages_per_producer: usize,
        notify_style: NotifyStyle,
        timeout: Timeout,
        delay: Duration,
    ) {
        let input_queue = Arc::new(Mutex::new(Queue::new()));
        let empty_condition = Arc::new(Condvar::new());
        let full_condition = Arc::new(Condvar::new());

        let output_vec = Arc::new(Mutex::new(vec![]));

        let consumers = (0..num_consumers)
            .map(|_| {
                consumer_thread(
                    input_queue.clone(),
                    empty_condition.clone(),
                    full_condition.clone(),
                    timeout,
                    notify_style,
                    output_vec.clone(),
                    max_queue_size,
                )
            })
            .collect::<Vec<_>>();
        let producers = (0..num_producers)
            .map(|_| {
                producer_thread(
                    messages_per_producer,
                    input_queue.clone(),
                    empty_condition.clone(),
                    full_condition.clone(),
                    timeout,
                    notify_style,
                    max_queue_size,
                )
            })
            .collect::<Vec<_>>();

        thread::sleep(delay);

        for producer in producers.into_iter() {
            producer.join().expect("Producer thread panicked");
        }

        {
            let mut input_queue = input_queue.lock();
            input_queue.should_continue = false;
        }
        empty_condition.notify_all();

        for consumer in consumers.into_iter() {
            consumer.join().expect("Consumer thread panicked");
        }

        let mut output_vec = output_vec.lock();
        assert_eq!(output_vec.len(), num_producers * messages_per_producer);
        output_vec.sort();
        for msg_idx in 0..messages_per_producer {
            for producer_idx in 0..num_producers {
                assert_eq!(msg_idx, output_vec[msg_idx * num_producers + producer_idx]);
            }
        }
    }

    fn consumer_thread(
        input_queue: Arc<Mutex<Queue>>,
        empty_condition: Arc<Condvar>,
        full_condition: Arc<Condvar>,
        timeout: Timeout,
        notify_style: NotifyStyle,
        output_queue: Arc<Mutex<Vec<usize>>>,
        max_queue_size: usize,
    ) -> thread::JoinHandle<()> {
        thread::spawn(move || {
            loop {
                let (should_notify, result) = {
                    let mut queue = input_queue.lock();
                    wait(
                        &empty_condition,
                        &mut queue,
                        |state| -> bool { !state.items.is_empty() || !state.should_continue },
                        &timeout,
                    );
                    if queue.items.is_empty() && !queue.should_continue {
                        return;
                    }
                    let should_notify = queue.items.len() == max_queue_size;
                    let result = queue.items.pop_front();
                    std::mem::drop(queue);
                    (should_notify, result)
                };
                notify(notify_style, &full_condition, should_notify);

                if let Some(result) = result {
                    output_queue.lock().push(result);
                }
            }
        })
    }

    fn producer_thread(
        num_messages: usize,
        queue: Arc<Mutex<Queue>>,
        empty_condition: Arc<Condvar>,
        full_condition: Arc<Condvar>,
        timeout: Timeout,
        notify_style: NotifyStyle,
        max_queue_size: usize,
    ) -> thread::JoinHandle<()> {
        thread::spawn(move || {
            for message in 0..num_messages {
                let should_notify = {
                    let mut queue = queue.lock();
                    wait(
                        &full_condition,
                        &mut queue,
                        |state| state.items.len() < max_queue_size,
                        &timeout,
                    );
                    let should_notify = queue.items.is_empty();
                    queue.items.push_back(message);
                    std::mem::drop(queue);
                    should_notify
                };
                notify(notify_style, &empty_condition, should_notify);
            }
        })
    }

    macro_rules! run_queue_tests {
        ( $( $name:ident(
            num_producers: $num_producers:expr,
            num_consumers: $num_consumers:expr,
            max_queue_size: $max_queue_size:expr,
            messages_per_producer: $messages_per_producer:expr,
            notification_style: $notification_style:expr,
            timeout: $timeout:expr,
            delay_seconds: $delay_seconds:expr);
        )* ) => {
            $(#[test]
            fn $name() {
                let delay = Duration::from_secs($delay_seconds);
                run_queue_test(
                    $num_producers,
                    $num_consumers,
                    $max_queue_size,
                    $messages_per_producer,
                    $notification_style,
                    $timeout,
                    delay,
                    );
            })*
        };
    }

    run_queue_tests! {
        sanity_check_queue(
            num_producers: 1,
            num_consumers: 1,
            max_queue_size: 1,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Bounded(Duration::from_secs(1)),
            delay_seconds: 0
        );
        sanity_check_queue_timeout(
            num_producers: 1,
            num_consumers: 1,
            max_queue_size: 1,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        new_test_without_timeout_5(
            num_producers: 1,
            num_consumers: 5,
            max_queue_size: 1,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        one_producer_one_consumer_one_slot(
            num_producers: 1,
            num_consumers: 1,
            max_queue_size: 1,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        one_producer_one_consumer_one_slot_timeout(
            num_producers: 1,
            num_consumers: 1,
            max_queue_size: 1,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 1
        );
        one_producer_one_consumer_hundred_slots(
            num_producers: 1,
            num_consumers: 1,
            max_queue_size: 100,
            messages_per_producer: 1_000_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        ten_producers_one_consumer_one_slot(
            num_producers: 10,
            num_consumers: 1,
            max_queue_size: 1,
            messages_per_producer: 10000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        ten_producers_one_consumer_hundred_slots_notify_all(
            num_producers: 10,
            num_consumers: 1,
            max_queue_size: 100,
            messages_per_producer: 10000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        ten_producers_one_consumer_hundred_slots_notify_one(
            num_producers: 10,
            num_consumers: 1,
            max_queue_size: 100,
            messages_per_producer: 10000,
            notification_style: NotifyStyle::One,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        one_producer_ten_consumers_one_slot(
            num_producers: 1,
            num_consumers: 10,
            max_queue_size: 1,
            messages_per_producer: 10000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        one_producer_ten_consumers_hundred_slots_notify_all(
            num_producers: 1,
            num_consumers: 10,
            max_queue_size: 100,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        one_producer_ten_consumers_hundred_slots_notify_one(
            num_producers: 1,
            num_consumers: 10,
            max_queue_size: 100,
            messages_per_producer: 100_000,
            notification_style: NotifyStyle::One,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        ten_producers_ten_consumers_one_slot(
            num_producers: 10,
            num_consumers: 10,
            max_queue_size: 1,
            messages_per_producer: 50000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        ten_producers_ten_consumers_hundred_slots_notify_all(
            num_producers: 10,
            num_consumers: 10,
            max_queue_size: 100,
            messages_per_producer: 50000,
            notification_style: NotifyStyle::All,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
        ten_producers_ten_consumers_hundred_slots_notify_one(
            num_producers: 10,
            num_consumers: 10,
            max_queue_size: 100,
            messages_per_producer: 50000,
            notification_style: NotifyStyle::One,
            timeout: Timeout::Forever,
            delay_seconds: 0
        );
    }
}
