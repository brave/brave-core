//! A buffer for constructing a string while avoiding heap allocation.

use core::hash::{Hash, Hasher};
use core::mem::MaybeUninit;
use core::{fmt, slice, str};

use crate::smart_display::{FormatterOptions, Metadata, SmartDisplay};

/// A buffer for construct a string while avoiding heap allocation.
///
/// The only requirement is that the buffer is large enough to hold the formatted string.
pub struct WriteBuffer<const SIZE: usize> {
    buf: [MaybeUninit<u8>; SIZE],
    len: usize,
}

impl<const SIZE: usize> fmt::Debug for WriteBuffer<SIZE> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DisplayBuffer")
            .field("buf", &self.as_str())
            .field("remaining_capacity", &self.remaining_capacity())
            .finish()
    }
}

impl<const SIZE: usize> WriteBuffer<SIZE> {
    /// Creates an empty buffer.
    #[inline]
    pub const fn new() -> Self {
        Self {
            buf: [MaybeUninit::uninit(); SIZE],
            len: 0,
        }
    }

    /// Obtain the contents of the buffer as a string.
    #[inline]
    pub fn as_str(&self) -> &str {
        self
    }

    /// Determine how many bytes are remaining in the buffer.
    #[inline]
    pub const fn remaining_capacity(&self) -> usize {
        SIZE - self.len
    }
}

impl<const SIZE: usize> Default for WriteBuffer<SIZE> {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl<const LEFT_SIZE: usize, const RIGHT_SIZE: usize> PartialOrd<WriteBuffer<RIGHT_SIZE>>
    for WriteBuffer<LEFT_SIZE>
{
    #[inline]
    fn partial_cmp(&self, other: &WriteBuffer<RIGHT_SIZE>) -> Option<core::cmp::Ordering> {
        self.as_str().partial_cmp(other.as_str())
    }
}

impl<const LEFT_SIZE: usize, const RIGHT_SIZE: usize> PartialEq<WriteBuffer<RIGHT_SIZE>>
    for WriteBuffer<LEFT_SIZE>
{
    #[inline]
    fn eq(&self, other: &WriteBuffer<RIGHT_SIZE>) -> bool {
        self.as_str() == other.as_str()
    }
}

impl<const SIZE: usize> Eq for WriteBuffer<SIZE> {}

impl<const SIZE: usize> Ord for WriteBuffer<SIZE> {
    #[inline]
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.as_str().cmp(other.as_str())
    }
}

impl<const SIZE: usize> Hash for WriteBuffer<SIZE> {
    #[inline]
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.as_str().hash(state)
    }
}

impl<const SIZE: usize> AsRef<str> for WriteBuffer<SIZE> {
    #[inline]
    fn as_ref(&self) -> &str {
        self
    }
}

impl<const SIZE: usize> AsRef<[u8]> for WriteBuffer<SIZE> {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl<const SIZE: usize> core::borrow::Borrow<str> for WriteBuffer<SIZE> {
    #[inline]
    fn borrow(&self) -> &str {
        self
    }
}

impl<const SIZE: usize> core::ops::Deref for WriteBuffer<SIZE> {
    type Target = str;

    #[inline]
    fn deref(&self) -> &Self::Target {
        // Safety: `MaybeUninit<T>` and `T` have the same layout. `buf` is only written to by the
        // `fmt::Write::write_str` implementation which writes a valid UTF-8 string to `buf`
        // and correctly sets `len`. All other safety requirements are upheld
        // by the original slice.
        unsafe {
            let slice = self.buf.get_unchecked(..self.len);
            let slice = slice::from_raw_parts(slice.as_ptr().cast(), slice.len());
            str::from_utf8_unchecked(slice)
        }
    }
}

impl<const SIZE: usize> fmt::Display for WriteBuffer<SIZE> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self)
    }
}

impl<const SIZE: usize> SmartDisplay for WriteBuffer<SIZE> {
    type Metadata = ();

    #[inline]
    fn metadata(&self, _: FormatterOptions) -> Metadata<'_, Self> {
        Metadata::new(self.len, self, ())
    }

    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.pad(self)
    }
}

impl<const SIZE: usize> fmt::Write for WriteBuffer<SIZE> {
    #[inline]
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let bytes = s.as_bytes();

        if let Some(buf) = self.buf.get_mut(self.len..(self.len + bytes.len())) {
            maybe_uninit_write_slice(buf, bytes);
            self.len += bytes.len();
            Ok(())
        } else {
            Err(fmt::Error)
        }
    }
}

/// Equivalent of [`MaybeUninit::write_slice`] that compiles on stable.
fn maybe_uninit_write_slice<'a, T>(this: &'a mut [MaybeUninit<T>], src: &[T]) -> &'a mut [T]
where
    T: Copy,
{
    #[allow(trivial_casts)]
    // Safety: `T` and `MaybeUninit<T>` have the same layout
    let uninit_src = unsafe { &*(src as *const [T] as *const [MaybeUninit<T>]) };

    this.copy_from_slice(uninit_src);

    // Safety: `MaybeUninit<T>` and `T` have the same layout. Valid elements have just been copied
    // into `this` so it is initialized. All other safety requirements are upheld by the
    // original slice.
    unsafe { slice::from_raw_parts_mut(this.as_mut_ptr().cast(), this.len()) }
}
