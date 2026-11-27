use lock_api::*;
use std::collections::BTreeMap;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex as StdMutex};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Mode {
    Shared,
    Exclusive,
    Upgradable,
}

// No token is Copy, Clone, or Default. Each acquisition has a unique identity,
// and transitions change its type without creating another acquisition.
struct Token {
    id: usize,
    allocation: Box<usize>,
    mode: Mode,
    live: bool,
    drops: Arc<AtomicUsize>,
}
impl Drop for Token {
    fn drop(&mut self) {
        assert!(!self.live, "lost an acquisition without unlocking it");
        self.drops.fetch_add(1, Ordering::Relaxed);
    }
}
struct Shared(Token);
struct Exclusive(Token);
struct Upgradable(Token);

struct State {
    next: usize,
    held: BTreeMap<usize, Mode>,
}
#[derive(Clone, Copy)]
#[repr(usize)]
enum PanicAt {
    Acquire = 1,
    Unlock,
    Upgrade,
    Downgrade,
}

struct Raw {
    state: StdMutex<State>,
    drops: Option<Arc<AtomicUsize>>,
    panic_at: AtomicUsize,
}
impl Raw {
    const INIT: Self = Self {
        state: StdMutex::new(State {
            next: 0,
            held: BTreeMap::new(),
        }),
        drops: None,
        panic_at: AtomicUsize::new(0),
    };
    fn new(drops: &Arc<AtomicUsize>) -> Self {
        Self {
            drops: Some(drops.clone()),
            ..Self::INIT
        }
    }
    fn fail_next(&self, point: PanicAt) {
        self.panic_at.store(point as usize, Ordering::Relaxed);
    }
    fn should_panic(&self, point: PanicAt) -> bool {
        self.panic_at
            .compare_exchange(point as usize, 0, Ordering::Relaxed, Ordering::Relaxed)
            .is_ok()
    }
    fn acquire(&self, mode: Mode) -> Option<Token> {
        assert!(!self.should_panic(PanicAt::Acquire), "acquisition panic");
        let mut state = self.state.lock().unwrap();
        if state.held.values().any(|&held| {
            held == Mode::Exclusive
                || mode == Mode::Exclusive
                || (held == Mode::Upgradable && mode == Mode::Upgradable)
        }) {
            return None;
        }
        state.next += 1;
        let id = state.next;
        state.held.insert(id, mode);
        Some(Token {
            id,
            allocation: Box::new(id),
            mode,
            live: true,
            drops: self.drops.clone().unwrap_or_default(),
        })
    }
    fn release(&self, mut token: Token, mode: Mode) {
        assert!(token.live);
        assert_eq!(*token.allocation, token.id);
        assert_eq!(token.mode, mode);
        assert_eq!(
            self.state.lock().unwrap().held.remove(&token.id),
            Some(mode)
        );
        token.live = false;
        assert!(!self.should_panic(PanicAt::Unlock), "unlock panic");
    }
    fn transition(&self, mut token: Token, from: Mode, to: Mode) -> Result<Token, Token> {
        let mut state = self.state.lock().unwrap();
        assert!(token.live);
        assert_eq!(*token.allocation, token.id);
        assert_eq!(token.mode, from);
        assert_eq!(state.held.get(&token.id), Some(&from));
        if to == Mode::Exclusive && state.held.len() != 1 {
            return Err(token);
        }
        state.held.insert(token.id, to);
        token.mode = to;
        drop(state);
        let point = if to == Mode::Exclusive {
            PanicAt::Upgrade
        } else {
            PanicAt::Downgrade
        };
        if self.should_panic(point) {
            self.release(token, to);
            panic!("transition panic");
        }
        Ok(token)
    }
}

// Blocking acquisitions panic if unavailable; no lock is acquired on that path.
unsafe impl RawMutex for Raw {
    const INIT: Self = Self::INIT;
    type Guard = Exclusive;
    fn lock(&self) -> Exclusive {
        Exclusive(self.acquire(Mode::Exclusive).expect("contended test lock"))
    }
    fn try_lock(&self) -> Option<Exclusive> {
        self.acquire(Mode::Exclusive).map(Exclusive)
    }
    unsafe fn unlock(&self, guard: Exclusive) {
        self.release(guard.0, Mode::Exclusive);
    }
    fn is_locked(&self) -> bool {
        !self.state.lock().unwrap().held.is_empty()
    }
}
unsafe impl RawMutexFair for Raw {
    unsafe fn unlock_fair(&self, guard: Exclusive) {
        unsafe { RawMutex::unlock(self, guard) };
    }
}
unsafe impl RawMutexTimed for Raw {
    // Unit represents an immediate deadline, so these only attempt acquisition.
    type Duration = ();
    type Instant = ();
    fn try_lock_for(&self, _: ()) -> Option<Exclusive> {
        RawMutex::try_lock(self)
    }
    fn try_lock_until(&self, _: ()) -> Option<Exclusive> {
        RawMutex::try_lock(self)
    }
}
unsafe impl RawRwLock for Raw {
    const INIT: Self = Self::INIT;
    type SharedGuard = Shared;
    type ExclusiveGuard = Exclusive;
    fn lock_shared(&self) -> Shared {
        Shared(self.acquire(Mode::Shared).expect("contended test lock"))
    }
    fn try_lock_shared(&self) -> Option<Shared> {
        self.acquire(Mode::Shared).map(Shared)
    }
    unsafe fn unlock_shared(&self, guard: Shared) {
        self.release(guard.0, Mode::Shared);
    }
    fn lock_exclusive(&self) -> Exclusive {
        RawMutex::lock(self)
    }
    fn try_lock_exclusive(&self) -> Option<Exclusive> {
        RawMutex::try_lock(self)
    }
    unsafe fn unlock_exclusive(&self, guard: Exclusive) {
        self.release(guard.0, Mode::Exclusive);
    }
    fn is_locked(&self) -> bool {
        RawMutex::is_locked(self)
    }
    fn is_locked_exclusive(&self) -> bool {
        self.state
            .lock()
            .unwrap()
            .held
            .values()
            .any(|&m| m == Mode::Exclusive)
    }
}
unsafe impl RawRwLockFair for Raw {
    unsafe fn unlock_shared_fair(&self, guard: Shared) {
        unsafe { self.unlock_shared(guard) };
    }
    unsafe fn unlock_exclusive_fair(&self, guard: Exclusive) {
        unsafe { self.unlock_exclusive(guard) };
    }
}
unsafe impl RawRwLockDowngrade for Raw {
    unsafe fn downgrade(&self, guard: Exclusive) -> Shared {
        Shared(
            self.transition(guard.0, Mode::Exclusive, Mode::Shared)
                .ok()
                .unwrap(),
        )
    }
}
unsafe impl RawRwLockTimed for Raw {
    type Duration = ();
    type Instant = ();
    fn try_lock_shared_for(&self, _: ()) -> Option<Shared> {
        self.try_lock_shared()
    }
    fn try_lock_shared_until(&self, _: ()) -> Option<Shared> {
        self.try_lock_shared()
    }
    fn try_lock_exclusive_for(&self, _: ()) -> Option<Exclusive> {
        self.try_lock_exclusive()
    }
    fn try_lock_exclusive_until(&self, _: ()) -> Option<Exclusive> {
        self.try_lock_exclusive()
    }
}
unsafe impl RawRwLockUpgrade for Raw {
    type UpgradableGuard = Upgradable;
    fn lock_upgradable(&self) -> Upgradable {
        Upgradable(self.acquire(Mode::Upgradable).expect("contended test lock"))
    }
    fn try_lock_upgradable(&self) -> Option<Upgradable> {
        self.acquire(Mode::Upgradable).map(Upgradable)
    }
    unsafe fn unlock_upgradable(&self, guard: Upgradable) {
        self.release(guard.0, Mode::Upgradable);
    }
    unsafe fn upgrade(&self, guard: Upgradable) -> Exclusive {
        Exclusive(
            self.transition(guard.0, Mode::Upgradable, Mode::Exclusive)
                .ok()
                .unwrap(),
        )
    }
    unsafe fn try_upgrade(&self, guard: Upgradable) -> Result<Exclusive, Upgradable> {
        self.transition(guard.0, Mode::Upgradable, Mode::Exclusive)
            .map(Exclusive)
            .map_err(Upgradable)
    }
}
unsafe impl RawRwLockUpgradeFair for Raw {
    unsafe fn unlock_upgradable_fair(&self, guard: Upgradable) {
        unsafe { self.unlock_upgradable(guard) };
    }
}
unsafe impl RawRwLockUpgradeDowngrade for Raw {
    unsafe fn downgrade_upgradable(&self, guard: Upgradable) -> Shared {
        Shared(
            self.transition(guard.0, Mode::Upgradable, Mode::Shared)
                .ok()
                .unwrap(),
        )
    }
    unsafe fn downgrade_to_upgradable(&self, guard: Exclusive) -> Upgradable {
        Upgradable(
            self.transition(guard.0, Mode::Exclusive, Mode::Upgradable)
                .ok()
                .unwrap(),
        )
    }
}
unsafe impl RawRwLockUpgradeTimed for Raw {
    fn try_lock_upgradable_for(&self, _: ()) -> Option<Upgradable> {
        self.try_lock_upgradable()
    }
    fn try_lock_upgradable_until(&self, _: ()) -> Option<Upgradable> {
        self.try_lock_upgradable()
    }
    unsafe fn try_upgrade_for(&self, guard: Upgradable, _: ()) -> Result<Exclusive, Upgradable> {
        unsafe { self.try_upgrade(guard) }
    }
    unsafe fn try_upgrade_until(&self, guard: Upgradable, _: ()) -> Result<Exclusive, Upgradable> {
        unsafe { self.try_upgrade(guard) }
    }
}

#[test]
fn mutex_state_lifetime() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = Mutex::from_raw(Raw::new(&drops), (1, 2));
    let mut guard = lock.lock();
    assert!(lock.try_lock().is_none());
    MutexGuard::unlocked(&mut guard, || assert!(!lock.is_locked()));
    assert_eq!(drops.load(Ordering::Relaxed), 1);
    MutexGuard::bump(&mut guard);
    MutexGuard::unlocked_fair(&mut guard, || {});
    let guard = MutexGuard::try_map(guard, |_| None::<&mut ()>).unwrap_err();
    let guard = MutexGuard::map(guard, |x| &mut x.0);
    let guard = MappedMutexGuard::try_map_or_err(guard, |x| Ok::<_, ()>(x)).unwrap();
    MappedMutexGuard::unlock_fair(guard);
    assert_eq!(drops.load(Ordering::Relaxed), 4);
    drop(lock.try_lock_for(()).unwrap());
    MutexGuard::unlock_fair(lock.try_lock_until(()).unwrap());
    assert_eq!(drops.load(Ordering::Relaxed), 6);
    assert!(!lock.is_locked());
}

#[test]
fn panic_restores_acquisition_and_mapping_drops_once() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = Mutex::from_raw(Raw::new(&drops), 0);
    let mut guard = lock.lock();
    assert!(
        catch_unwind(AssertUnwindSafe(|| MutexGuard::unlocked(
            &mut guard,
            || panic!("test")
        )))
        .is_err()
    );
    assert!(lock.is_locked());
    drop(guard);
    assert_eq!(drops.load(Ordering::Relaxed), 2);
    assert!(
        catch_unwind(AssertUnwindSafe(|| MutexGuard::map(
            lock.lock(),
            |_| -> &mut () { panic!("test") }
        )))
        .is_err()
    );
    assert_eq!(drops.load(Ordering::Relaxed), 3);
    assert!(!lock.is_locked());
}

#[test]
fn rwlock_state_transitions() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = RwLock::from_raw(Raw::new(&drops), (1, 2));
    let mut guard = lock.upgradable_read();
    let reader = lock.read();
    assert!(guard.try_with_upgraded(|_| ()).is_none());
    assert!(guard.try_with_upgraded_for((), |_| ()).is_none());
    assert!(guard.try_with_upgraded_until((), |_| ()).is_none());
    let guard = RwLockUpgradableReadGuard::try_upgrade(guard).unwrap_err();
    let guard = RwLockUpgradableReadGuard::try_upgrade_for(guard, ()).unwrap_err();
    let mut guard = RwLockUpgradableReadGuard::try_upgrade_until(guard, ()).unwrap_err();
    drop(reader);
    assert!(catch_unwind(AssertUnwindSafe(|| guard.with_upgraded(|_| panic!("test")))).is_err());
    guard.try_with_upgraded(|x| x.0 += 1).unwrap();
    guard.try_with_upgraded_for((), |x| x.0 += 1).unwrap();
    guard.try_with_upgraded_until((), |x| x.0 += 1).unwrap();
    let guard = RwLockUpgradableReadGuard::upgrade(guard);
    let guard = RwLockWriteGuard::downgrade_to_upgradable(guard);
    let guard = RwLockUpgradableReadGuard::try_upgrade_for(guard, ()).unwrap();
    let guard = RwLockWriteGuard::downgrade(guard);
    assert_eq!(guard.0, 4);
    let guard = RwLockReadGuard::map(guard, |x| &x.0);
    let guard = MappedRwLockReadGuard::map(guard, |x| x);
    MappedRwLockReadGuard::unlock_fair(guard);
    assert_eq!(drops.load(Ordering::Relaxed), 2);
    let guard = RwLockWriteGuard::map(lock.write(), |x| &mut x.1);
    let guard = MappedRwLockWriteGuard::map(guard, |x| x);
    MappedRwLockWriteGuard::unlock_fair(guard);
    assert_eq!(drops.load(Ordering::Relaxed), 3);
    assert!(!lock.is_locked());
}

#[test]
fn rwlock_reacquisition() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = RwLock::from_raw(Raw::new(&drops), 0);
    let mut guard = lock.try_read_for(()).unwrap();
    RwLockReadGuard::unlocked(&mut guard, || {});
    RwLockReadGuard::unlocked_fair(&mut guard, || {});
    RwLockReadGuard::bump(&mut guard);
    RwLockReadGuard::unlock_fair(guard);
    let mut guard = lock.try_write_until(()).unwrap();
    RwLockWriteGuard::unlocked(&mut guard, || {});
    RwLockWriteGuard::unlocked_fair(&mut guard, || {});
    RwLockWriteGuard::bump(&mut guard);
    RwLockWriteGuard::unlock_fair(guard);
    let mut guard = lock.try_upgradable_read_for(()).unwrap();
    RwLockUpgradableReadGuard::unlocked(&mut guard, || {});
    RwLockUpgradableReadGuard::unlocked_fair(&mut guard, || {});
    RwLockUpgradableReadGuard::bump(&mut guard);
    RwLockUpgradableReadGuard::unlock_fair(guard);
    assert_eq!(drops.load(Ordering::Relaxed), 12);
    assert!(!lock.is_locked());
}

#[cfg(feature = "arc_lock")]
#[test]
fn arc_tokens_and_transitions() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = Arc::new(Mutex::from_raw(Raw::new(&drops), 0));
    let mut guard = lock.try_lock_arc_for(()).unwrap();
    ArcMutexGuard::unlocked(&mut guard, || {});
    ArcMutexGuard::bump(&mut guard);
    let returned = ArcMutexGuard::into_arc_fair(guard);
    assert!(Arc::ptr_eq(&lock, &returned));
    assert_eq!(Arc::strong_count(&lock), 2);
    drop(returned);
    drop(lock.lock_arc());
    assert_eq!(drops.load(Ordering::Relaxed), 4);

    let lock = Arc::new(RwLock::from_raw(Raw::new(&drops), 0));
    let guard = lock.upgradable_read_arc();
    let reader = lock.read();
    let guard = ArcRwLockUpgradableReadGuard::try_upgrade(guard).unwrap_err();
    drop(reader);
    let guard = ArcRwLockUpgradableReadGuard::try_upgrade_until(guard, ()).unwrap();
    let guard = ArcRwLockWriteGuard::downgrade_to_upgradable(guard);
    let mut guard = ArcRwLockUpgradableReadGuard::try_upgrade_for(guard, ()).unwrap();
    ArcRwLockWriteGuard::bump(&mut guard);
    let mut guard = ArcRwLockWriteGuard::downgrade_to_upgradable(guard);
    assert!(catch_unwind(AssertUnwindSafe(|| guard.with_upgraded(|_| panic!("test")))).is_err());
    let guard = ArcRwLockUpgradableReadGuard::downgrade(guard);
    let returned = ArcRwLockReadGuard::into_arc(guard);
    assert!(Arc::ptr_eq(&lock, &returned));
    drop(returned);
    assert_eq!(Arc::strong_count(&lock), 1);
    assert_eq!(drops.load(Ordering::Relaxed), 7);
    assert!(!lock.is_locked());
}

#[cfg(feature = "atomic_usize")]
#[test]
fn reentrant_state_belongs_to_final_unlock() {
    struct ThreadId;
    unsafe impl GetThreadId for ThreadId {
        const INIT: Self = Self;
        fn nonzero_thread_id(&self) -> std::num::NonZeroUsize {
            // This test only uses the lock from a single thread.
            std::num::NonZeroUsize::new(1).unwrap()
        }
    }
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = ReentrantMutex::from_raw(Raw::new(&drops), ThreadId, 0);
    let first = lock.lock();
    let second = lock.lock();
    drop(first);
    assert_eq!(drops.load(Ordering::Relaxed), 0);
    let mut second = second;
    ReentrantMutexGuard::bump(&mut second);
    assert_eq!(drops.load(Ordering::Relaxed), 1);
    ReentrantMutexGuard::unlocked(&mut second, || {});
    ReentrantMutexGuard::unlock_fair(second);
    assert_eq!(drops.load(Ordering::Relaxed), 3);
    assert!(!lock.is_locked());
}

struct SpuriousCondvar {
    panic_after_relock: std::sync::atomic::AtomicBool,
}
unsafe impl RawCondvar for SpuriousCondvar {
    const INIT: Self = Self {
        panic_after_relock: std::sync::atomic::AtomicBool::new(false),
    };
    type RawMutex = Raw;
    unsafe fn wait(&self, mutex: &Raw, guard: &mut Exclusive) {
        // A spurious wake needs no notification. No unwind is allowed while the
        // caller's state is moved out, so abort if reacquisition panics.
        let result = catch_unwind(AssertUnwindSafe(|| unsafe {
            RawMutex::unlock(mutex, std::ptr::read(guard))
        }));
        let replacement = catch_unwind(AssertUnwindSafe(|| RawMutex::lock(mutex)))
            .unwrap_or_else(|_| std::process::abort());
        unsafe { std::ptr::write(guard, replacement) };
        if let Err(payload) = result {
            std::panic::resume_unwind(payload);
        }
        if self.panic_after_relock.swap(false, Ordering::Relaxed) {
            panic!("test");
        }
    }
    fn notify_one(&self) -> bool {
        false
    }
    fn notify_all(&self) -> usize {
        0
    }
}
unsafe impl RawCondvarTimed for SpuriousCondvar {
    fn checked_duration_to_instant(_: &()) -> Option<()> {
        Some(())
    }
    unsafe fn wait_until(&self, mutex: &Raw, guard: &mut Exclusive, _: &()) -> bool {
        unsafe { self.wait(mutex, guard) };
        true
    }
}

#[test]
fn condvar_replaces_state_including_on_unwind() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = Mutex::from_raw(Raw::new(&drops), 0);
    let condvar = Condvar::<SpuriousCondvar>::new();
    let mut guard = lock.lock();
    condvar.wait(&mut guard);
    assert_eq!(drops.load(Ordering::Relaxed), 1);
    assert!(condvar.wait_for(&mut guard, ()).timed_out());
    assert!(condvar.wait_until(&mut guard, ()).timed_out());
    assert_eq!(drops.load(Ordering::Relaxed), 3);
    condvar
        .raw()
        .panic_after_relock
        .store(true, Ordering::Relaxed);
    assert!(catch_unwind(AssertUnwindSafe(|| condvar.wait(&mut guard))).is_err());
    assert!(lock.is_locked());
    drop(guard);
    assert_eq!(drops.load(Ordering::Relaxed), 5);
    assert!(!lock.is_locked());
}

#[test]
fn raw_guard_can_unlock_on_drop() {
    struct RawDropUnlock(std::sync::OnceLock<Arc<std::sync::atomic::AtomicBool>>);
    struct DropUnlock(Arc<std::sync::atomic::AtomicBool>);
    impl Drop for DropUnlock {
        fn drop(&mut self) {
            assert!(self.0.swap(false, Ordering::Release));
        }
    }
    unsafe impl RawMutex for RawDropUnlock {
        const INIT: Self = Self(std::sync::OnceLock::new());
        type Guard = DropUnlock;
        fn lock(&self) -> DropUnlock {
            loop {
                if let Some(guard) = self.try_lock() {
                    return guard;
                }
                std::hint::spin_loop();
            }
        }
        fn try_lock(&self) -> Option<DropUnlock> {
            let state = self
                .0
                .get_or_init(|| Arc::new(std::sync::atomic::AtomicBool::new(false)));
            state
                .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
                .ok()
                .map(|_| DropUnlock(state.clone()))
        }
        unsafe fn unlock(&self, guard: DropUnlock) {
            drop(guard);
        }
        fn is_locked(&self) -> bool {
            self.0.get().is_some_and(|s| s.load(Ordering::Relaxed))
        }
    }
    unsafe impl RawMutexFair for RawDropUnlock {
        unsafe fn unlock_fair(&self, guard: DropUnlock) {
            drop(guard);
        }
    }
    let lock = Mutex::<RawDropUnlock, _>::new((1, 2));
    let mut guard = lock.lock();
    MutexGuard::bump(&mut guard);
    MutexGuard::unlocked(&mut guard, || assert!(!lock.is_locked()));
    let guard = MutexGuard::map(guard, |x| &mut x.0);
    MappedMutexGuard::unlock_fair(guard);
    assert!(!lock.is_locked());
    drop(lock.lock());
    assert!(!lock.is_locked());
    assert_eq!(Arc::strong_count(unsafe { lock.raw() }.0.get().unwrap()), 1);
}

fn expect_panic(f: impl FnOnce()) {
    assert!(catch_unwind(AssertUnwindSafe(f)).is_err());
}

#[test]
fn panicking_raw_unlock_restores_borrowed_mutex_guard() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = Mutex::from_raw(Raw::new(&drops), 17);
    let mut guard = lock.lock();
    for operation in 0..3 {
        unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
        expect_panic(|| match operation {
            0 => MutexGuard::unlocked(&mut guard, || panic!("closure must not run")),
            1 => MutexGuard::unlocked_fair(&mut guard, || panic!("closure must not run")),
            _ => MutexGuard::bump(&mut guard),
        });
        assert!(lock.is_locked());
        assert_eq!(*guard, 17);
    }
    drop(guard);
    assert!(!lock.is_locked());
    assert_eq!(drops.load(Ordering::Relaxed), 4);
}

#[test]
fn panicking_raw_unlock_restores_each_rwlock_mode() {
    let lock = RwLock::<Raw, _>::new(17);
    macro_rules! check {
        ($acquire:ident, $guard:ident) => {{
            let mut guard = lock.$acquire();
            for operation in 0..3 {
                unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
                expect_panic(|| match operation {
                    0 => $guard::unlocked(&mut guard, || panic!("closure must not run")),
                    1 => $guard::unlocked_fair(&mut guard, || panic!("closure must not run")),
                    _ => $guard::bump(&mut guard),
                });
                assert!(lock.is_locked());
                assert_eq!(*guard, 17);
            }
            drop(guard);
            assert!(!lock.is_locked());
        }};
    }
    check!(read, RwLockReadGuard);
    check!(write, RwLockWriteGuard);
    check!(upgradable_read, RwLockUpgradableReadGuard);
}

#[test]
fn consuming_operations_release_on_panic() {
    let lock = Mutex::<Raw, _>::new(0);
    let guard = lock.lock();
    unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
    expect_panic(|| drop(guard));
    assert!(!lock.is_locked());
    let guard = MutexGuard::map(lock.lock(), |x| x);
    unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
    expect_panic(|| MappedMutexGuard::unlock_fair(guard));
    assert!(!lock.is_locked());

    let lock = RwLock::<Raw, _>::new(0);
    let guard = lock.upgradable_read();
    unsafe { lock.raw() }.fail_next(PanicAt::Upgrade);
    expect_panic(|| drop(RwLockUpgradableReadGuard::upgrade(guard)));
    assert!(!lock.is_locked());
    let guard = lock.write();
    unsafe { lock.raw() }.fail_next(PanicAt::Downgrade);
    expect_panic(|| drop(RwLockWriteGuard::downgrade(guard)));
    assert!(!lock.is_locked());
}

#[test]
fn condvar_restores_after_raw_unlock_panic() {
    let lock = Mutex::<Raw, _>::new(0);
    let condvar = Condvar::<SpuriousCondvar>::new();
    let mut guard = lock.lock();
    unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
    expect_panic(|| condvar.wait(&mut guard));
    assert!(lock.is_locked());
    drop(guard);
    assert!(!lock.is_locked());
}

#[cfg(feature = "atomic_usize")]
struct SingleThread;
#[cfg(feature = "atomic_usize")]
unsafe impl GetThreadId for SingleThread {
    const INIT: Self = Self;
    fn nonzero_thread_id(&self) -> std::num::NonZeroUsize {
        // These test locks are only used from the thread running their test.
        std::num::NonZeroUsize::new(1).unwrap()
    }
}

#[cfg(feature = "atomic_usize")]
#[test]
fn reentrant_metadata_is_restored_on_bump_and_unlock_panic() {
    let drops = Arc::new(AtomicUsize::new(0));
    let lock = ReentrantMutex::from_raw(Raw::new(&drops), SingleThread, 17);
    let mut guard = lock.lock();
    for operation in 0..3 {
        unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
        expect_panic(|| match operation {
            0 => ReentrantMutexGuard::bump(&mut guard),
            1 => ReentrantMutexGuard::unlocked(&mut guard, || panic!("closure must not run")),
            _ => ReentrantMutexGuard::unlocked_fair(&mut guard, || panic!("closure must not run")),
        });
        assert!(lock.is_owned_by_current_thread());
        let recursive = lock
            .try_lock()
            .expect("ownership metadata was not restored");
        drop(recursive);
        assert_eq!(*guard, 17);
    }
    // Nested bump must not release the underlying acquisition.
    let nested = lock.lock();
    let before = drops.load(Ordering::Relaxed);
    ReentrantMutexGuard::bump(&mut guard);
    assert_eq!(drops.load(Ordering::Relaxed), before);
    drop(nested);
    unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
    expect_panic(|| ReentrantMutexGuard::unlock_fair(guard));
    assert!(!lock.is_locked());
    assert!(!lock.is_owned_by_current_thread());
    assert_eq!(drops.load(Ordering::Relaxed), 4);
}

#[cfg(feature = "arc_lock")]
#[test]
fn arc_ownership_is_not_leaked_on_raw_panic() {
    let lock = Arc::new(Mutex::<Raw, _>::new(17));
    for fair in [false, true] {
        let guard = lock.lock_arc();
        unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
        expect_panic(|| {
            drop(if fair {
                ArcMutexGuard::into_arc_fair(guard)
            } else {
                ArcMutexGuard::into_arc(guard)
            });
        });
        assert_eq!(Arc::strong_count(&lock), 1);
        assert!(!lock.is_locked());
    }
    let lock = Arc::new(RwLock::<Raw, _>::new(17));
    macro_rules! check {
        ($acquire:ident, $point:ident, $convert:expr) => {{
            let guard = lock.$acquire();
            unsafe { lock.raw() }.fail_next(PanicAt::$point);
            expect_panic(|| {
                drop(($convert)(guard));
            });
            assert_eq!(Arc::strong_count(&lock), 1);
            assert!(!lock.is_locked());
        }};
    }
    check!(
        upgradable_read_arc,
        Upgrade,
        ArcRwLockUpgradableReadGuard::upgrade
    );
    check!(
        upgradable_read_arc,
        Upgrade,
        ArcRwLockUpgradableReadGuard::try_upgrade
    );
    check!(upgradable_read_arc, Upgrade, |g| {
        ArcRwLockUpgradableReadGuard::try_upgrade_for(g, ())
    });
    check!(upgradable_read_arc, Upgrade, |g| {
        ArcRwLockUpgradableReadGuard::try_upgrade_until(g, ())
    });
    check!(write_arc, Downgrade, ArcRwLockWriteGuard::downgrade);
    check!(
        write_arc,
        Downgrade,
        ArcRwLockWriteGuard::downgrade_to_upgradable
    );
    check!(
        upgradable_read_arc,
        Downgrade,
        ArcRwLockUpgradableReadGuard::downgrade
    );
    check!(read_arc, Unlock, ArcRwLockReadGuard::into_arc);
    check!(read_arc, Unlock, ArcRwLockReadGuard::into_arc_fair);
    check!(write_arc, Unlock, ArcRwLockWriteGuard::into_arc);
    check!(write_arc, Unlock, ArcRwLockWriteGuard::into_arc_fair);
    check!(
        upgradable_read_arc,
        Unlock,
        ArcRwLockUpgradableReadGuard::into_arc
    );
    check!(
        upgradable_read_arc,
        Unlock,
        ArcRwLockUpgradableReadGuard::into_arc_fair
    );

    #[cfg(feature = "atomic_usize")]
    {
        let lock = Arc::new(ReentrantMutex::from_raw(Raw::INIT, SingleThread, 17));
        let mut guard = lock.lock_arc();
        unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
        expect_panic(|| ArcReentrantMutexGuard::bump(&mut guard));
        assert!(lock.is_owned_by_current_thread());
        unsafe { lock.raw() }.fail_next(PanicAt::Unlock);
        expect_panic(|| {
            drop(ArcReentrantMutexGuard::into_arc_fair(guard));
        });
        assert_eq!(Arc::strong_count(&lock), 1);
        assert!(!lock.is_locked());
    }
}

// Catch any incorrectly propagated panic in the child. The parent only accepts
// SIGABRT, not a test failure or a panic that reached the caller.
#[cfg(all(unix, not(miri)))]
#[test]
fn abort_child() {
    let Ok(case) = std::env::var("LOCK_API_ABORT_CASE") else {
        return;
    };
    let _ = catch_unwind(AssertUnwindSafe(|| {
        if case == "relock" || case == "bump_relock" || case == "wait_relock" {
            let lock = Mutex::<Raw, _>::new(0);
            let mut guard = lock.lock();
            unsafe { lock.raw() }.fail_next(PanicAt::Acquire);
            if case == "relock" {
                MutexGuard::unlocked(&mut guard, || ());
            } else if case == "bump_relock" {
                MutexGuard::bump(&mut guard);
            } else {
                Condvar::<SpuriousCondvar>::new().wait(&mut guard);
            }
        } else {
            #[cfg(feature = "arc_lock")]
            if let Some(case) = case.strip_prefix("arc_") {
                let lock = Arc::new(RwLock::<Raw, _>::new(0));
                let mut guard = lock.upgradable_read_arc();
                unsafe { lock.raw() }.fail_next(if case == "downgrade" {
                    PanicAt::Downgrade
                } else {
                    PanicAt::Upgrade
                });
                match case {
                    "upgrade" | "downgrade" => guard.with_upgraded(|_| ()),
                    "try_upgrade" => {
                        guard.try_with_upgraded(|_| ());
                    }
                    "try_upgrade_for" => {
                        guard.try_with_upgraded_for((), |_| ());
                    }
                    "try_upgrade_until" => {
                        guard.try_with_upgraded_until((), |_| ());
                    }
                    _ => panic!("unknown case"),
                }
                return;
            }
            let lock = RwLock::<Raw, _>::new(0);
            let mut guard = lock.upgradable_read();
            unsafe { lock.raw() }.fail_next(if case == "downgrade" {
                PanicAt::Downgrade
            } else {
                PanicAt::Upgrade
            });
            match case.as_str() {
                "upgrade" | "downgrade" => guard.with_upgraded(|_| ()),
                "try_upgrade" => {
                    guard.try_with_upgraded(|_| ());
                }
                "try_upgrade_for" => {
                    guard.try_with_upgraded_for((), |_| ());
                }
                "try_upgrade_until" => {
                    guard.try_with_upgraded_until((), |_| ());
                }
                _ => panic!("unknown case"),
            }
        }
    }));
    std::process::exit(77);
}

#[cfg(all(unix, not(miri)))]
#[test]
fn ownership_restoration_failures_abort() {
    use std::os::unix::process::ExitStatusExt;
    for case in [
        "relock",
        "bump_relock",
        "wait_relock",
        "upgrade",
        "downgrade",
        "try_upgrade",
        "try_upgrade_for",
        "try_upgrade_until",
        #[cfg(feature = "arc_lock")]
        "arc_upgrade",
        #[cfg(feature = "arc_lock")]
        "arc_downgrade",
        #[cfg(feature = "arc_lock")]
        "arc_try_upgrade",
        #[cfg(feature = "arc_lock")]
        "arc_try_upgrade_for",
        #[cfg(feature = "arc_lock")]
        "arc_try_upgrade_until",
    ] {
        let output = std::process::Command::new("sh")
            .args(["-c", "ulimit -c 0; exec \"$@\"", "sh"])
            .arg(std::env::current_exe().unwrap())
            .args(["--exact", "abort_child", "--nocapture"])
            .env("LOCK_API_ABORT_CASE", case)
            .output()
            .unwrap();
        assert_eq!(
            output.status.signal(),
            Some(6),
            "{case}: {}",
            String::from_utf8_lossy(&output.stderr)
        );
    }
}
