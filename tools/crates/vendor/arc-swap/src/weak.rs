use core::ptr;

use alloc::rc::Weak as RcWeak;
use alloc::sync::Weak;

use crate::RefCnt;

unsafe impl<T> RefCnt for Weak<T> {
    type Base = T;
    // The data pointer is in a word-aligned allocation, so it is at least 2-aligned.
    const TAG: usize = 1;
    fn as_ptr(me: &Self) -> *mut T {
        if Weak::ptr_eq(&Weak::new(), me) {
            ptr::null_mut()
        } else {
            Weak::as_ptr(me) as *mut T
        }
    }
    fn into_ptr(me: Self) -> *mut T {
        if Weak::ptr_eq(&Weak::new(), &me) {
            ptr::null_mut()
        } else {
            Weak::into_raw(me) as *mut T
        }
    }
    unsafe fn from_ptr(ptr: *const T) -> Self {
        if ptr.is_null() {
            Weak::new()
        } else {
            Weak::from_raw(ptr)
        }
    }
}

unsafe impl<T> RefCnt for RcWeak<T> {
    type Base = T;
    // The data pointer is in a word-aligned allocation, so it is at least 2-aligned.
    const TAG: usize = 1;
    fn as_ptr(me: &Self) -> *mut T {
        if RcWeak::ptr_eq(&RcWeak::new(), me) {
            ptr::null_mut()
        } else {
            RcWeak::as_ptr(me) as *mut T
        }
    }
    fn into_ptr(me: Self) -> *mut T {
        if RcWeak::ptr_eq(&RcWeak::new(), &me) {
            ptr::null_mut()
        } else {
            RcWeak::into_raw(me) as *mut T
        }
    }
    unsafe fn from_ptr(ptr: *const T) -> Self {
        if ptr.is_null() {
            RcWeak::new()
        } else {
            RcWeak::from_raw(ptr)
        }
    }
}

// Strong and weak pointers to the same allocation have equal addresses, but must not share debts.
#[cfg(test)]
macro_rules! collision_tests {
    ($name: ident, $strategy: ty, $ptr: ident, $weak: ty, $wrap: expr, $empty: expr) => {
        mod $name {
            use super::*;
            use crate::Guard;

            #[allow(deprecated)] // We use "deprecated" testing strategies in here.
            type StrongSwap = ArcSwapAny<$ptr<&'static str>, $strategy>;
            #[allow(deprecated)]
            type WeakSwap = ArcSwapAny<$weak, $strategy>;

            fn setup() -> ($ptr<&'static str>, StrongSwap, WeakSwap) {
                let data = $ptr::new("Hello");
                let strong = StrongSwap::new($ptr::clone(&data));
                let weak = WeakSwap::new(($wrap)($ptr::downgrade(&data)));
                (data, strong, weak)
            }

            // A weak store must not pay a debt of a strong reader.
            #[test]
            fn strong_guard_dropped() {
                let (data, strong, weak) = setup();
                let guard = strong.load();
                weak.store($empty);
                drop(guard);
                drop(strong);
                assert_eq!(1, $ptr::strong_count(&data));
                assert_eq!(0, $ptr::weak_count(&data));
            }

            #[test]
            fn strong_guard_into_inner() {
                let (data, strong, weak) = setup();
                let guard = strong.load();
                weak.store($empty);
                let full = Guard::into_inner(guard);
                drop(strong);
                assert_eq!(2, $ptr::strong_count(&data));
                drop(full);
                assert_eq!(1, $ptr::strong_count(&data));
                assert_eq!(0, $ptr::weak_count(&data));
            }

            // A strong store must not pay a debt of a weak reader.
            #[test]
            fn weak_guard() {
                let (data, strong, weak) = setup();
                let guard = weak.load();
                strong.store($ptr::new("Other"));
                drop(guard);
                drop(weak);
                drop(strong);
                assert_eq!(1, $ptr::strong_count(&data));
                assert_eq!(0, $ptr::weak_count(&data));
            }
        }
    };
}

macro_rules! t {
    ($name: ident, $strategy: ty) => {
        #[cfg(test)]
        mod $name {
            use alloc::rc::{Rc, Weak as RcWeak};
            use alloc::sync::{Arc, Weak};

            use crate::ArcSwapAny;

            collision_tests!(
                arc_weak,
                $strategy,
                Arc,
                Weak<&'static str>,
                core::convert::identity,
                Weak::new()
            );
            collision_tests!(
                arc_option_weak,
                $strategy,
                Arc,
                Option<Weak<&'static str>>,
                Some,
                None
            );
            collision_tests!(
                rc_weak,
                $strategy,
                Rc,
                RcWeak<&'static str>,
                core::convert::identity,
                RcWeak::new()
            );

            #[allow(deprecated)] // We use "deprecated" testing strategies in here.
            type ArcSwapWeak<T> = ArcSwapAny<Weak<T>, $strategy>;

            // Convert to weak, push it through the shared and pull it out again.
            #[test]
            fn there_and_back() {
                let data = Arc::new("Hello");
                let shared = ArcSwapWeak::new(Arc::downgrade(&data));
                assert_eq!(1, Arc::strong_count(&data));
                assert_eq!(1, Arc::weak_count(&data));
                let weak = shared.load();
                assert_eq!("Hello", *weak.upgrade().unwrap());
                assert!(Arc::ptr_eq(&data, &weak.upgrade().unwrap()));
            }

            // Replace a weak pointer with a NULL one
            #[test]
            fn reset() {
                let data = Arc::new("Hello");
                let shared = ArcSwapWeak::new(Arc::downgrade(&data));
                assert_eq!(1, Arc::strong_count(&data));
                assert_eq!(1, Arc::weak_count(&data));

                // An empty weak (eg. NULL)
                shared.store(Weak::new());
                assert_eq!(1, Arc::strong_count(&data));
                assert_eq!(0, Arc::weak_count(&data));

                let weak = shared.load();
                assert!(weak.upgrade().is_none());
            }

            // Destroy the underlying data while the weak is still stored inside. Should make it go
            // NULL-ish
            #[test]
            fn destroy() {
                let data = Arc::new("Hello");
                let shared = ArcSwapWeak::new(Arc::downgrade(&data));

                drop(data);
                let weak = shared.load();
                assert!(weak.upgrade().is_none());
            }
        }
    };
}

t!(tests_default, crate::DefaultStrategy);
#[cfg(feature = "internal-test-strategies")]
t!(
    tests_full_slots,
    crate::strategy::test_strategies::FillFastSlots
);
