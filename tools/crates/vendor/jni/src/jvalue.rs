use std::char::{CharTryFromError, DecodeUtf16Error};
use std::convert::{TryFrom, TryInto};
use std::fmt::Debug;

use log::trace;

use crate::signature::JavaType;
use crate::{errors::*, objects::JObject, signature::Primitive, sys::*};

#[cfg(doc)]
use crate::env::Env;

/// A Java owned local reference or primitive value.
///
/// This type is used for values returned from Java method calls. If the Java
/// method returns an object reference, it will take the form of an owned
/// [`JObject`].
///
/// See also [`JValue`], which is used for Java method call parameters. It is
/// different from this type in that it *borrows* an object reference instead
/// of owning one.
#[allow(missing_docs)]
#[derive(Debug)]
pub enum JValueOwned<'local> {
    Object(JObject<'local>),
    Byte(jbyte),
    Char(jchar),
    Short(jshort),
    Int(jint),
    Long(jlong),
    Bool(jboolean),
    Float(jfloat),
    Double(jdouble),
    Void,
}

/// A Java borrowed local reference or primitive value.
///
/// This type is used for parameters passed to Java method calls. If the Java
/// method is to be passed an object reference, it takes the form of a borrowed
/// <code>&[JObject]</code>.
///
/// See also [`JValueOwned`], which is used for Java method return values. It is
/// different from this type in that it *owns* an object reference instead
/// of borrowing one.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug)]
pub enum JValue<'obj_ref> {
    Object(&'obj_ref JObject<'obj_ref>),
    Byte(jbyte),
    Char(jchar),
    Short(jshort),
    Int(jint),
    Long(jlong),
    Bool(jboolean),
    Float(jfloat),
    Double(jdouble),
    Void,
}

impl<'local> JValueOwned<'local> {
    /// Convert the enum to its jni-compatible equivalent.
    pub fn as_jni(&self) -> jvalue {
        self.borrow().as_jni()
    }

    /// Get the type name for the enum variant.
    pub fn type_name(&self) -> &'static str {
        self.borrow().type_name()
    }

    /// Get the primitive type for the enum variant. If it's not a primitive
    /// (i.e. an Object), returns None.
    pub fn primitive_type(&self) -> Option<Primitive> {
        self.borrow().primitive_type()
    }

    /// Check if the value is a `null` object reference.
    pub fn is_null(&self) -> bool {
        matches!(self, Self::Object(obj) if obj.as_raw().is_null())
    }

    /// Check if the value is a `null` object reference, and return a [Error::NullPtr] error if so,
    /// otherwise, return the value unchanged.
    ///
    /// This can be useful with APIs where a null represents an error but the implementation doesn't
    /// throw a Java exception.
    ///
    /// **Beware:** Since `null` object references may be a valid (and safe) in many contexts, you
    /// should only use this method if you are sure that `null` represents an error in the specific
    /// context where you're using it.
    ///
    /// **Note**: In many cases if `null` represents an error then the API would throw a Java
    /// exception, which would be automatically translated to an [Error::JavaException] error by
    /// this crate.
    pub fn check_null(self) -> Result<Self> {
        match self {
            Self::Object(obj) => {
                if obj.as_raw().is_null() {
                    Err(Error::NullPtr("Null Object"))
                } else {
                    Ok(Self::Object(obj))
                }
            }
            _ => Ok(self),
        }
    }

    /// Try to unwrap to an Object.
    ///
    /// See [`Self::into_object`] as a more-ergonomic alias.
    pub fn l(self) -> Result<JObject<'local>> {
        match self {
            Self::Object(obj) => Ok(obj),
            _ => Err(Error::WrongJValueType("object", self.type_name())),
        }
    }

    /// Try to unwrap to an Object.
    #[doc(alias = "l")]
    pub fn into_object(self) -> Result<JObject<'local>> {
        self.l()
    }

    /// Try to unwrap to a boolean.
    ///
    /// See [`Self::into_bool`] as a more-ergonomic alias.
    pub fn z(self) -> Result<bool> {
        self.borrow().z()
    }

    /// Try to unwrap to a boolean.
    #[doc(alias = "z")]
    pub fn into_bool(self) -> Result<bool> {
        self.borrow().z()
    }

    /// Try to unwrap to a byte.
    ///
    /// See [`Self::into_byte`] as a more-ergonomic alias.
    pub fn b(self) -> Result<jbyte> {
        self.borrow().b()
    }

    /// Try to unwrap to a byte.
    #[doc(alias = "b")]
    pub fn into_byte(self) -> Result<jbyte> {
        self.borrow().b()
    }

    /// Try to unwrap to a `jchar`.
    ///
    /// See [`Self::into_char`] as a more-ergonomic alias.
    pub fn c(self) -> Result<jchar> {
        self.borrow().c()
    }

    /// Try to unwrap to a `jchar`.
    ///
    /// **Warning:** A Java `char` is a UTF-16 code unit, which cannot always be converted to a Rust
    /// `char` (which is a Unicode scalar value).
    ///
    /// See [`char_from_java`] for more information.
    #[doc(alias = "c")]
    pub fn into_char(self) -> Result<jchar> {
        self.borrow().c()
    }

    /// Try to unwrap a Java `char` and then convert it to a Rust `char`.
    ///
    /// **Warning:** This conversion is likely to fail. Using it is not recommended. Prefer
    /// [`JValueOwned::i_char`] where possible. See [`char_from_java`] for more information.
    ///
    /// # Errors
    ///
    /// This method can fail with two kinds of errors:
    ///
    /// * [`Error::WrongJValueType`]: `self` does not contain a Java `char`.
    /// * [`Error::InvalidUtf16`]: `self` contains a Java `char`, but it is one half of a surrogate
    ///   pair.
    #[deprecated = "Use `into_char` to get the raw Java jchar, and then (attempt to) convert it to a Rust char using `char_from_java` if needed. This method is likely to fail and is not recommended."]
    pub fn c_char(self) -> Result<char> {
        let char = self.c()?;

        char_from_java(char).map_err(|source| Error::InvalidUtf16 { source })
    }

    /// Try to unwrap to a double.
    ///
    /// See [`Self::into_double`] as a more-ergonomic alias.
    pub fn d(self) -> Result<jdouble> {
        self.borrow().d()
    }

    /// Try to unwrap to a double.
    #[doc(alias = "d")]
    pub fn into_double(self) -> Result<jdouble> {
        self.borrow().d()
    }

    /// Try to unwrap to a float.
    ///
    /// See [`Self::into_float`] as a more-ergonomic alias.
    pub fn f(self) -> Result<jfloat> {
        self.borrow().f()
    }

    /// Try to unwrap to a float.
    #[doc(alias = "f")]
    pub fn into_float(self) -> Result<jfloat> {
        self.borrow().f()
    }

    /// Try to unwrap to an int.
    ///
    /// See [`Self::into_int`] as a more-ergonomic alias.
    pub fn i(self) -> Result<jint> {
        self.borrow().i()
    }

    /// Try to unwrap to an int.
    #[doc(alias = "i")]
    pub fn into_int(self) -> Result<jint> {
        self.borrow().i()
    }

    /// Try to unwrap a Rust `char` from a Java `int`. See [`char_from_java_int`] for details.
    ///
    /// # Errors
    ///
    /// This method can fail with two kinds of errors:
    ///
    /// * [`Error::WrongJValueType`]: `self` does not contain a Java `int`.
    /// * [`Error::InvalidUtf32`]: `self` contains a Java `int`, but it is not a valid UTF-32 unit.
    pub fn i_char(self) -> Result<char> {
        self.i_into_char()
    }

    /// Try to unwrap a Rust `char` from a Java `int`. See [`char_from_java_int`] for details.
    ///
    /// # Errors
    ///
    /// This method can fail with two kinds of errors:
    ///
    /// * [`Error::WrongJValueType`]: `self` does not contain a Java `int`.
    /// * [`Error::InvalidUtf32`]: `self` contains a Java `int`, but it is not a valid UTF-32 unit.
    #[doc(alias = "i_char")]
    pub fn i_into_char(self) -> Result<char> {
        let char = self.i()?;

        char_from_java_int(char).map_err(|source| Error::InvalidUtf32 { char, source })
    }

    /// Try to unwrap to a long.
    ///
    /// See [`Self::into_long`] as a more-ergonomic alias.
    pub fn j(self) -> Result<jlong> {
        self.borrow().j()
    }

    /// Try to unwrap to a long.
    #[doc(alias = "j")]
    pub fn into_long(self) -> Result<jlong> {
        self.borrow().j()
    }

    /// Try to unwrap to a short.
    ///
    /// See [`Self::into_short`] as a more-ergonomic alias.
    pub fn s(self) -> Result<jshort> {
        self.borrow().s()
    }

    /// Try to unwrap to a short.
    #[doc(alias = "s")]
    pub fn into_short(self) -> Result<jshort> {
        self.borrow().s()
    }

    /// Try to unwrap to a void.
    ///
    /// See [`Self::into_void`] as a more-ergonomic alias.
    pub fn v(self) -> Result<()> {
        self.borrow().v()
    }

    /// Try to unwrap to a void.
    #[doc(alias = "v")]
    pub fn into_void(self) -> Result<()> {
        self.borrow().v()
    }

    /// Copies or borrows the value in this `JValueOwned`.
    ///
    /// If the value is a primitive type, it is copied. If the value is an
    /// object reference, it is borrowed.
    #[inline]
    pub fn borrow(&'_ self) -> JValue<'_> {
        match self {
            Self::Object(o) => JValue::Object(o),
            Self::Byte(v) => JValue::Byte(*v),
            Self::Char(v) => JValue::Char(*v),
            Self::Short(v) => JValue::Short(*v),
            Self::Int(v) => JValue::Int(*v),
            Self::Long(v) => JValue::Long(*v),
            Self::Bool(v) => JValue::Bool(*v),
            Self::Float(v) => JValue::Float(*v),
            Self::Double(v) => JValue::Double(*v),
            Self::Void => JValue::Void,
        }
    }
}

impl<'obj_ref> JValue<'obj_ref> {
    /// Convert the enum to its jni-compatible equivalent.
    pub fn as_jni(&self) -> jvalue {
        let val: jvalue = match self {
            Self::Object(obj) => jvalue { l: obj.as_raw() },
            Self::Byte(byte) => jvalue { b: *byte },
            Self::Char(char) => jvalue { c: *char },
            Self::Short(short) => jvalue { s: *short },
            Self::Int(int) => jvalue { i: *int },
            Self::Long(long) => jvalue { j: *long },
            Self::Bool(boolean) => jvalue { b: *boolean as i8 },
            Self::Float(float) => jvalue { f: *float },
            Self::Double(double) => jvalue { d: *double },
            Self::Void => jvalue {
                l: ::std::ptr::null_mut(),
            },
        };
        trace!("converted {:?} to jvalue {:?}", self, unsafe {
            ::std::mem::transmute::<jvalue, u64>(val)
        });
        val
    }

    /// Convert the enum to its jni-compatible equivalent.
    #[deprecated = "Use `as_jni` instead."]
    pub fn to_jni(self) -> jvalue {
        self.as_jni()
    }

    /// Get the type name for the enum variant.
    pub fn type_name(&self) -> &'static str {
        match *self {
            Self::Void => "void",
            Self::Object(_) => "object",
            Self::Byte(_) => "byte",
            Self::Char(_) => "char",
            Self::Short(_) => "short",
            Self::Int(_) => "int",
            Self::Long(_) => "long",
            Self::Bool(_) => "bool",
            Self::Float(_) => "float",
            Self::Double(_) => "double",
        }
    }

    /// Get the corresponding Java type.
    pub fn java_type(&self) -> JavaType {
        match *self {
            Self::Object(_) => JavaType::Object,
            Self::Void => JavaType::Primitive(Primitive::Void),
            Self::Byte(_) => JavaType::Primitive(Primitive::Byte),
            Self::Char(_) => JavaType::Primitive(Primitive::Char),
            Self::Short(_) => JavaType::Primitive(Primitive::Short),
            Self::Int(_) => JavaType::Primitive(Primitive::Int),
            Self::Long(_) => JavaType::Primitive(Primitive::Long),
            Self::Bool(_) => JavaType::Primitive(Primitive::Boolean),
            Self::Float(_) => JavaType::Primitive(Primitive::Float),
            Self::Double(_) => JavaType::Primitive(Primitive::Double),
        }
    }

    /// Get the primitive type for the enum variant. If it's not a primitive
    /// (i.e. an Object), returns None.
    pub fn primitive_type(&self) -> Option<Primitive> {
        Some(match *self {
            Self::Object(_) => return None,
            Self::Void => Primitive::Void,
            Self::Byte(_) => Primitive::Byte,
            Self::Char(_) => Primitive::Char,
            Self::Short(_) => Primitive::Short,
            Self::Int(_) => Primitive::Int,
            Self::Long(_) => Primitive::Long,
            Self::Bool(_) => Primitive::Boolean,
            Self::Float(_) => Primitive::Float,
            Self::Double(_) => Primitive::Double,
        })
    }

    /// Try to unwrap to an Object.
    pub fn l(self) -> Result<&'obj_ref JObject<'obj_ref>> {
        match self {
            Self::Object(obj) => Ok(obj),
            _ => Err(Error::WrongJValueType("object", self.type_name())),
        }
    }

    /// Try to unwrap to a boolean.
    pub fn z(self) -> Result<bool> {
        match self {
            Self::Bool(b) => Ok(b == JNI_TRUE),
            _ => Err(Error::WrongJValueType("bool", self.type_name())),
        }
    }

    /// Try to unwrap to a byte.
    pub fn b(self) -> Result<jbyte> {
        match self {
            Self::Byte(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jbyte", self.type_name())),
        }
    }

    /// Try to unwrap to a char.
    pub fn c(self) -> Result<jchar> {
        match self {
            Self::Char(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jchar", self.type_name())),
        }
    }

    /// Try to unwrap a Java `char` and then convert it to a Rust `char`.
    ///
    /// **Warning:** This conversion is likely to fail. Using it is not recommended. Prefer
    /// [`JValue::i_char`] where possible. See [`char_from_java`] for more information.
    ///
    /// # Errors
    ///
    /// This method can fail with two kinds of errors:
    ///
    /// * [`Error::WrongJValueType`]: `self` does not contain a Java `char`.
    /// * [`Error::InvalidUtf16`]: `self` contains a Java `char`, but it is one half of a surrogate
    ///   pair.
    pub fn c_char(self) -> Result<char> {
        let char = self.c()?;

        char_from_java(char).map_err(|source| Error::InvalidUtf16 { source })
    }

    /// Try to unwrap to a double.
    pub fn d(self) -> Result<jdouble> {
        match self {
            Self::Double(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jdouble", self.type_name())),
        }
    }

    /// Try to unwrap to a float.
    pub fn f(self) -> Result<jfloat> {
        match self {
            Self::Float(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jfloat", self.type_name())),
        }
    }

    /// Try to unwrap to an int.
    pub fn i(self) -> Result<jint> {
        match self {
            Self::Int(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jint", self.type_name())),
        }
    }

    /// Try to unwrap a Rust `char` from a Java `int`. See [`char_from_java_int`] for details.
    ///
    /// # Errors
    ///
    /// This method can fail with two kinds of errors:
    ///
    /// * [`Error::WrongJValueType`]: `self` does not contain a Java `int`.
    /// * [`Error::InvalidUtf32`]: `self` contains a Java `int`, but it is not a valid UTF-32 unit.
    pub fn i_char(self) -> Result<char> {
        let char = self.i()?;

        char_from_java_int(char).map_err(|source| Error::InvalidUtf32 { char, source })
    }

    /// Try to unwrap to a long.
    pub fn j(self) -> Result<jlong> {
        match self {
            Self::Long(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jlong", self.type_name())),
        }
    }

    /// Try to unwrap to a short.
    pub fn s(self) -> Result<jshort> {
        match self {
            Self::Short(b) => Ok(b),
            _ => Err(Error::WrongJValueType("jshort", self.type_name())),
        }
    }

    /// Try to unwrap to a void.
    pub fn v(self) -> Result<()> {
        match self {
            Self::Void => Ok(()),
            _ => Err(Error::WrongJValueType("void", self.type_name())),
        }
    }

    /// Converts a Rust `char` to a Java `int`. See [`char_to_java_int`] for details.
    pub fn int_from_char(char: char) -> Self {
        Self::Int(char_to_java_int(char))
    }
}

impl<'obj_ref> From<&'obj_ref JValueOwned<'obj_ref>> for JValue<'obj_ref> {
    fn from(other: &'obj_ref JValueOwned) -> Self {
        other.borrow()
    }
}

impl<'local, T: Into<JObject<'local>>> From<T> for JValueOwned<'local> {
    fn from(other: T) -> Self {
        Self::Object(other.into())
    }
}

impl<'obj_ref, T: AsRef<JObject<'obj_ref>>> From<&'obj_ref T> for JValue<'obj_ref> {
    fn from(other: &'obj_ref T) -> Self {
        Self::Object(other.as_ref())
    }
}

impl<'local> TryFrom<JValueOwned<'local>> for JObject<'local> {
    type Error = Error;

    fn try_from(value: JValueOwned<'local>) -> Result<Self> {
        value.l()
    }
}

impl From<jboolean> for JValueOwned<'_> {
    fn from(other: jboolean) -> Self {
        Self::Bool(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jboolean {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.borrow().try_into()
    }
}

impl From<jboolean> for JValue<'_> {
    fn from(other: jboolean) -> Self {
        Self::Bool(other)
    }
}

impl TryFrom<JValue<'_>> for jboolean {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        match value {
            JValue::Bool(b) => Ok(b),
            _ => Err(Error::WrongJValueType("bool", value.type_name())),
        }
    }
}

// jchar
impl From<jchar> for JValueOwned<'_> {
    fn from(other: jchar) -> Self {
        Self::Char(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jchar {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.c()
    }
}

impl From<jchar> for JValue<'_> {
    fn from(other: jchar) -> Self {
        Self::Char(other)
    }
}

impl TryFrom<JValue<'_>> for jchar {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.c()
    }
}

/// Converts a Rust `char` to a Java `char`, if possible.
///
/// **Warning:** This conversion is likely to fail. Using it is not recommended. Prefer
/// [`JValue::int_from_char`] where possible. See [`char_to_java`] for more information.
impl TryFrom<char> for JValueOwned<'_> {
    type Error = CharToJavaError;

    fn try_from(value: char) -> std::result::Result<Self, Self::Error> {
        Ok(Self::Char(char_to_java(value)?))
    }
}

/// Converts a Rust `char` to a Java `char`, if possible.
///
/// **Warning:** This conversion is likely to fail. Using it is not recommended. Prefer
/// [`JValue::int_from_char`] where possible. See [`char_to_java`] for more information.
impl TryFrom<char> for JValue<'_> {
    type Error = CharToJavaError;

    fn try_from(value: char) -> std::result::Result<Self, Self::Error> {
        Ok(Self::Char(char_to_java(value)?))
    }
}

/// Converts a Java `char` to a Rust `char`, if possible. (Error-prone; see warning.)
///
/// **Warning:** Converting a single Java `char` to a Rust `char` is not recommended. This can only
/// succeed for code points up to U+FFFF. Code points above that are represented in Java as *two*
/// `char`s, each representing one half of a UTF-16 [surrogate pair], which this function cannot
/// handle. If possible, use one of these alternatives instead:
///
/// * Use Java `int`s containing UTF-32. You can encode a Java `String` as a sequence of UTF-32
///   `int`s using its [`codePoints`] method, then use [`char_from_java_int`] to convert each one to
///   a Rust `char`.
///
/// * Convert a Java `String` using [`Env::get_string`].
///
/// * Convert multiple Java `char`s at a time (such as in a Java `char[]` array) using
///   [`char::decode_utf16`] (which this function is a simple wrapper around). That will properly
///   convert any surrogate pairs among the Java `char`s.
///
/// # See Also
///
/// * [`JValue::c_char`], a wrapper around this function that unwraps [`JValue::Char`]
/// * [`JValueOwned::c_char`], a wrapper around this function that unwraps [`JValueOwned::Char`]
/// * [`char_to_java`], the opposite of this function
/// * [`char_from_java_int`], a UTF-32 alternative to this function that is unlikely to fail
///
/// # Errors
///
/// This function returns an error if the provided Java `char` is part of a UTF-16 surrogate pair,
/// which cannot be converted to a Rust `char` by itself.
///
/// [`codePoints`]:
///     https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/String.html#codePoints()
/// [surrogate pair]: https://en.wikipedia.org/wiki/Surrogate_pair
pub fn char_from_java(char: jchar) -> std::result::Result<char, DecodeUtf16Error> {
    char::decode_utf16([char]).next().unwrap()
}

/// Converts a Rust `char` to a Java `char`, if possible. (Error-prone; see warning.)
///
/// **Warning:** Converting a Rust `char` to a single Java `char` is not recommended. This can only
/// succeed for code points up to U+FFFF. Code points above that are represented in Java as *two*
/// `char`s, each representing one half of a UTF-16 [surrogate pair], which this function cannot
/// handle. If possible, use one of these alternatives instead:
///
/// * Use Java `int`s containing UTF-32. You can convert a Rust `char` to a Java `int` containing
///   UTF-32 using [`char_to_java_int`], which never fails.
///
/// * Convert a Rust [`str`] to a Java `String` using [`Env::new_string`].
///
/// * Convert a Rust `char` to multiple Java `char`s at a time using [`char::encode_utf16`] (which
///   this function is a wrapper around). That will properly generate surrogate pairs as needed.
///
/// # See Also
///
/// * [`JValue`] and [`JValueOwned`]'s implementation of `TryFrom<char>`, a wrapper for this function that produces
///   [`JValue::Char`] and [`JValueOwned::Char`]
/// * [`char_from_java`], the opposite of this function
/// * [`char_to_java_int`], a UTF-32 alternative to this function that never fails
///
/// # Errors
///
/// This function returns an error if the provided `char` cannot be represented in UTF-16 without a
/// surrogate pair, and therefore cannot be converted to a single Java `char`.
///
/// [surrogate pair]: https://en.wikipedia.org/wiki/Surrogate_pair
pub fn char_to_java(char: char) -> std::result::Result<jchar, CharToJavaError> {
    if char.len_utf16() != 1 {
        return Err(CharToJavaError { char });
    }

    let mut buf = [0u16; 1];
    let buf: &mut [u16] = char.encode_utf16(&mut buf);
    Ok(buf[0])
}

// jshort
impl From<jshort> for JValueOwned<'_> {
    fn from(other: jshort) -> Self {
        Self::Short(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jshort {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.s()
    }
}

impl From<jshort> for JValue<'_> {
    fn from(other: jshort) -> Self {
        Self::Short(other)
    }
}

impl TryFrom<JValue<'_>> for jshort {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.s()
    }
}

// jfloat
impl From<jfloat> for JValueOwned<'_> {
    fn from(other: jfloat) -> Self {
        Self::Float(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jfloat {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.f()
    }
}

impl From<jfloat> for JValue<'_> {
    fn from(other: jfloat) -> Self {
        Self::Float(other)
    }
}

impl TryFrom<JValue<'_>> for jfloat {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.f()
    }
}

// jdouble
impl From<jdouble> for JValueOwned<'_> {
    fn from(other: jdouble) -> Self {
        Self::Double(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jdouble {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.d()
    }
}

impl From<jdouble> for JValue<'_> {
    fn from(other: jdouble) -> Self {
        Self::Double(other)
    }
}

impl TryFrom<JValue<'_>> for jdouble {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.d()
    }
}

// jint
impl From<jint> for JValueOwned<'_> {
    fn from(other: jint) -> Self {
        Self::Int(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jint {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.i()
    }
}

impl From<jint> for JValue<'_> {
    fn from(other: jint) -> Self {
        Self::Int(other)
    }
}

impl TryFrom<JValue<'_>> for jint {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.i()
    }
}

/// Converts a Rust `char` to a Java `int`.
///
/// This is the form expected or produced by certain Java APIs that process UTF-32 units, such as [`String.codePointAt`].
///
/// As discussed in [`char_to_java`], Rust `char` cannot always be converted to Java `char`, but can always be converted to Java `int`. This is the recommended way to pass a Rust `char` to Java code.
///
/// # See Also
///
/// * [`JValue::int_from_char`], a wrapper for this function that returns [`JValue::Int`]
/// * [`char_from_java_int`], the opposite of this function
/// * [`char_to_java`], an alternative to this function that converts to Java `char` but is likely to fail
///
/// [`String.codePointAt`]: https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/String.html#codePointAt(int)
pub fn char_to_java_int(char: char) -> jint {
    u32::from(char) as jint
}

/// Converts a Java `int` to a Rust `char`.
///
/// This is the form expected or produced by certain Java APIs that process UTF-32 units, such as [`String.codePointAt`].
///
/// As discussed in [`char_from_java`], Rust `char` cannot always be converted from Java `char`, but can always be converted from Java `int` (provided that the `int` contains a valid UTF-32 unit). This is the recommended way to receive a Rust `char` from Java code.
///
/// # See Also
///
/// * [`JValue::i_char`], a wrapper for this function that unwraps [`JValue::Int`]
/// * [`JValueOwned::i_char`], a wrapper for this function that unwraps [`JValueOwned::Int`]
/// * [`char_to_java_int`], the opposite of this function
/// * [`char_from_java`], an alternative to this function that converts from Java `char` but is likely to fail
///
/// # Errors
///
/// Returns an error if the Java `int` doesn't represent a valid UTF-32 unit.
///
/// [`String.codePointAt`]: https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/String.html#codePointAt(int)
pub fn char_from_java_int(jint: jint) -> std::result::Result<char, CharTryFromError> {
    char::try_from(jint as u32)
}

// jlong
impl From<jlong> for JValueOwned<'_> {
    fn from(other: jlong) -> Self {
        Self::Long(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jlong {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.j()
    }
}

impl From<jlong> for JValue<'_> {
    fn from(other: jlong) -> Self {
        Self::Long(other)
    }
}

impl TryFrom<JValue<'_>> for jlong {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.j()
    }
}

// jbyte
impl From<jbyte> for JValueOwned<'_> {
    fn from(other: jbyte) -> Self {
        Self::Byte(other)
    }
}

impl TryFrom<JValueOwned<'_>> for jbyte {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.b()
    }
}

impl From<jbyte> for JValue<'_> {
    fn from(other: jbyte) -> Self {
        Self::Byte(other)
    }
}

impl TryFrom<JValue<'_>> for jbyte {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.b()
    }
}

// jvoid
impl From<()> for JValueOwned<'_> {
    fn from(_: ()) -> Self {
        Self::Void
    }
}

impl TryFrom<JValueOwned<'_>> for () {
    type Error = Error;

    fn try_from(value: JValueOwned) -> Result<Self> {
        value.v()
    }
}

impl From<()> for JValue<'_> {
    fn from(_: ()) -> Self {
        Self::Void
    }
}

impl TryFrom<JValue<'_>> for () {
    type Error = Error;

    fn try_from(value: JValue) -> Result<Self> {
        value.v()
    }
}
