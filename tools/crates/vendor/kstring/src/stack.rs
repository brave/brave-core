use std::fmt;

pub(crate) type Len = u8;

/// Fixed-size stack-allocated string
#[derive(Copy, Clone)]
pub struct StackString<const CAPACITY: usize> {
    len: Len,
    buffer: StrBuffer<CAPACITY>,
}

impl<const CAPACITY: usize> StackString<CAPACITY> {
    pub const CAPACITY: usize = CAPACITY;
    pub const EMPTY: Self = Self::empty();

    const fn empty() -> Self {
        Self {
            len: 0,
            buffer: StrBuffer::empty(),
        }
    }

    /// Create a `StackString` from a `&str`, if it'll fit within `Self::CAPACITY`
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let s = kstring::StackString::<3>::try_new("foo");
    /// assert_eq!(s.as_deref(), Some("foo"));
    /// let s = kstring::StackString::<3>::try_new("foobar");
    /// assert_eq!(s, None);
    /// ```
    #[inline]
    #[must_use]
    pub fn try_new(s: &str) -> Option<Self> {
        let len = s.as_bytes().len();
        if len <= Self::CAPACITY {
            #[cfg(feature = "unsafe")]
            let stack = {
                unsafe {
                    // SAFETY: We've confirmed `len` is within size
                    Self::new_unchecked(s)
                }
            };
            #[cfg(not(feature = "unsafe"))]
            let stack = { Self::new(s) };
            Some(stack)
        } else {
            None
        }
    }

    /// Create a `StackString` from a `&str`
    ///
    /// # Panic
    ///
    /// Calling this function with a string larger than `Self::CAPACITY` will panic
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let s = kstring::StackString::<3>::new("foo");
    /// assert_eq!(s, "foo");
    /// ```
    #[inline]
    #[must_use]
    pub fn new(s: &str) -> Self {
        let len = s.as_bytes().len() as u8;
        debug_assert!(Self::CAPACITY <= Len::MAX.into());
        let buffer = StrBuffer::new(s);
        Self { len, buffer }
    }

    /// Create a `StackString` from a `&str`
    ///
    /// # Safety
    ///
    /// Calling this function with a string larger than `Self::CAPACITY` is undefined behavior.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let s = unsafe {
    ///     // SAFETY: Literal is short-enough
    ///     kstring::StackString::<3>::new_unchecked("foo")
    /// };
    /// assert_eq!(s, "foo");
    /// ```
    #[inline]
    #[must_use]
    #[cfg(feature = "unsafe")]
    pub unsafe fn new_unchecked(s: &str) -> Self {
        let len = s.as_bytes().len() as u8;
        debug_assert!(Self::CAPACITY <= Len::MAX.into());
        let buffer = StrBuffer::new_unchecked(s);
        Self { len, buffer }
    }

    /// Extracts a string slice containing the entire `StackString`.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let s = kstring::StackString::<3>::try_new("foo").unwrap();
    ///
    /// assert_eq!("foo", s.as_str());
    /// ```
    #[inline]
    #[must_use]
    pub fn as_str(&self) -> &str {
        let len = self.len as usize;
        #[cfg(feature = "unsafe")]
        unsafe {
            // SAFETY: Constructors guarantee that `buffer[..len]` is a `str`,
            // and we don't mutate the data afterwards.
            self.buffer.as_str_unchecked(len)
        }
        #[cfg(not(feature = "unsafe"))]
        self.buffer.as_str(len)
    }

    /// Converts a `StackString` into a mutable string slice.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let mut s = kstring::StackString::<6>::try_new("foobar").unwrap();
    /// let s_mut_str = s.as_mut_str();
    ///
    /// s_mut_str.make_ascii_uppercase();
    ///
    /// assert_eq!("FOOBAR", s_mut_str);
    /// ```
    #[inline]
    #[must_use]
    pub fn as_mut_str(&mut self) -> &mut str {
        let len = self.len as usize;
        #[cfg(feature = "unsafe")]
        unsafe {
            // SAFETY: Constructors guarantee that `buffer[..len]` is a `str`,
            // and we don't mutate the data afterwards.
            self.buffer.as_mut_str_unchecked(len)
        }
        #[cfg(not(feature = "unsafe"))]
        self.buffer.as_mut_str(len)
    }

    /// Returns the length of this `StasckString`, in bytes, not [`char`]s or
    /// graphemes. In other words, it might not be what a human considers the
    /// length of the string.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let a = kstring::StackString::<3>::try_new("foo").unwrap();
    /// assert_eq!(a.len(), 3);
    ///
    /// let fancy_f = kstring::StackString::<4>::try_new("ƒoo").unwrap();
    /// assert_eq!(fancy_f.len(), 4);
    /// assert_eq!(fancy_f.chars().count(), 3);
    /// ```
    #[inline]
    #[must_use]
    pub fn len(&self) -> usize {
        self.len as usize
    }

    /// Returns `true` if this `StackString` has a length of zero, and `false` otherwise.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let mut v = kstring::StackString::<20>::EMPTY;
    /// assert!(v.is_empty());
    ///
    /// let a = kstring::StackString::<3>::try_new("foo").unwrap();
    /// assert!(!a.is_empty());
    /// ```
    #[inline]
    #[must_use]
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Truncates this `StackString`, removing all contents.
    ///
    /// # Examples
    ///
    /// Basic usage:
    ///
    /// ```
    /// let mut s = kstring::StackString::<3>::try_new("foo").unwrap();
    ///
    /// s.clear();
    ///
    /// assert!(s.is_empty());
    /// assert_eq!(0, s.len());
    /// ```
    #[inline]
    pub fn clear(&mut self) {
        self.len = 0;
    }

    /// Shortens this `StackString` to the specified length.
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
    /// let mut s = kstring::StackString::<5>::try_new("hello").unwrap();
    ///
    /// s.truncate(2);
    ///
    /// assert_eq!(s, "he");
    /// ```
    #[inline]
    pub fn truncate(&mut self, new_len: usize) {
        if new_len <= self.len() {
            assert!(self.is_char_boundary(new_len));
            self.len = new_len as u8;
        }
    }
}

impl<const CAPACITY: usize> Default for StackString<CAPACITY> {
    fn default() -> Self {
        Self::empty()
    }
}

impl<const CAPACITY: usize> std::ops::Deref for StackString<CAPACITY> {
    type Target = str;

    #[inline]
    fn deref(&self) -> &str {
        self.as_str()
    }
}

impl<const CAPACITY: usize> Eq for StackString<CAPACITY> {}

impl<const C1: usize, const C2: usize> PartialEq<StackString<C1>> for StackString<C2> {
    #[inline]
    fn eq(&self, other: &StackString<C1>) -> bool {
        PartialEq::eq(self.as_str(), other.as_str())
    }
}

impl<const CAPACITY: usize> PartialEq<str> for StackString<CAPACITY> {
    #[inline]
    fn eq(&self, other: &str) -> bool {
        PartialEq::eq(self.as_str(), other)
    }
}

impl<'s, const CAPACITY: usize> PartialEq<&'s str> for StackString<CAPACITY> {
    #[inline]
    fn eq(&self, other: &&str) -> bool {
        PartialEq::eq(self.as_str(), *other)
    }
}

impl<const CAPACITY: usize> PartialEq<String> for StackString<CAPACITY> {
    #[inline]
    fn eq(&self, other: &String) -> bool {
        PartialEq::eq(self.as_str(), other.as_str())
    }
}

impl<const CAPACITY: usize> Ord for StackString<CAPACITY> {
    #[inline]
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.as_str().cmp(other.as_str())
    }
}

impl<const C1: usize, const C2: usize> PartialOrd<StackString<C1>> for StackString<C2> {
    #[inline]
    fn partial_cmp(&self, other: &StackString<C1>) -> Option<std::cmp::Ordering> {
        self.as_str().partial_cmp(other.as_str())
    }
}

impl<const CAPACITY: usize> PartialOrd<str> for StackString<CAPACITY> {
    #[inline]
    fn partial_cmp(&self, other: &str) -> Option<std::cmp::Ordering> {
        self.as_str().partial_cmp(other)
    }
}

impl<'s, const CAPACITY: usize> PartialOrd<&'s str> for StackString<CAPACITY> {
    #[inline]
    fn partial_cmp(&self, other: &&str) -> Option<std::cmp::Ordering> {
        self.as_str().partial_cmp(other)
    }
}

impl<const CAPACITY: usize> PartialOrd<String> for StackString<CAPACITY> {
    #[inline]
    fn partial_cmp(&self, other: &String) -> Option<std::cmp::Ordering> {
        self.as_str().partial_cmp(other.as_str())
    }
}

impl<const CAPACITY: usize> std::hash::Hash for StackString<CAPACITY> {
    #[inline]
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.as_str().hash(state);
    }
}

impl<const CAPACITY: usize> fmt::Debug for StackString<CAPACITY> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(self.as_str(), f)
    }
}

impl<const CAPACITY: usize> fmt::Display for StackString<CAPACITY> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(self.as_str(), f)
    }
}

impl<const CAPACITY: usize> AsRef<str> for StackString<CAPACITY> {
    #[inline]
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

impl<const CAPACITY: usize> AsRef<[u8]> for StackString<CAPACITY> {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl<const CAPACITY: usize> AsRef<std::ffi::OsStr> for StackString<CAPACITY> {
    #[inline]
    fn as_ref(&self) -> &std::ffi::OsStr {
        (**self).as_ref()
    }
}

impl<const CAPACITY: usize> AsRef<std::path::Path> for StackString<CAPACITY> {
    #[inline]
    fn as_ref(&self) -> &std::path::Path {
        std::path::Path::new(self)
    }
}

impl<const CAPACITY: usize> std::borrow::Borrow<str> for StackString<CAPACITY> {
    #[inline]
    fn borrow(&self) -> &str {
        self.as_str()
    }
}

#[derive(Copy, Clone)]
#[repr(transparent)]
pub(crate) struct StrBuffer<const CAPACITY: usize>([u8; CAPACITY]);

impl<const CAPACITY: usize> StrBuffer<CAPACITY> {
    pub(crate) const fn empty() -> Self {
        let array = [0; CAPACITY];
        StrBuffer(array)
    }

    #[inline]
    pub(crate) fn new(s: &str) -> Self {
        let len = s.as_bytes().len();
        debug_assert!(len <= CAPACITY);
        let mut buffer = Self::default();
        if let Some(buffer) = buffer.0.get_mut(..len) {
            buffer.copy_from_slice(s.as_bytes());
        } else {
            panic!("`{}` is larger than capacity {}", s, CAPACITY);
        }
        buffer
    }

    #[inline]
    #[cfg(not(feature = "unsafe"))]
    pub(crate) fn as_str(&self, len: usize) -> &str {
        let slice = self.0.get(..len).unwrap();
        std::str::from_utf8(slice).unwrap()
    }

    #[inline]
    #[cfg(not(feature = "unsafe"))]
    pub(crate) fn as_mut_str(&mut self, len: usize) -> &mut str {
        let slice = self.0.get_mut(..len).unwrap();
        std::str::from_utf8_mut(slice).unwrap()
    }
}

impl<const CAPACITY: usize> StrBuffer<CAPACITY> {
    #[inline]
    #[cfg(feature = "unsafe")]
    pub(crate) unsafe fn new_unchecked(s: &str) -> Self {
        let len = s.as_bytes().len();
        debug_assert!(len <= CAPACITY);
        let mut buffer = Self::default();
        buffer
            .0
            .get_unchecked_mut(..len)
            .copy_from_slice(s.as_bytes());
        buffer
    }

    #[inline]
    #[cfg(feature = "unsafe")]
    pub(crate) unsafe fn as_str_unchecked(&self, len: usize) -> &str {
        let slice = self.0.get_unchecked(..len);
        std::str::from_utf8_unchecked(slice)
    }

    #[inline]
    #[cfg(feature = "unsafe")]
    pub(crate) unsafe fn as_mut_str_unchecked(&mut self, len: usize) -> &mut str {
        let slice = self.0.get_unchecked_mut(..len);
        std::str::from_utf8_unchecked_mut(slice)
    }
}

impl<const CAPACITY: usize> Default for StrBuffer<CAPACITY> {
    fn default() -> Self {
        Self::empty()
    }
}
