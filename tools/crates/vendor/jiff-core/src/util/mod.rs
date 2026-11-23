/*!
Small shared utilities used by Jiff.
*/

#[cfg(feature = "alloc")]
pub(crate) mod crc32;

/// A slice that is either `'static` or on the heap.
///
/// This is useful for representing a sequence of data that can be either
/// created at runtime and put on the heap (when dynamic memory allocation is
/// needed), or when it needs to be constructed at compile time. For example,
/// this is used to represent time zone transitions inside this crate's TZif
/// representation.
///
/// The downside of this type is that `T` cannot contain any borrows.
///
/// This is similar to `SmallStr`, but there is no array-only variant. As such,
/// this isn't intended for small data. (Indeed, time zone transitions can get
/// pretty long.) This also makes the API simpler: all constructors are
/// infallible.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MaybeStaticSlice<T: 'static> {
    kind: MaybeStaticSliceKind<T>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
enum MaybeStaticSliceKind<T: 'static> {
    Static(&'static [T]),
    #[cfg(feature = "alloc")]
    Heap(alloc::boxed::Box<[T]>),
}

impl<T: 'static> MaybeStaticSlice<T> {
    /// Creates a new static slice from the data provided.
    #[inline]
    pub const fn statik(data: &'static [T]) -> MaybeStaticSlice<T> {
        let kind = MaybeStaticSliceKind::Static(data);
        MaybeStaticSlice { kind }
    }

    /// Creates a new slice on the heap from the data provided.
    #[cfg(feature = "alloc")]
    #[inline]
    pub const fn heap(data: alloc::boxed::Box<[T]>) -> MaybeStaticSlice<T> {
        let kind = MaybeStaticSliceKind::Heap(data);
        MaybeStaticSlice { kind }
    }

    /// Returns the underlying data as a slice.
    #[inline]
    pub const fn as_slice(&self) -> &[T] {
        match self.kind {
            MaybeStaticSliceKind::Static(slice) => slice,
            #[cfg(feature = "alloc")]
            MaybeStaticSliceKind::Heap(ref slice) => slice,
        }
    }
}

impl<T: 'static> From<&'static [T]> for MaybeStaticSlice<T> {
    #[inline]
    fn from(data: &'static [T]) -> MaybeStaticSlice<T> {
        MaybeStaticSlice::statik(data)
    }
}

#[cfg(feature = "alloc")]
impl<T: 'static> From<alloc::boxed::Box<[T]>> for MaybeStaticSlice<T> {
    #[inline]
    fn from(data: alloc::boxed::Box<[T]>) -> MaybeStaticSlice<T> {
        MaybeStaticSlice::heap(data)
    }
}

impl<T: 'static> core::ops::Deref for MaybeStaticSlice<T> {
    type Target = [T];

    #[inline]
    fn deref(&self) -> &[T] {
        self.as_slice()
    }
}

/// A "small" string that usually lives in an array.
///
/// When the string is too big, it spills over into the heap. Generally
/// speaking, this should only be used when spilling into the heap is
/// exceptionally rare. For example, for representing pathological user data
/// (like very long time zone abbreviations).
///
/// The `ARRAY_CAPACITY_MAX` parameter defines the maximum length of a string
/// that will fit in an array backed storage. This number cannot be any
/// bigger than `255`.
///
/// In core-only environments, this always uses array backed storage or static
/// data. Static data is only used when `SmallStr::from("some static string")`
/// is used to construct a `SmallStr`. This is useful when constructing static
/// data structures.
#[derive(Clone, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct SmallStr<const ARRAY_CAPACITY_MAX: usize> {
    kind: SmallStrKind<ARRAY_CAPACITY_MAX>,
}

#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
enum SmallStrKind<const ARRAY_CAPACITY_MAX: usize> {
    Array(ArrayStr<ARRAY_CAPACITY_MAX>),
    // TODO: Reconsider this variant. I'm not sure it's really necessary, and
    // it makes the size of all `SmallStr` values bigger.
    Static(&'static str),
    #[cfg(feature = "alloc")]
    Heap(alloc::boxed::Box<str>),
}

impl<const ARRAY_CAPACITY_MAX: usize> SmallStr<ARRAY_CAPACITY_MAX> {
    /// Creates a new array-or-heap backed string depending on the length.
    ///
    /// In environments with dynamic memory allocation, this never returns
    /// `None`. In core-only environments, this may return `None` when the
    /// string length exceeds the maximum capacity.
    #[inline]
    pub fn new(s: &str) -> Option<SmallStr<ARRAY_CAPACITY_MAX>> {
        SmallStr::try_array(s).or_else(|| {
            #[cfg(not(feature = "alloc"))]
            {
                None
            }
            #[cfg(feature = "alloc")]
            {
                Some(SmallStr::from(alloc::boxed::Box::<str>::from(s)))
            }
        })
    }

    /// Like `new` but always spills to the heap when the given string does not
    /// fit in this string's fixed capacity.
    ///
    /// This is only available when the `alloc` crate feature is enabled.
    #[cfg(feature = "alloc")]
    #[inline]
    pub fn new_or_heap(s: &str) -> SmallStr<ARRAY_CAPACITY_MAX> {
        SmallStr::try_array(s).unwrap_or_else(|| {
            SmallStr::from(alloc::boxed::Box::<str>::from(s))
        })
    }

    /// Like `SmallStr::new`, but this never returns a string on the heap.
    ///
    /// This is useful when you want a `SmallStr` that is guaranteed to never
    /// be on the heap. For example, when constructing static data.
    ///
    /// If the string exceeds the maximum capacity, then `None` is returned.
    #[inline]
    pub const fn try_array(s: &str) -> Option<SmallStr<ARRAY_CAPACITY_MAX>> {
        let Some(astr) = ArrayStr::new(s) else { return None };
        let kind = SmallStrKind::Array(astr);
        Some(SmallStr { kind })
    }

    /// Like `SmallStr::new`, but this never returns a string on the heap.
    ///
    /// This is useful when you want a `SmallStr` that is guaranteed to never
    /// be on the heap. For example, when constructing static data.
    ///
    /// # Panics
    ///
    /// If the string exceeds the maximum capacity, then this routine panics.
    #[inline]
    pub const fn array(s: &str) -> SmallStr<ARRAY_CAPACITY_MAX> {
        // MSRV(1.83): We can use `unwrap()` in a const context, so this
        // routine isn't as necessary. But it's still nice.
        let Some(astr) = ArrayStr::new(s) else { panic!("string too big") };
        let kind = SmallStrKind::Array(astr);
        SmallStr { kind }
    }

    /// Like `SmallStr::new`, but only accepts a static string.
    ///
    /// This never fails.
    #[inline]
    pub const fn statik(s: &'static str) -> SmallStr<ARRAY_CAPACITY_MAX> {
        let kind = SmallStrKind::Static(s);
        SmallStr { kind }
    }

    /// Returns the maximum capacity for this small string's array storage.
    #[inline]
    pub const fn array_capacity_max() -> usize {
        ARRAY_CAPACITY_MAX
    }

    /// Returns this small string as a string slice.
    #[inline]
    pub const fn as_str(&self) -> &str {
        match self.kind {
            SmallStrKind::Array(ref astr) => astr.as_str(),
            SmallStrKind::Static(s) => s,
            #[cfg(feature = "alloc")]
            SmallStrKind::Heap(ref s) => s,
        }
    }
}

/// Return a `SmallStr` that is guaranteed to live as an array.
impl<const ARRAY_CAPACITY_MAX: usize> From<ArrayStr<ARRAY_CAPACITY_MAX>>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn from(s: ArrayStr<ARRAY_CAPACITY_MAX>) -> SmallStr<ARRAY_CAPACITY_MAX> {
        let kind = SmallStrKind::Array(s);
        SmallStr { kind }
    }
}

/// Return a `SmallStr` that is guaranteed to live as a static string.
impl<const ARRAY_CAPACITY_MAX: usize> From<&'static str>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn from(s: &'static str) -> SmallStr<ARRAY_CAPACITY_MAX> {
        SmallStr::statik(s)
    }
}

/// Return a `SmallStr` that is guaranteed to live on the heap.
#[cfg(feature = "alloc")]
impl<const ARRAY_CAPACITY_MAX: usize> From<alloc::boxed::Box<str>>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn from(s: alloc::boxed::Box<str>) -> SmallStr<ARRAY_CAPACITY_MAX> {
        let kind = SmallStrKind::Heap(s);
        SmallStr { kind }
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> core::ops::Deref
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    type Target = str;

    #[inline]
    fn deref(&self) -> &str {
        SmallStr::<ARRAY_CAPACITY_MAX>::as_str(self)
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> AsRef<str>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> PartialEq<str>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn eq(&self, rhs: &str) -> bool {
        self.as_str() == rhs
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> PartialEq<&str>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn eq(&self, rhs: &&str) -> bool {
        self.as_str() == *rhs
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> PartialEq<SmallStr<ARRAY_CAPACITY_MAX>>
    for str
{
    #[inline]
    fn eq(&self, rhs: &SmallStr<ARRAY_CAPACITY_MAX>) -> bool {
        self == rhs.as_str()
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> PartialOrd<str>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn partial_cmp(&self, rhs: &str) -> Option<core::cmp::Ordering> {
        self.as_str().partial_cmp(rhs)
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> PartialOrd<&str>
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn partial_cmp(&self, rhs: &&str) -> Option<core::cmp::Ordering> {
        self.as_str().partial_cmp(*rhs)
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> PartialOrd<SmallStr<ARRAY_CAPACITY_MAX>>
    for str
{
    #[inline]
    fn partial_cmp(
        &self,
        rhs: &SmallStr<ARRAY_CAPACITY_MAX>,
    ) -> Option<core::cmp::Ordering> {
        self.partial_cmp(rhs.as_str())
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> core::fmt::Debug
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Debug::fmt(self.as_str(), f)
    }
}

impl<const ARRAY_CAPACITY_MAX: usize> core::fmt::Display
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(self.as_str(), f)
    }
}

#[cfg(feature = "defmt")]
impl<const ARRAY_CAPACITY_MAX: usize> defmt::Format
    for SmallStr<ARRAY_CAPACITY_MAX>
{
    #[inline]
    fn format(&self, f: defmt::Formatter) {
        defmt::write!(f, "{=str}", self.as_str())
    }
}

/// A simple array-backed string type with a fixed capacity.
///
/// This is used by Jiff in lieu of a `Box<str>` for supporting core-only
/// environments without a dynamic memory allocator. For example, this is used
/// to represent time zone abbreviations which can be relied upon to be short.
///
/// `N` must be less than `256` so that its length can be represented by an
/// unsigned 8-bit integer.
///
/// An `ArrayStr` is guaranteed to be valid UTF-8.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct ArrayStr<const N: usize> {
    // If it's advantageous enough, we could use an array of uninitialized
    // bytes. But it's not clear that it's worth doing. ---AG
    /// The number of bytes used by the string in `bytes`.
    ///
    /// (We could technically save this byte in some cases and use a NUL
    /// terminator. For example, since we don't permit NUL bytes in POSIX time
    /// zone abbreviation strings, but this is simpler and only one byte and
    /// generalizes. And we're not really trying to micro-optimize the storage
    /// requirements when we use these array strings. Or at least, I don't know
    /// of a reason to.)
    len: u8,
    /// The UTF-8 bytes that make up the string.
    ///
    /// This array---the entire array---is always valid UTF-8. And
    /// the `0..self.len` sub-slice is also always valid UTF-8.
    bytes: [u8; N],
}

impl<const N: usize> ArrayStr<N> {
    /// Creates a new fixed capacity string.
    ///
    /// If the given string exceeds `N` bytes, then this returns
    /// `None`. Similarly, if `N` exceeds `u8::MAX`, then this
    /// returns `None`.
    #[inline]
    pub const fn new(s: &str) -> Option<ArrayStr<N>> {
        let len = s.len();
        if len > N || N > u8::MAX as usize {
            return None;
        }
        let mut bytes = [0; N];
        let mut i = 0;
        while i < s.as_bytes().len() {
            bytes[i] = s.as_bytes()[i];
            i += 1;
        }
        // OK because an `ArrayStr<N>` with `N > 255` bails out above.
        Some(ArrayStr { len: len as u8, bytes })
    }

    /// Returns the capacity of this array string.
    #[inline]
    pub const fn capacity() -> usize {
        N
    }

    /// Append the bytes given to the end of this string.
    ///
    /// If the capacity would be exceeded, then this is a no-op and `false`
    /// is returned. Otherwise, all of `s` is written to this array string and
    /// `true` is returned.
    #[inline]
    pub fn push_str(&mut self, s: &str) -> bool {
        let len = self.len as usize;
        let Some(new_len) = len.checked_add(s.len()) else { return false };
        if new_len > N {
            return false;
        }

        let mut i = len;
        while i < new_len {
            self.bytes[i] = s.as_bytes()[i - len];
            i += 1;
        }
        // OK because an `ArrayStr<N>` with `N > 255` cannot be created.
        self.len = new_len as u8;
        true
    }

    /// Returns this array string as a string slice.
    #[inline]
    pub const fn as_str(&self) -> &str {
        // SAFETY: Firstly, the unchecked UTF-8 conversion is correct because
        // the constructor and all mutators only accept `&str`. All mutators
        // (just `push_str` at time of writing) only ever concatenates the
        // given string to what is already there. Any UTF-8 string concatenated
        // with any other UTF-8 string produces a UTF-8 string.
        //
        // Secondly, `self.bytes` is always valid and initialized. And
        // `self.len` is managed as the length of bytes that make up the
        // string.
        unsafe {
            core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                self.bytes.as_ptr(),
                self.len as usize,
            ))
        }
    }
}

impl<const N: usize> core::ops::Deref for ArrayStr<N> {
    type Target = str;

    #[inline]
    fn deref(&self) -> &str {
        ArrayStr::<N>::as_str(self)
    }
}

impl<const N: usize> AsRef<str> for ArrayStr<N> {
    #[inline]
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

impl<const N: usize> PartialEq<str> for ArrayStr<N> {
    #[inline]
    fn eq(&self, rhs: &str) -> bool {
        self.as_str() == rhs
    }
}

impl<const N: usize> PartialEq<&str> for ArrayStr<N> {
    #[inline]
    fn eq(&self, rhs: &&str) -> bool {
        self.as_str() == *rhs
    }
}

impl<const N: usize> PartialEq<ArrayStr<N>> for str {
    #[inline]
    fn eq(&self, rhs: &ArrayStr<N>) -> bool {
        self == rhs.as_str()
    }
}

impl<const N: usize> PartialOrd<str> for ArrayStr<N> {
    #[inline]
    fn partial_cmp(&self, rhs: &str) -> Option<core::cmp::Ordering> {
        self.as_str().partial_cmp(rhs)
    }
}

impl<const N: usize> PartialOrd<&str> for ArrayStr<N> {
    #[inline]
    fn partial_cmp(&self, rhs: &&str) -> Option<core::cmp::Ordering> {
        self.as_str().partial_cmp(*rhs)
    }
}

impl<const N: usize> PartialOrd<ArrayStr<N>> for str {
    #[inline]
    fn partial_cmp(&self, rhs: &ArrayStr<N>) -> Option<core::cmp::Ordering> {
        self.partial_cmp(rhs.as_str())
    }
}

impl<const N: usize> core::fmt::Debug for ArrayStr<N> {
    #[inline]
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Debug::fmt(self.as_str(), f)
    }
}

impl<const N: usize> core::fmt::Display for ArrayStr<N> {
    #[inline]
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(self.as_str(), f)
    }
}

impl<const N: usize> core::fmt::Write for ArrayStr<N> {
    #[inline]
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        if self.push_str(s) { Ok(()) } else { Err(core::fmt::Error) }
    }
}

#[cfg(feature = "defmt")]
impl<const N: usize> defmt::Format for ArrayStr<N> {
    #[inline]
    fn format(&self, f: defmt::Formatter) {
        defmt::write!(f, "{=str}", self.as_str())
    }
}

/// Parses an `OsStr` into a `&str` when `&[u8]` isn't easily available.
///
/// The main difference between this and `OsStr::to_str` is that this will
/// be a zero-cost conversion on Unix platforms to `&[u8]`. On Windows, this
/// will do UTF-8 validation and return an error if it's invalid UTF-8.
// MSRV(1.74): Use `OsStr::as_encoded_bytes` and delete this routine.
#[cfg(feature = "std")]
pub(crate) fn os_str_bytes<'o, O>(os_str: &'o O) -> Option<&'o [u8]>
where
    O: ?Sized + AsRef<std::ffi::OsStr>,
{
    let os_str = os_str.as_ref();
    #[cfg(unix)]
    {
        use std::os::unix::ffi::OsStrExt;
        Some(os_str.as_bytes())
    }
    #[cfg(not(unix))]
    {
        // It is suspect that we're doing UTF-8 validation and then throwing
        // away the fact that we did UTF-8 validation. So this could lead
        // to an extra UTF-8 check if the caller ultimately needs UTF-8. If
        // that's important, we can add a new API that returns a `&str`. But it
        // probably won't matter because an `OsStr` in this crate is usually
        // just an environment variable.
        os_str.to_str().map(|s| s.as_bytes())
    }
}

#[cfg(test)]
mod tests {
    use core::fmt::Write;

    use super::*;

    #[test]
    fn fmt_write() {
        let mut dst = ArrayStr::<5>::new("").unwrap();
        assert!(write!(&mut dst, "abcd").is_ok());
        assert!(write!(&mut dst, "e").is_ok());
        assert!(write!(&mut dst, "f").is_err());
    }

    #[test]
    fn array_str_size() {
        assert_eq!(7, core::mem::size_of::<ArrayStr::<6>>());
    }
}
