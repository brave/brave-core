use core::ops::{ Deref, DerefMut };


pub struct ScopeGuard<'a, T>(pub &'a mut T, pub fn(&mut T));

impl<T> Deref for ScopeGuard<'_, T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        self.0
    }
}

impl<T> DerefMut for ScopeGuard<'_, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.0
    }
}

impl<T> Drop for ScopeGuard<'_, T> {
    #[inline]
    fn drop(&mut self) {
        (self.1)(self.0);
    }
}
