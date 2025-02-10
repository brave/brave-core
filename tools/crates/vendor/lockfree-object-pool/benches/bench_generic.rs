#[macro_export]
macro_rules! pull_ {
    ($pool:ident, 1) => {
        $pool.pull()
    };
    ($pool:ident, 2) => {
        $pool.pull(|| Vec::with_capacity(4096))
    };
    ($pool:ident, 3) => {
        $pool.create().unwrap()
    };
}

#[macro_export]
macro_rules! pull_forward_ {
    ($pool:ident, 1) => {
        $pool.pull_owned()
    };

    ($pool:ident, 3) => {
        $pool.clone().create_owned().unwrap()
    };
}

#[macro_export]
macro_rules! bench_alloc_impl_ {
    ($group:expr, $name:literal, $expression:expr, $pull_impl:tt) => {
        $group.bench_function($name, |b| {
            let pool = $expression;
            let mut items = Vec::new();
            b.iter(|| {
                items.push(pull_!(pool, $pull_impl));
            });
        });
    };
}

#[macro_export]
macro_rules! bench_free_impl_ {
    ($group:expr, $name:literal, $expression:expr, $pull_impl:tt) => {
        $group.bench_function($name, |b| {
            b.iter_custom(|iter| {
                use std::time::Instant;
                let pool = $expression;
                let mut items = Vec::new();
                for _ in 0..iter {
                    items.push(pull_!(pool, $pull_impl));
                }
                let start = Instant::now();
                items.clear();
                start.elapsed()
            });
        });
    };
}

#[macro_export]
macro_rules! bench_alloc_mt_impl_ {
    ($group:expr, $name:literal, $expression:expr, $pull_impl:tt) => {
        $group.bench_function($name, |b| {
            b.iter_custom(|iter| {
                use std::sync::Arc;
                use std::sync::Barrier;
                use std::thread;
                use std::time::Instant;

                let pool = Arc::new($expression);
                let start_barrier = Arc::new(Barrier::new(6));
                let stop_barrier = Arc::new(Barrier::new(6));
                let mut children = Vec::new();
                for _ in 0..5 {
                    let pool = Arc::clone(&pool);
                    let start_barrier = Arc::clone(&start_barrier);
                    let stop_barrier = Arc::clone(&stop_barrier);
                    let child = thread::spawn(move || {
                        let mut items = Vec::with_capacity(iter as usize);
                        start_barrier.wait();
                        for _ in 0..iter {
                            items.push(pull_!(pool, $pull_impl));
                        }
                        stop_barrier.wait();
                    });
                    children.push(child);
                }

                start_barrier.wait();
                let start = Instant::now();
                stop_barrier.wait();
                let duration = start.elapsed() / 5;

                for child in children {
                    child.join().unwrap();
                }

                duration
            });
        });
    };
}

#[macro_export]
macro_rules! bench_free_mt_impl_ {
    ($group:expr, $name:literal, $expression:expr, $pull_impl:tt) => {
        $group.bench_function($name, |b| {
            b.iter_custom(|iter| {
                use std::sync::Arc;
                use std::sync::Barrier;
                use std::thread;
                use std::time::Instant;
                let pool = Arc::new($expression);
                let start_barrier = Arc::new(Barrier::new(6));
                let stop_barrier = Arc::new(Barrier::new(6));
                let mut children = Vec::new();
                for _ in 0..5 {
                    let pool = Arc::clone(&pool);
                    let start_barrier = Arc::clone(&start_barrier);
                    let stop_barrier = Arc::clone(&stop_barrier);
                    let child = thread::spawn(move || {
                        let mut items = Vec::with_capacity(iter as usize);
                        for _ in 0..iter {
                            items.push(pull_!(pool, $pull_impl));
                        }
                        start_barrier.wait();
                        items.clear();
                        stop_barrier.wait();
                    });
                    children.push(child);
                }

                start_barrier.wait();
                let start = Instant::now();
                stop_barrier.wait();
                let duration = start.elapsed() / 5;

                for child in children {
                    child.join().unwrap();
                }

                duration
            });
        });
    };
}

#[macro_export]
macro_rules! bench_forward_impl_ {
    ($group:expr, $name:literal, $expression:expr, $nb_readder:ident, $nb_writter:ident, $pull_impl:tt) => {
        $group.bench_function($name, |b| {
            b.iter_custom(|iter| {
                use bench_tools::Queue;
                use std::sync::Arc;
                use std::sync::Barrier;
                use std::thread;
                use std::time::Instant;

                let pool = Arc::new($expression);

                let queue = Arc::new(Queue::new());
                let start_barrier = Arc::new(Barrier::new($nb_readder + $nb_writter + 1));
                let stop_reader_barrier = Arc::new(Barrier::new($nb_readder + 1));
                let stop_writer_barrier = Arc::new(Barrier::new($nb_writter + 1));
                let mut children = Vec::new();
                for _ in 0..$nb_readder {
                    let queue = Arc::clone(&queue);
                    let start_barrier = Arc::clone(&start_barrier);
                    let stop_reader_barrier = Arc::clone(&stop_reader_barrier);
                    let child = thread::spawn(move || {
                        start_barrier.wait();
                        loop {
                            let elt = queue.pop();
                            if elt.is_none() {
                                break;
                            }
                        }
                        stop_reader_barrier.wait();
                    });
                    children.push(child);
                }
                for _ in 0..$nb_writter {
                    let pool = Arc::clone(&pool);
                    let queue = Arc::clone(&queue);
                    let start_barrier = Arc::clone(&start_barrier);
                    let stop_writer_barrier = Arc::clone(&stop_writer_barrier);
                    let child = thread::spawn(move || {
                        start_barrier.wait();
                        for _ in 0..iter {
                            queue.push(pull_forward_!(pool, $pull_impl))
                        }
                        stop_writer_barrier.wait();
                    });
                    children.push(child);
                }

                start_barrier.wait();
                let start = Instant::now();
                stop_writer_barrier.wait();
                queue.stop();
                stop_reader_barrier.wait();
                let duration = start.elapsed() / 5;

                for child in children {
                    child.join().unwrap();
                }

                duration
            });
        });
    };
}
