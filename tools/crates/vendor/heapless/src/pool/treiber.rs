use core::mem::ManuallyDrop;

#[cfg_attr(target_arch = "x86", path = "treiber/cas.rs")]
#[cfg_attr(arm_llsc, path = "treiber/llsc.rs")]
mod impl_;

pub use impl_::{AtomicPtr, NonNullPtr};

pub struct Stack<N>
where
    N: Node,
{
    top: AtomicPtr<N>,
}

impl<N> Stack<N>
where
    N: Node,
{
    pub const fn new() -> Self {
        Self {
            top: AtomicPtr::null(),
        }
    }

    /// # Safety
    /// - `node` must be a valid pointer
    /// - aliasing rules must be enforced by the caller. e.g, the same `node` may not be pushed more than once
    pub unsafe fn push(&self, node: NonNullPtr<N>) {
        impl_::push(self, node)
    }

    pub fn try_pop(&self) -> Option<NonNullPtr<N>> {
        impl_::try_pop(self)
    }
}

pub trait Node: Sized {
    type Data;

    fn next(&self) -> &AtomicPtr<Self>;
    fn next_mut(&mut self) -> &mut AtomicPtr<Self>;
}

pub union UnionNode<T> {
    next: ManuallyDrop<AtomicPtr<UnionNode<T>>>,
    pub data: ManuallyDrop<T>,
}

impl<T> Node for UnionNode<T> {
    type Data = T;

    fn next(&self) -> &AtomicPtr<Self> {
        unsafe { &self.next }
    }

    fn next_mut(&mut self) -> &mut AtomicPtr<Self> {
        unsafe { &mut self.next }
    }
}

pub struct StructNode<T> {
    pub next: ManuallyDrop<AtomicPtr<StructNode<T>>>,
    pub data: ManuallyDrop<T>,
}

impl<T> Node for StructNode<T> {
    type Data = T;

    fn next(&self) -> &AtomicPtr<Self> {
        &self.next
    }

    fn next_mut(&mut self) -> &mut AtomicPtr<Self> {
        &mut self.next
    }
}

#[cfg(test)]
mod tests {
    use core::mem;

    use super::*;

    #[test]
    fn node_is_never_zero_sized() {
        struct Zst;

        assert_ne!(mem::size_of::<UnionNode<Zst>>(), 0);
    }
}
