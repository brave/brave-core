use super::*;

impl<T> Format for alloc::boxed::Box<T>
where
    T: ?Sized + Format,
{
    delegate_format!(T, self, &*self);
}

impl<T> Format for alloc::rc::Rc<T>
where
    T: ?Sized + Format,
{
    delegate_format!(T, self, &*self);
}

#[cfg(not(no_cas))]
impl<T> Format for alloc::sync::Arc<T>
where
    T: ?Sized + Format,
{
    delegate_format!(T, self, &*self);
}

impl<T> Format for alloc::vec::Vec<T>
where
    T: Format,
{
    delegate_format!([T], self, self.as_slice());
}

impl Format for alloc::string::String {
    delegate_format!(str, self, self.as_str());
}

impl<'a, T> Format for alloc::borrow::Cow<'a, [T]>
where
    T: 'a + Format,
    [T]: alloc::borrow::ToOwned<Owned = alloc::vec::Vec<T>>,
{
    delegate_format!([T], self, self.as_ref());
}

impl<'a> Format for alloc::borrow::Cow<'a, str> {
    delegate_format!(str, self, self.as_ref());
}

impl Format for alloc::string::FromUtf8Error {
    delegate_format!(core::str::Utf8Error, self, &self.utf8_error());
}

impl Format for alloc::ffi::IntoStringError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "C string contained non-utf8 bytes")
    }
}

impl Format for alloc::ffi::NulError {
    fn format(&self, fmt: Formatter) {
        crate::write!(
            fmt,
            "nul byte found in provided data at position: {}",
            self.nul_position()
        )
    }
}
