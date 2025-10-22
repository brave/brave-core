use core::marker::PhantomData;

pub struct Parse<T, U>(T, PhantomData<U>);

// FIXME: should be `PhantomData<fn(T)->U>`
// blocked by feature(const_fn_fn_ptr_basics)

impl<T, U> Parse<T, U> {
    pub const fn new(t: T) -> Self {
        Self(t, PhantomData)
    }
}

impl Parse<&str, bool> {
    pub const fn const_eval(&self) -> bool {
        if crate::str::equal(self.0, "true") {
            return true;
        }
        if crate::str::equal(self.0, "false") {
            return false;
        }
        constfn_panic!("parse error")
    }
}

impl Parse<&str, &str> {
    pub const fn const_eval(&self) -> &str {
        self.0
    }
}

impl Parse<&str, char> {
    pub const fn const_eval(&self) -> char {
        let s = self.0.as_bytes();
        if let Some((ch, count)) = crate::utf8::next_char(s) {
            if count == s.len() {
                return ch;
            }
        }
        constfn_panic!("parse error")
    }
}

trait IsSignedInteger {
    const OUTPUT: bool;
}

macro_rules! mark_signed_integer {
    ($s: expr, $($ty:ty),+) => {
        $(
            impl IsSignedInteger for $ty {
                const OUTPUT: bool = $s;
            }
        )+
    }
}

mark_signed_integer!(true, i8, i16, i32, i64, i128, isize);
mark_signed_integer!(false, u8, u16, u32, u64, u128, usize);

macro_rules! impl_integer_parse {
    ($($ty: ty),+) => {$(
        impl Parse<&str, $ty> {
            pub const fn const_eval(&self) -> $ty {
                let s = self.0.as_bytes();
                let is_signed = <$ty as IsSignedInteger>::OUTPUT;
                let (is_positive, digits) = match s {
                    [] => constfn_panic!("parse error"),
                    [x, xs @ ..] => match x {
                        b'+' => (true, xs),
                        b'-' if is_signed => (false, xs),
                        _ => (true, s),
                    },
                };

                let mut ans: $ty = 0;
                let mut i = 0;
                while i < digits.len() {
                    let x = crate::ascii::num_from_dec_digit(digits[i]);

                    match ans.checked_mul(10) {
                        Some(val) => {
                            let val = if is_positive {
                                val.checked_add(x as _)
                            } else {
                                val.checked_sub(x as _)
                            };
                            match val {
                                Some(val) => ans = val,
                                None => constfn_panic!("parse error"),
                            }
                        }
                        None => constfn_panic!("parse error"),
                    };

                    i += 1;
                }

                ans
            }
        }
    )+};
}

impl_integer_parse!(u8, u16, u32, u64, u128, usize);
impl_integer_parse!(i8, i16, i32, i64, i128, isize);

/// Parse a value from a string slice.
///
/// The output type must be one of
///
/// + [`&str`]
/// + [`char`]
/// + [`bool`]
/// + [`u8`], [`u16`], [`u32`], [`u64`], [`u128`], [`usize`]
/// + [`i8`], [`i16`], [`i32`], [`i64`], [`i128`], [`isize`]
///
/// # Examples
///
/// ```
/// const S1: &str = "true";
/// const X1: bool = const_str::parse!(S1, bool);
/// assert_eq!(X1, true);
///
/// const S2: &str = "42";
/// const X2: u32 = const_str::parse!(S2, u32);
/// assert_eq!(X2, 42);
///
/// const S3: &str = "-1";
/// const X3: i8 = const_str::parse!(S3, i8);
/// assert_eq!(X3, -1);
/// ```
#[macro_export]
macro_rules! parse {
    ($s: expr, $ty: ty) => {{
        $crate::__ctfe::Parse::<_, $ty>::new($s).const_eval()
    }};
}

#[test]
fn test_parse() {
    macro_rules! test_parse {
        ($s: expr, $ty: tt) => {{
            const OUTPUT: $ty = $crate::parse!($s, $ty);
            let ans: $ty = $s.parse().unwrap();
            assert_eq!(OUTPUT, ans)
        }};
    }

    test_parse!("true", bool);
    test_parse!("false", bool);

    test_parse!("啊", char);

    test_parse!("0", u8);
    test_parse!("-1", i8);
    test_parse!("+42000", u32);
    test_parse!("-42000", i32);
}
