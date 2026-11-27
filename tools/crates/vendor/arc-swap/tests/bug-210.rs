//! Regression test for #210: a writer of another instance reusing a freed address must not pay a
//! debt on that address. `FakeArc` makes address reuse deterministic; readers check they never see
//! an object meant for another instance. Timing dependent natively, a smoke run under Miri.

use std::array;
#[cfg(not(miri))]
use std::hint;
use std::mem;
use std::ops::Deref;
use std::sync::atomic::{fence, AtomicBool, AtomicUsize, Ordering};
use std::sync::{Mutex, MutexGuard, PoisonError};
use std::thread;
use std::time::{Duration, Instant};

use arc_swap::strategy::{DefaultStrategy, Strategy};
use arc_swap::{ArcSwapAny, RefCnt};

const OTHERS: usize = 8;
// About twenty slots are live at a time.
const SLOT_CNT: usize = 4096;
const MIRI_ROUNDS: usize = 40;
const READERS: usize = if cfg!(miri) { 1 } else { 4 };

struct Slot {
    count: AtomicUsize,
    // Atomic, the tag may be read while another thread rewrites the slot.
    tag: AtomicUsize,
}

#[allow(clippy::declare_interior_mutable_const)]
const EMPTY_SLOT: Slot = Slot {
    count: AtomicUsize::new(0),
    tag: AtomicUsize::new(0),
};

static SLOTS: [Slot; SLOT_CNT] = [EMPTY_SLOT; SLOT_CNT];
static NEXT_FRESH: AtomicUsize = AtomicUsize::new(0);
static FREE: Mutex<Vec<usize>> = Mutex::new(Vec::new());

/// Serializes the tests, they share the arena (a failed one leaves it inconsistent).
static LOCK: Mutex<()> = Mutex::new(());

fn lock() -> MutexGuard<'static, ()> {
    LOCK.lock().unwrap_or_else(PoisonError::into_inner)
}

fn free_list() -> MutexGuard<'static, Vec<usize>> {
    FREE.lock().unwrap_or_else(PoisonError::into_inner)
}

/// Arc-like pointer into a static arena; LIFO slot reuse makes address reuse deterministic.
struct FakeArc(&'static Slot);

impl FakeArc {
    fn new(tag: usize) -> Self {
        let reused = free_list().pop();
        let idx = reused.unwrap_or_else(|| NEXT_FRESH.fetch_add(1, Ordering::Relaxed));
        let slot = SLOTS.get(idx).expect("Arena exhausted");
        slot.count.store(1, Ordering::Relaxed);
        slot.tag.store(tag, Ordering::Relaxed);
        FakeArc(slot)
    }
}

impl Deref for FakeArc {
    type Target = Slot;

    fn deref(&self) -> &Slot {
        self.0
    }
}

impl Clone for FakeArc {
    fn clone(&self) -> Self {
        self.count.fetch_add(1, Ordering::Relaxed);
        FakeArc(self.0)
    }
}

impl Drop for FakeArc {
    fn drop(&mut self) {
        if self.count.fetch_sub(1, Ordering::Release) == 1 {
            fence(Ordering::Acquire);
            let idx =
                (self.0 as *const Slot as usize - SLOTS.as_ptr() as usize) / mem::size_of::<Slot>();
            free_list().push(idx);
        }
    }
}

unsafe impl RefCnt for FakeArc {
    type Base = Slot;

    fn into_ptr(me: Self) -> *mut Slot {
        let ptr = me.0 as *const Slot as *mut Slot;
        mem::forget(me);
        ptr
    }

    fn as_ptr(me: &Self) -> *mut Slot {
        me.0 as *const Slot as *mut Slot
    }

    unsafe fn from_ptr(ptr: *const Slot) -> Self {
        FakeArc(&*ptr)
    }
}

/// Every slot ever handed out is unreferenced and on the free list exactly once.
fn check_teardown() -> Result<(), String> {
    let handed_out = NEXT_FRESH.load(Ordering::Relaxed);
    let mut seen = vec![false; handed_out];
    for &idx in free_list().iter() {
        if mem::replace(&mut seen[idx], true) {
            return Err(format!("Slot {idx} is on the free list twice"));
        }
    }
    for (idx, slot) in SLOTS.iter().take(handed_out).enumerate() {
        let count = slot.count.load(Ordering::Relaxed);
        if count != 0 {
            return Err(format!("Slot {idx} has leftover count {count}"));
        }
        if !seen[idx] {
            return Err(format!("Slot {idx} is missing from the free list"));
        }
    }
    Ok(())
}

/// Stops the reader and spinner threads, even if the writer panics.
struct StopOnDrop<'a>(&'a AtomicBool);

impl Drop for StopOnDrop<'_> {
    fn drop(&mut self) {
        self.0.store(true, Ordering::Relaxed);
    }
}

/// Frees the old `a` value, reuses its slot for `v` and stores `v` into the other instances, which
/// pays debts on the address freed in the previous round.
fn write_round<S>(a: &ArcSwapAny<FakeArc, S>, others: &[ArcSwapAny<FakeArc, S>; OTHERS])
where
    S: Strategy<FakeArc>,
{
    a.store(FakeArc::new(0));
    let v = FakeArc::new(1);
    for o in others {
        o.store(v.clone());
    }
}

fn run<S>(budget: Duration)
where
    S: Strategy<FakeArc> + Default + Send + Sync,
{
    let _lock = lock();
    let a = ArcSwapAny::<_, S>::from(FakeArc::new(0));
    let others: [ArcSwapAny<FakeArc, S>; OTHERS] =
        array::from_fn(|_| ArcSwapAny::from(FakeArc::new(1)));
    let done = AtomicBool::new(false);

    let wrong_total = thread::scope(|scope| {
        let stop = StopOnDrop(&done);
        let readers = (0..READERS)
            .map(|_| {
                scope.spawn(|| {
                    let mut wrong = 0;
                    while !done.load(Ordering::Relaxed) {
                        let g = a.load();
                        if g.tag.load(Ordering::Relaxed) != 0 {
                            wrong += 1;
                        }
                    }
                    wrong
                })
            })
            .collect::<Vec<_>>();

        // Oversubscribe so readers get preempted inside `load`. Plain spinners, as more readers
        // would slow every `pay_all` walk.
        #[cfg(not(miri))]
        for _ in 0..(4 * num_cpus::get() + 4).min(64) {
            scope.spawn(|| {
                while !done.load(Ordering::Relaxed) {
                    hint::spin_loop();
                }
            });
        }

        let start = Instant::now();
        for round in 0.. {
            // A fixed number of rounds under Miri, its time is not comparable.
            let over = if cfg!(miri) {
                round >= MIRI_ROUNDS
            } else {
                start.elapsed() >= budget
            };
            if over {
                break;
            }
            write_round(&a, &others);
        }

        drop(stop);
        readers
            .into_iter()
            .map(|r| r.join().unwrap())
            .sum::<usize>()
    });

    drop(a);
    drop(others);
    let teardown = check_teardown();
    assert!(
        teardown.is_ok() && wrong_total == 0,
        "Teardown: {:?}, wrong loads: {}",
        teardown,
        wrong_total
    );
}

#[test]
fn cross_instance_default() {
    run::<DefaultStrategy>(Duration::from_secs(5));
}

#[test]
#[cfg(feature = "internal-test-strategies")]
#[allow(deprecated)] // We use a "deprecated" testing strategy.
fn cross_instance_fill_fast_slots() {
    use arc_swap::strategy::test_strategies::FillFastSlots;

    run::<FillFastSlots>(Duration::from_secs(1));
}
