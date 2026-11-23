use core::mem;
#[rustversion::since(1.39)]
use core::pin::Pin;
use core::ptr;

use crate::imports::{Arc, Rc};

/// A trait describing smart reference counted pointers.
///
/// Note that in a way [`Option<Arc<T>>`][Option] is also a smart reference counted pointer, just
/// one that can hold NULL.
///
/// The trait is unsafe, because a wrong implementation will break the [ArcSwapAny]
/// implementation and lead to UB.
///
/// This is not actually expected for downstream crate to implement, this is just means to reuse
/// code for [Arc] and [`Option<Arc>`][Option] variants. However, it is theoretically possible (if
/// you have your own [Arc] implementation).
///
/// It is also implemented for [Rc], but that is not considered very useful (because the
/// [ArcSwapAny] is not `Send` or `Sync`, therefore there's very little advantage for it to be
/// atomic).
///
/// # Safety
///
/// Aside from the obvious properties (like that incrementing and decrementing a reference count
/// cancel each out and that having less references tracked than how many things actually point to
/// the value is fine as long as the count doesn't drop to 0), it also must satisfy that if two
/// pointers have the same value, they point to the same object. This is specifically not true for
/// ZSTs, but it is true for `Arc`s of ZSTs, because they have the reference counts just after the
/// value. It would be fine to point to a type-erased version of the same object, though (if one
/// could use this trait with unsized types in the first place).
///
/// Two types whose pointers can be equal must manage the same reference count, unless they use
/// different [`TAG`](#associatedconstant.TAG)s. Types sharing the count (eg. `Arc`, `Option<Arc>`
/// and `Pin<Arc>`) must use the same tag. If `as_ptr` or `into_ptr` can return NULL, that value
/// must not own any reference count (NULL debts are shared by all the types).
///
/// Methods in this trait must not panic nor unwind.
///
/// Furthermore, the type should be Pin (eg. if the type is cloned or moved, it should still
/// point/deref to the same place in memory).
///
/// [Arc]: std::sync::Arc
/// [Rc]: std::rc::Rc
/// [ArcSwapAny]: crate::ArcSwapAny
pub unsafe trait RefCnt: Clone {
    /// The base type the pointer points to.
    type Base;

    /// Distinguishes debts of types that share addresses but not reference counts (eg. `Arc` and
    /// `Weak`).
    ///
    /// Non-null pointers of this type must have all the bits up to the highest set bit of `TAG`
    /// zero. Passing a pointer violating that to an `ArcSwapAny` panics.
    const TAG: usize = 0;

    /// Converts the smart pointer into a raw pointer, without affecting the reference count.
    ///
    /// This can be seen as kind of freezing the pointer ‒ it'll be later converted back using
    /// [`from_ptr`](#method.from_ptr).
    ///
    /// The pointer must point to the value stored (and the value must be the same as one returned
    /// by [`as_ptr`](#method.as_ptr).
    fn into_ptr(me: Self) -> *mut Self::Base;

    /// Provides a view into the smart pointer as a raw pointer.
    ///
    /// This must not affect the reference count ‒ the pointer is only borrowed.
    fn as_ptr(me: &Self) -> *mut Self::Base;

    /// Converts a raw pointer back into the smart pointer, without affecting the reference count.
    ///
    /// This is only called on values previously returned by [`into_ptr`](#method.into_ptr).
    /// However, it is not guaranteed to be 1:1 relation ‒ `from_ptr` may be called more times than
    /// `into_ptr` temporarily provided the reference count never drops under 1 during that time
    /// (the implementation sometimes owes a reference). These extra pointers will either be
    /// converted back using `into_ptr` or forgotten.
    ///
    /// # Safety
    ///
    /// This must not be called by code outside of this crate.
    unsafe fn from_ptr(ptr: *const Self::Base) -> Self;

    /// Increments the reference count by one.
    ///
    /// Return the pointer to the inner thing as a side effect.
    fn inc(me: &Self) -> *mut Self::Base {
        Self::into_ptr(Self::clone(me))
    }

    /// Decrements the reference count by one.
    ///
    /// Note this is called on a raw pointer (one previously returned by
    /// [`into_ptr`](#method.into_ptr). This may lead to dropping of the reference count to 0 and
    /// destruction of the internal pointer.
    ///
    /// # Safety
    ///
    /// This must not be called by code outside of this crate.
    unsafe fn dec(ptr: *const Self::Base) {
        drop(Self::from_ptr(ptr));
    }
}

/// Panics if the pointer has the bits needed for `T::TAG` occupied.
pub(crate) fn check_tag<T: RefCnt>(val: &T) {
    // All the bits up to the highest set bit of TAG (none for TAG == 0).
    let mask = (!0usize).checked_shr(T::TAG.leading_zeros()).unwrap_or(0);
    assert_eq!(
        T::as_ptr(val) as usize & mask,
        0,
        "Pointer not aligned enough for RefCnt::TAG"
    );
}

unsafe impl<T> RefCnt for Arc<T> {
    type Base = T;
    fn into_ptr(me: Arc<T>) -> *mut T {
        Arc::into_raw(me) as *mut T
    }
    fn as_ptr(me: &Arc<T>) -> *mut T {
        // Slightly convoluted way to do this, but this avoids stacked borrows violations. The same
        // intention as
        //
        // me as &T as *const T as *mut T
        //
        // We first create a "shallow copy" of me - one that doesn't really own its ref count
        // (that's OK, me _does_ own it, so it can't be destroyed in the meantime).
        // Then we can use into_raw (which preserves not having the ref count).
        //
        // We need to "revert" the changes we did. In current std implementation, the combination
        // of from_raw and forget is no-op. But formally, into_raw shall be paired with from_raw
        // and that read shall be paired with forget to properly "close the brackets". In future
        // versions of STD, these may become something else that's not really no-op (unlikely, but
        // possible), so we future-proof it a bit.

        // SAFETY: &T cast to *const T will always be aligned, initialised and valid for reads
        let ptr = Arc::into_raw(unsafe { ptr::read(me) });
        let ptr = ptr as *mut T;

        // SAFETY: We got the pointer from into_raw just above
        mem::forget(unsafe { Arc::from_raw(ptr) });

        ptr
    }
    unsafe fn from_ptr(ptr: *const T) -> Arc<T> {
        Arc::from_raw(ptr)
    }
}

unsafe impl<T> RefCnt for Rc<T> {
    type Base = T;
    fn into_ptr(me: Rc<T>) -> *mut T {
        Rc::into_raw(me) as *mut T
    }
    fn as_ptr(me: &Rc<T>) -> *mut T {
        // Slightly convoluted way to do this, but this avoids stacked borrows violations. The same
        // intention as
        //
        // me as &T as *const T as *mut T
        //
        // We first create a "shallow copy" of me - one that doesn't really own its ref count
        // (that's OK, me _does_ own it, so it can't be destroyed in the meantime).
        // Then we can use into_raw (which preserves not having the ref count).
        //
        // We need to "revert" the changes we did. In current std implementation, the combination
        // of from_raw and forget is no-op. But formally, into_raw shall be paired with from_raw
        // and that read shall be paired with forget to properly "close the brackets". In future
        // versions of STD, these may become something else that's not really no-op (unlikely, but
        // possible), so we future-proof it a bit.

        // SAFETY: &T cast to *const T will always be aligned, initialised and valid for reads
        let ptr = Rc::into_raw(unsafe { ptr::read(me) });
        let ptr = ptr as *mut T;

        // SAFETY: We got the pointer from into_raw just above
        mem::forget(unsafe { Rc::from_raw(ptr) });

        ptr
    }
    unsafe fn from_ptr(ptr: *const T) -> Rc<T> {
        Rc::from_raw(ptr)
    }
}

unsafe impl<T: RefCnt> RefCnt for Option<T> {
    type Base = T::Base;
    const TAG: usize = T::TAG;
    fn into_ptr(me: Option<T>) -> *mut T::Base {
        me.map(T::into_ptr).unwrap_or_else(ptr::null_mut)
    }
    fn as_ptr(me: &Option<T>) -> *mut T::Base {
        me.as_ref().map(T::as_ptr).unwrap_or_else(ptr::null_mut)
    }
    unsafe fn from_ptr(ptr: *const T::Base) -> Option<T> {
        if ptr.is_null() {
            None
        } else {
            Some(T::from_ptr(ptr))
        }
    }
}

// Pin is only available since Rust 1.33, but Pin::into_inner is from 1.39.
#[rustversion::since(1.39)]
unsafe impl<T> RefCnt for Pin<Arc<T>> {
    type Base = T;

    fn into_ptr(me: Pin<Arc<T>>) -> *mut T {
        // SAFETY: We only expose an opaque pointer, which maintains the `Pin` invariant.
        Arc::into_raw(unsafe { Pin::into_inner_unchecked(me) }) as *mut T
    }

    fn as_ptr(me: &Pin<Arc<T>>) -> *mut T {
        // Slightly convoluted way to do this, but this avoids stacked borrows violations. The same
        // intention as
        //
        // me as &T as *const T as *mut T
        //
        // We first create a "shallow copy" of me - one that doesn't really own its ref count
        // (that's OK, me _does_ own it, so it can't be destroyed in the meantime).
        // Then we can use into_raw (which preserves not having the ref count).
        //
        // We need to "revert" the changes we did. In current std implementation, the combination
        // of from_raw and forget is no-op. But formally, into_raw shall be paired with from_raw
        // and that read shall be paired with forget to properly "close the brackets". In future
        // versions of STD, these may become something else that's not really no-op (unlikely, but
        // possible), so we future-proof it a bit.

        // SAFETY: &T cast to *const T will always be aligned, initialised and valid for reads
        // We only expose an opaque pointer, which maintains the `Pin` invariant.
        let me = Arc::into_raw(unsafe { Pin::into_inner_unchecked(ptr::read(me)) });
        let ptr = me as *mut T;

        // SAFETY: We got the pointer from into_raw just above
        mem::forget(unsafe { Arc::from_raw(ptr) });

        ptr
    }

    unsafe fn from_ptr(ptr: *const T) -> Self {
        // SAFETY: `ptr` came from a previous `{into_ptr,as_ptr}` call, which is pinned.
        unsafe { Pin::new_unchecked(Arc::from_raw(ptr)) }
    }
}

// Pin is only available since Rust 1.33, but Pin::into_inner is from 1.39.
#[rustversion::since(1.39)]
unsafe impl<T> RefCnt for Pin<Rc<T>> {
    type Base = T;

    fn into_ptr(me: Pin<Rc<T>>) -> *mut T {
        // SAFETY: We only expose an opaque pointer, which maintains the `Pin` invariant.
        Rc::into_raw(unsafe { Pin::into_inner_unchecked(me) }) as *mut T
    }

    fn as_ptr(me: &Pin<Rc<T>>) -> *mut T {
        // Slightly convoluted way to do this, but this avoids stacked borrows violations. The same
        // intention as
        //
        // me as &T as *const T as *mut T
        //
        // We first create a "shallow copy" of me - one that doesn't really own its ref count
        // (that's OK, me _does_ own it, so it can't be destroyed in the meantime).
        // Then we can use into_raw (which preserves not having the ref count).
        //
        // We need to "revert" the changes we did. In current std implementation, the combination
        // of from_raw and forget is no-op. But formally, into_raw shall be paired with from_raw
        // and that read shall be paired with forget to properly "close the brackets". In future
        // versions of STD, these may become something else that's not really no-op (unlikely, but
        // possible), so we future-proof it a bit.

        // SAFETY: &T cast to *const T will always be aligned, initialised and valid for reads
        // We only expose an opaque pointer, which maintains the `Pin` invariant.
        let me = Rc::into_raw(unsafe { Pin::into_inner_unchecked(ptr::read(me)) });
        let ptr = me as *mut T;

        // SAFETY: We got the pointer from into_raw just above
        mem::forget(unsafe { Rc::from_raw(ptr) });

        ptr
    }

    unsafe fn from_ptr(ptr: *const T) -> Self {
        // SAFETY: `ptr` came from a previous `{into_ptr,as_ptr}` call, which is pinned.
        unsafe { Pin::new_unchecked(Rc::from_raw(ptr)) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ArcSwapAny;

    #[test]
    fn ref_cnt_arc() {
        struct Data(u32);

        let arc = Arc::new(Data(114514));
        let ptr = RefCnt::as_ptr(&arc);
        assert_eq!(ptr, RefCnt::into_ptr(arc));

        let arc: Arc<Data> = unsafe { RefCnt::from_ptr(ptr) };
        assert_eq!(arc.0, 114514);
        assert_eq!(ptr, RefCnt::as_ptr(&arc));
        assert_eq!(ptr, RefCnt::into_ptr(arc));

        // Let it drop.
        let _: Arc<Data> = unsafe { RefCnt::from_ptr(ptr) };
    }

    /// A pointer into the middle of the `Arc`'s data, so it is odd.
    #[derive(Clone)]
    struct Odd(Arc<[u8; 2]>);

    unsafe impl RefCnt for Odd {
        type Base = [u8; 2];
        const TAG: usize = 1;
        fn into_ptr(me: Self) -> *mut [u8; 2] {
            (RefCnt::into_ptr(me.0) as *mut u8).wrapping_add(1) as *mut [u8; 2]
        }
        fn as_ptr(me: &Self) -> *mut [u8; 2] {
            (RefCnt::as_ptr(&me.0) as *mut u8).wrapping_add(1) as *mut [u8; 2]
        }
        unsafe fn from_ptr(ptr: *const [u8; 2]) -> Self {
            Odd(RefCnt::from_ptr(
                (ptr as *const u8).wrapping_sub(1) as *const [u8; 2]
            ))
        }
    }

    #[test]
    #[should_panic(expected = "Pointer not aligned enough")]
    fn misaligned_tag() {
        ArcSwapAny::<Odd>::new(Odd(Arc::new([0, 0])));
    }

    // Pin is only available since Rust 1.33, but Pin::into_inner is from 1.39.
    #[rustversion::since(1.39)]
    mod pin {
        use super::*;
        use core::marker::PhantomPinned;

        #[test]
        fn ref_cnt_pin_arc() {
            struct Unmovable {
                value: u32,
                _phantom: PhantomPinned,
            }

            let pinned = Arc::pin(Unmovable {
                value: 114514,
                _phantom: PhantomPinned,
            });
            let ptr = RefCnt::as_ptr(&pinned);
            assert_eq!(ptr, RefCnt::into_ptr(pinned));

            let pinned: Pin<Arc<Unmovable>> = unsafe { RefCnt::from_ptr(ptr) };
            assert_eq!(pinned.value, 114514);
            assert_eq!(ptr, RefCnt::as_ptr(&pinned));
            assert_eq!(ptr, RefCnt::into_ptr(pinned));

            // Let it drop.
            let _: Pin<Arc<Unmovable>> = unsafe { RefCnt::from_ptr(ptr) };
        }

        #[test]
        fn ref_cnt_pin_rc() {
            struct Unmovable {
                value: u32,
                _phantom: PhantomPinned,
            }

            let pinned = Rc::pin(Unmovable {
                value: 114514,
                _phantom: PhantomPinned,
            });
            let ptr = RefCnt::as_ptr(&pinned);
            assert_eq!(ptr, RefCnt::into_ptr(pinned));

            let pinned: Pin<Rc<Unmovable>> = unsafe { RefCnt::from_ptr(ptr) };
            assert_eq!(pinned.value, 114514);
            assert_eq!(ptr, RefCnt::as_ptr(&pinned));
            assert_eq!(ptr, RefCnt::into_ptr(pinned));

            // Let it drop.
            let _: Pin<Rc<Unmovable>> = unsafe { RefCnt::from_ptr(ptr) };
        }
    }
}
