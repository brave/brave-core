use crate::thread_parker::{ThreadParker, ThreadParkerT, UnparkHandleT};
use crate::word_lock::WordLock;
use core::{
    cell::{Cell, UnsafeCell},
    mem,
    ptr::{self, NonNull},
    sync::atomic::{AtomicPtr, AtomicUsize, Ordering},
};
use smallvec::SmallVec;
use std::time::Instant;

static NUM_THREADS: AtomicUsize = AtomicUsize::new(0);

/// Aborts the process if it is dropped while unwinding.
///
/// Parking-lot operations manipulate shared intrusive state without unwind
/// guards. Once an operation has started, unwinding could leave bucket locks
/// held, queue links inconsistent, or a transferred `ThreadData` inaccessible.
struct AbortOnPanic;

impl AbortOnPanic {
    /// Prevents the guard from being dropped after a successful operation.
    #[inline]
    fn disarm(self) {
        mem::forget(self);
    }
}

impl Drop for AbortOnPanic {
    fn drop(&mut self) {
        panic!("aborting due to panic in parking lot");
    }
}

/// Holds the pointer to the currently active `HashTable`.
///
/// # Safety
///
/// Except for the initial value of null, it must always point to a valid `HashTable` instance.
/// Any `HashTable` this global static has ever pointed to must never be freed.
static HASHTABLE: AtomicPtr<HashTable> = AtomicPtr::new(ptr::null_mut());

// Even with 3x more buckets than threads, the memory overhead per thread is
// still only a few hundred bytes per thread.
const LOAD_FACTOR: usize = 3;

struct HashTable {
    // Hash buckets for the table
    entries: Box<[Bucket]>,

    // Number of bits used for the hash function
    hash_bits: u32,

    // Previous table. This is only kept to keep leak detectors happy.
    _prev: Option<NonNull<HashTable>>,
}

impl HashTable {
    #[inline]
    fn new(num_threads: usize, prev: Option<NonNull<HashTable>>) -> Box<HashTable> {
        let new_size = (num_threads * LOAD_FACTOR).next_power_of_two();
        let hash_bits = 0usize.leading_zeros() - new_size.leading_zeros() - 1;

        let mut entries = Vec::with_capacity(new_size);
        for _ in 0..new_size {
            entries.push(Bucket::new());
        }

        Box::new(HashTable {
            entries: entries.into_boxed_slice(),
            hash_bits,
            _prev: prev,
        })
    }
}

#[repr(align(64))]
struct Bucket {
    // Lock protecting the queue
    mutex: WordLock,

    // Linked list of threads waiting on this bucket
    queue_head: Cell<Option<NonNull<ThreadData>>>,
    queue_tail: Cell<Option<NonNull<ThreadData>>>,
}

impl Bucket {
    #[inline]
    pub fn new() -> Self {
        Self {
            mutex: WordLock::new(),
            queue_head: Cell::new(None),
            queue_tail: Cell::new(None),
        }
    }
}

// A ThreadData has two stable ownership states:
//
// * While it is not linked into a bucket queue, it is owned by its thread and
//   its non-atomic fields are unshared.
// * While it is linked into a bucket queue, it is owned by the parking lot. Its
//   queue fields and lifetime are protected by the lock of the bucket that
//   currently contains it.
//
// To transfer ownership to the parking lot, the local thread initializes the
// queue fields and prepares the parker while holding the destination bucket
// lock, then links the ThreadData into the queue. Once linked, the local thread
// must not access fields owned by the parking lot.
//
// To transfer ownership back, an unparker removes the ThreadData from the queue,
// writes the result fields, and calls `unpark_lock` while still holding the
// bucket lock. It must not access the ThreadData after `unpark_lock`, since the
// parked thread may immediately resume and destroy it. The returned
// UnparkHandle remains usable without access to the ThreadData. A successful
// return from `park` or `park_until` completes the transfer back to the local
// thread and synchronizes with the unparker's preceding writes.
//
// A timeout does not by itself transfer ownership back. The local thread must
// use the atomic key to find and lock the current bucket, resolve any concurrent
// unpark or requeue, and remove itself from the queue before it again owns the
// ThreadData.
struct ThreadData {
    // Accessed concurrently through the ThreadParker protocol rather than the
    // bucket lock.
    parker: ThreadParker,

    // Written by the parking lot under the bucket lock. This is atomic because
    // a timing-out local thread must read it before regaining ownership, and a
    // concurrent requeue may change it.
    key: AtomicUsize,

    // Fields transferred between local ownership and parking-lot ownership as
    // described above.
    state: UnsafeCell<ThreadDataState>,
}

struct ThreadDataState {
    // Linked list of parked threads. Accessed only while holding the lock of
    // the bucket that contains this ThreadData.
    next_in_queue: Option<NonNull<ThreadData>>,

    // Written by the parking lot before `unpark_lock` and read locally only
    // after a successful return from the parker.
    unpark_token: UnparkToken,

    // Initialized before linking and subsequently read only under the bucket
    // lock.
    park_token: ParkToken,

    // Deadlock detector fields governed by the same ownership rules.
    #[cfg(feature = "deadlock_detection")]
    deadlock_data: deadlock::DeadlockData,
}

impl ThreadData {
    fn new() -> ThreadData {
        // Keep track of the total number of live ThreadData objects and resize
        // the hash table accordingly.
        let num_threads = NUM_THREADS.fetch_add(1, Ordering::Relaxed) + 1;
        grow_hashtable(num_threads);

        ThreadData {
            parker: ThreadParker::new(),
            key: AtomicUsize::new(0),
            state: UnsafeCell::new(ThreadDataState {
                next_in_queue: None,
                unpark_token: DEFAULT_UNPARK_TOKEN,
                park_token: DEFAULT_PARK_TOKEN,
                #[cfg(feature = "deadlock_detection")]
                deadlock_data: deadlock::DeadlockData::new(),
            }),
        }
    }

    /// Returns the state under the ownership model described above.
    ///
    /// # Safety
    ///
    /// The caller must have exclusive access to the state. Either the current
    /// thread must own this `ThreadData`, or the parking lot must own it and the
    /// caller must hold the lock that protects it.
    #[inline]
    #[allow(clippy::mut_from_ref)]
    unsafe fn state(&self) -> &mut ThreadDataState {
        unsafe { self.state.get().as_mut_unchecked() }
    }
}

// Invokes the given closure with a reference to the current thread `ThreadData`.
#[inline(always)]
fn with_thread_data<T>(f: impl FnOnce(&ThreadData) -> T) -> T {
    // Unlike word_lock::ThreadData, parking_lot::ThreadData is always expensive
    // to construct. Try to use a thread-local version if possible. Otherwise just
    // create a ThreadData on the stack
    let mut thread_data_storage = None;
    thread_local!(static THREAD_DATA: ThreadData = ThreadData::new());
    let thread_data = THREAD_DATA
        .try_with(|thread_data| NonNull::from(thread_data))
        .unwrap_or_else(|_| NonNull::from(thread_data_storage.get_or_insert_with(ThreadData::new)));

    f(unsafe { thread_data.as_ref() })
}

impl Drop for ThreadData {
    fn drop(&mut self) {
        NUM_THREADS.fetch_sub(1, Ordering::Relaxed);
    }
}

/// Returns a reference to the latest hash table, creating one if it doesn't exist yet.
/// The reference is valid forever. However, the `HashTable` it references might become stale
/// at any point. Meaning it still exists, but it is not the instance in active use.
#[inline]
fn get_hashtable() -> &'static HashTable {
    let table = HASHTABLE.load(Ordering::Acquire);

    // If there is no table, create one
    if table.is_null() {
        create_hashtable()
    } else {
        // SAFETY: when not null, `HASHTABLE` always points to a `HashTable` that is never freed.
        unsafe { table.as_ref_unchecked() }
    }
}

/// Returns a reference to the latest hash table, creating one if it doesn't exist yet.
/// The reference is valid forever. However, the `HashTable` it references might become stale
/// at any point. Meaning it still exists, but it is not the instance in active use.
#[cold]
fn create_hashtable() -> &'static HashTable {
    let new_table = Box::into_raw(HashTable::new(LOAD_FACTOR, None));

    // If this fails then it means some other thread created the hash table first.
    let table = match HASHTABLE.compare_exchange(
        ptr::null_mut(),
        new_table,
        Ordering::AcqRel,
        Ordering::Acquire,
    ) {
        Ok(_) => new_table,
        Err(old_table) => {
            // Free the table we created
            // SAFETY: `new_table` is created from `Box::into_raw` above and only freed here.
            unsafe {
                let _ = Box::from_raw(new_table);
            }
            old_table
        }
    };
    // SAFETY: The `HashTable` behind `table` is never freed. It is either the table pointer we
    // created here, or it is one loaded from `HASHTABLE`.
    unsafe { table.as_ref_unchecked() }
}

// Grow the hash table so that it is big enough for the given number of threads.
// This isn't performance-critical since it is only done when a ThreadData is
// created, which only happens once per thread.
fn grow_hashtable(num_threads: usize) {
    // Lock all buckets in the existing table and get a reference to it
    let old_table = loop {
        let table = get_hashtable();

        // Check if we need to resize the existing table
        if table.entries.len() >= LOAD_FACTOR * num_threads {
            return;
        }

        // Lock all buckets in the old table
        for bucket in &table.entries[..] {
            bucket.mutex.lock();
        }

        // Now check if our table is still the latest one. Another thread could
        // have grown the hash table between us reading HASHTABLE and locking
        // the buckets.
        if ptr::eq(HASHTABLE.load(Ordering::Relaxed), table) {
            break table;
        }

        // Unlock buckets and try again
        for bucket in &table.entries[..] {
            // SAFETY: We hold the lock here, as required
            unsafe { bucket.mutex.unlock() };
        }
    };

    // Create the new table
    let mut new_table = HashTable::new(num_threads, Some(NonNull::from(old_table)));

    // Move the entries from the old table to the new one
    for bucket in &old_table.entries[..] {
        // SAFETY: Parking-lot operations maintain correctly linked queues. A
        // `ThreadData` remains valid for as long as it is linked in a queue.
        unsafe { rehash_bucket_into(bucket, &mut new_table) };
    }

    // Publish the new table. No races are possible at this point because
    // any other thread trying to grow the hash table is blocked on the bucket
    // locks in the old table.
    HASHTABLE.store(Box::into_raw(new_table), Ordering::Release);

    // Unlock all buckets in the old table
    for bucket in &old_table.entries[..] {
        // SAFETY: We hold the lock here, as required
        unsafe { bucket.mutex.unlock() };
    }
}

/// Iterates through all `ThreadData` objects in the bucket and inserts them
/// into the buckets corresponding to their keys in the given table.
///
/// # Safety
///
/// The given `bucket` must have a correctly constructed linked list under `queue_head`, containing
/// `ThreadData` instances that must stay valid at least as long as the given `table` is in use.
///
/// The given `table` must only contain buckets with correctly constructed linked lists.
unsafe fn rehash_bucket_into(bucket: &'static Bucket, table: &mut HashTable) {
    let mut current = bucket.queue_head.get();
    while let Some(current_ptr) = current {
        let current_ref = unsafe { current_ptr.as_ref() };
        let next = unsafe { current_ref.state() }.next_in_queue;
        let hash = hash(current_ref.key.load(Ordering::Relaxed), table.hash_bits);
        if let Some(tail) = table.entries[hash].queue_tail.get() {
            unsafe { tail.as_ref().state() }.next_in_queue = Some(current_ptr);
        } else {
            table.entries[hash].queue_head.set(Some(current_ptr));
        }
        table.entries[hash].queue_tail.set(Some(current_ptr));
        unsafe { current_ref.state() }.next_in_queue = None;
        current = next;
    }
}

// Hash function for addresses
#[cfg(target_pointer_width = "32")]
#[inline]
fn hash(key: usize, bits: u32) -> usize {
    key.wrapping_mul(0x9E3779B9) >> (32 - bits)
}
#[cfg(target_pointer_width = "64")]
#[inline]
fn hash(key: usize, bits: u32) -> usize {
    key.wrapping_mul(0x9E3779B97F4A7C15) >> (64 - bits)
}

/// Locks the bucket for the given key and returns a reference to it.
/// The returned bucket must be unlocked again in order to not cause deadlocks.
#[inline]
fn lock_bucket(key: usize) -> &'static Bucket {
    loop {
        let hashtable = get_hashtable();

        let hash = hash(key, hashtable.hash_bits);
        let bucket = &hashtable.entries[hash];

        // Lock the bucket
        bucket.mutex.lock();

        // If no other thread has rehashed the table before we grabbed the lock
        // then we are good to go! The lock we grabbed prevents any rehashes.
        if ptr::eq(HASHTABLE.load(Ordering::Relaxed), hashtable) {
            return bucket;
        }

        // Unlock the bucket and try again
        // SAFETY: We hold the lock here, as required
        unsafe { bucket.mutex.unlock() };
    }
}

/// Locks the bucket for the given key and returns a reference to it. But checks that the key
/// hasn't been changed in the meantime due to a requeue.
/// The returned bucket must be unlocked again in order to not cause deadlocks.
#[inline]
fn lock_bucket_checked(key: &AtomicUsize) -> (usize, &'static Bucket) {
    loop {
        let hashtable = get_hashtable();
        let current_key = key.load(Ordering::Relaxed);

        let hash = hash(current_key, hashtable.hash_bits);
        let bucket = &hashtable.entries[hash];

        // Lock the bucket
        bucket.mutex.lock();

        // Check that both the hash table and key are correct while the bucket
        // is locked. Note that the key can't change once we locked the proper
        // bucket for it, so we just keep trying until we have the correct key.
        if ptr::eq(HASHTABLE.load(Ordering::Relaxed), hashtable)
            && key.load(Ordering::Relaxed) == current_key
        {
            return (current_key, bucket);
        }

        // Unlock the bucket and try again
        // SAFETY: We hold the lock here, as required
        unsafe { bucket.mutex.unlock() };
    }
}

/// Locks the two buckets for the given pair of keys and returns references to them.
/// The returned buckets must be unlocked again in order to not cause deadlocks.
///
/// If both keys hash to the same value, both returned references will be to the same bucket. Be
/// careful to only unlock it once in this case, always use `unlock_bucket_pair`.
#[inline]
fn lock_bucket_pair(key1: usize, key2: usize) -> (&'static Bucket, &'static Bucket) {
    loop {
        let hashtable = get_hashtable();

        let hash1 = hash(key1, hashtable.hash_bits);
        let hash2 = hash(key2, hashtable.hash_bits);

        // Get the bucket at the lowest hash/index first
        let bucket1 = if hash1 <= hash2 {
            &hashtable.entries[hash1]
        } else {
            &hashtable.entries[hash2]
        };

        // Lock the first bucket
        bucket1.mutex.lock();

        // If no other thread has rehashed the table before we grabbed the lock
        // then we are good to go! The lock we grabbed prevents any rehashes.
        if ptr::eq(HASHTABLE.load(Ordering::Relaxed), hashtable) {
            // Now lock the second bucket and return the two buckets
            if hash1 == hash2 {
                return (bucket1, bucket1);
            } else if hash1 < hash2 {
                let bucket2 = &hashtable.entries[hash2];
                bucket2.mutex.lock();
                return (bucket1, bucket2);
            } else {
                let bucket2 = &hashtable.entries[hash1];
                bucket2.mutex.lock();
                return (bucket2, bucket1);
            }
        }

        // Unlock the bucket and try again
        // SAFETY: We hold the lock here, as required
        unsafe { bucket1.mutex.unlock() };
    }
}

/// Unlocks a pair of buckets.
///
/// # Safety
///
/// Both buckets must be locked.
#[inline]
unsafe fn unlock_bucket_pair(bucket1: &Bucket, bucket2: &Bucket) {
    unsafe { bucket1.mutex.unlock() };
    if !ptr::eq(bucket1, bucket2) {
        unsafe { bucket2.mutex.unlock() };
    }
}

/// Result of a park operation.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum ParkResult {
    /// We were unparked by another thread with the given token.
    Unparked(UnparkToken),

    /// The validation callback returned false.
    Invalid,

    /// The timeout expired.
    TimedOut,
}

impl ParkResult {
    /// Returns true if we were unparked by another thread.
    #[inline]
    pub const fn is_unparked(self) -> bool {
        matches!(self, ParkResult::Unparked(_))
    }
}

/// Result of an unpark operation.
#[derive(Copy, Clone, Default, Eq, PartialEq, Debug)]
#[non_exhaustive]
pub struct UnparkResult {
    /// The number of threads that were unparked.
    pub unparked_threads: usize,

    /// The number of threads that were requeued.
    pub requeued_threads: usize,

    /// Whether any threads remain parked with the original key after the
    /// operation.
    ///
    /// This is `false` if the operation was aborted before the queue was
    /// inspected.
    pub have_more_threads: bool,
}

/// Operation that `unpark_requeue` should perform.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum RequeueOp {
    /// Abort the operation without doing anything.
    Abort,

    /// Unpark one thread and requeue the rest onto the target queue.
    UnparkOneRequeueRest,

    /// Requeue all threads onto the target queue.
    RequeueAll,

    /// Unpark one thread and leave the rest parked. No requeuing is done.
    UnparkOne,

    /// Requeue one thread and leave the rest parked on the original queue.
    RequeueOne,
}

/// Operation that `unpark_filter` should perform for each thread.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum FilterOp {
    /// Unpark the thread and continue scanning the list of parked threads.
    Unpark,

    /// Don't unpark the thread and continue scanning the list of parked threads.
    Skip,

    /// Don't unpark the thread and stop scanning the list of parked threads.
    Stop,
}

/// A value which is passed from an unparker to a parked thread.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub struct UnparkToken(pub usize);

/// A value associated with a parked thread which can be used by `unpark_filter`.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub struct ParkToken(pub usize);

/// A default unpark token to use.
pub const DEFAULT_UNPARK_TOKEN: UnparkToken = UnparkToken(0);

/// A default park token to use.
pub const DEFAULT_PARK_TOKEN: ParkToken = ParkToken(0);

/// Parks the current thread in the queue associated with the given key.
///
/// The `validate` function is called while the queue is locked and can abort
/// the operation by returning false. If `validate` returns true then the
/// current thread is appended to the queue and the queue is unlocked.
///
/// The `before_sleep` function is called after the queue is unlocked but before
/// the thread is put to sleep. The thread will then sleep until it is unparked
/// or the given timeout is reached.
///
/// The `timed_out` function is also called while the queue is locked, but only
/// if the timeout was reached. It is passed the key of the queue it was in when
/// it timed out, which may be different from the original key if
/// `unpark_requeue` was called. It is also passed a bool which indicates
/// whether it was the last thread in the queue.
///
/// A timeout is the earliest point at which this function may return
/// [`ParkResult::TimedOut`]. Scheduling and platform-specific behavior may
/// delay the actual return.
///
/// # Safety
///
/// You should only call this function with an address that you control, since
/// you could otherwise interfere with the operation of other synchronization
/// primitives.
///
/// The `validate` and `timed_out` functions are called while the queue is
/// locked and must not unwind or call into any function in `parking_lot`.
///
/// The `before_sleep` function is called outside the queue lock and is allowed
/// to call `unpark_one`, `unpark_all`, `unpark_requeue` or `unpark_filter`, but
/// it is not allowed to call `park` or unwind.
///
/// The parking-lot functions are not reentrant. Calling this function from an
/// asynchronous signal handler may cause undefined behavior, including
/// internal-state corruption or deadlock.
#[inline]
pub unsafe fn park(
    key: usize,
    validate: impl FnOnce() -> bool,
    before_sleep: impl FnOnce(),
    timed_out: impl FnOnce(usize, bool),
    park_token: ParkToken,
    timeout: Option<Instant>,
) -> ParkResult {
    let abort_on_panic = AbortOnPanic;

    // Grab our thread data, this also ensures that the hash table exists
    let result = with_thread_data(|thread_data| {
        // Lock the bucket for the given key
        let bucket = lock_bucket(key);

        // If the validation function fails, just return
        if !validate() {
            // SAFETY: We hold the lock here, as required
            unsafe { bucket.mutex.unlock() };
            return ParkResult::Invalid;
        }

        // Append our thread data to the queue and unlock the bucket
        let thread_data_ptr = NonNull::from(thread_data);
        let state = unsafe { thread_data.state() };
        state.next_in_queue = None;
        state.park_token = park_token;
        #[cfg(feature = "deadlock_detection")]
        {
            state.deadlock_data.parked_with_timeout = timeout.is_some();
        }
        thread_data.key.store(key, Ordering::Relaxed);
        unsafe { thread_data.parker.prepare_park() };
        if let Some(tail) = bucket.queue_tail.get() {
            unsafe { tail.as_ref().state() }.next_in_queue = Some(thread_data_ptr);
        } else {
            bucket.queue_head.set(Some(thread_data_ptr));
        }
        bucket.queue_tail.set(Some(thread_data_ptr));
        // SAFETY: We hold the lock here, as required
        unsafe { bucket.mutex.unlock() };

        // Invoke the pre-sleep callback
        before_sleep();

        // Park our thread and determine whether we were woken up by an unpark
        // or by our timeout. Note that this isn't precise: we can still be
        // unparked since we are still in the queue.
        let unparked = match timeout {
            Some(timeout) => unsafe { thread_data.parker.park_until(timeout) },
            None => {
                unsafe { thread_data.parker.park() };
                // Handle a pending deadlock-detector backtrace request.
                unsafe { deadlock::on_unpark(thread_data) };
                true
            }
        };

        // If we were unparked, return now
        if unparked {
            return ParkResult::Unparked(unsafe { thread_data.state() }.unpark_token);
        }

        // Lock our bucket again. Note that the hashtable may have been rehashed in
        // the meantime. Our key may also have changed if we were requeued.
        let (key, bucket) = lock_bucket_checked(&thread_data.key);

        // Now we need to check again if we were unparked or timed out. Unlike the
        // last check this is precise because we hold the bucket lock.
        if !unsafe { thread_data.parker.timed_out() } {
            // SAFETY: We hold the lock here, as required
            unsafe { bucket.mutex.unlock() };
            return ParkResult::Unparked(unsafe { thread_data.state() }.unpark_token);
        }

        // We timed out, so we now need to remove our thread from the queue
        let mut current = bucket.queue_head.get();
        let mut previous: Option<NonNull<ThreadData>> = None;
        let mut was_last_thread = true;
        while let Some(current_ptr) = current {
            let current_ref = unsafe { current_ptr.as_ref() };
            let next = unsafe { current_ref.state() }.next_in_queue;
            if current_ptr == thread_data_ptr {
                if let Some(previous) = previous {
                    unsafe { previous.as_ref().state() }.next_in_queue = next;
                } else {
                    bucket.queue_head.set(next);
                }
                if bucket.queue_tail.get() == Some(current_ptr) {
                    bucket.queue_tail.set(previous);
                } else {
                    // Scan the rest of the queue to see if there are any other
                    // entries with the given key.
                    let mut scan = next;
                    while let Some(scan_ptr) = scan {
                        let scan_ref = unsafe { scan_ptr.as_ref() };
                        if scan_ref.key.load(Ordering::Relaxed) == key {
                            was_last_thread = false;
                            break;
                        }
                        scan = unsafe { scan_ref.state() }.next_in_queue;
                    }
                }

                // Callback to indicate that we timed out, and whether we were the
                // last thread on the queue.
                timed_out(key, was_last_thread);
                break;
            } else {
                if current_ref.key.load(Ordering::Relaxed) == key {
                    was_last_thread = false;
                }
                previous = Some(current_ptr);
                current = next;
            }
        }

        // There should be no way for our thread to have been removed from the queue
        // if we timed out.
        debug_assert!(current.is_some());

        // Unlock the bucket, we are done
        // SAFETY: We hold the lock here, as required
        unsafe { bucket.mutex.unlock() };
        ParkResult::TimedOut
    });

    abort_on_panic.disarm();
    result
}

/// Unparks one thread from the queue associated with the given key.
///
/// The `callback` function is called while the queue is locked and before the
/// target thread is woken up. The `UnparkResult` argument to the function
/// indicates whether a thread was found in the queue and whether this was the
/// last thread in the queue. This value is also returned by `unpark_one`.
///
/// The `callback` function should return an `UnparkToken` value which will be
/// passed to the thread that is unparked. If no thread is unparked then the
/// returned value is ignored.
///
/// # Safety
///
/// You should only call this function with an address that you control, since
/// you could otherwise interfere with the operation of other synchronization
/// primitives.
///
/// The `callback` function is called while the queue is locked and must not
/// unwind or call into any function in `parking_lot`.
///
/// The parking-lot functions are not reentrant. Calling this function from an
/// asynchronous signal handler may cause undefined behavior, including
/// internal-state corruption or deadlock.
#[inline]
pub unsafe fn unpark_one(
    key: usize,
    callback: impl FnOnce(UnparkResult) -> UnparkToken,
) -> UnparkResult {
    let abort_on_panic = AbortOnPanic;

    // Lock the bucket for the given key
    let bucket = lock_bucket(key);

    // Find a thread with a matching key and remove it from the queue
    let mut current = bucket.queue_head.get();
    let mut previous: Option<NonNull<ThreadData>> = None;
    let mut result = UnparkResult::default();
    while let Some(current_ptr) = current {
        let current_ref = unsafe { current_ptr.as_ref() };
        let next = unsafe { current_ref.state() }.next_in_queue;
        if current_ref.key.load(Ordering::Relaxed) == key {
            // Remove the thread from the queue
            if let Some(previous) = previous {
                unsafe { previous.as_ref().state() }.next_in_queue = next;
            } else {
                bucket.queue_head.set(next);
            }
            if bucket.queue_tail.get() == Some(current_ptr) {
                bucket.queue_tail.set(previous);
            } else {
                // Scan the rest of the queue to see if there are any other
                // entries with the given key.
                let mut scan = next;
                while let Some(scan_ptr) = scan {
                    let scan_ref = unsafe { scan_ptr.as_ref() };
                    if scan_ref.key.load(Ordering::Relaxed) == key {
                        result.have_more_threads = true;
                        break;
                    }
                    scan = unsafe { scan_ref.state() }.next_in_queue;
                }
            }

            // Invoke the callback before waking up the thread
            result.unparked_threads = 1;
            let token = callback(result);

            // Set the token for the target thread
            unsafe { current_ref.state() }.unpark_token = token;

            // Commit the unpark before unlocking the queue. The target may now
            // destroy its ThreadData, but the handle remains valid for the
            // deferred wake.
            let handle = unsafe { current_ref.parker.unpark_lock() };
            // SAFETY: We hold the lock here, as required
            unsafe { bucket.mutex.unlock() };
            unsafe { handle.unpark() };

            abort_on_panic.disarm();
            return result;
        } else {
            previous = Some(current_ptr);
            current = next;
        }
    }

    // No threads with a matching key were found in the bucket
    callback(result);
    // SAFETY: We hold the lock here, as required
    unsafe { bucket.mutex.unlock() };

    abort_on_panic.disarm();
    result
}

/// Unparks all threads in the queue associated with the given key.
///
/// The given `UnparkToken` is passed to all unparked threads.
///
/// This function returns the number of threads that were unparked.
///
/// # Safety
///
/// You should only call this function with an address that you control, since
/// you could otherwise interfere with the operation of other synchronization
/// primitives.
///
/// The parking-lot functions are not reentrant. Calling this function from an
/// asynchronous signal handler may cause undefined behavior, including
/// internal-state corruption or deadlock.
#[inline]
pub unsafe fn unpark_all(key: usize, unpark_token: UnparkToken) -> usize {
    let abort_on_panic = AbortOnPanic;

    // Lock the bucket for the given key
    let bucket = lock_bucket(key);

    // Remove all threads with the given key in the bucket
    let mut current = bucket.queue_head.get();
    let mut previous: Option<NonNull<ThreadData>> = None;
    let mut threads = SmallVec::<[_; 8]>::new();
    while let Some(current_ptr) = current {
        let current_ref = unsafe { current_ptr.as_ref() };
        let next = unsafe { current_ref.state() }.next_in_queue;
        if current_ref.key.load(Ordering::Relaxed) == key {
            // Remove the thread from the queue
            if let Some(previous) = previous {
                unsafe { previous.as_ref().state() }.next_in_queue = next;
            } else {
                bucket.queue_head.set(next);
            }
            if bucket.queue_tail.get() == Some(current_ptr) {
                bucket.queue_tail.set(previous);
            }

            // Set the token for the target thread
            unsafe { current_ref.state() }.unpark_token = unpark_token;

            // Commit the unpark now and defer the wake until the queue is unlocked.
            threads.push(unsafe { current_ref.parker.unpark_lock() });
            current = next;
        } else {
            previous = Some(current_ptr);
            current = next;
        }
    }

    // Unlock the bucket
    // SAFETY: We hold the lock here, as required
    unsafe { bucket.mutex.unlock() };

    // Now that we are outside the lock, wake up all the threads that we removed
    // from the queue.
    let num_threads = threads.len();
    for handle in threads.into_iter() {
        unsafe { handle.unpark() };
    }

    abort_on_panic.disarm();
    num_threads
}

/// Removes all threads from the queue associated with `key_from`, optionally
/// unparks the first one and requeues the rest onto the queue associated with
/// `key_to`.
///
/// The `validate` function is called while both queues are locked. Its return
/// value will determine which operation is performed, or whether the operation
/// should be aborted. See `RequeueOp` for details about the different possible
/// return values.
///
/// If `validate` returns [`RequeueOp::Abort`], the queues are left unchanged,
/// `callback` is not called, and a default [`UnparkResult`] is returned.
/// Otherwise, `callback` is called while both queues are locked. It is passed
/// the `RequeueOp` returned by `validate` and an `UnparkResult` indicating
/// whether a thread was unparked and whether threads remain parked with
/// `key_from`. Threads requeued to `key_to` are not included. This
/// `UnparkResult` value is also returned by `unpark_requeue`.
///
/// The `callback` function should return an `UnparkToken` value which will be
/// passed to the thread that is unparked. If no thread is unparked then the
/// returned value is ignored.
///
/// # Safety
///
/// You should only call this function with an address that you control, since
/// you could otherwise interfere with the operation of other synchronization
/// primitives.
///
/// The `validate` and `callback` functions are called while both queues are locked
/// and must not unwind or call into any function in `parking_lot`.
///
/// The parking-lot functions are not reentrant. Calling this function from an
/// asynchronous signal handler may cause undefined behavior, including
/// internal-state corruption or deadlock.
#[inline]
pub unsafe fn unpark_requeue(
    key_from: usize,
    key_to: usize,
    validate: impl FnOnce() -> RequeueOp,
    callback: impl FnOnce(RequeueOp, UnparkResult) -> UnparkToken,
) -> UnparkResult {
    let abort_on_panic = AbortOnPanic;

    // Lock the two buckets for the given key
    let (bucket_from, bucket_to) = lock_bucket_pair(key_from, key_to);

    // If the validation function fails, just return
    let mut result = UnparkResult::default();
    let op = validate();
    if op == RequeueOp::Abort {
        // SAFETY: Both buckets are locked, as required.
        unsafe { unlock_bucket_pair(bucket_from, bucket_to) };
        abort_on_panic.disarm();
        return result;
    }

    // Remove all threads with the given key in the source bucket
    let mut current = bucket_from.queue_head.get();
    let mut previous: Option<NonNull<ThreadData>> = None;
    let mut requeue_threads: Option<NonNull<ThreadData>> = None;
    let mut requeue_threads_tail: Option<NonNull<ThreadData>> = None;
    let mut wakeup_thread = None;
    while let Some(current_ptr) = current {
        let current_ref = unsafe { current_ptr.as_ref() };
        let next = unsafe { current_ref.state() }.next_in_queue;
        if current_ref.key.load(Ordering::Relaxed) == key_from {
            // Remove the thread from the queue
            if let Some(previous) = previous {
                unsafe { previous.as_ref().state() }.next_in_queue = next;
            } else {
                bucket_from.queue_head.set(next);
            }
            if bucket_from.queue_tail.get() == Some(current_ptr) {
                bucket_from.queue_tail.set(previous);
            }

            // Prepare the first thread for wakeup and requeue the rest.
            if (op == RequeueOp::UnparkOneRequeueRest || op == RequeueOp::UnparkOne)
                && wakeup_thread.is_none()
            {
                wakeup_thread = Some(current_ptr);
                result.unparked_threads = 1;
            } else {
                if let Some(tail) = requeue_threads_tail {
                    unsafe { tail.as_ref().state() }.next_in_queue = Some(current_ptr);
                } else {
                    requeue_threads = Some(current_ptr);
                }
                requeue_threads_tail = Some(current_ptr);
                current_ref.key.store(key_to, Ordering::Relaxed);
                result.requeued_threads += 1;
            }
            if op == RequeueOp::UnparkOne || op == RequeueOp::RequeueOne {
                // Scan the rest of the queue to see if there are any other
                // entries with the given key.
                let mut scan = next;
                while let Some(scan_ptr) = scan {
                    let scan_ref = unsafe { scan_ptr.as_ref() };
                    if scan_ref.key.load(Ordering::Relaxed) == key_from {
                        result.have_more_threads = true;
                        break;
                    }
                    scan = unsafe { scan_ref.state() }.next_in_queue;
                }
                break;
            }
            current = next;
        } else {
            previous = Some(current_ptr);
            current = next;
        }
    }

    // Add the requeued threads to the destination bucket
    if let Some(requeue_threads) = requeue_threads {
        let requeue_threads_tail = requeue_threads_tail.unwrap();
        unsafe { requeue_threads_tail.as_ref().state() }.next_in_queue = None;
        if let Some(tail) = bucket_to.queue_tail.get() {
            unsafe { tail.as_ref().state() }.next_in_queue = Some(requeue_threads);
        } else {
            bucket_to.queue_head.set(Some(requeue_threads));
        }
        bucket_to.queue_tail.set(Some(requeue_threads_tail));
    }

    // Invoke the callback before waking up the thread
    let token = callback(op, result);

    // Commit the unpark before unlocking the queues, then perform the wake.
    if let Some(wakeup_thread) = wakeup_thread {
        let wakeup_thread = unsafe { wakeup_thread.as_ref() };
        unsafe { wakeup_thread.state() }.unpark_token = token;
        let handle = unsafe { wakeup_thread.parker.unpark_lock() };
        // SAFETY: Both buckets are locked, as required.
        unsafe { unlock_bucket_pair(bucket_from, bucket_to) };
        unsafe { handle.unpark() };
    } else {
        // SAFETY: Both buckets are locked, as required.
        unsafe { unlock_bucket_pair(bucket_from, bucket_to) };
    }

    abort_on_panic.disarm();
    result
}

/// Unparks a number of threads from the front of the queue associated with
/// `key` depending on the results of a filter function which inspects the
/// `ParkToken` associated with each thread.
///
/// The `filter` function is called for each thread in the queue or until
/// `FilterOp::Stop` is returned. This function is passed the `ParkToken`
/// associated with a particular thread, which is unparked if `FilterOp::Unpark`
/// is returned.
///
/// The `callback` function is also called while the queue is locked. It is
/// passed an `UnparkResult` indicating the number of threads that were unparked
/// and whether there are still parked threads in the queue. This `UnparkResult`
/// value is also returned by `unpark_filter`.
///
/// The `callback` function should return an `UnparkToken` value which will be
/// passed to all threads that are unparked. If no thread is unparked then the
/// returned value is ignored.
///
/// # Safety
///
/// You should only call this function with an address that you control, since
/// you could otherwise interfere with the operation of other synchronization
/// primitives.
///
/// The `filter` and `callback` functions are called while the queue is locked
/// and must not unwind or call into any function in `parking_lot`.
///
/// The parking-lot functions are not reentrant. Calling this function from an
/// asynchronous signal handler may cause undefined behavior, including
/// internal-state corruption or deadlock.
#[inline]
pub unsafe fn unpark_filter(
    key: usize,
    filter: impl FnMut(ParkToken) -> FilterOp,
    callback: impl FnOnce(UnparkResult) -> UnparkToken,
) -> UnparkResult {
    let abort_on_panic = AbortOnPanic;
    let mut filter = filter;

    // Lock the bucket for the given key
    let bucket = lock_bucket(key);

    // Go through the queue looking for threads with a matching key
    let mut current = bucket.queue_head.get();
    let mut previous: Option<NonNull<ThreadData>> = None;
    let mut threads = SmallVec::<[_; 8]>::new();
    let mut result = UnparkResult::default();
    while let Some(current_ptr) = current {
        let current_ref = unsafe { current_ptr.as_ref() };
        let next = unsafe { current_ref.state() }.next_in_queue;
        if current_ref.key.load(Ordering::Relaxed) == key {
            // Call the filter function with the thread's ParkToken
            match filter(unsafe { current_ref.state() }.park_token) {
                FilterOp::Unpark => {
                    // Remove the thread from the queue
                    if let Some(previous) = previous {
                        unsafe { previous.as_ref().state() }.next_in_queue = next;
                    } else {
                        bucket.queue_head.set(next);
                    }
                    if bucket.queue_tail.get() == Some(current_ptr) {
                        bucket.queue_tail.set(previous);
                    }

                    // Add the thread to our list of threads to unpark
                    threads.push((current_ptr, None));

                    current = next;
                }
                FilterOp::Skip => {
                    result.have_more_threads = true;
                    previous = Some(current_ptr);
                    current = next;
                }
                FilterOp::Stop => {
                    result.have_more_threads = true;
                    break;
                }
            }
        } else {
            previous = Some(current_ptr);
            current = next;
        }
    }

    // Invoke the callback before waking up the threads
    result.unparked_threads = threads.len();
    let token = callback(result);

    // Pass the token to all threads that are going to be unparked and prepare
    // them for unparking.
    for t in threads.iter_mut() {
        let thread = unsafe { t.0.as_ref() };
        unsafe { thread.state() }.unpark_token = token;
        t.1 = Some(unsafe { thread.parker.unpark_lock() });
    }

    // SAFETY: We hold the lock here, as required
    unsafe { bucket.mutex.unlock() };

    // Now that we are outside the lock, wake up all the threads that we removed
    // from the queue.
    for (_, handle) in threads.into_iter() {
        unsafe { handle.unwrap_unchecked().unpark() };
    }

    abort_on_panic.disarm();
    result
}

/// \[Experimental\] Deadlock detection
///
/// Enabled via the `deadlock_detection` feature flag.
pub mod deadlock {
    #[cfg(feature = "deadlock_detection")]
    use super::deadlock_impl;

    #[cfg(feature = "deadlock_detection")]
    pub(super) use super::deadlock_impl::DeadlockData;

    /// Records that the current thread owns the resource identified by `key`.
    ///
    /// This is a no-op if the `deadlock_detection` feature is not enabled.
    /// Each recorded acquisition should be paired with a call to
    /// [`release_resource`] using the same key. The same key may be recorded
    /// more than once if a resource is acquired repeatedly by the same thread.
    /// Call this after the resource has been acquired.
    ///
    /// # Safety
    ///
    /// This must not be called from a callback passed to a parking-lot
    /// function.
    #[inline]
    pub unsafe fn acquire_resource(_key: usize) {
        let abort_on_panic = super::AbortOnPanic;

        #[cfg(feature = "deadlock_detection")]
        unsafe {
            deadlock_impl::acquire_resource(_key);
        }

        abort_on_panic.disarm();
    }

    /// Stops recording one acquisition of the resource identified by `key` by
    /// the current thread.
    ///
    /// This is a no-op if the `deadlock_detection` feature is not enabled.
    /// The key should match an acquisition previously recorded by this thread.
    /// The bookkeeping may be removed before the underlying resource is
    /// physically released.
    ///
    /// # Safety
    ///
    /// This must not be called from a callback passed to a parking-lot
    /// function.
    #[inline]
    pub unsafe fn release_resource(_key: usize) {
        let abort_on_panic = super::AbortOnPanic;

        #[cfg(feature = "deadlock_detection")]
        unsafe {
            deadlock_impl::release_resource(_key);
        }

        abort_on_panic.disarm();
    }

    /// Detects and returns all currently deadlocked thread components.
    ///
    /// Each inner vector contains the threads in one mutually deadlocked
    /// component.
    ///
    /// Waits with a deadline are excluded because they can resolve by timing
    /// out.
    ///
    /// Reporting a component removes its threads from the parking queues. Each
    /// thread captures its backtrace and then remains blocked permanently.
    #[cfg(feature = "deadlock_detection")]
    #[inline]
    pub fn check_deadlock() -> Vec<Vec<deadlock_impl::DeadlockedThread>> {
        let abort_on_panic = super::AbortOnPanic;
        let result = deadlock_impl::check_deadlock();
        abort_on_panic.disarm();
        result
    }

    #[inline]
    pub(super) unsafe fn on_unpark(_td: &super::ThreadData) {
        let abort_on_panic = super::AbortOnPanic;

        #[cfg(feature = "deadlock_detection")]
        unsafe {
            deadlock_impl::on_unpark(_td);
        }

        abort_on_panic.disarm();
    }
}

#[cfg(feature = "deadlock_detection")]
mod deadlock_impl {
    use super::{HASHTABLE, NUM_THREADS, ThreadData, get_hashtable, with_thread_data};
    use crate::thread_parker::{ThreadParkerT, UnparkHandleT};
    use crate::word_lock::WordLock;
    use backtrace::Backtrace;
    use petgraph::graphmap::DiGraphMap;
    use std::collections::HashMap;
    use std::ptr::NonNull;
    use std::sync::atomic::Ordering;
    use std::sync::mpsc;
    use std::thread::ThreadId;

    /// Representation of a deadlocked thread.
    pub struct DeadlockedThread {
        thread_id: ThreadId,
        backtrace: Backtrace,
    }

    impl DeadlockedThread {
        /// The identifier of the deadlocked thread.
        pub fn thread_id(&self) -> ThreadId {
            self.thread_id
        }

        /// The backtrace captured on the deadlocked thread.
        pub fn backtrace(&self) -> &Backtrace {
            &self.backtrace
        }
    }

    pub struct DeadlockData {
        // Resource keys currently recorded as owned by this thread.
        resources: Vec<usize>,

        // Set before linking to indicate whether the current wait has a
        // deadline, then read by the detector under the bucket lock.
        pub(super) parked_with_timeout: bool,

        // Installed by the detector while the ThreadData is owned by the
        // parking lot and taken locally only after the parker returns.
        deadlock_backtrace: Option<mpsc::Sender<DeadlockedThread>>,

        // Immutable after construction.
        thread_id: ThreadId,
    }

    impl DeadlockData {
        pub fn new() -> Self {
            DeadlockData {
                resources: Vec::new(),
                parked_with_timeout: false,
                deadlock_backtrace: None,
                thread_id: std::thread::current().id(),
            }
        }
    }

    pub(super) unsafe fn on_unpark(td: &ThreadData) {
        let state = unsafe { td.state() };
        if let Some(sender) = state.deadlock_data.deadlock_backtrace.take() {
            sender
                .send(DeadlockedThread {
                    thread_id: state.deadlock_data.thread_id,
                    backtrace: Backtrace::new(),
                })
                .unwrap();
            // Drop the sender before parking permanently so that the detector
            // observes the channel closing.
            drop(sender);

            // This ThreadData has been removed from its queue and cannot be
            // reached by another parking-lot wake operation. Park permanently
            // after reporting the backtrace.
            unsafe { td.parker.prepare_park() };
            unsafe { td.parker.park() };
            unreachable!("unparked deadlocked thread!");
        }
    }

    pub unsafe fn acquire_resource(key: usize) {
        with_thread_data(|thread_data| {
            unsafe { thread_data.state() }
                .deadlock_data
                .resources
                .push(key);
        });
    }

    pub unsafe fn release_resource(key: usize) {
        with_thread_data(|thread_data| {
            let resources = &mut unsafe { thread_data.state() }.deadlock_data.resources;

            // During TLS destruction the original ThreadData may no longer be
            // accessible, in which case with_thread_data supplies a temporary
            // empty ThreadData. There is no bookkeeping left to update.
            if let Some(p) = resources.iter().rposition(|x| *x == key) {
                resources.swap_remove(p);
            }
        });
    }

    pub fn check_deadlock() -> Vec<Vec<DeadlockedThread>> {
        // First use an approximate snapshot to avoid the precise scan in the
        // common case where no cycle is observed.
        if unsafe { check_wait_graph_fast() } {
            // Verify the result using a consistent snapshot.
            unsafe { check_wait_graph_slow() }
        } else {
            Vec::new()
        }
    }

    // Build an approximate wait graph while locking one bucket at a time. The
    // resulting snapshot may combine states from different points in time, so
    // a cycle only triggers the precise slow check.
    unsafe fn check_wait_graph_fast() -> bool {
        'retry: loop {
            let table = get_hashtable();
            let thread_count = NUM_THREADS.load(Ordering::Relaxed);
            let mut graph =
                DiGraphMap::<usize, ()>::with_capacity(thread_count * 2, thread_count * 2);

            for b in &table.entries[..] {
                b.mutex.lock();

                // Hash-table growth may have moved this bucket's queue while
                // we were waiting for its lock. Restart with the current table
                // rather than traversing queue links no longer protected by
                // this bucket.
                if !core::ptr::eq(HASHTABLE.load(Ordering::Relaxed), table) {
                    unsafe { b.mutex.unlock() };
                    continue 'retry;
                }

                let mut current = b.queue_head.get();
                while let Some(current_ptr) = current {
                    let thread_data = unsafe { current_ptr.as_ref() };
                    let state = unsafe { thread_data.state() };
                    // Exclude timed waits because they can eventually resolve
                    // without being unparked by another thread.
                    if !state.deadlock_data.parked_with_timeout {
                        let thread = current_ptr.as_ptr().addr();
                        // Each owned resource points to its owning thread.
                        for &resource in &state.deadlock_data.resources {
                            graph.add_edge(resource, thread, ());
                        }
                        // The thread points to the queue key on which it is parked.
                        graph.add_edge(thread, thread_data.key.load(Ordering::Relaxed), ());
                    }
                    current = state.next_in_queue;
                }
                // SAFETY: We hold the lock here, as required
                unsafe { b.mutex.unlock() };
            }

            return petgraph::algo::is_cyclic_directed(&graph);
        }
    }

    #[derive(Hash, PartialEq, Eq, PartialOrd, Ord, Copy, Clone)]
    enum WaitGraphNode {
        Thread(NonNull<ThreadData>),
        Resource(usize),
    }

    use self::WaitGraphNode::*;

    // Unlike the fast check, this locks every bucket while constructing the
    // wait graph and selecting deadlocked components. Reported threads are
    // removed from their queues before the buckets are unlocked.
    unsafe fn check_wait_graph_slow() -> Vec<Vec<DeadlockedThread>> {
        // Reporting mutates the global queues, so only one slow check may run
        // at a time.
        static DEADLOCK_DETECTION_LOCK: WordLock = WordLock::new();
        DEADLOCK_DETECTION_LOCK.lock();

        let mut table = get_hashtable();
        loop {
            // Lock all buckets in the candidate table.
            for b in &table.entries[..] {
                b.mutex.lock();
            }

            // Another thread may have replaced the hash table while its
            // buckets were being locked. In that case, retry with the current
            // table.
            let new_table = get_hashtable();
            if core::ptr::eq(new_table, table) {
                break;
            }

            // Unlock the candidate table's buckets and try again.
            for b in &table.entries[..] {
                // SAFETY: We hold the lock here, as required
                unsafe { b.mutex.unlock() };
            }

            table = new_table;
        }

        let thread_count = NUM_THREADS.load(Ordering::Relaxed);
        let mut graph =
            DiGraphMap::<WaitGraphNode, ()>::with_capacity(thread_count * 2, thread_count * 2);

        for b in &table.entries[..] {
            let mut current = b.queue_head.get();
            while let Some(current_ptr) = current {
                let thread_data = unsafe { current_ptr.as_ref() };
                let state = unsafe { thread_data.state() };
                // Exclude timed waits because they can eventually resolve
                // without being unparked by another thread.
                if !state.deadlock_data.parked_with_timeout {
                    // Each owned resource points to its owning thread.
                    for &resource in &state.deadlock_data.resources {
                        graph.add_edge(Resource(resource), Thread(current_ptr), ());
                    }
                    // The thread points to the queue key on which it is parked.
                    graph.add_edge(
                        Thread(current_ptr),
                        Resource(thread_data.key.load(Ordering::Relaxed)),
                        (),
                    );
                }
                current = state.next_in_queue;
            }
        }

        let components = graph_components(&graph);
        let deadlocked_thread_count = components.iter().map(Vec::len).sum();
        let mut senders = HashMap::with_capacity(deadlocked_thread_count);
        let mut receivers = Vec::with_capacity(components.len());
        // Use one channel per component, with one sender stored in each thread
        // that must report a backtrace.
        for component in components {
            let (sender, receiver) = mpsc::channel();
            for thread in component {
                let previous = senders.insert(thread, sender.clone());
                debug_assert!(previous.is_none());
            }
            // Drop the detector's sender so only the per-thread copies keep
            // the channel open.
            drop(sender);
            receivers.push(receiver);
        }

        // The components contain raw ThreadData pointers. Keeping every bucket
        // locked preserves each queued ThreadData until it has been unlinked
        // and passed to unpark_lock.
        let mut handles = Vec::with_capacity(deadlocked_thread_count);
        for bucket in &table.entries[..] {
            let mut current = bucket.queue_head.get();
            let mut previous: Option<NonNull<ThreadData>> = None;
            while let Some(current_ptr) = current {
                let thread_data = unsafe { current_ptr.as_ref() };
                let state = unsafe { thread_data.state() };
                let next = state.next_in_queue;
                if let Some(sender) = senders.remove(&current_ptr) {
                    if let Some(previous) = previous {
                        unsafe { previous.as_ref().state() }.next_in_queue = next;
                    } else {
                        bucket.queue_head.set(next);
                    }
                    if bucket.queue_tail.get() == Some(current_ptr) {
                        bucket.queue_tail.set(previous);
                    }

                    // The reporting thread parks permanently after capturing
                    // its backtrace, so it must no longer be visible to normal
                    // queue operations.
                    state.next_in_queue = None;
                    state.deadlock_data.deadlock_backtrace = Some(sender);
                    handles.push(unsafe { thread_data.parker.unpark_lock() });
                } else {
                    previous = Some(current_ptr);
                }
                current = next;
            }
        }
        debug_assert!(senders.is_empty());

        for b in &table.entries[..] {
            // SAFETY: We hold the lock here, as required
            unsafe { b.mutex.unlock() };
        }

        // Platform wake operations may block, so perform them only after all
        // bucket locks have been released.
        for handle in handles {
            unsafe { handle.unpark() };
        }

        // Collect until every per-thread sender for the component is dropped.
        let results = receivers
            .into_iter()
            .map(|receiver| receiver.iter().collect())
            .collect();

        unsafe { DEADLOCK_DETECTION_LOCK.unlock() };

        results
    }

    // Returns the thread nodes in each nontrivial strongly connected component.
    fn graph_components(g: &DiGraphMap<WaitGraphNode, ()>) -> Vec<Vec<NonNull<ThreadData>>> {
        petgraph::algo::kosaraju_scc(g)
            .into_iter()
            .filter(|component| component.len() > 1)
            .filter_map(|component| {
                let threads = component
                    .into_iter()
                    .filter_map(|node| match node {
                        Thread(thread) => Some(thread),
                        Resource(_) => None,
                    })
                    .collect::<Vec<_>>();
                (!threads.is_empty()).then_some(threads)
            })
            .collect()
    }

    #[cfg(test)]
    mod tests {
        use super::*;

        #[test]
        fn overlapping_cycles_form_one_component() {
            let storage = [0u8; 3];
            let thread1 = NonNull::from(&storage[0]).cast::<ThreadData>();
            let thread2 = NonNull::from(&storage[1]).cast::<ThreadData>();
            let thread3 = NonNull::from(&storage[2]).cast::<ThreadData>();
            let mut graph = DiGraphMap::new();

            graph.add_edge(Thread(thread1), Resource(1), ());
            graph.add_edge(Resource(1), Thread(thread2), ());
            graph.add_edge(Thread(thread2), Resource(2), ());
            graph.add_edge(Resource(2), Thread(thread1), ());

            graph.add_edge(Thread(thread2), Resource(3), ());
            graph.add_edge(Resource(3), Thread(thread3), ());
            graph.add_edge(Thread(thread3), Resource(4), ());
            graph.add_edge(Resource(4), Thread(thread2), ());

            let mut components = graph_components(&graph);
            assert_eq!(components.len(), 1);
            components[0].sort_unstable();

            let mut expected = vec![thread1, thread2, thread3];
            expected.sort_unstable();
            assert_eq!(components[0], expected);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{DEFAULT_PARK_TOKEN, DEFAULT_UNPARK_TOKEN, ThreadData};
    use std::{
        ptr,
        sync::{
            Arc,
            atomic::{AtomicIsize, AtomicPtr, AtomicUsize, Ordering},
        },
        thread,
        time::Duration,
    };

    /// Calls a closure for every `ThreadData` currently parked on a given key
    fn for_each(key: usize, mut f: impl FnMut(&ThreadData)) {
        let bucket = super::lock_bucket(key);

        let mut current = bucket.queue_head.get();
        while let Some(current_ptr) = current {
            let current_ref = unsafe { current_ptr.as_ref() };
            if current_ref.key.load(Ordering::Relaxed) == key {
                f(current_ref);
            }
            current = unsafe { current_ref.state() }.next_in_queue;
        }

        // SAFETY: We hold the lock here, as required
        unsafe { bucket.mutex.unlock() };
    }

    macro_rules! test {
        ( $( $name:ident(
            repeats: $repeats:expr,
            latches: $latches:expr,
            delay: $delay:expr,
            threads: $threads:expr,
            single_unparks: $single_unparks:expr);
        )* ) => {
            $(#[test]
            fn $name() {
                let delay = Duration::from_micros($delay);
                for _ in 0..$repeats {
                    run_parking_test($latches, delay, $threads, $single_unparks);
                }
            })*
        };
    }

    test! {
        unpark_all_one_fast(
            repeats: 1000, latches: 1, delay: 0, threads: 1, single_unparks: 0
        );
        unpark_all_hundred_fast(
            repeats: 100, latches: 1, delay: 0, threads: 100, single_unparks: 0
        );
        unpark_one_one_fast(
            repeats: 1000, latches: 1, delay: 0, threads: 1, single_unparks: 1
        );
        unpark_one_hundred_fast(
            repeats: 20, latches: 1, delay: 0, threads: 100, single_unparks: 100
        );
        unpark_one_fifty_then_fifty_all_fast(
            repeats: 50, latches: 1, delay: 0, threads: 100, single_unparks: 50
        );
        unpark_all_one(
            repeats: 100, latches: 1, delay: 10000, threads: 1, single_unparks: 0
        );
        unpark_all_hundred(
            repeats: 100, latches: 1, delay: 10000, threads: 100, single_unparks: 0
        );
        unpark_one_one(
            repeats: 10, latches: 1, delay: 10000, threads: 1, single_unparks: 1
        );
        unpark_one_fifty(
            repeats: 1, latches: 1, delay: 10000, threads: 50, single_unparks: 50
        );
        unpark_one_fifty_then_fifty_all(
            repeats: 2, latches: 1, delay: 10000, threads: 100, single_unparks: 50
        );
        hundred_unpark_all_one_fast(
            repeats: 100, latches: 100, delay: 0, threads: 1, single_unparks: 0
        );
        hundred_unpark_all_one(
            repeats: 1, latches: 100, delay: 10000, threads: 1, single_unparks: 0
        );
    }

    fn run_parking_test(
        num_latches: usize,
        delay: Duration,
        num_threads: usize,
        num_single_unparks: usize,
    ) {
        let mut tests = Vec::with_capacity(num_latches);

        for _ in 0..num_latches {
            let test = Arc::new(SingleLatchTest::new(num_threads));
            let mut threads = Vec::with_capacity(num_threads);
            for _ in 0..num_threads {
                let test = test.clone();
                threads.push(thread::spawn(move || test.run()));
            }
            tests.push((test, threads));
        }

        for unpark_index in 0..num_single_unparks {
            thread::sleep(delay);
            for (test, _) in &tests {
                test.unpark_one(unpark_index);
            }
        }

        for (test, threads) in tests {
            test.finish(num_single_unparks);
            for thread in threads {
                thread.join().expect("Test thread panic");
            }
        }
    }

    struct SingleLatchTest {
        semaphore: AtomicIsize,
        num_awake: AtomicUsize,
        /// Holds the pointer to the last *unprocessed* woken up thread.
        last_awoken: AtomicPtr<ThreadData>,
        /// Total number of threads participating in this test.
        num_threads: usize,
    }

    impl SingleLatchTest {
        pub fn new(num_threads: usize) -> Self {
            Self {
                // This implements a fair (FIFO) semaphore, and it starts out unavailable.
                semaphore: AtomicIsize::new(0),
                num_awake: AtomicUsize::new(0),
                last_awoken: AtomicPtr::new(ptr::null_mut()),
                num_threads,
            }
        }

        pub fn run(&self) {
            // Get one slot from the semaphore
            self.down();

            // Report back to the test verification code that this thread woke up
            let this_thread_ptr = super::with_thread_data(|t| t as *const _ as *mut _);
            self.last_awoken.store(this_thread_ptr, Ordering::SeqCst);
            self.num_awake.fetch_add(1, Ordering::SeqCst);
        }

        pub fn unpark_one(&self, single_unpark_index: usize) {
            // last_awoken should be null at all times except between self.up() and at the bottom
            // of this method where it's reset to null again
            assert!(self.last_awoken.load(Ordering::SeqCst).is_null());

            let mut queue: Vec<*mut ThreadData> = Vec::with_capacity(self.num_threads);
            for_each(self.semaphore_addr(), |thread_data| {
                queue.push(thread_data as *const _ as *mut _);
            });
            assert!(queue.len() <= self.num_threads - single_unpark_index);

            let num_awake_before_up = self.num_awake.load(Ordering::SeqCst);

            self.up();

            // Wait for a parked thread to wake up and update num_awake + last_awoken.
            while self.num_awake.load(Ordering::SeqCst) != num_awake_before_up + 1 {
                thread::yield_now();
            }

            // At this point the other thread should have set last_awoken inside the run() method
            let last_awoken = self.last_awoken.load(Ordering::SeqCst);
            assert!(!last_awoken.is_null());
            if !queue.is_empty() && queue[0] != last_awoken {
                panic!(
                    "Woke up wrong thread:\n\tqueue: {:?}\n\tlast awoken: {:?}",
                    queue, last_awoken
                );
            }
            self.last_awoken.store(ptr::null_mut(), Ordering::SeqCst);
        }

        pub fn finish(&self, num_single_unparks: usize) {
            // The amount of threads not unparked via unpark_one
            let mut num_threads_left = self.num_threads.checked_sub(num_single_unparks).unwrap();

            // Wake remaining threads up with unpark_all. Has to be in a loop, because there might
            // still be threads that has not yet parked.
            while num_threads_left > 0 {
                let mut num_waiting_on_address = 0;
                for_each(self.semaphore_addr(), |_thread_data| {
                    num_waiting_on_address += 1;
                });
                assert!(num_waiting_on_address <= num_threads_left);

                let num_awake_before_unpark = self.num_awake.load(Ordering::SeqCst);

                let num_unparked =
                    unsafe { super::unpark_all(self.semaphore_addr(), DEFAULT_UNPARK_TOKEN) };
                assert!(num_unparked >= num_waiting_on_address);
                assert!(num_unparked <= num_threads_left);

                // Wait for all unparked threads to wake up and update num_awake + last_awoken.
                while self.num_awake.load(Ordering::SeqCst)
                    != num_awake_before_unpark + num_unparked
                {
                    thread::yield_now()
                }

                num_threads_left = num_threads_left.checked_sub(num_unparked).unwrap();
            }
            // By now, all threads should have been woken up
            assert_eq!(self.num_awake.load(Ordering::SeqCst), self.num_threads);

            // Make sure no thread is parked on our semaphore address
            let mut num_waiting_on_address = 0;
            for_each(self.semaphore_addr(), |_thread_data| {
                num_waiting_on_address += 1;
            });
            assert_eq!(num_waiting_on_address, 0);
        }

        pub fn down(&self) {
            let old_semaphore_value = self.semaphore.fetch_sub(1, Ordering::SeqCst);

            if old_semaphore_value > 0 {
                // We acquired the semaphore. Done.
                return;
            }

            // We need to wait.
            let validate = || true;
            let before_sleep = || {};
            let timed_out = |_, _| {};
            unsafe {
                super::park(
                    self.semaphore_addr(),
                    validate,
                    before_sleep,
                    timed_out,
                    DEFAULT_PARK_TOKEN,
                    None,
                );
            }
        }

        pub fn up(&self) {
            let old_semaphore_value = self.semaphore.fetch_add(1, Ordering::SeqCst);

            // Check if anyone was waiting on the semaphore. If they were, then pass ownership to them.
            if old_semaphore_value < 0 {
                // We need to continue until we have actually unparked someone. It might be that
                // the thread we want to pass ownership to has decremented the semaphore counter,
                // but not yet parked.
                loop {
                    match unsafe {
                        super::unpark_one(self.semaphore_addr(), |_| DEFAULT_UNPARK_TOKEN)
                            .unparked_threads
                    } {
                        1 => break,
                        0 => (),
                        i => panic!("Should not wake up {} threads", i),
                    }
                }
            }
        }

        fn semaphore_addr(&self) -> usize {
            core::ptr::from_ref(&self.semaphore).addr()
        }
    }
}
