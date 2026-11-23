#![allow(missing_docs)]

use std::char::{CharTryFromError, DecodeUtf16Error};

use thiserror::Error;

use crate::signature::RuntimeMethodSignature;
use crate::sys;

pub type Result<T> = std::result::Result<T, Error>;

#[cfg(doc)]
use crate::{
    JValue, JValueOwned,
    strings::{char_from_java_int, char_to_java, char_to_java_int},
};

/// Policies for handling [`EnvOutcome`] results within native methods.
mod policy;
pub use policy::*;

#[derive(Debug, Error)]
#[non_exhaustive]
pub enum Error {
    #[error("JavaVM singleton uninitialized")]
    UninitializedJavaVM,
    #[error("Invalid JValue type cast: {0}. Actual type: {1}")]
    WrongJValueType(&'static str, &'static str),

    /// An object with an inappropriate type was given
    ///
    /// For example, this may represent an `ArrayStoreException` that was caught
    #[error("Invalid object type")]
    WrongObjectType,
    #[error("Invalid constructor return type (must be void)")]
    InvalidCtorReturn,
    #[error("Invalid number or type of arguments passed to java method: {0}")]
    InvalidArgList(RuntimeMethodSignature),
    #[error("Object behind weak reference freed")]
    ObjectFreed,
    /// A class could not be found by a `ClassLoader` (`ClassNotFoundException`)
    ///
    /// Note: This specifically represents a `ClassNotFoundException` thrown by
    /// a class *loader* and is not expected to be returned by [`jni::Env::find_class`],
    /// [`jni::Env::load_class`] or [`jni::refs::LoaderContext`] that will
    /// generally return [Self::NoClassDefFound] in case a class could not be found.
    #[error("Class not found: {name:?}")]
    ClassNotFound { name: String },

    /// Indicates that the JVM could not resolve a Java class by name.
    ///
    /// This error corresponds to the JVM raising
    /// `java.lang.NoClassDefFoundError` during JNI class resolution (e.g. via
    /// [`jni::Env::find_class`]) or when emulating the same lookup semantics in
    /// higher-level helpers (such as [`jni::refs::LoaderContext::load_class`]).
    ///
    /// ## Meaning
    /// This does **not** strictly mean “the requested class does not exist”. It
    /// means that the JVM could not produce a usable class definition for the
    /// requested name in the current class-loader context.
    ///
    /// Common causes include:
    /// - The class is not present on the class path / module path.
    /// - The class exists but is not visible to the active class loader.
    /// - A transitive dependency of the requested class is missing or not
    ///   visible.
    ///
    /// ## Not included
    /// Failures that are reported as other linkage or format errors are
    /// surfaced via other [enum@Error] variants when possible (for example
    /// [`Error::ClassFormatError`], [`Error::ClassCircularityError`], or
    /// [`Error::LinkageError`]).
    ///
    /// ## Notes
    /// When returned from [`jni::refs::LoaderContext`] APIs, this variant may
    /// also be used to normalize Java's `ClassNotFoundException` (from
    /// `Class.forName`) into a single “class resolution failed” outcome, so
    /// that [`jni::refs::LoaderContext::load_class`] results are consistent
    /// with [`jni::Env::find_class`] (especially considering that `::load_class`
    /// may internally call `::find_class` as a fallback).
    #[error("failed to resolve Java class '{requested}' (class not found or linkage error)")]
    NoClassDefFound {
        requested: String,
        cause: Option<jni::refs::Global<jni::objects::JThrowable<'static>>>,
    },

    #[error("The class data does not specify a valid class")]
    ClassFormatError,
    #[error("A class or interface would be its own superclass or superinterface")]
    ClassCircularityError,
    #[error("Failed to link/verify Java class")]
    LinkageError {
        requested: String,
        cause: Option<jni::refs::Global<jni::objects::JThrowable<'static>>>,
    },
    #[error("Method not found: {name} {sig}")]
    MethodNotFound { name: String, sig: String },
    /// Represents a `NoSuchMethodError` exception
    ///
    /// For example, caught by [`crate::Env::register_native_methods`]
    #[error("No such method: {0}")]
    NoSuchMethod(String),
    #[error("Field not found: {name} {sig}")]
    FieldNotFound { name: String, sig: String },
    #[error("Java exception was thrown")]
    JavaException,
    #[error("Exception in initializer")]
    ExceptionInInitializer {
        exception: Option<jni::refs::Global<jni::objects::JThrowable<'static>>>,
    },
    /// A class could not be instantiated (`InstantiationException`), e.g.
    /// because it is abstract or an interface.
    #[error("Class cannot be instantiated")]
    Instantiation,
    #[error("Env null method pointer for {0}")]
    EnvMethodNotFound(&'static str),
    #[error("Null pointer in {0}")]
    NullPtr(&'static str),
    #[error("Mutex already locked")]
    TryLock,
    #[error("Field already set: {0}")]
    FieldAlreadySet(String),
    #[error("Throw failed with error code {0}")]
    ThrowFailed(i32),
    #[error("Parse failed for input: {0}")]
    ParseFailed(String),
    #[error("JNI call failed")]
    JniCall(#[source] JniError),

    /// [`JValue::c_char`] or [`JValueOwned::c_char`] was used, and although the value does indeed contain a Java `char`, it is part of a UTF-16 [surrogate pair] and cannot be converted to a Rust `char` by itself.
    ///
    /// [surrogate pair]: https://en.wikipedia.org/wiki/Surrogate_pair
    #[error("A Java `char` has the value 0x{char:x}; it is part of a UTF-16 surrogate pair and cannot be converted to a Rust `char` by itself", char = source.unpaired_surrogate())]
    InvalidUtf16 {
        /// The cause of this error. Use [`DecodeUtf16Error::unpaired_surrogate`] to get the Java `char` in question.
        #[source]
        source: DecodeUtf16Error,
    },

    /// [`JValue::i_char`] or [`JValueOwned::i_char`] was used, and although the value does indeed contain a Java `int`, it is not a valid UTF-32 unit.
    #[error(
        "A Java `int` has the value 0x{char:x}, which is not a valid UTF-32 unit; cannot convert it to a Rust `char`"
    )]
    InvalidUtf32 {
        /// The Java `int` that doesn't contain a valid UTF-32 unit.
        char: sys::jint,

        /// The cause of this error.
        #[source]
        source: CharTryFromError,
    },

    #[error("This Java virtual machine is too old; at least Java 1.4 is required")]
    UnsupportedVersion,

    #[error("The thread can't be detached while AttachGuards exist")]
    ThreadAttachmentGuarded,

    /// A Java exception that was caught and cleared
    ///
    /// Unlike [`Error::JavaException`], this error indicates that the exception was caught and cleared by the JNI code,
    /// rather than being left as a pending exception that may propagate back to the JVM.
    #[error("Caught Java exception: {msg}\nStack trace:\n{stack}")]
    #[non_exhaustive]
    CaughtJavaException {
        exception: jni::refs::Global<jni::objects::JThrowable<'static>>,
        name: String,
        msg: String,
        stack: String,
    },

    /// An array or string index was out of bounds
    #[error("Index out of bounds")]
    IndexOutOfBounds,

    /// A monitor operation was attempted on an object the thread does not own
    /// (`IllegalMonitorStateException`)
    #[error("Illegal monitor state")]
    IllegalMonitorState,

    /// A security manager denied the operation (`SecurityException`)
    #[error("Security violation")]
    SecurityViolation,
}

#[derive(Debug, Error)]
pub enum JniError {
    #[error("Unknown error")]
    Unknown,
    #[error("Current thread is not attached to the Java VM")]
    ThreadDetached,
    #[error("JNI version error")]
    WrongVersion,
    #[error("Not enough memory")]
    NoMemory,
    #[error("VM already created")]
    AlreadyCreated,
    /// An invalid argument was given
    ///
    /// This may also represent an `IllegalArgumentException` that was caught.
    #[error("Invalid arguments")]
    InvalidArguments,
    #[error("Error code {0}")]
    Other(sys::jint),
}

impl<T> From<::std::sync::TryLockError<T>> for Error {
    fn from(_: ::std::sync::TryLockError<T>) -> Self {
        Error::TryLock
    }
}

pub fn jni_error_code_to_result(code: sys::jint) -> Result<()> {
    match code {
        sys::JNI_OK => Ok(()),
        sys::JNI_ERR => Err(JniError::Unknown),
        sys::JNI_EDETACHED => Err(JniError::ThreadDetached),
        sys::JNI_EVERSION => Err(JniError::WrongVersion),
        sys::JNI_ENOMEM => Err(JniError::NoMemory),
        sys::JNI_EEXIST => Err(JniError::AlreadyCreated),
        sys::JNI_EINVAL => Err(JniError::InvalidArguments),
        _ => Err(JniError::Other(code)),
    }
    .map_err(Error::JniCall)
}

#[derive(Debug)]
pub struct Exception {
    pub class: String,
    pub msg: String,
}

pub trait ToException {
    fn to_exception(&self) -> Exception;
}

/// An error that occurred while starting the JVM using the JNI Invocation API.
///
/// This only exists if the "invocation" feature is enabled.
#[cfg(feature = "invocation")]
#[derive(Debug, Error)]
#[non_exhaustive]
pub enum StartJvmError {
    #[error(
        "Either the 'invocation' feature is not enabled or the JNI invocation API is not available on this platform"
    )]
    Unsupported,

    /// An attempt was made to find a JVM using [java-locator], but it failed.
    ///
    /// If this happens, give an explicit location to [`JavaVM::with_libjvm`] or set the
    /// `JAVA_HOME` environment variable.
    ///
    /// [java-locator]: https://docs.rs/java-locator/
    /// [`JavaVM::with_libjvm`]: crate::JavaVM::with_libjvm
    #[cfg(not(target_os = "android"))]
    #[error(
        "Couldn't automatically discover the Java VM's location (try setting the JAVA_HOME environment variable): {0}"
    )]
    NotFound(
        #[from]
        #[source]
        java_locator::errors::JavaLocatorError,
    ),
    /// A place holder for the `libloading` error, when built without the `libloading` dependency
    #[cfg(target_os = "android")]
    #[error(
        "Couldn't automatically discover the Java VM's location (try setting the JAVA_HOME environment variable): {0}"
    )]
    NotFound(String),

    /// An error occurred in trying to load the JVM shared library.
    ///
    /// On Windows, if this happens it may be necessary to add your `$JAVA_HOME/bin` directory
    /// to the DLL search path by adding it to the `PATH` environment variable.
    #[cfg(not(target_os = "android"))]
    #[error("Couldn't load the Java VM shared library ({0}): {1}")]
    LoadError(String, #[source] libloading::Error),
    /// A place holder for the `libloading` error, when built without the `libloading` dependency
    #[cfg(target_os = "android")]
    #[error("Couldn't load the Java VM shared library ({0}): {1}")]
    LoadError(String, String),

    /// The JNI function `JNI_CreateJavaVM` returned an error.
    #[error("{0}")]
    Create(
        #[from]
        #[source]
        Error,
    ),
}

#[cfg(feature = "invocation")]
pub type StartJvmResult<T> = std::result::Result<T, StartJvmError>;

/// Raised by [`char_to_java`] and the implementation of `TryFrom<char>` for [`JValue`] / [`JValueOwned`] when a Rust [`char`] is not representable as a Java `char`.
///
/// See [`char_to_java`] for more information.
#[derive(Debug, Error)]
#[error("The code point U+{char_as_u32:X} {char:?} cannot be converted to a Java `char`, because it is not representable as a single UTF-16 unit.", char_as_u32 = u32::from(*char))]
pub struct CharToJavaError {
    /// The character that could not be converted.
    pub char: char,
}
