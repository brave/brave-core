use super::*;

impl<T> Format for core::ptr::NonNull<T> {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "{}", self.as_ptr())
    }
}

#[cfg(c_variadic)]
macro_rules! fnptr_format_cvariadic {
    ($($Arg:ident),+) => {
        impl<Ret, $($Arg),*> Format for extern "C" fn($($Arg),* , ...) -> Ret {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, "{}", (*self as usize) as *const ())
            }
        }
        impl<Ret, $($Arg),*> Format for unsafe extern "C" fn($($Arg),* , ...) -> Ret {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, "{}", (*self as usize) as *const ())
            }
        }
    };
    () => {
        // C variadics require at least one other argument
    };
}

macro_rules! fnptr_format_args {
    ($($Arg:ident),*) => {
        impl<Ret, $($Arg),*> Format for extern "Rust" fn($($Arg),*) -> Ret {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, "{}", (*self as usize) as *const ())
            }
        }
        impl<Ret, $($Arg),*> Format for extern "C" fn($($Arg),*) -> Ret {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, "{}", (*self as usize) as *const ())
            }
        }
        impl<Ret, $($Arg),*> Format for unsafe extern "Rust" fn($($Arg),*) -> Ret {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, "{}", (*self as usize) as *const ())
            }
        }
        impl<Ret, $($Arg),*> Format for unsafe extern "C" fn($($Arg),*) -> Ret {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, "{}", (*self as usize) as *const ())
            }
        }
        #[cfg(c_variadic)]
        fnptr_format_cvariadic!{ $($Arg),* }
    };
}

// core::ptr has fnptr impls up to 12 arguments
// https://doc.rust-lang.org/src/core/ptr/mod.rs.html#1994
fnptr_format_args! {}
fnptr_format_args! { A }
fnptr_format_args! { A, B }
fnptr_format_args! { A, B, C }
fnptr_format_args! { A, B, C, D }
fnptr_format_args! { A, B, C, D, E }
fnptr_format_args! { A, B, C, D, E, F }
fnptr_format_args! { A, B, C, D, E, F, G }
fnptr_format_args! { A, B, C, D, E, F, G, H }
fnptr_format_args! { A, B, C, D, E, F, G, H, I }
fnptr_format_args! { A, B, C, D, E, F, G, H, I, J }
fnptr_format_args! { A, B, C, D, E, F, G, H, I, J, K }
fnptr_format_args! { A, B, C, D, E, F, G, H, I, J, K, L }
