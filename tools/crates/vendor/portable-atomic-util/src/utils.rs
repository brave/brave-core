// SPDX-License-Identifier: Apache-2.0 OR MIT

#![cfg_attr(not(feature = "alloc"), allow(dead_code, unused_macros, unused_imports))]

// rustfmt-compatible cfg_select/cfg_if alternative
// Note: This macro is cfg_sel!({ }), not cfg_sel! { }.
// An extra brace is used in input to make contents rustfmt-able.
macro_rules! cfg_sel {
    ({#[cfg(else)] { $($output:tt)* }}) => {
        $($output)*
    };
    ({
        #[cfg($cfg:meta)]
        { $($output:tt)* }
        $($( $rest:tt )+)?
    }) => {
        #[cfg($cfg)]
        cfg_sel! {{#[cfg(else)] { $($output)* }}}
        $(
            #[cfg(not($cfg))]
            cfg_sel! {{ $($rest)+ }}
        )?
    };
}

// This module provides core::ptr strict_provenance/exposed_provenance polyfill for pre-1.84 rustc.
pub(crate) mod ptr {
    cfg_sel!({
        #[cfg(not(portable_atomic_no_strict_provenance))]
        {
            pub(crate) use core::ptr::without_provenance_mut;
        }
        #[cfg(else)]
        {
            #[inline(always)]
            #[must_use]
            pub(crate) const fn without_provenance_mut<T>(addr: usize) -> *mut T {
                // An int-to-pointer transmute currently has exactly the intended semantics: it creates a
                // pointer without provenance. Note that this is *not* a stable guarantee about transmute
                // semantics, it relies on sysroot crates having special status.
                // SAFETY: every valid integer is also a valid pointer (as long as you don't dereference that
                // pointer).
                #[cfg(miri)]
                unsafe {
                    core::mem::transmute(addr)
                }
                // const transmute requires Rust 1.56.
                // Using transmute doesn't work with CHERI: https://github.com/kent-weak-memory/rust/blob/0c0ca909de877f889629057e1ddf139527446d75/library/core/src/ptr/mod.rs#L607
                #[cfg(not(miri))]
                {
                    addr as *mut T
                }
            }

            pub(crate) trait PtrExt<T: ?Sized>: Copy {
                #[must_use]
                fn addr(self) -> usize;
            }
            impl<T: ?Sized> PtrExt<T> for *const T {
                #[inline(always)]
                #[must_use]
                fn addr(self) -> usize {
                    // A pointer-to-integer transmute currently has exactly the right semantics: it returns the
                    // address without exposing the provenance. Note that this is *not* a stable guarantee about
                    // transmute semantics, it relies on sysroot crates having special status.
                    // SAFETY: Pointer-to-integer transmutes are valid (if you are okay with losing the
                    // provenance).
                    #[cfg(miri)]
                    unsafe {
                        core::mem::transmute(self as *const ())
                    }
                    // Using transmute doesn't work with CHERI: https://github.com/kent-weak-memory/rust/blob/0c0ca909de877f889629057e1ddf139527446d75/library/core/src/ptr/mut_ptr.rs#L210
                    #[cfg(not(miri))]
                    {
                        self as *const () as usize
                    }
                }
            }
        }
    });

    /// Creates a new pointer with the metadata of `other`.
    #[inline]
    #[must_use]
    pub(crate) fn with_metadata_of<T, U: ?Sized>(this: *mut T, mut other: *const U) -> *mut U {
        let target = &mut other as *mut *const U as *mut *const u8;

        // SAFETY: In case of a thin pointer, this operations is identical
        // to a simple assignment. In case of a fat pointer, with the current
        // fat pointer layout implementation, the first field of such a
        // pointer is always the data pointer, which is likewise assigned.
        unsafe { *target = this as *const u8 }
        other as *mut U
    }

    // Based on <pointer>::byte_add stabilized in Rust 1.75.
    #[inline]
    #[must_use]
    pub(crate) unsafe fn byte_add<T: ?Sized>(ptr: *mut T, count: usize) -> *mut T {
        // SAFETY: the caller must uphold the safety contract for `add`.
        unsafe { with_metadata_of((ptr as *mut u8).add(count), ptr) }
    }

    // Based on <pointer>::byte_sub stabilized in Rust 1.75.
    #[inline]
    #[must_use]
    pub(crate) unsafe fn byte_sub<T: ?Sized>(ptr: *mut T, count: usize) -> *mut T {
        // SAFETY: the caller must uphold the safety contract for `sub`.
        unsafe { with_metadata_of((ptr as *mut u8).sub(count), ptr) }
    }
}
