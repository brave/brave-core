use super::*;

impl<T> Format for core::cell::Cell<T>
where
    T: Format + Copy,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "Cell {{ value: {=?} }})", self.get())
    }
}

impl<T> Format for core::cell::RefCell<T>
where
    T: Format,
{
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        internp!("RefCell {{ value: <borrowed> }}|RefCell {{ value: {=?} }}")
    }

    #[inline]
    fn _format_data(&self) {
        match self.try_borrow() {
            Err(_) => export::u8(&0),
            Ok(x) => {
                export::u8(&1);
                export::istr(&T::_format_tag());
                x._format_data()
            }
        }
    }
}

impl Format for core::cell::BorrowError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "BorrowError")
    }
}

impl Format for core::cell::BorrowMutError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "BorrowMutError")
    }
}
