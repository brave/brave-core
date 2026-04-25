use core::{
    cell::UnsafeCell,
    ptr::{self, NonNull},
};

use super::{Node, Stack};

pub struct AtomicPtr<N>
where
    N: Node,
{
    inner: UnsafeCell<Option<NonNull<N>>>,
}

impl<N> AtomicPtr<N>
where
    N: Node,
{
    pub const fn null() -> Self {
        Self {
            inner: UnsafeCell::new(None),
        }
    }
}

pub struct NonNullPtr<N>
where
    N: Node,
{
    inner: NonNull<N>,
}

impl<N> NonNullPtr<N>
where
    N: Node,
{
    pub fn as_ptr(&self) -> *mut N {
        self.inner.as_ptr().cast()
    }

    pub fn from_static_mut_ref(ref_: &'static mut N) -> Self {
        Self {
            inner: NonNull::from(ref_),
        }
    }
}

impl<N> Clone for NonNullPtr<N>
where
    N: Node,
{
    fn clone(&self) -> Self {
        Self { inner: self.inner }
    }
}

impl<N> Copy for NonNullPtr<N> where N: Node {}

pub unsafe fn push<N>(stack: &Stack<N>, mut node: NonNullPtr<N>)
where
    N: Node,
{
    let top_addr = ptr::addr_of!(stack.top) as *mut usize;

    loop {
        let top = arch::load_link(top_addr);

        node.inner
            .as_mut()
            .next_mut()
            .inner
            .get()
            .write(NonNull::new(top as *mut _));

        if arch::store_conditional(node.inner.as_ptr() as usize, top_addr).is_ok() {
            break;
        }
    }
}

pub fn try_pop<N>(stack: &Stack<N>) -> Option<NonNullPtr<N>>
where
    N: Node,
{
    unsafe {
        let top_addr = ptr::addr_of!(stack.top) as *mut usize;

        loop {
            let top = arch::load_link(top_addr);

            if let Some(top) = NonNull::new(top as *mut N) {
                let next = &top.as_ref().next();

                if arch::store_conditional(
                    next.inner
                        .get()
                        .read()
                        .map(|non_null| non_null.as_ptr() as usize)
                        .unwrap_or_default(),
                    top_addr,
                )
                .is_ok()
                {
                    break Some(NonNullPtr { inner: top });
                }
            } else {
                arch::clear_load_link();

                break None;
            }
        }
    }
}

#[cfg(arm_llsc)]
mod arch {
    use core::arch::asm;

    #[inline(always)]
    pub fn clear_load_link() {
        unsafe { asm!("clrex", options(nomem, nostack)) }
    }

    /// # Safety
    /// - `addr` must be a valid pointer
    #[inline(always)]
    pub unsafe fn load_link(addr: *const usize) -> usize {
        let value;
        asm!("ldrex {}, [{}]", out(reg) value, in(reg) addr, options(nostack));
        value
    }

    /// # Safety
    /// - `addr` must be a valid pointer
    #[inline(always)]
    pub unsafe fn store_conditional(value: usize, addr: *mut usize) -> Result<(), ()> {
        let outcome: usize;
        asm!("strex {}, {}, [{}]", out(reg) outcome, in(reg) value, in(reg) addr, options(nostack));
        if outcome == 0 {
            Ok(())
        } else {
            Err(())
        }
    }
}
