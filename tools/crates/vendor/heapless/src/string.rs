use core::{
    cmp::Ordering,
    fmt,
    fmt::Write,
    hash, iter, ops,
    str::{self, Utf8Error},
};

use crate::Vec;

/// A fixed capacity [`String`](https://doc.rust-lang.org/std/string/struct.String.html)
pub struct String<const N: usize> {
    vec: Vec<u8, N>,
}

impl<const N: usize> String<N> {
    /// Constructs a new, empty `String` with a fixed capacity of `N` bytes
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// // allocate the string on the stack
    /// let mut s: String<4> = String::new();
    ///
    /// // allocate the string in a static variable
    /// static mut S: String<4> = String::new();
    /// ```
    #[inline]
    pub const fn new() -> Self {
        Self { vec: Vec::new() }
    }

    /// Convert UTF-8 bytes into a `String`.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::{String, Vec};
    ///
    /// let mut sparkle_heart = Vec::<u8, 4>::new();
    /// sparkle_heart.extend_from_slice(&[240, 159, 146, 150]);
    ///
    /// let sparkle_heart: String<4> = String::from_utf8(sparkle_heart)?;
    /// assert_eq!("💖", sparkle_heart);
    /// # Ok::<(), core::str::Utf8Error>(())
    /// ```
    ///
    /// Invalid UTF-8:
    ///
    /// ```
    /// use core::str::Utf8Error;
    /// use heapless::{String, Vec};
    ///
    /// let mut vec = Vec::<u8, 4>::new();
    /// vec.extend_from_slice(&[0, 159, 146, 150]);
    ///
    /// let e: Utf8Error = String::from_utf8(vec).unwrap_err();
    /// assert_eq!(e.valid_up_to(), 1);
    /// # Ok::<(), core::str::Utf8Error>(())
    /// ```
    #[inline]
    pub fn from_utf8(vec: Vec<u8, N>) -> Result<Self, Utf8Error> {
        core::str::from_utf8(&vec)?;
        Ok(Self { vec })
    }

    /// Convert UTF-8 bytes into a `String`, without checking that the string
    /// contains valid UTF-8.
    ///
    /// # Safety
    ///
    /// The bytes passed in must be valid UTF-8.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::{String, Vec};
    ///
    /// let mut sparkle_heart = Vec::<u8, 4>::new();
    /// sparkle_heart.extend_from_slice(&[240, 159, 146, 150]);
    ///
    /// // Safety: `sparkle_heart` Vec is known to contain valid UTF-8
    /// let sparkle_heart: String<4> = unsafe { String::from_utf8_unchecked(sparkle_heart) };
    /// assert_eq!("💖", sparkle_heart);
    /// ```
    #[inline]
    pub unsafe fn from_utf8_unchecked(vec: Vec<u8, N>) -> Self {
        Self { vec }
    }

    /// Converts a `String` into a byte vector.
    ///
    /// This consumes the `String`, so we do not need to copy its contents.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let s: String<4> = String::try_from("ab")?;
    /// let b = s.into_bytes();
    /// assert!(b.len() == 2);
    ///
    /// assert_eq!(&['a' as u8, 'b' as u8], &b[..]);
    /// # Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn into_bytes(self) -> Vec<u8, N> {
        self.vec
    }

    /// Extracts a string slice containing the entire string.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<4> = String::try_from("ab")?;
    /// assert!(s.as_str() == "ab");
    ///
    /// let _s = s.as_str();
    /// // s.push('c'); // <- cannot borrow `s` as mutable because it is also borrowed as immutable
    /// # Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn as_str(&self) -> &str {
        unsafe { str::from_utf8_unchecked(self.vec.as_slice()) }
    }

    /// Converts a `String` into a mutable string slice.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<4> = String::try_from("ab")?;
    /// let s = s.as_mut_str();
    /// s.make_ascii_uppercase();
    /// # Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn as_mut_str(&mut self) -> &mut str {
        unsafe { str::from_utf8_unchecked_mut(self.vec.as_mut_slice()) }
    }

    /// Returns a mutable reference to the contents of this `String`.
    ///
    /// # Safety
    ///
    /// This function is unsafe because it does not check that the bytes passed
    /// to it are valid UTF-8. If this constraint is violated, it may cause
    /// memory unsafety issues with future users of the `String`, as the rest of
    /// the library assumes that `String`s are valid UTF-8.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("hello")?;
    ///
    /// unsafe {
    ///     let vec = s.as_mut_vec();
    ///     assert_eq!(&[104, 101, 108, 108, 111][..], &vec[..]);
    ///
    ///     vec.reverse();
    /// }
    /// assert_eq!(s, "olleh");
    /// # Ok::<(), ()>(())
    /// ```
    pub unsafe fn as_mut_vec(&mut self) -> &mut Vec<u8, N> {
        &mut self.vec
    }

    /// Appends a given string slice onto the end of this `String`.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("foo")?;
    ///
    /// assert!(s.push_str("bar").is_ok());
    ///
    /// assert_eq!("foobar", s);
    ///
    /// assert!(s.push_str("tender").is_err());
    /// # Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn push_str(&mut self, string: &str) -> Result<(), ()> {
        self.vec.extend_from_slice(string.as_bytes())
    }

    /// Returns the maximum number of elements the String can hold
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<4> = String::new();
    /// assert!(s.capacity() == 4);
    /// ```
    #[inline]
    pub fn capacity(&self) -> usize {
        self.vec.capacity()
    }

    /// Appends the given [`char`] to the end of this `String`.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("abc")?;
    ///
    /// s.push('1').unwrap();
    /// s.push('2').unwrap();
    /// s.push('3').unwrap();
    ///
    /// assert!("abc123" == s.as_str());
    ///
    /// assert_eq!("abc123", s);
    /// # Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn push(&mut self, c: char) -> Result<(), ()> {
        match c.len_utf8() {
            1 => self.vec.push(c as u8).map_err(|_| {}),
            _ => self
                .vec
                .extend_from_slice(c.encode_utf8(&mut [0; 4]).as_bytes()),
        }
    }

    /// Shortens this `String` to the specified length.
    ///
    /// If `new_len` is greater than the string's current length, this has no
    /// effect.
    ///
    /// Note that this method has no effect on the allocated capacity
    /// of the string
    ///
    /// # Panics
    ///
    /// Panics if `new_len` does not lie on a [`char`] boundary.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("hello")?;
    ///
    /// s.truncate(2);
    ///
    /// assert_eq!("he", s);
    /// # Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn truncate(&mut self, new_len: usize) {
        if new_len <= self.len() {
            assert!(self.is_char_boundary(new_len));
            self.vec.truncate(new_len)
        }
    }

    /// Removes the last character from the string buffer and returns it.
    ///
    /// Returns [`None`] if this `String` is empty.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("foo")?;
    ///
    /// assert_eq!(s.pop(), Some('o'));
    /// assert_eq!(s.pop(), Some('o'));
    /// assert_eq!(s.pop(), Some('f'));
    ///
    /// assert_eq!(s.pop(), None);
    /// Ok::<(), ()>(())
    /// ```
    pub fn pop(&mut self) -> Option<char> {
        let ch = self.chars().rev().next()?;

        // pop bytes that correspond to `ch`
        for _ in 0..ch.len_utf8() {
            unsafe {
                self.vec.pop_unchecked();
            }
        }

        Some(ch)
    }

    /// Removes a [`char`] from this `String` at a byte position and returns it.
    ///
    /// Note: Because this shifts over the remaining elements, it has a
    /// worst-case performance of *O*(*n*).
    ///
    /// # Panics
    ///
    /// Panics if `idx` is larger than or equal to the `String`'s length,
    /// or if it does not lie on a [`char`] boundary.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("foo").unwrap();
    ///
    /// assert_eq!(s.remove(0), 'f');
    /// assert_eq!(s.remove(1), 'o');
    /// assert_eq!(s.remove(0), 'o');
    /// ```
    #[inline]
    pub fn remove(&mut self, index: usize) -> char {
        let ch = match self[index..].chars().next() {
            Some(ch) => ch,
            None => panic!("cannot remove a char from the end of a string"),
        };

        let next = index + ch.len_utf8();
        let len = self.len();
        let ptr = self.vec.as_mut_ptr();
        unsafe {
            core::ptr::copy(ptr.add(next), ptr.add(index), len - next);
            self.vec.set_len(len - (next - index));
        }
        ch
    }

    /// Truncates this `String`, removing all contents.
    ///
    /// While this means the `String` will have a length of zero, it does not
    /// touch its capacity.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// use heapless::String;
    ///
    /// let mut s: String<8> = String::try_from("foo")?;
    ///
    /// s.clear();
    ///
    /// assert!(s.is_empty());
    /// assert_eq!(0, s.len());
    /// assert_eq!(8, s.capacity());
    /// Ok::<(), ()>(())
    /// ```
    #[inline]
    pub fn clear(&mut self) {
        self.vec.clear()
    }
}

impl<const N: usize> Default for String<N> {
    fn default() -> Self {
        Self::new()
    }
}

impl<'a, const N: usize> TryFrom<&'a str> for String<N> {
    type Error = ();
    fn try_from(s: &'a str) -> Result<Self, Self::Error> {
        let mut new = String::new();
        new.push_str(s)?;
        Ok(new)
    }
}

impl<const N: usize> str::FromStr for String<N> {
    type Err = ();

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let mut new = String::new();
        new.push_str(s)?;
        Ok(new)
    }
}

impl<const N: usize> iter::FromIterator<char> for String<N> {
    fn from_iter<T: IntoIterator<Item = char>>(iter: T) -> Self {
        let mut new = String::new();
        for c in iter {
            new.push(c).unwrap();
        }
        new
    }
}

impl<'a, const N: usize> iter::FromIterator<&'a char> for String<N> {
    fn from_iter<T: IntoIterator<Item = &'a char>>(iter: T) -> Self {
        let mut new = String::new();
        for c in iter {
            new.push(*c).unwrap();
        }
        new
    }
}

impl<'a, const N: usize> iter::FromIterator<&'a str> for String<N> {
    fn from_iter<T: IntoIterator<Item = &'a str>>(iter: T) -> Self {
        let mut new = String::new();
        for c in iter {
            new.push_str(c).unwrap();
        }
        new
    }
}

impl<const N: usize> Clone for String<N> {
    fn clone(&self) -> Self {
        Self {
            vec: self.vec.clone(),
        }
    }
}

impl<const N: usize> fmt::Debug for String<N> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        <str as fmt::Debug>::fmt(self, f)
    }
}

impl<const N: usize> fmt::Display for String<N> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        <str as fmt::Display>::fmt(self, f)
    }
}

impl<const N: usize> hash::Hash for String<N> {
    #[inline]
    fn hash<H: hash::Hasher>(&self, hasher: &mut H) {
        <str as hash::Hash>::hash(self, hasher)
    }
}

impl<const N: usize> fmt::Write for String<N> {
    fn write_str(&mut self, s: &str) -> Result<(), fmt::Error> {
        self.push_str(s).map_err(|_| fmt::Error)
    }

    fn write_char(&mut self, c: char) -> Result<(), fmt::Error> {
        self.push(c).map_err(|_| fmt::Error)
    }
}

impl<const N: usize> ops::Deref for String<N> {
    type Target = str;

    fn deref(&self) -> &str {
        self.as_str()
    }
}

impl<const N: usize> ops::DerefMut for String<N> {
    fn deref_mut(&mut self) -> &mut str {
        self.as_mut_str()
    }
}

impl<const N: usize> AsRef<str> for String<N> {
    #[inline]
    fn as_ref(&self) -> &str {
        self
    }
}

impl<const N: usize> AsRef<[u8]> for String<N> {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl<const N1: usize, const N2: usize> PartialEq<String<N2>> for String<N1> {
    fn eq(&self, rhs: &String<N2>) -> bool {
        str::eq(&**self, &**rhs)
    }

    fn ne(&self, rhs: &String<N2>) -> bool {
        str::ne(&**self, &**rhs)
    }
}

// String<N> == str
impl<const N: usize> PartialEq<str> for String<N> {
    #[inline]
    fn eq(&self, other: &str) -> bool {
        str::eq(&self[..], &other[..])
    }
    #[inline]
    fn ne(&self, other: &str) -> bool {
        str::ne(&self[..], &other[..])
    }
}

// String<N> == &'str
impl<const N: usize> PartialEq<&str> for String<N> {
    #[inline]
    fn eq(&self, other: &&str) -> bool {
        str::eq(&self[..], &other[..])
    }
    #[inline]
    fn ne(&self, other: &&str) -> bool {
        str::ne(&self[..], &other[..])
    }
}

// str == String<N>
impl<const N: usize> PartialEq<String<N>> for str {
    #[inline]
    fn eq(&self, other: &String<N>) -> bool {
        str::eq(&self[..], &other[..])
    }
    #[inline]
    fn ne(&self, other: &String<N>) -> bool {
        str::ne(&self[..], &other[..])
    }
}

// &'str == String<N>
impl<const N: usize> PartialEq<String<N>> for &str {
    #[inline]
    fn eq(&self, other: &String<N>) -> bool {
        str::eq(&self[..], &other[..])
    }
    #[inline]
    fn ne(&self, other: &String<N>) -> bool {
        str::ne(&self[..], &other[..])
    }
}

impl<const N: usize> Eq for String<N> {}

impl<const N1: usize, const N2: usize> PartialOrd<String<N2>> for String<N1> {
    #[inline]
    fn partial_cmp(&self, other: &String<N2>) -> Option<Ordering> {
        PartialOrd::partial_cmp(&**self, &**other)
    }
}

impl<const N: usize> Ord for String<N> {
    #[inline]
    fn cmp(&self, other: &Self) -> Ordering {
        Ord::cmp(&**self, &**other)
    }
}

macro_rules! impl_try_from_num {
    ($num:ty, $size:expr) => {
        impl<const N: usize> core::convert::TryFrom<$num> for String<N> {
            type Error = ();
            fn try_from(s: $num) -> Result<Self, Self::Error> {
                let mut new = String::new();
                write!(&mut new, "{}", s).map_err(|_| ())?;
                Ok(new)
            }
        }
    };
}

impl_try_from_num!(i8, 4);
impl_try_from_num!(i16, 6);
impl_try_from_num!(i32, 11);
impl_try_from_num!(i64, 20);

impl_try_from_num!(u8, 3);
impl_try_from_num!(u16, 5);
impl_try_from_num!(u32, 10);
impl_try_from_num!(u64, 20);

#[cfg(test)]
mod tests {
    use crate::{String, Vec};
    use core::convert::TryFrom;

    #[test]
    fn static_new() {
        static mut _S: String<8> = String::new();
    }

    #[test]
    fn clone() {
        let s1: String<20> = String::try_from("abcd").unwrap();
        let mut s2 = s1.clone();
        s2.push_str(" efgh").unwrap();

        assert_eq!(s1, "abcd");
        assert_eq!(s2, "abcd efgh");
    }

    #[test]
    fn cmp() {
        let s1: String<4> = String::try_from("abcd").unwrap();
        let s2: String<4> = String::try_from("zzzz").unwrap();

        assert!(s1 < s2);
    }

    #[test]
    fn cmp_heterogenous_size() {
        let s1: String<4> = String::try_from("abcd").unwrap();
        let s2: String<8> = String::try_from("zzzz").unwrap();

        assert!(s1 < s2);
    }

    #[test]
    fn debug() {
        use core::fmt::Write;

        let s: String<8> = String::try_from("abcd").unwrap();
        let mut std_s = std::string::String::new();
        write!(std_s, "{:?}", s).unwrap();
        assert_eq!("\"abcd\"", std_s);
    }

    #[test]
    fn display() {
        use core::fmt::Write;

        let s: String<8> = String::try_from("abcd").unwrap();
        let mut std_s = std::string::String::new();
        write!(std_s, "{}", s).unwrap();
        assert_eq!("abcd", std_s);
    }

    #[test]
    fn empty() {
        let s: String<4> = String::new();
        assert!(s.capacity() == 4);
        assert_eq!(s, "");
        assert_eq!(s.len(), 0);
        assert_ne!(s.len(), 4);
    }

    #[test]
    fn try_from() {
        let s: String<4> = String::try_from("123").unwrap();
        assert!(s.len() == 3);
        assert_eq!(s, "123");

        let e: () = String::<2>::try_from("123").unwrap_err();
        assert_eq!(e, ());
    }

    #[test]
    fn from_str() {
        use core::str::FromStr;

        let s: String<4> = String::<4>::from_str("123").unwrap();
        assert!(s.len() == 3);
        assert_eq!(s, "123");

        let e: () = String::<2>::from_str("123").unwrap_err();
        assert_eq!(e, ());
    }

    #[test]
    fn from_iter() {
        let mut v: Vec<char, 5> = Vec::new();
        v.push('h').unwrap();
        v.push('e').unwrap();
        v.push('l').unwrap();
        v.push('l').unwrap();
        v.push('o').unwrap();
        let string1: String<5> = v.iter().collect(); //&char
        let string2: String<5> = "hello".chars().collect(); //char
        assert_eq!(string1, "hello");
        assert_eq!(string2, "hello");
    }

    #[test]
    #[should_panic]
    fn from_panic() {
        let _: String<4> = String::try_from("12345").unwrap();
    }

    #[test]
    fn try_from_num() {
        let v: String<20> = String::try_from(18446744073709551615 as u64).unwrap();
        assert_eq!(v, "18446744073709551615");

        let e: () = String::<2>::try_from(18446744073709551615 as u64).unwrap_err();
        assert_eq!(e, ());
    }

    #[test]
    fn into_bytes() {
        let s: String<4> = String::try_from("ab").unwrap();
        let b: Vec<u8, 4> = s.into_bytes();
        assert_eq!(b.len(), 2);
        assert_eq!(&['a' as u8, 'b' as u8], &b[..]);
    }

    #[test]
    fn as_str() {
        let s: String<4> = String::try_from("ab").unwrap();

        assert_eq!(s.as_str(), "ab");
        // should be moved to fail test
        //    let _s = s.as_str();
        // s.push('c'); // <- cannot borrow `s` as mutable because it is also borrowed as immutable
    }

    #[test]
    fn as_mut_str() {
        let mut s: String<4> = String::try_from("ab").unwrap();
        let s = s.as_mut_str();
        s.make_ascii_uppercase();
        assert_eq!(s, "AB");
    }

    #[test]
    fn push_str() {
        let mut s: String<8> = String::try_from("foo").unwrap();
        assert!(s.push_str("bar").is_ok());
        assert_eq!("foobar", s);
        assert_eq!(s, "foobar");
        assert!(s.push_str("tender").is_err());
        assert_eq!("foobar", s);
        assert_eq!(s, "foobar");
    }

    #[test]
    fn push() {
        let mut s: String<6> = String::try_from("abc").unwrap();
        assert!(s.push('1').is_ok());
        assert!(s.push('2').is_ok());
        assert!(s.push('3').is_ok());
        assert!(s.push('4').is_err());
        assert!("abc123" == s.as_str());
    }

    #[test]
    fn as_bytes() {
        let s: String<8> = String::try_from("hello").unwrap();
        assert_eq!(&[104, 101, 108, 108, 111], s.as_bytes());
    }

    #[test]
    fn truncate() {
        let mut s: String<8> = String::try_from("hello").unwrap();
        s.truncate(6);
        assert_eq!(s.len(), 5);
        s.truncate(2);
        assert_eq!(s.len(), 2);
        assert_eq!("he", s);
        assert_eq!(s, "he");
    }

    #[test]
    fn pop() {
        let mut s: String<8> = String::try_from("foo").unwrap();
        assert_eq!(s.pop(), Some('o'));
        assert_eq!(s.pop(), Some('o'));
        assert_eq!(s.pop(), Some('f'));
        assert_eq!(s.pop(), None);
    }

    #[test]
    fn pop_uenc() {
        let mut s: String<8> = String::try_from("é").unwrap();
        assert_eq!(s.len(), 3);
        match s.pop() {
            Some(c) => {
                assert_eq!(s.len(), 1);
                assert_eq!(c, '\u{0301}'); // accute accent of e
                ()
            }
            None => assert!(false),
        };
    }

    #[test]
    fn is_empty() {
        let mut v: String<8> = String::new();
        assert!(v.is_empty());
        let _ = v.push('a');
        assert!(!v.is_empty());
    }

    #[test]
    fn clear() {
        let mut s: String<8> = String::try_from("foo").unwrap();
        s.clear();
        assert!(s.is_empty());
        assert_eq!(0, s.len());
        assert_eq!(8, s.capacity());
    }

    #[test]
    fn remove() {
        let mut s: String<8> = String::try_from("foo").unwrap();
        assert_eq!(s.remove(0), 'f');
        assert_eq!(s.as_str(), "oo");
    }

    #[test]
    fn remove_uenc() {
        let mut s: String<8> = String::try_from("ĝėēƶ").unwrap();
        assert_eq!(s.remove(2), 'ė');
        assert_eq!(s.remove(2), 'ē');
        assert_eq!(s.remove(2), 'ƶ');
        assert_eq!(s.as_str(), "ĝ");
    }

    #[test]
    fn remove_uenc_combo_characters() {
        let mut s: String<8> = String::try_from("héy").unwrap();
        assert_eq!(s.remove(2), '\u{0301}');
        assert_eq!(s.as_str(), "hey");
    }
}
