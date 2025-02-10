#[macro_export]
macro_rules! test_generic_01 {
    ($name:ident, $expression:expr) => {
        #[test]
        fn $name() {
            let pool = $expression;
            for _ in 0..2 {
                let mut v = pool.pull();
                assert_eq!(*v, 0);
                *v += 1;
            }
        }
    };
}

#[macro_export]
macro_rules! test_generic_02 {
    ($name:ident, $expression:expr) => {
        #[test]
        fn $name() {
            use std::sync::mpsc;
            use std::sync::Arc;
            use std::thread;

            let pool = Arc::new($expression);

            let (tx, rx) = mpsc::channel();
            let mut children = Vec::new();

            for id in 0..5 {
                let thread_tx = tx.clone();
                let thread_pool = Arc::clone(&pool);

                let child = thread::spawn(move || {
                    let mut msg = thread_pool.pull();
                    *msg = id;
                    thread_tx.send(*msg).unwrap();
                });
                children.push(child);
            }

            let mut msgs = Vec::new();
            for _ in 0..5 {
                let msg = rx.recv().unwrap();
                if !msgs.contains(&msg) && msg < 5 {
                    msgs.push(msg);
                }
            }
            assert_eq!(msgs.len(), 5);

            for child in children {
                child.join().unwrap();
            }
        }
    };
}

#[macro_export]
macro_rules! test_recycle_generic_01 {
    ($name:ident, $expression:expr) => {
        #[test]
        fn $name() {
            let pool = $expression;

            let mut item1 = pool.pull();
            *item1 = 5;
            drop(item1);

            let item2 = pool.pull();
            assert_eq!(*item2, 5);
        }
    };
}
