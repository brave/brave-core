use core::{
    marker::PhantomData,
    num::{NonZeroU32, NonZeroU64},
    ptr::NonNull,
    sync::atomic::{AtomicU64, Ordering},
};

use super::{Node, Stack};

pub struct AtomicPtr<N>
where
    N: Node,
{
    inner: AtomicU64,
    _marker: PhantomData<*mut N>,
}

impl<N> AtomicPtr<N>
where
    N: Node,
{
    pub const fn null() -> Self {
        Self {
            inner: AtomicU64::new(0),
            _marker: PhantomData,
        }
    }

    fn compare_and_exchange_weak(
        &self,
        current: Option<NonNullPtr<N>>,
        new: Option<NonNullPtr<N>>,
        success: Ordering,
        failure: Ordering,
    ) -> Result<(), Option<NonNullPtr<N>>> {
        self.inner
            .compare_exchange_weak(
                current
                    .map(|pointer| pointer.into_u64())
                    .unwrap_or_default(),
                new.map(|pointer| pointer.into_u64()).unwrap_or_default(),
                success,
                failure,
            )
            .map(drop)
            .map_err(NonNullPtr::from_u64)
    }

    fn load(&self, order: Ordering) -> Option<NonNullPtr<N>> {
        NonZeroU64::new(self.inner.load(order)).map(|inner| NonNullPtr {
            inner,
            _marker: PhantomData,
        })
    }

    fn store(&self, value: Option<NonNullPtr<N>>, order: Ordering) {
        self.inner.store(
            value.map(|pointer| pointer.into_u64()).unwrap_or_default(),
            order,
        )
    }
}

pub struct NonNullPtr<N>
where
    N: Node,
{
    inner: NonZeroU64,
    _marker: PhantomData<*mut N>,
}

impl<N> Clone for NonNullPtr<N>
where
    N: Node,
{
    fn clone(&self) -> Self {
        *self
    }
}

impl<N> Copy for NonNullPtr<N> where N: Node {}

impl<N> NonNullPtr<N>
where
    N: Node,
{
    pub fn as_ptr(&self) -> *mut N {
        self.inner.get() as *mut N
    }

    pub fn from_static_mut_ref(ref_: &'static mut N) -> NonNullPtr<N> {
        let non_null = NonNull::from(ref_);
        Self::from_non_null(non_null)
    }

    fn from_non_null(ptr: NonNull<N>) -> Self {
        let address = ptr.as_ptr() as u32;
        let tag = initial_tag().get();

        let value = (u64::from(tag) << 32) | u64::from(address);

        Self {
            inner: unsafe { NonZeroU64::new_unchecked(value) },
            _marker: PhantomData,
        }
    }

    fn from_u64(value: u64) -> Option<Self> {
        NonZeroU64::new(value).map(|inner| Self {
            inner,
            _marker: PhantomData,
        })
    }

    fn non_null(&self) -> NonNull<N> {
        unsafe { NonNull::new_unchecked(self.inner.get() as *mut N) }
    }

    fn tag(&self) -> NonZeroU32 {
        unsafe { NonZeroU32::new_unchecked((self.inner.get() >> 32) as u32) }
    }

    fn into_u64(self) -> u64 {
        self.inner.get()
    }

    fn increase_tag(&mut self) {
        let address = self.as_ptr() as u32;

        let new_tag = self
            .tag()
            .get()
            .checked_add(1)
            .map(|val| unsafe { NonZeroU32::new_unchecked(val) })
            .unwrap_or_else(initial_tag)
            .get();

        let value = (u64::from(new_tag) << 32) | u64::from(address);

        self.inner = unsafe { NonZeroU64::new_unchecked(value) };
    }
}

fn initial_tag() -> NonZeroU32 {
    unsafe { NonZeroU32::new_unchecked(1) }
}

pub unsafe fn push<N>(stack: &Stack<N>, new_top: NonNullPtr<N>)
where
    N: Node,
{
    let mut top = stack.top.load(Ordering::Relaxed);

    loop {
        new_top
            .non_null()
            .as_ref()
            .next()
            .store(top, Ordering::Relaxed);

        if let Err(p) = stack.top.compare_and_exchange_weak(
            top,
            Some(new_top),
            Ordering::Release,
            Ordering::Relaxed,
        ) {
            top = p;
        } else {
            return;
        }
    }
}

pub fn try_pop<N>(stack: &Stack<N>) -> Option<NonNullPtr<N>>
where
    N: Node,
{
    loop {
        if let Some(mut top) = stack.top.load(Ordering::Acquire) {
            let next = unsafe { top.non_null().as_ref().next().load(Ordering::Relaxed) };

            if stack
                .top
                .compare_and_exchange_weak(Some(top), next, Ordering::Release, Ordering::Relaxed)
                .is_ok()
            {
                top.increase_tag();

                return Some(top);
            }
        } else {
            // stack observed as empty
            return None;
        }
    }
}
