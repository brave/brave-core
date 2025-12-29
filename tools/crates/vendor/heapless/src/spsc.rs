//! Fixed capacity Single Producer Single Consumer (SPSC) queue
//!
//! Implementation based on <https://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular>
//!
//! # Portability
//!
//! This module requires CAS atomic instructions which are not available on all architectures
//! (e.g.  ARMv6-M (`thumbv6m-none-eabi`) and MSP430 (`msp430-none-elf`)). These atomics can be
//! emulated however with [`portable-atomic`](https://crates.io/crates/portable-atomic), which is
//! enabled with the `cas` feature and is enabled by default for `thumbv6m-none-eabi` and `riscv32`
//! targets.
//!
//! # Examples
//!
//! - `Queue` can be used as a plain queue
//!
//! ```
//! use heapless::spsc::Queue;
//!
//! let mut rb: Queue<u8, 4> = Queue::new();
//!
//! assert!(rb.enqueue(0).is_ok());
//! assert!(rb.enqueue(1).is_ok());
//! assert!(rb.enqueue(2).is_ok());
//! assert!(rb.enqueue(3).is_err()); // full
//!
//! assert_eq!(rb.dequeue(), Some(0));
//! ```
//!
//! - `Queue` can be `split` and then be used in Single Producer Single Consumer mode.
//!
//! "no alloc" applications can create a `&'static mut` reference to a `Queue` -- using a static
//! variable -- and then `split` it: this consumes the static reference. The resulting `Consumer`
//! and `Producer` can then be moved into different execution contexts (threads, interrupt handlers,
//! etc.)
//!
//! ```
//! use heapless::spsc::{Producer, Queue};
//!
//! enum Event { A, B }
//!
//! fn main() {
//!     let queue: &'static mut Queue<Event, 4> = {
//!         static mut Q: Queue<Event, 4> = Queue::new();
//!         unsafe { &mut Q }
//!     };
//!
//!     let (producer, mut consumer) = queue.split();
//!
//!     // `producer` can be moved into `interrupt_handler` using a static mutex or the mechanism
//!     // provided by the concurrency framework you are using (e.g. a resource in RTIC)
//!
//!     loop {
//!         match consumer.dequeue() {
//!             Some(Event::A) => { /* .. */ },
//!             Some(Event::B) => { /* .. */ },
//!             None => { /* sleep */ },
//!         }
//! #       break
//!     }
//! }
//!
//! // this is a different execution context that can preempt `main`
//! fn interrupt_handler(producer: &mut Producer<'static, Event, 4>) {
//! #   let condition = true;
//!
//!     // ..
//!
//!     if condition {
//!         producer.enqueue(Event::A).ok().unwrap();
//!     } else {
//!         producer.enqueue(Event::B).ok().unwrap();
//!     }
//!
//!     // ..
//! }
//! ```
//!
//! # Benchmarks
//!
//! Measured on a ARM Cortex-M3 core running at 8 MHz and with zero Flash wait cycles
//!
//! `-C opt-level`         |`3`|
//! -----------------------|---|
//! `Consumer<u8>::dequeue`| 15|
//! `Queue<u8>::dequeue`   | 12|
//! `Producer<u8>::enqueue`| 16|
//! `Queue<u8>::enqueue`   | 14|
//!
//! - All execution times are in clock cycles. 1 clock cycle = 125 ns.
//! - Execution time is *dependent* of `mem::size_of::<T>()`. Both operations include one
//! `memcpy(T)` in their successful path.
//! - The optimization level is indicated in the first row.
//! - The numbers reported correspond to the successful path (i.e. `Some` is returned by `dequeue`
//! and `Ok` is returned by `enqueue`).

use core::{cell::UnsafeCell, fmt, hash, mem::MaybeUninit, ptr};

#[cfg(not(feature = "portable-atomic"))]
use core::sync::atomic;
#[cfg(feature = "portable-atomic")]
use portable_atomic as atomic;

use atomic::{AtomicUsize, Ordering};

/// A statically allocated single producer single consumer queue with a capacity of `N - 1` elements
///
/// *IMPORTANT*: To get better performance use a value for `N` that is a power of 2 (e.g. `16`, `32`,
/// etc.).
pub struct Queue<T, const N: usize> {
    // this is from where we dequeue items
    pub(crate) head: AtomicUsize,

    // this is where we enqueue new items
    pub(crate) tail: AtomicUsize,

    pub(crate) buffer: [UnsafeCell<MaybeUninit<T>>; N],
}

impl<T, const N: usize> Queue<T, N> {
    const INIT: UnsafeCell<MaybeUninit<T>> = UnsafeCell::new(MaybeUninit::uninit());

    #[inline]
    fn increment(val: usize) -> usize {
        (val + 1) % N
    }

    /// Creates an empty queue with a fixed capacity of `N - 1`
    pub const fn new() -> Self {
        // Const assert N > 1
        crate::sealed::greater_than_1::<N>();

        Queue {
            head: AtomicUsize::new(0),
            tail: AtomicUsize::new(0),
            buffer: [Self::INIT; N],
        }
    }

    /// Returns the maximum number of elements the queue can hold
    #[inline]
    pub const fn capacity(&self) -> usize {
        N - 1
    }

    /// Returns the number of elements in the queue
    #[inline]
    pub fn len(&self) -> usize {
        let current_head = self.head.load(Ordering::Relaxed);
        let current_tail = self.tail.load(Ordering::Relaxed);

        current_tail.wrapping_sub(current_head).wrapping_add(N) % N
    }

    /// Returns `true` if the queue is empty
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.head.load(Ordering::Relaxed) == self.tail.load(Ordering::Relaxed)
    }

    /// Returns `true` if the queue is full
    #[inline]
    pub fn is_full(&self) -> bool {
        Self::increment(self.tail.load(Ordering::Relaxed)) == self.head.load(Ordering::Relaxed)
    }

    /// Iterates from the front of the queue to the back
    pub fn iter(&self) -> Iter<'_, T, N> {
        Iter {
            rb: self,
            index: 0,
            len: self.len(),
        }
    }

    /// Returns an iterator that allows modifying each value
    pub fn iter_mut(&mut self) -> IterMut<'_, T, N> {
        let len = self.len();
        IterMut {
            rb: self,
            index: 0,
            len,
        }
    }

    /// Adds an `item` to the end of the queue
    ///
    /// Returns back the `item` if the queue is full
    #[inline]
    pub fn enqueue(&mut self, val: T) -> Result<(), T> {
        unsafe { self.inner_enqueue(val) }
    }

    /// Returns the item in the front of the queue, or `None` if the queue is empty
    #[inline]
    pub fn dequeue(&mut self) -> Option<T> {
        unsafe { self.inner_dequeue() }
    }

    /// Returns a reference to the item in the front of the queue without dequeuing, or
    /// `None` if the queue is empty.
    ///
    /// # Examples
    /// ```
    /// use heapless::spsc::Queue;
    ///
    /// let mut queue: Queue<u8, 235> = Queue::new();
    /// let (mut producer, mut consumer) = queue.split();
    /// assert_eq!(None, consumer.peek());
    /// producer.enqueue(1);
    /// assert_eq!(Some(&1), consumer.peek());
    /// assert_eq!(Some(1), consumer.dequeue());
    /// assert_eq!(None, consumer.peek());
    /// ```
    pub fn peek(&self) -> Option<&T> {
        if !self.is_empty() {
            let head = self.head.load(Ordering::Relaxed);
            Some(unsafe { &*(self.buffer.get_unchecked(head).get() as *const T) })
        } else {
            None
        }
    }

    // The memory for enqueueing is "owned" by the tail pointer.
    // NOTE: This internal function uses internal mutability to allow the [`Producer`] to enqueue
    // items without doing pointer arithmetic and accessing internal fields of this type.
    unsafe fn inner_enqueue(&self, val: T) -> Result<(), T> {
        let current_tail = self.tail.load(Ordering::Relaxed);
        let next_tail = Self::increment(current_tail);

        if next_tail != self.head.load(Ordering::Acquire) {
            (self.buffer.get_unchecked(current_tail).get()).write(MaybeUninit::new(val));
            self.tail.store(next_tail, Ordering::Release);

            Ok(())
        } else {
            Err(val)
        }
    }

    // The memory for enqueueing is "owned" by the tail pointer.
    // NOTE: This internal function uses internal mutability to allow the [`Producer`] to enqueue
    // items without doing pointer arithmetic and accessing internal fields of this type.
    unsafe fn inner_enqueue_unchecked(&self, val: T) {
        let current_tail = self.tail.load(Ordering::Relaxed);

        (self.buffer.get_unchecked(current_tail).get()).write(MaybeUninit::new(val));
        self.tail
            .store(Self::increment(current_tail), Ordering::Release);
    }

    /// Adds an `item` to the end of the queue, without checking if it's full
    ///
    /// # Unsafety
    ///
    /// If the queue is full this operation will leak a value (T's destructor won't run on
    /// the value that got overwritten by `item`), *and* will allow the `dequeue` operation
    /// to create a copy of `item`, which could result in `T`'s destructor running on `item`
    /// twice.
    pub unsafe fn enqueue_unchecked(&mut self, val: T) {
        self.inner_enqueue_unchecked(val)
    }

    // The memory for dequeuing is "owned" by the head pointer,.
    // NOTE: This internal function uses internal mutability to allow the [`Consumer`] to dequeue
    // items without doing pointer arithmetic and accessing internal fields of this type.
    unsafe fn inner_dequeue(&self) -> Option<T> {
        let current_head = self.head.load(Ordering::Relaxed);

        if current_head == self.tail.load(Ordering::Acquire) {
            None
        } else {
            let v = (self.buffer.get_unchecked(current_head).get() as *const T).read();

            self.head
                .store(Self::increment(current_head), Ordering::Release);

            Some(v)
        }
    }

    // The memory for dequeuing is "owned" by the head pointer,.
    // NOTE: This internal function uses internal mutability to allow the [`Consumer`] to dequeue
    // items without doing pointer arithmetic and accessing internal fields of this type.
    unsafe fn inner_dequeue_unchecked(&self) -> T {
        let current_head = self.head.load(Ordering::Relaxed);
        let v = (self.buffer.get_unchecked(current_head).get() as *const T).read();

        self.head
            .store(Self::increment(current_head), Ordering::Release);

        v
    }

    /// Returns the item in the front of the queue, without checking if there is something in the
    /// queue
    ///
    /// # Unsafety
    ///
    /// If the queue is empty this operation will return uninitialized memory.
    pub unsafe fn dequeue_unchecked(&mut self) -> T {
        self.inner_dequeue_unchecked()
    }

    /// Splits a queue into producer and consumer endpoints
    pub fn split(&mut self) -> (Producer<'_, T, N>, Consumer<'_, T, N>) {
        (Producer { rb: self }, Consumer { rb: self })
    }
}

impl<T, const N: usize> Default for Queue<T, N> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T, const N: usize> Clone for Queue<T, N>
where
    T: Clone,
{
    fn clone(&self) -> Self {
        let mut new: Queue<T, N> = Queue::new();

        for s in self.iter() {
            unsafe {
                // NOTE(unsafe) new.capacity() == self.capacity() >= self.len()
                // no overflow possible
                new.enqueue_unchecked(s.clone());
            }
        }

        new
    }
}

impl<T, const N: usize, const N2: usize> PartialEq<Queue<T, N2>> for Queue<T, N>
where
    T: PartialEq,
{
    fn eq(&self, other: &Queue<T, N2>) -> bool {
        self.len() == other.len() && self.iter().zip(other.iter()).all(|(v1, v2)| v1 == v2)
    }
}

impl<T, const N: usize> Eq for Queue<T, N> where T: Eq {}

/// An iterator over the items of a queue
pub struct Iter<'a, T, const N: usize> {
    rb: &'a Queue<T, N>,
    index: usize,
    len: usize,
}

impl<'a, T, const N: usize> Clone for Iter<'a, T, N> {
    fn clone(&self) -> Self {
        Self {
            rb: self.rb,
            index: self.index,
            len: self.len,
        }
    }
}

/// A mutable iterator over the items of a queue
pub struct IterMut<'a, T, const N: usize> {
    rb: &'a mut Queue<T, N>,
    index: usize,
    len: usize,
}

impl<'a, T, const N: usize> Iterator for Iter<'a, T, N> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.len {
            let head = self.rb.head.load(Ordering::Relaxed);

            let i = (head + self.index) % N;
            self.index += 1;

            Some(unsafe { &*(self.rb.buffer.get_unchecked(i).get() as *const T) })
        } else {
            None
        }
    }
}

impl<'a, T, const N: usize> Iterator for IterMut<'a, T, N> {
    type Item = &'a mut T;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.len {
            let head = self.rb.head.load(Ordering::Relaxed);

            let i = (head + self.index) % N;
            self.index += 1;

            Some(unsafe { &mut *(self.rb.buffer.get_unchecked(i).get() as *mut T) })
        } else {
            None
        }
    }
}

impl<'a, T, const N: usize> DoubleEndedIterator for Iter<'a, T, N> {
    fn next_back(&mut self) -> Option<Self::Item> {
        if self.index < self.len {
            let head = self.rb.head.load(Ordering::Relaxed);

            // self.len > 0, since it's larger than self.index > 0
            let i = (head + self.len - 1) % N;
            self.len -= 1;
            Some(unsafe { &*(self.rb.buffer.get_unchecked(i).get() as *const T) })
        } else {
            None
        }
    }
}

impl<'a, T, const N: usize> DoubleEndedIterator for IterMut<'a, T, N> {
    fn next_back(&mut self) -> Option<Self::Item> {
        if self.index < self.len {
            let head = self.rb.head.load(Ordering::Relaxed);

            // self.len > 0, since it's larger than self.index > 0
            let i = (head + self.len - 1) % N;
            self.len -= 1;
            Some(unsafe { &mut *(self.rb.buffer.get_unchecked(i).get() as *mut T) })
        } else {
            None
        }
    }
}

impl<T, const N: usize> Drop for Queue<T, N> {
    fn drop(&mut self) {
        for item in self {
            unsafe {
                ptr::drop_in_place(item);
            }
        }
    }
}

impl<T, const N: usize> fmt::Debug for Queue<T, N>
where
    T: fmt::Debug,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_list().entries(self.iter()).finish()
    }
}

impl<T, const N: usize> hash::Hash for Queue<T, N>
where
    T: hash::Hash,
{
    fn hash<H: hash::Hasher>(&self, state: &mut H) {
        // iterate over self in order
        for t in self.iter() {
            hash::Hash::hash(t, state);
        }
    }
}

impl<'a, T, const N: usize> IntoIterator for &'a Queue<T, N> {
    type Item = &'a T;
    type IntoIter = Iter<'a, T, N>;

    fn into_iter(self) -> Self::IntoIter {
        self.iter()
    }
}

impl<'a, T, const N: usize> IntoIterator for &'a mut Queue<T, N> {
    type Item = &'a mut T;
    type IntoIter = IterMut<'a, T, N>;

    fn into_iter(self) -> Self::IntoIter {
        self.iter_mut()
    }
}

/// A queue "consumer"; it can dequeue items from the queue
/// NOTE the consumer semantically owns the `head` pointer of the queue
pub struct Consumer<'a, T, const N: usize> {
    rb: &'a Queue<T, N>,
}

unsafe impl<'a, T, const N: usize> Send for Consumer<'a, T, N> where T: Send {}

/// A queue "producer"; it can enqueue items into the queue
/// NOTE the producer semantically owns the `tail` pointer of the queue
pub struct Producer<'a, T, const N: usize> {
    rb: &'a Queue<T, N>,
}

unsafe impl<'a, T, const N: usize> Send for Producer<'a, T, N> where T: Send {}

impl<'a, T, const N: usize> Consumer<'a, T, N> {
    /// Returns the item in the front of the queue, or `None` if the queue is empty
    #[inline]
    pub fn dequeue(&mut self) -> Option<T> {
        unsafe { self.rb.inner_dequeue() }
    }

    /// Returns the item in the front of the queue, without checking if there are elements in the
    /// queue
    ///
    /// See [`Queue::dequeue_unchecked`] for safety
    #[inline]
    pub unsafe fn dequeue_unchecked(&mut self) -> T {
        self.rb.inner_dequeue_unchecked()
    }

    /// Returns if there are any items to dequeue. When this returns `true`, at least the
    /// first subsequent dequeue will succeed
    #[inline]
    pub fn ready(&self) -> bool {
        !self.rb.is_empty()
    }

    /// Returns the number of elements in the queue
    #[inline]
    pub fn len(&self) -> usize {
        self.rb.len()
    }

    /// Returns the maximum number of elements the queue can hold
    #[inline]
    pub fn capacity(&self) -> usize {
        self.rb.capacity()
    }

    /// Returns the item in the front of the queue without dequeuing, or `None` if the queue is
    /// empty
    ///
    /// # Examples
    /// ```
    /// use heapless::spsc::Queue;
    ///
    /// let mut queue: Queue<u8, 235> = Queue::new();
    /// let (mut producer, mut consumer) = queue.split();
    /// assert_eq!(None, consumer.peek());
    /// producer.enqueue(1);
    /// assert_eq!(Some(&1), consumer.peek());
    /// assert_eq!(Some(1), consumer.dequeue());
    /// assert_eq!(None, consumer.peek());
    /// ```
    #[inline]
    pub fn peek(&self) -> Option<&T> {
        self.rb.peek()
    }
}

impl<'a, T, const N: usize> Producer<'a, T, N> {
    /// Adds an `item` to the end of the queue, returns back the `item` if the queue is full
    #[inline]
    pub fn enqueue(&mut self, val: T) -> Result<(), T> {
        unsafe { self.rb.inner_enqueue(val) }
    }

    /// Adds an `item` to the end of the queue, without checking if the queue is full
    ///
    /// See [`Queue::enqueue_unchecked`] for safety
    #[inline]
    pub unsafe fn enqueue_unchecked(&mut self, val: T) {
        self.rb.inner_enqueue_unchecked(val)
    }

    /// Returns if there is any space to enqueue a new item. When this returns true, at
    /// least the first subsequent enqueue will succeed.
    #[inline]
    pub fn ready(&self) -> bool {
        !self.rb.is_full()
    }

    /// Returns the number of elements in the queue
    #[inline]
    pub fn len(&self) -> usize {
        self.rb.len()
    }

    /// Returns the maximum number of elements the queue can hold
    #[inline]
    pub fn capacity(&self) -> usize {
        self.rb.capacity()
    }
}

#[cfg(test)]
mod tests {
    use std::hash::{Hash, Hasher};

    use crate::spsc::Queue;

    #[test]
    fn full() {
        let mut rb: Queue<i32, 3> = Queue::new();

        assert_eq!(rb.is_full(), false);

        rb.enqueue(1).unwrap();
        assert_eq!(rb.is_full(), false);

        rb.enqueue(2).unwrap();
        assert_eq!(rb.is_full(), true);
    }

    #[test]
    fn empty() {
        let mut rb: Queue<i32, 3> = Queue::new();

        assert_eq!(rb.is_empty(), true);

        rb.enqueue(1).unwrap();
        assert_eq!(rb.is_empty(), false);

        rb.enqueue(2).unwrap();
        assert_eq!(rb.is_empty(), false);
    }

    #[test]
    #[cfg_attr(miri, ignore)] // too slow
    fn len() {
        let mut rb: Queue<i32, 3> = Queue::new();

        assert_eq!(rb.len(), 0);

        rb.enqueue(1).unwrap();
        assert_eq!(rb.len(), 1);

        rb.enqueue(2).unwrap();
        assert_eq!(rb.len(), 2);

        for _ in 0..1_000_000 {
            let v = rb.dequeue().unwrap();
            println!("{}", v);
            rb.enqueue(v).unwrap();
            assert_eq!(rb.len(), 2);
        }
    }

    #[test]
    #[cfg_attr(miri, ignore)] // too slow
    fn try_overflow() {
        const N: usize = 23;
        let mut rb: Queue<i32, N> = Queue::new();

        for i in 0..N as i32 - 1 {
            rb.enqueue(i).unwrap();
        }

        for _ in 0..1_000_000 {
            for i in 0..N as i32 - 1 {
                let d = rb.dequeue().unwrap();
                assert_eq!(d, i);
                rb.enqueue(i).unwrap();
            }
        }
    }

    #[test]
    fn sanity() {
        let mut rb: Queue<i32, 10> = Queue::new();

        let (mut p, mut c) = rb.split();

        assert_eq!(p.ready(), true);

        assert_eq!(c.ready(), false);

        assert_eq!(c.dequeue(), None);

        p.enqueue(0).unwrap();

        assert_eq!(c.dequeue(), Some(0));
    }

    #[test]
    fn static_new() {
        static mut _Q: Queue<i32, 4> = Queue::new();
    }

    #[test]
    fn drop() {
        struct Droppable;
        impl Droppable {
            fn new() -> Self {
                unsafe {
                    COUNT += 1;
                }
                Droppable
            }
        }

        impl Drop for Droppable {
            fn drop(&mut self) {
                unsafe {
                    COUNT -= 1;
                }
            }
        }

        static mut COUNT: i32 = 0;

        {
            let mut v: Queue<Droppable, 4> = Queue::new();
            v.enqueue(Droppable::new()).ok().unwrap();
            v.enqueue(Droppable::new()).ok().unwrap();
            v.dequeue().unwrap();
        }

        assert_eq!(unsafe { COUNT }, 0);

        {
            let mut v: Queue<Droppable, 4> = Queue::new();
            v.enqueue(Droppable::new()).ok().unwrap();
            v.enqueue(Droppable::new()).ok().unwrap();
        }

        assert_eq!(unsafe { COUNT }, 0);
    }

    #[test]
    fn iter() {
        let mut rb: Queue<i32, 4> = Queue::new();

        rb.enqueue(0).unwrap();
        rb.dequeue().unwrap();
        rb.enqueue(1).unwrap();
        rb.enqueue(2).unwrap();
        rb.enqueue(3).unwrap();

        let mut items = rb.iter();

        // assert_eq!(items.next(), Some(&0));
        assert_eq!(items.next(), Some(&1));
        assert_eq!(items.next(), Some(&2));
        assert_eq!(items.next(), Some(&3));
        assert_eq!(items.next(), None);
    }

    #[test]
    fn iter_double_ended() {
        let mut rb: Queue<i32, 4> = Queue::new();

        rb.enqueue(0).unwrap();
        rb.enqueue(1).unwrap();
        rb.enqueue(2).unwrap();

        let mut items = rb.iter();

        assert_eq!(items.next(), Some(&0));
        assert_eq!(items.next_back(), Some(&2));
        assert_eq!(items.next(), Some(&1));
        assert_eq!(items.next(), None);
        assert_eq!(items.next_back(), None);
    }

    #[test]
    fn iter_mut() {
        let mut rb: Queue<i32, 4> = Queue::new();

        rb.enqueue(0).unwrap();
        rb.enqueue(1).unwrap();
        rb.enqueue(2).unwrap();

        let mut items = rb.iter_mut();

        assert_eq!(items.next(), Some(&mut 0));
        assert_eq!(items.next(), Some(&mut 1));
        assert_eq!(items.next(), Some(&mut 2));
        assert_eq!(items.next(), None);
    }

    #[test]
    fn iter_mut_double_ended() {
        let mut rb: Queue<i32, 4> = Queue::new();

        rb.enqueue(0).unwrap();
        rb.enqueue(1).unwrap();
        rb.enqueue(2).unwrap();

        let mut items = rb.iter_mut();

        assert_eq!(items.next(), Some(&mut 0));
        assert_eq!(items.next_back(), Some(&mut 2));
        assert_eq!(items.next(), Some(&mut 1));
        assert_eq!(items.next(), None);
        assert_eq!(items.next_back(), None);
    }

    #[test]
    fn wrap_around() {
        let mut rb: Queue<i32, 4> = Queue::new();

        rb.enqueue(0).unwrap();
        rb.enqueue(1).unwrap();
        rb.enqueue(2).unwrap();
        rb.dequeue().unwrap();
        rb.dequeue().unwrap();
        rb.dequeue().unwrap();
        rb.enqueue(3).unwrap();
        rb.enqueue(4).unwrap();

        assert_eq!(rb.len(), 2);
    }

    #[test]
    fn ready_flag() {
        let mut rb: Queue<i32, 3> = Queue::new();
        let (mut p, mut c) = rb.split();
        assert_eq!(c.ready(), false);
        assert_eq!(p.ready(), true);

        p.enqueue(0).unwrap();

        assert_eq!(c.ready(), true);
        assert_eq!(p.ready(), true);

        p.enqueue(1).unwrap();

        assert_eq!(c.ready(), true);
        assert_eq!(p.ready(), false);

        c.dequeue().unwrap();

        assert_eq!(c.ready(), true);
        assert_eq!(p.ready(), true);

        c.dequeue().unwrap();

        assert_eq!(c.ready(), false);
        assert_eq!(p.ready(), true);
    }

    #[test]
    fn clone() {
        let mut rb1: Queue<i32, 4> = Queue::new();
        rb1.enqueue(0).unwrap();
        rb1.enqueue(0).unwrap();
        rb1.dequeue().unwrap();
        rb1.enqueue(0).unwrap();
        let rb2 = rb1.clone();
        assert_eq!(rb1.capacity(), rb2.capacity());
        assert_eq!(rb1.len(), rb2.len());
        assert!(rb1.iter().zip(rb2.iter()).all(|(v1, v2)| v1 == v2));
    }

    #[test]
    fn eq() {
        // generate two queues with same content
        // but different buffer alignment
        let mut rb1: Queue<i32, 4> = Queue::new();
        rb1.enqueue(0).unwrap();
        rb1.enqueue(0).unwrap();
        rb1.dequeue().unwrap();
        rb1.enqueue(0).unwrap();
        let mut rb2: Queue<i32, 4> = Queue::new();
        rb2.enqueue(0).unwrap();
        rb2.enqueue(0).unwrap();
        assert!(rb1 == rb2);
        // test for symmetry
        assert!(rb2 == rb1);
        // test for changes in content
        rb1.enqueue(0).unwrap();
        assert!(rb1 != rb2);
        rb2.enqueue(1).unwrap();
        assert!(rb1 != rb2);
        // test for refexive relation
        assert!(rb1 == rb1);
        assert!(rb2 == rb2);
    }

    #[test]
    fn hash_equality() {
        // generate two queues with same content
        // but different buffer alignment
        let rb1 = {
            let mut rb1: Queue<i32, 4> = Queue::new();
            rb1.enqueue(0).unwrap();
            rb1.enqueue(0).unwrap();
            rb1.dequeue().unwrap();
            rb1.enqueue(0).unwrap();
            rb1
        };
        let rb2 = {
            let mut rb2: Queue<i32, 4> = Queue::new();
            rb2.enqueue(0).unwrap();
            rb2.enqueue(0).unwrap();
            rb2
        };
        let hash1 = {
            let mut hasher1 = hash32::FnvHasher::default();
            rb1.hash(&mut hasher1);
            let hash1 = hasher1.finish();
            hash1
        };
        let hash2 = {
            let mut hasher2 = hash32::FnvHasher::default();
            rb2.hash(&mut hasher2);
            let hash2 = hasher2.finish();
            hash2
        };
        assert_eq!(hash1, hash2);
    }
}
