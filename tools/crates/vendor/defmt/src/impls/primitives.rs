use crate::export;

use super::*;

macro_rules! prim {
    ($ty:ty, $fmt: literal, $self_:ident, $write:expr) => {
        impl Format for $ty {
            default_format!();

            #[inline]
            fn _format_tag() -> Str {
                internp!($fmt)
            }

            #[inline]
            fn _format_data(&$self_) {
                $write
            }
        }
    };
}

prim!(i8, "{=i8}", self, export::i8(self));
prim!(i16, "{=i16}", self, export::i16(self));
prim!(i32, "{=i32}", self, export::i32(self));
prim!(i64, "{=i64}", self, export::i64(self));
prim!(i128, "{=i128}", self, export::i128(self));
prim!(isize, "{=isize}", self, export::isize(self));
prim!(u8, "{=u8}", self, export::u8(self));
prim!(u16, "{=u16}", self, export::u16(self));
prim!(u32, "{=u32}", self, export::u32(self));
prim!(u64, "{=u64}", self, export::u64(self));
prim!(u128, "{=u128}", self, export::u128(self));
prim!(usize, "{=usize}", self, export::usize(self));
prim!(f32, "{=f32}", self, export::f32(self));
prim!(f64, "{=f64}", self, export::f64(self));
prim!(str, "{=str}", self, export::str(self));
prim!(bool, "{=bool}", self, export::bool(self));
prim!(Str, "{=istr}", self, export::istr(self));
prim!(char, "{=char}", self, export::char(self));

impl<T> Format for [T]
where
    T: Format,
{
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        internp!("{=[?]}")
    }

    #[inline]
    fn _format_data(&self) {
        export::fmt_slice(self);
    }
}

impl<T> Format for &'_ T
where
    T: Format + ?Sized,
{
    delegate_format!(T, self, self);
}

impl<T> Format for &'_ mut T
where
    T: Format + ?Sized,
{
    delegate_format!(T, self, self);
}

// Format raw pointer as hexadecimal
//
// First cast raw pointer to thin pointer, then to usize and finally format as hexadecimal.
impl<T> Format for *const T
where
    T: ?Sized,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "0x{:x}", *self as *const () as usize);
    }
}

impl<T> Format for *mut T
where
    T: ?Sized,
{
    fn format(&self, fmt: Formatter) {
        Format::format(&(*self as *const T), fmt)
    }
}
