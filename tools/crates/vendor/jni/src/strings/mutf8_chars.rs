use jni_sys::{JNI_TRUE, jboolean};
use std::{borrow::Cow, marker::PhantomData, os::raw::c_char};

use log::warn;

use crate::{
    Env, JavaVM,
    errors::*,
    objects::{JString, Reference},
    strings::{JNIStr, JNIString},
};

#[cfg(doc)]
use ::{
    std::ffi::{CStr, CString},
    std::string::ToString as _,
};

/// Borrows the contents of a `java.lang.String` object, in Java's [modified
/// UTF-8] encoding.
///
/// This guard type is returned by [JString::mutf8_chars] and represents the
/// borrowed contents of a `java.lang.String` object that will be automatically
/// released when dropped.
///
/// This can be dereferenced to obtain a [`JNIStr`] which can in turn be
/// converted to a utf8 Rust string. (See [`JNIStr::to_str`] or `to_string`).
///
/// For example:
///
/// ```rust,no_run
/// # use jni::{errors::Result, Env, objects::*};
/// #
/// # fn f(env: &mut Env) -> Result<()> {
/// let string = JString::from_str(env, "Hello, world!")?;
/// let rust_utf8_string = string.mutf8_chars(env)?.to_string();
/// # Ok(())
/// # }
/// ```
///
/// # JNI String Types
///
/// From the point of view of JNI a [JString] is merely a reference to a
/// `java.lang.String` object and to access the underlying data, you need to use
/// JNI ([JString::mutf8_chars]) to explicitly borrow the underlying bytes
/// of the string.
///
/// [JNIStr] is to [JNIString] as `str` is to `String` or `CStr` is to
/// `CString`.
///
/// [JNIStr] and [JNIString] represent nul-terminated strings, like [`CStr`] and
/// [`CString`], that are encoded in [modified UTF-8].
///
/// This type is a guard that holds a temporary [JNIStr] reference to the
/// underlying bytes of a `java.lang.String` object.
///
/// [modified UTF-8]: https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8
pub struct MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    obj: StringRef,
    chars: *const c_char,
    is_copy: bool,
    _lifetime: PhantomData<&'local ()>,
}

impl<'local, StringRef> std::fmt::Debug for MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("MUTF8Chars")
            .field("obj", self.obj.as_ref())
            .field("chars", &self.chars)
            .field("is_copy", &self.is_copy)
            .finish()
    }
}

/// Borrows the contents of a `java.lang.String` object, in Java's [modified
/// UTF-8] encoding.
#[deprecated(note = "Renamed to MUTF8Chars, use JString::mutf8_chars() to get it")]
pub type JavaStr<'local, StringRef> = MUTF8Chars<'local, StringRef>;

impl<'local, StringRef> MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    /// Constructs a [`MUTF8Chars`] from a `Env` and a `JString`.
    ///
    /// The string will be `NULL` terminated and encoded as [Modified
    /// UTF-8](https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8) /
    /// [CESU-8](https://en.wikipedia.org/wiki/CESU-8).
    ///
    /// The implementation may either create a copy of the character array for
    /// the given `String` or it may pin it to avoid it being collected by the
    /// garbage collector.
    ///
    /// Returns a [`MUTF8Chars`] that will automatically release the underlying
    /// character array when dropped (see [Self::release_string_utf_chars]).
    pub(crate) fn from_get_string_utf_chars(env: &Env<'_>, obj: StringRef) -> Result<Self> {
        let obj = null_check!(obj, "get_string_utf_chars obj argument")?;

        // SAFETY:
        // - We have checked that the object is not null.
        // - Having a `JString` guarantees that the reference is for a `java.lang.String`
        //   (it would require unsafe code for that to be violated)
        // - The pointer is immediately wrapped to ensure that the pointer will
        //   be released when dropped.
        unsafe {
            let mut is_copy: jboolean = false;
            let ptr: *const c_char = jni_call_only_check_null_ret!(
                env,
                v1_1,
                GetStringUTFChars,
                obj.as_raw(),
                &mut is_copy as *mut _
            )?;

            let is_copy = is_copy == JNI_TRUE;
            Ok(Self {
                obj,
                chars: ptr,
                is_copy,
                _lifetime: PhantomData,
            })
        }
    }

    /// Destroys the [`MUTF8Chars`] without freeing the underlying contents, and
    /// returns a raw pointer to it.
    ///
    /// The returned pointer is the same as the one returned by the
    /// [`as_ptr`][JNIStr::as_ptr] method. It points to a null-terminated
    /// string in [modified UTF-8] encoding (which is similar, but not
    /// identical, to [CESU-8]). It is valid when returned by this method, and
    /// will remain valid until freed (see below).
    ///
    /// [CESU-8]: https://en.wikipedia.org/wiki/CESU-8
    /// [modified UTF-8]: https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8
    ///
    /// This also returns the status for whether the original string was copied or not.
    ///
    /// # Warning
    ///
    /// After calling this method, the underlying string must be manually
    /// freed. This can be done either by reconstructing the [`MUTF8Chars`] using
    /// [`MUTF8Chars::from_raw`] and then dropping it, or by passing the pointer
    /// to the JNI function `ReleaseStringUTFChars`.
    pub fn into_raw(self) -> (*const c_char, bool) {
        let _dont_call_drop = std::mem::ManuallyDrop::new(self);
        (_dont_call_drop.chars, _dont_call_drop.is_copy)
    }

    /// Constructs a [`MUTF8Chars`] from raw components.
    ///
    /// The required components are, a [`Env`], a reference to a
    /// `java.lang.String` object, and a pointer to the characters of that
    /// `java.lang.String`.
    ///
    /// # Safety
    ///
    /// `ptr` must be a valid, non-null pointer, previously returned by
    /// [`MUTF8Chars::into_raw`] or the JNI function `GetStringUTFChars`. `ptr`
    /// must not belong to another [`MUTF8Chars`] at the same time.
    ///
    /// `obj` must be a non-null reference to the same `java.lang.String` object
    /// that `ptr` was obtained from.
    ///
    /// `is_copy` must be a boolean indicating whether the string was copied or
    /// not (as returned by `GetStringUTFChars`).
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{errors::Result, Env, objects::JString, strings::MUTF8Chars};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let jstring = JString::from_str(env, "foo")?;
    /// let chars = jstring.mutf8_chars(env)?;
    ///
    /// let (ptr, is_copy) = chars.into_raw();
    /// // Do whatever you need with the pointer
    /// let chars = unsafe { MUTF8Chars::from_raw(env, &jstring, ptr, is_copy) };
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn from_raw(
        _env: &Env<'_>,
        obj: StringRef,
        ptr: *const c_char,
        is_copy: bool,
    ) -> Self {
        Self {
            obj,
            chars: ptr,
            is_copy,
            _lifetime: PhantomData,
        }
    }

    /// Returns whether the string was copied or not.
    pub fn is_copy(&self) -> bool {
        self.is_copy
    }
}

impl<'local, StringRef> std::fmt::Display for MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let jni_str: &JNIStr = self;
        jni_str.fmt(f)
    }
}

impl<'local, StringRef> ::std::ops::Deref for MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    type Target = JNIStr;
    fn deref(&self) -> &Self::Target {
        self.into()
    }
}

impl<'local, 'java_str, StringRef> From<&'java_str MUTF8Chars<'local, StringRef>>
    for &'java_str JNIStr
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn from(other: &'java_str MUTF8Chars<'local, StringRef>) -> &'java_str JNIStr {
        unsafe { JNIStr::from_ptr(other.chars) }
    }
}

impl<'local, StringRef> From<MUTF8Chars<'local, StringRef>> for JNIString
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn from(other: MUTF8Chars<'local, StringRef>) -> JNIString {
        let jni_str: &JNIStr = &other;
        jni_str.to_owned()
    }
}

impl<'local, 'java_str, StringRef> From<&'java_str MUTF8Chars<'local, StringRef>>
    for Cow<'java_str, str>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn from(other: &'java_str MUTF8Chars<'local, StringRef>) -> Cow<'java_str, str> {
        let jni_str: &JNIStr = other;
        jni_str.into()
    }
}

impl<'local, StringRef> From<MUTF8Chars<'local, StringRef>> for String
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn from(other: MUTF8Chars<'local, StringRef>) -> String {
        let cow: Cow<str> = (&other).into();
        cow.into_owned()
    }
}

impl<'local, StringRef> Drop for MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn drop(&mut self) {
        unsafe fn release_string_utf_chars(
            obj: jni_sys::jobject,
            chars: *const c_char,
        ) -> Result<()> {
            // Error: Since we can't construct a `MUTF8Chars` without a valid `Env` reference we know
            // `JavaVM::singleton()` must be initialized and won't panic.
            JavaVM::singleton()?.with_top_local_frame(|env| {
                // This method is safe to call in case of pending exceptions (see the chapter 2 of the spec)
                unsafe {
                    ex_safe_jni_call_no_post_check_ex!(env, v1_1, ReleaseStringUTFChars, obj, chars)
                };

                Ok(())
            })
        }

        match unsafe { release_string_utf_chars(self.obj.as_raw(), self.chars) } {
            Ok(()) => {}
            Err(e) => warn!("error dropping java str: {}", e),
        }
    }
}

impl<'local, StringRef> AsRef<JNIStr> for MUTF8Chars<'local, StringRef>
where
    StringRef: AsRef<JString<'local>> + Reference,
{
    fn as_ref(&self) -> &JNIStr {
        self
    }
}
