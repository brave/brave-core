//! Debt handling.
//!
//! A debt is a reference count of a smart pointer that is owed. This module provides a lock-free
//! storage for debts.
//!
//! Each thread has its own node with bunch of slots. Only that thread can allocate debts in there,
//! but others are allowed to inspect and pay them. The nodes form a linked list for the reason of
//! inspection. The nodes are never removed (even after the thread terminates), but if the thread
//! gives it up, another (new) thread can claim it.
//!
//! The writers walk the whole chain and pay the debts (by bumping the ref counts) of the just
//! removed pointer ‒ the confirmed ones and the unconfirmed ones taken on their own storage.
//!
//! Each node has some fast (but fallible) nodes and a fallback node, with different algorithms to
//! claim them (see the relevant submodules).

use core::sync::atomic::AtomicUsize;
use core::sync::atomic::Ordering::*;

pub(crate) use self::list::{LocalNode, Node};
use super::RefCnt;

mod fast;
mod helping;
mod list;

/// One debt slot.
///
/// It may contain an „owed“ reference count.
#[derive(Debug)]
pub(crate) struct Debt {
    /// The owed pointer (encoded), or `NONE`.
    pub(crate) ptr: AtomicUsize,
    /// Address of the storage an unconfirmed debt was taken on, or `CONFIRMED`.
    ///
    /// An unconfirmed debt may name a reused address, so writers skip other storages' unconfirmed
    /// debts. Confirmed debts carry no key, because the `ArcSwap` may move while a guard lives.
    storage: AtomicUsize,
}

impl Debt {
    /// The `storage` of a confirmed debt (no `AtomicPtr` lives at address 0).
    const CONFIRMED: usize = 0;

    /// The value of pointer `3` should be pretty safe, for two reasons:
    ///
    /// * It's an odd number, but the pointers we have are likely aligned at least to the word size,
    ///   because the data at the end of the `Arc` has the counters.
    /// * It's in the very first page where NULL lives, so it's not mapped.
    ///
    /// Encoded values can't be `3` either (that needs an address 1..=3, in the same page).
    pub(crate) const NONE: usize = 0b11;

    /// The value stored in a debt slot for the given pointer.
    ///
    /// NULL stays untagged, because it carries no reference count of any kind.
    #[inline]
    pub(crate) fn encode<T: RefCnt>(ptr: *const T::Base) -> usize {
        if ptr.is_null() {
            0
        } else {
            ptr as usize | T::TAG
        }
    }
}

impl Default for Debt {
    fn default() -> Self {
        Debt {
            ptr: AtomicUsize::new(Self::NONE),
            storage: AtomicUsize::new(Self::CONFIRMED),
        }
    }
}

impl Debt {
    /// Marks the debt confirmed: writers of any storage may pay it from now on.
    #[inline]
    pub(crate) fn mark_confirmed(&self) {
        // Relaxed: a writer of this storage matches the address anyway and a writer of a moved
        // one runs after the move, which needs exclusive access, so after the whole `load`.
        self.storage.store(Self::CONFIRMED, Relaxed);
    }

    /// Tries to pay the given debt.
    ///
    /// If the debt is still there, for the given pointer, it is paid and `true` is returned. If it
    /// is empty or if there's some other pointer, it is not paid and `false` is returned, meaning
    /// the debt was paid previously by someone else.
    ///
    /// # Notes
    ///
    /// * It is possible that someone paid the debt and then someone else put a debt for the same
    ///   pointer in there. This is fine, as we'll just pay the debt for that someone else.
    /// * Only the reader that put the debt there pays it this way, so it knows the type.
    /// * The tag lets `Arc` and `Weak` of the same object coexist. It also relies on the fact the
    ///   same thing is not stuffed both inside an `Arc` and `Rc` or something like that, but that
    ///   sounds like a reasonable assumption. Someone storing it through `ArcSwap<T>` and someone
    ///   else with `ArcSwapOption<T>` will work.
    #[inline]
    pub(crate) fn pay<T: RefCnt>(&self, ptr: *const T::Base) -> bool {
        self.ptr
            // On failure, we have observed that the debt has been paid, but we need to establish a
            // happens-before relationship with that debt being paid before we do anything that
            // relies on the Arc's strong counter being incremented, so we need to Acquire.
            // Arc does not sufficiently take care of this, as the memory model permits its Drop's
            // `fetch_sub` to observe itself to be last remaining instance before the `fetch_add`s
            // from the debt being repaid, and only after that observation does Arc have an Acquire
            // fence.
            //
            // Unfortunately, we need SeqCst on the success
            .compare_exchange(Self::encode::<T>(ptr), Self::NONE, SeqCst, SeqCst)
            .is_ok()
    }

    /// Writer-side payment: pays a debt on `ptr` if it is confirmed or was taken on `storage_addr`.
    fn pay_for_storage<T: RefCnt>(&self, ptr: *const T::Base, storage_addr: usize) -> bool {
        let encoded = Self::encode::<T>(ptr);
        // SeqCst: keeps the load in the same total order as the writer's pointer swap and the
        // reader's confirm load (see fast.rs); it also acquires the `storage` store.
        if self.ptr.load(SeqCst) != encoded {
            return false;
        }
        let owner = self.storage.load(Relaxed);
        if owner != storage_addr && owner != Self::CONFIRMED {
            return false;
        }
        self.ptr
            .compare_exchange(encoded, Self::NONE, SeqCst, SeqCst)
            .is_ok()
    }

    /// Pays all the debts on the given pointer and the storage.
    pub(crate) fn pay_all<T, R>(ptr: *const T::Base, storage_addr: usize, replacement: R)
    where
        T: RefCnt,
        R: Fn() -> T,
    {
        LocalNode::with(|local| {
            let val = unsafe { T::from_ptr(ptr) };
            // Pre-pay one ref count that can be safely put into a debt slot to pay it.
            T::inc(&val);

            Node::traverse::<(), _>(|node| {
                // Make the cooldown trick know we are poking into this node.
                let _reservation = node.reserve_writer();

                local.help(node, storage_addr, &replacement);

                let all_slots = node
                    .fast_slots()
                    .chain(core::iter::once(node.helping_slot()));
                for slot in all_slots {
                    // Skips unconfirmed debts of other storages (they may name a reused address).
                    if slot.pay_for_storage::<T>(ptr, storage_addr) {
                        // Pre-pay one more, for another future slot
                        T::inc(&val);
                    }
                }

                None
            });
            // Implicit dec by dropping val in here, pair for the above
        })
    }
}

#[cfg(test)]
mod tests {
    use alloc::sync::Arc;

    /// Checks the assumption that arcs to ZSTs have different pointer values.
    #[test]
    fn arc_zst() {
        struct A;
        struct B;

        let a = Arc::new(A);
        let b = Arc::new(B);

        let aref: &A = &a;
        let bref: &B = &b;

        let aptr = aref as *const _ as usize;
        let bptr = bref as *const _ as usize;
        assert_ne!(aptr, bptr);
    }
}
