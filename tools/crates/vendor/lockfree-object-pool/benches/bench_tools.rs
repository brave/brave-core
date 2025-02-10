use std::collections::VecDeque;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Condvar, Mutex};

pub struct Queue<T> {
    data: Mutex<VecDeque<T>>,
    condvar: Condvar,
    stop: AtomicBool,
}

impl<T> Queue<T> {
    pub fn new() -> Self {
        Self {
            data: Mutex::new(VecDeque::new()),
            condvar: Condvar::new(),
            stop: AtomicBool::new(false),
        }
    }

    pub fn push(&self, value: T) {
        let mut datas = self.data.lock().unwrap();
        let empty = datas.is_empty();
        datas.push_back(value);
        if empty {
            self.condvar.notify_all();
        }
    }

    pub fn stop(&self) {
        let _datas = self.data.lock().unwrap();
        self.stop.store(true, Ordering::Relaxed);
        self.condvar.notify_all();
    }

    pub fn pop(&self) -> Option<T> {
        let mut datas = self.data.lock().unwrap();
        if !datas.is_empty() {
            datas.pop_front()
        } else if self.stop.load(Ordering::Relaxed) {
            None
        } else {
            self.condvar
                .wait_while(datas, |datas| {
                    datas.is_empty() && !self.stop.load(Ordering::Relaxed)
                })
                .unwrap()
                .pop_front()
        }
    }
}

impl<T> Default for Queue<T> {
    fn default() -> Self {
        Self::new()
    }
}
