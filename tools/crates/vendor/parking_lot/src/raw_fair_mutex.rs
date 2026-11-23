use crate::raw_mutex::RawMutex;
use lock_api::RawMutexFair;

/// Raw fair mutex type backed by the parking lot.
pub struct RawFairMutex(RawMutex);

unsafe impl lock_api::RawMutex for RawFairMutex {
    const INIT: Self = RawFairMutex(<RawMutex as lock_api::RawMutex>::INIT);

    type Guard = <RawMutex as lock_api::RawMutex>::Guard;

    #[inline]
    fn lock(&self) -> Self::Guard {
        self.0.lock()
    }

    #[inline]
    fn try_lock(&self) -> Option<Self::Guard> {
        self.0.try_lock()
    }

    #[inline]
    unsafe fn unlock(&self, guard: Self::Guard) {
        unsafe { self.unlock_fair(guard) }
    }

    #[inline]
    fn is_locked(&self) -> bool {
        self.0.is_locked()
    }
}

unsafe impl lock_api::RawMutexFair for RawFairMutex {
    #[inline]
    unsafe fn unlock_fair(&self, guard: Self::Guard) {
        unsafe { self.0.unlock_fair(guard) }
    }

    #[inline]
    unsafe fn bump(&self, guard: &mut Self::Guard) {
        unsafe { self.0.bump(guard) }
    }
}

unsafe impl lock_api::RawMutexTimed for RawFairMutex {
    type Duration = <RawMutex as lock_api::RawMutexTimed>::Duration;
    type Instant = <RawMutex as lock_api::RawMutexTimed>::Instant;

    #[inline]
    fn try_lock_until(&self, timeout: Self::Instant) -> Option<Self::Guard> {
        self.0.try_lock_until(timeout)
    }

    #[inline]
    fn try_lock_for(&self, timeout: Self::Duration) -> Option<Self::Guard> {
        self.0.try_lock_for(timeout)
    }
}
