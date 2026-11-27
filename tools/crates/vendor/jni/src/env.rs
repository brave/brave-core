use std::{
    convert::TryInto,
    marker::PhantomData,
    os::raw::{c_char, c_void},
    panic::{AssertUnwindSafe, catch_unwind, resume_unwind},
    ptr,
    sync::{Mutex, MutexGuard},
};

use jni_sys::jobject;

use crate::{
    DEFAULT_LOCAL_FRAME_CAPACITY, JNIVersion, JavaVM,
    descriptors::Desc,
    errors::*,
    jni_sig,
    objects::{
        Auto, AutoElements, AutoElementsCritical, Global, IntoAuto, JByteBuffer, JClass,
        JClassLoader, JFieldID, JList, JMap, JMethodID, JObject, JStaticFieldID, JStaticMethodID,
        JString, JThrowable, JValue, JValueOwned, ReleaseMode, TypeArray, Weak,
    },
    signature::{FieldSignature, JavaType, MethodSignature, Primitive},
    strings::{JNIStr, MUTF8Chars},
    sys::{
        self, JNINativeMethod, jboolean, jbyte, jchar, jdouble, jfloat, jint, jlong, jshort, jsize,
        jvalue,
    },
};
use crate::{
    errors::ErrorPolicy,
    objects::{
        Cast, JBooleanArray, JByteArray, JCharArray, JDoubleArray, JFloatArray, JIntArray,
        JLongArray, JObjectArray, JPrimitiveArray, JShortArray, LoaderContext,
        type_array_sealed::TypeArraySealed,
    },
};
use crate::{objects::AsJArrayRaw, signature::ReturnType};

use super::{AttachGuard, objects::Reference};

#[cfg(doc)]
use crate::{jni_str, objects::JThread, strings::JNIString};

/// A non-transparent wrapper around a raw [`sys::JNIEnv`] pointer that provides
/// safe access to the Java Native Interface (JNI) functions.
///
/// Since [Env] is not a type that is publicly constructible, you must obtain a
/// reference, either through a thread attachment, by upgrading an
/// [`EnvUnowned`] within a native method, or via the (unsafe)
/// [`AttachGuard::from_unowned`] APIs.
///
/// See:
/// - [`JavaVM::attach_current_thread`]
/// - [`JavaVM::attach_current_thread_for_scope`]
/// - [`JavaVM::with_local_frame`]
/// - [`JavaVM::with_top_local_frame`]
/// - [`EnvUnowned::with_env`]
///
/// [Env] is a non-transparent wrapper that is not FFI-safe, so it must not be
/// used to try and capture a raw [`sys::JNIEnv`] pointer passed to a native
/// method.
///
/// See [`EnvUnowned`] for a wrapper that is FFI-safe and can be used to capture
/// a `sys::JNIEnv` pointer passed to a native method.
///
/// # Overview of the API
///
/// All the [`Env`] methods aim to follow the JNI specification naming closely
/// enough to make it easier to map between the two. This means that method
/// names, parameter types, and return types will generally be consistent with
/// the JNI specification.
///
/// JNI APIs that can logically be thought of as methods, such as
/// `GetObjectArrayElement`, can be found as methods on the relevant Rust type,
/// in this case [`JObjectArray::get_element`].
///
/// # Exception handling
///
/// Since we're calling into the JVM with this, many methods also have the
/// potential to cause an exception to get thrown. If this is the case, an `Err`
/// result will be returned with the error kind [`Error::JavaException`]. Note
/// that this will _not_ clear the exception - it's up to the caller to decide
/// whether to do so or to let it continue being thrown.
///
/// # References and Lifetimes
///
/// As in C JNI, interactions with Java objects happen through
/// <dfn>references</dfn>, either local or global.
///
/// This crate provides various safe wrappers around JNI references, such as
/// [`JObject`], [`JClass`], [`JString`] and [`JList`] which all implement the
/// [`Reference`] trait.
///
/// By default these types represent <dfn>local references</dfn>, and they will
/// be associated with a lifetime that names the local reference frame they
/// belong to.
///
/// So for example `JObject<'local_1>`  and  `JObject<'local_2>` are local
/// reference that are owned by two different local reference frames and
/// although they may both be usable from a higher stack frame, it's not
/// possible to simply move ownership of a local reference from one local
/// reference frame to another.
///
/// A global reference can be represented by a [`Global`], which will wrap one
/// of the above primitive reference types like `Global<JObject<'static>>`. In
/// this case the `'static` lifetime indicates that the reference is not tied to
/// a local reference frame and can be used from any thread.
///
/// So long as there is at least one reference to a Java object, the JVM garbage
/// collector will not reclaim it.
///
/// <dfn>Global references</dfn> exist until deleted. Deletion occurs when the
/// `Global` is dropped.
///
/// <dfn>Local references</dfn> belong to a local reference frame, and exist
/// until [deleted][Env::delete_local_ref] or until the local reference frame is
/// exited. A new <dfn>local reference frame</dfn> is entered when a native
/// method is called from Java, or when Rust code does so explicitly using
/// [`Env::with_local_frame`]. That local reference frame is exited when the
/// native method or `with_local_frame` returns. When a local reference frame is
/// exited, all local references created inside it are deleted.
///
/// Unlike C JNI, this crate creates a separate `Env` for each local reference
/// frame. The associated Rust lifetime `'local` represents that local reference
/// frame. Rust's borrow checker will ensure that local references are not used
/// after their local reference frame exits (which would cause undefined
/// behavior).
///
/// Unlike global references, local references are not deleted when dropped by
/// default. This is for performance: it is faster for the JVM to delete all of
/// the local references in a frame all at once, than to delete each local
/// reference one at a time. However, this can cause a memory leak if the local
/// reference frame remains entered for a long time, such as a long-lasting
/// loop, in which case local references should be deleted explicitly. Local
/// references can be deleted when dropped if desired; use
/// [`Env::delete_local_ref`] or wrap with [`IntoAuto::auto`] to arrange that.
///
/// ## Lifetime Names
///
/// This crate uses the following convention for lifetime names:
///
/// * `'local` is the lifetime of a local reference frame, as described above.
///
/// * `'other_local`, `'other_local_1`, and `'other_local_2` are the lifetimes
///   of some other local reference frame, which may be but doesn't have to be
///   the same as `'local`. For example, [`Env::new_local_ref`] accepts a local
///   reference in any local reference frame `'other_local` and creates a new
///   local reference to the same object in `'local`.
///
/// * `'obj_ref` is the lifetime of a borrow of a JNI reference, like
///   <code>&amp;[JObject]</code> or <code>&amp;[Global]</code>. For example,
///   [`Env::get_list`] constructs a new [`JList`] that borrows a `&'obj_ref
///   JObject`.
///
/// ## `null` Java references
/// `null` Java references are handled by the following rules:
///   - Methods that require non-null references will return [Error::NullPtr] if
///     passed a `null` reference.
///   - If a JNI function returns `null` to indicate an error (e.g.
///     [`Env::new_int_array`]), it is converted to `Err`/[`Error::NullPtr`], or
///     some other more applicable error type, such as
///     [`Error::MethodNotFound`].
///   - Otherwise `null` is considered a valid Java reference and is represented
///     as [`JObject::null()`].
///
/// ## A `&mut Env` represents the top JNI stack frame
///
/// Each `Env<'local>` represents a single local reference frame in the JVM.
///
/// A **mutable** `&mut Env` is special: it always represents the *current top*
/// local reference frame on the JNI stack.
///
/// * You need `&mut Env` when creating new local references, since those can
///   only be created in the top frame.
/// * A shared `&Env` is fine for operations that don’t produce new local
///   references.
///
/// If you only have a shared `&Env` but need to create temporary local
/// references (that won’t escape the scope), you can materialize a new mutable
/// handle to the top frame using:
///
/// * [`Env::with_local_frame`] – pushes a new frame, returns a mutable `Env`
///   for it.
/// * [`Env::with_top_local_frame`] – borrows a mutable `Env` for the existing
///   top frame without pushing a new one.
///
/// If you don’t even have an `Env` (e.g. inside a `Drop` implementation), you
/// can still get one via:
///
/// * [`JavaVM::attach_current_thread`]
/// * [`JavaVM::with_top_local_frame`] - if you can guarantee that the thread is
///   already attached, and you just need a handle to the existing top frame.
///
/// (and you can access a [`JavaVM`] via [`JavaVM::singleton`]).
///
/// These APIs are safe because they constrain the new [`Env`] lifetime to the
/// scope of the call, so any references created cannot leak.
///
/// ## Runtime Top Frame Checks
///
/// Rust’s type system ensures that a mutable `Env<'local>` only produces
/// references tied to its frame.
///
/// Normally this crate also guarantees that a `&mut Env` represents the top
/// frame.
///
/// However, because the JVM is global and multiple crates may independently use
/// jni-rs, some valid usage patterns can be misused to create multiple mutable
/// `Env`s in the same scope. For example:
///
/// ```rust,no_run
/// # use jni::{JavaVM, errors::Result, objects::JString};
/// # fn f(vm: &JavaVM) -> Result<()> {
/// vm.attach_current_thread(|env1| -> Result<()> {
///     vm.attach_current_thread(|env2| -> Result<()> {
///         // env1 is still mutable, but no longer the top frame.
///         // This will panic at runtime:
///         JString::from_str(env1, "not valid here").unwrap();
///         Ok(())
///     })
/// })
/// # ; Ok(())
/// # }
/// ```
///
/// To prevent unsoundness, this crate enforces a **runtime check**:
///
/// * Creating a new local reference with a mutable `Env` that is *not* the top
///   frame will cause a panic.
///
/// ### Rule of thumb
///
/// * Keep only one `Env` in scope at a time.
/// * Always shadow outer `env` variables when introducing a new one.
///
/// As long as you follow this pattern, the type system will ensure that local
/// references are only created from the valid top frame.
///
/// ## Troubleshooting `cannot borrow as mutable`
///
/// If a function takes two or more parameters, one of them is `Env`, and
/// another is something returned by a `Env` method (like [`JObject`]), then
/// calls to that function may not compile:
///
/// ```rust,compile_fail
/// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
/// #
/// # fn f(env: &mut Env) -> Result<()> {
/// fn example_function(
///     env: &mut Env,
///     obj: &JObject,
/// ) {
///     // ...
/// }
///
/// example_function(
///     env,
///     // ERROR: cannot borrow `*env` as mutable more than once at a time
///     &env.new_object(
///         jni_str!("com/example/SomeClass"),
///         jni_sig!("()V"),
///         &[],
///     )?,
/// )
/// # ; Ok(())
/// # }
/// ```
///
/// This comes from the compiler needing to borrow `env` as mutable for the call
/// to `new_object`, but also needing to borrow it as mutable for the call to
/// `example_function`.
///
/// In the past we used to recommend here that you could avoid this by
/// reordering the parameters so that `Env` comes last, but in practice this led
/// to very inconsistent APIs where some functions had `Env` first and some had
/// it last, which was confusing.
///
/// Now the recommendation is to just make sure that reference type arguments
/// are derived before the call:
///
/// ```rust,no_run
/// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::JObject};
/// #
/// # fn f(env: &mut Env) -> Result<()> {
/// # fn example_function(
/// #     env: &mut Env,
/// #     obj: &JObject,
/// # ) {
/// #    // ...
/// # }
/// let obj = env.new_object(
///    jni_str!("com/example/SomeClass"),
///    jni_sig!("()V"),
///    &[],
/// )?;
/// example_function(env, &obj)
/// # ; Ok(())
/// # }
/// ```
///
/// Quite often reference type arguments have already been derived in some outer
/// scope and don't involve any JNI method calls so this isn't too bad in
/// practice and it keeps the APIs more consistent / predictable, with `Env`
/// always coming first.
///
/// # Checked and unchecked methods
///
/// Some of the methods come in two versions: checked (e.g. `call_method`) and
/// unchecked (e.g. `call_method_unchecked`). Under the hood, checked methods
/// perform some checks to ensure the validity of provided signatures, names and
/// arguments, and then call the corresponding unchecked method.
///
/// Checked methods are more flexible as they allow passing class names and
/// method/field descriptors as strings and may perform lookups of class objects
/// and method/field ids for you, also performing all the needed precondition
/// checks. However, these lookup operations are expensive, so if you need to
/// call the same method (or access the same field) multiple times, it is
/// [recommended](https://docs.oracle.com/en/java/javase/11/docs/specs/jni/design.html#accessing-fields-and-methods)
/// to cache the instance of the class and the method/field id, e.g.
///   - in loops
///   - when calling the same Java callback repeatedly.
///
/// If you do not cache references to classes and method/field ids, you will
/// *not* benefit from the unchecked methods.
///
/// Calling unchecked methods with invalid arguments and/or invalid class and
/// method descriptors may lead to segmentation fault.
///
/// # Zero-copy `AsRef<JNIStr>` arguments
///
/// The JNI specification for many functions requires that string arguments are
/// passed as NUL terminated, Modified UTF-8 encoded byte arrays.
///
/// Anything that accepts an `AsRef<JNIStr>` can directly accept a `JNIStr`
/// literal like `jni_str!("java/lang/String")` which will be encoded as MUTF-8
/// at compile time.
///
/// See [jni_str!] for more details.
///
/// For non-literal, dynamic strings then you can use [JNIString::new] to encode
/// strings as MUTF-8 at runtime (derefs as [`JNIStr`]).
///
#[derive(Debug)]
pub struct Env<'local> {
    /// A non-null JNIEnv pointer
    pub(crate) raw: *mut sys::JNIEnv,
    /// The current [`jni::AttachGuard`] nesting level, which we assert matches
    /// the top/current nesting level whenever some API will return a new local
    /// reference
    pub(crate) level: usize,
    owns_attachment: bool,
    _lifetime: PhantomData<&'local ()>,
}

impl Drop for Env<'_> {
    fn drop(&mut self) {
        // NOOP - we just implement Drop so that the compiler won't consider
        // Env to be FFI safe.
    }
}

#[cfg(test)]
mod test {
    use crate::Env;
    static_assertions::assert_not_impl_any!(Env: Send);
    static_assertions::assert_not_impl_any!(Env: Sync);
}

impl<'local> Env<'local> {
    /// Returns an `UnsupportedVersion` error if the current JNI version is
    /// lower than the one given.
    ///
    /// Returns `JavaException` if called while there is a pending exception.
    #[allow(unused)]
    fn ensure_version(&self, version: JNIVersion) -> Result<()> {
        if self.version()? < version {
            Err(Error::UnsupportedVersion)
        } else {
            Ok(())
        }
    }

    /// Create a [`Env`] associated with single JNI local reference frame.
    ///
    /// `level` represents the nesting level of the [`AttachGuard`] that owns
    /// this [`Env`]. This can be compared to [JavaVM::thread_attach_guard_level()]
    /// to check that this [`Env`] is associated with the top-most JNI stack frame.
    ///
    /// `owns_attachment` should be true if this [`Env`] is associated with an
    /// attachment that will detach the thread when dropped.
    pub(crate) unsafe fn new(raw: *mut sys::JNIEnv, level: usize, owns_attachment: bool) -> Self {
        let env = Env {
            raw,
            level,
            owns_attachment,
            _lifetime: PhantomData,
        };
        // Assuming that the application doesn't break the safety rules for
        // keeping the `AttachGuard` on the stack, and not re-ordering them,
        // we can assert that we only ever create an `Env` for the top-most
        // guard on the stack.
        env.assert_top();
        env
    }

    /// Runtime check that this [`Env`] represents the top JNI stack frame
    ///
    /// Any lower-level API that returns a new local reference must call this
    /// method to ensure the reference is tied to the correct JNI stack frame.
    ///
    /// All safe APIs that return a new local reference already call this but you
    /// may need to call this in `unsafe` code that uses [`crate::sys`] functions
    /// directly.
    ///
    /// See the safety documentation for [`AttachGuard`] for more details.
    #[inline]
    pub fn assert_top(&self) {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        assert_eq!(
            self.level,
            JavaVM::thread_attach_guard_level(),
            r#" jni runtime check failure: attempt to create a new local reference using an Env<'not_top> that is not associated with the top JNI stack frame.

This implies that there is more than one mutable Env in scope.

This should only be possible by misusing APIs like JavaVM::attach_current_thread or Env::with_local_frame to materialize a mutable Env when there is already one in scope.

To fix this, ensure that you only ever have one mutable Env in scope at a time, which is associated with the top JNI stack frame.

See the jni-rs Env documentation for more details.
"#
        );
    }

    /// Returns true if this [`Env`] is associated with a scoped attachment that
    /// will also detach the thread when it is dropped.
    ///
    /// An owned attachment would come from a call to
    /// [`JavaVM::attach_current_thread_for_scope`] but only if the thread was
    /// not already attached.
    ///
    /// This can be used to recognise when
    /// [`JavaVM::attach_current_thread_for_scope`] really needed to attach the
    /// thread, or if the thread was already attached.
    ///
    /// This is mostly useful for diagnostic purposes. For example the `Drop`
    /// implementation for [`Global`] and [`Weak`] will print a warning if they
    /// are dropped on a thread that is not attached, which is recognised with
    /// this method.
    pub fn owns_attachment(&self) -> bool {
        self.owns_attachment
    }

    /// Get the raw Env pointer
    pub fn get_raw(&self) -> *mut sys::JNIEnv {
        self.raw
    }

    /// Get the JNI version that this [`Env`] supports.
    ///
    /// This can return an error if called while there is a pending JNI exception
    pub fn version(&self) -> Result<JNIVersion> {
        // Safety: GetVersion is 1.1 API that must be valid
        Ok(JNIVersion::from(unsafe {
            jni_call_no_post_check_ex!(self, v1_1, GetVersion)?
        }))
    }

    #[doc(hidden)]
    #[deprecated(since = "0.22.0", note = "Renamed to `version` instead")]
    pub fn get_version(&self) -> Result<JNIVersion> {
        self.version()
    }

    /// Load a class from a buffer of raw class data.
    ///
    /// If `name` is null, the name of the class is inferred from the buffer.
    ///
    /// Note: This requires `&mut` because it returns a new local reference to a class.
    ///
    /// # Safety
    ///
    /// The `buf` pointer must be valid for `buf_len` bytes.
    unsafe fn define_class_impl(
        &mut self,
        name: *const c_char,
        loader: &JClassLoader,
        buf: *const jbyte,
        buf_len: usize,
    ) -> Result<JClass<'local>> {
        if buf_len > jsize::MAX as usize {
            return Err(Error::JniCall(JniError::InvalidArguments));
        }
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        // Safety:
        // - DefineClass is 1.1 API that must be valid
        // - It is valid to potentially pass a `null` `name` to `DefineClass`, since the name can be
        // read from the bytecode.
        // - A null loader corresponds to the bootstrap class loader.
        //
        // Throws:
        // - `ClassFormatError`: if the class data does not specify a valid class.
        // - `ClassCircularityError`: if a class or interface would be its own superclass or
        //   superinterface.
        // - `OutOfMemoryError`: if the system runs out of memory.
        // - `SecurityException`: if the caller attempts to define a class in the `"java"` package
        //   tree.
        unsafe {
            jni_call_with_catch_and_null_check!(
                catch |env| {
                    crate::exceptions::JClassFormatError =>
                        Err(Error::ClassFormatError),
                    crate::exceptions::JClassCircularityError =>
                        Err(Error::ClassCircularityError),
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    crate::exceptions::JSecurityException =>
                        Err(Error::SecurityViolation),
                    else => Err(Error::NullPtr("Unexpected Exception")),
                },
                self,
                v1_1,
                DefineClass,
                name,
                loader.as_raw(),
                buf,
                buf_len as jsize
            )
            .map(|class| JClass::from_raw(self, class))
        }
    }

    /// Load a class from a buffer of raw class data.
    ///
    /// If `name` is `None` then the name of the loaded class will be inferred, otherwise the given
    /// `name` must match the name encoded within the class file data.
    ///
    /// Typically the `loader` should come from [`JClassLoader::get_system_class_loader`] or the
    /// loader from some other application-specific class.
    ///
    /// If `loader` is `null` then the bootstrap class loader will be used, which may not be able to
    /// load dependencies of the defined class.
    ///
    /// Alternatively, call [`Self::define_class_jbyte`] if your data comes from a [`JByteArray`] or
    /// `&[jbyte]`.
    ///
    /// - Returns [`Error::ClassFormatError`] if the class data does not specify a valid class.
    /// - Returns [`Error::ClassCircularityError`] if a class or interface would be its own
    ///   superclass or superinterface.
    /// - Returns [`Error::JniCall`] + [`JniError::NoMemory`] if the JVM reports running out of
    ///   memory.
    /// - Returns [`Error::SecurityViolation`] if the caller attempts to define a class in the
    ///   `"java"` package tree.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    ///
    /// # Portability Note
    ///
    /// The JNI `DefineClass` function may not be supported by all JVM implementations, even though
    /// it is part of the JNI specification.
    ///
    /// In particular Android's ART runtime does not support it, since all classes must be
    /// pre-compiled into DEX format.
    pub fn define_class<'any_loader, N, L>(
        &mut self,
        name: Option<N>,
        loader: L,
        buf: &[u8],
    ) -> Result<JClass<'local>>
    where
        N: AsRef<JNIStr>,
        L: AsRef<JClassLoader<'any_loader>>,
    {
        let name: Option<&JNIStr> = name.as_ref().map(|n| n.as_ref());
        let name = name.map_or(ptr::null(), |n| n.as_ptr());
        // Safety: we know the pointer for the u8 slice is valid for buf.len() bytes
        unsafe {
            self.define_class_impl(
                name,
                loader.as_ref(),
                buf.as_ptr() as *const jbyte,
                buf.len(),
            )
        }
    }

    /// Load a class from a buffer of raw class data.
    ///
    /// If `name` is `None` then the name of the loaded class will be inferred, otherwise the given
    /// `name` must match the name encoded within the class file data.
    ///
    /// Typically the `loader` should come from [`JClassLoader::get_system_class_loader`] or the
    /// loader from some other application-specific class.
    ///
    /// If `loader` is `null` then the bootstrap class loader will be used, which may not be able to
    /// load dependencies of the defined class.
    ///
    /// This is the same as [`Self::define_class`] but takes a `&[jbyte]` instead of `&[u8]`.
    ///
    /// - Returns [`Error::ClassFormatError`] if the class data does not specify a valid class.
    /// - Returns [`Error::ClassCircularityError`] if a class or interface would be its own
    ///   superclass or superinterface.
    /// - Returns [`Error::JniCall`] + [`JniError::NoMemory`] if the JVM reports running out of
    ///   memory.
    /// - Returns [`Error::SecurityViolation`] if the caller attempts to define a class in the
    ///   `"java"` package tree.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    ///
    /// # Portability Note
    ///
    /// The JNI `DefineClass` function may not be supported by all JVM implementations, even though
    /// it is part of the JNI specification.
    ///
    /// In particular Android's ART runtime does not support it, since all classes must be
    /// pre-compiled into DEX format.
    pub fn define_class_jbyte<'any_loader, N, L>(
        &mut self,
        name: Option<N>,
        loader: L,
        buf: &[jbyte],
    ) -> Result<JClass<'local>>
    where
        N: AsRef<JNIStr>,
        L: AsRef<JClassLoader<'any_loader>>,
    {
        let name: Option<&JNIStr> = name.as_ref().map(|n| n.as_ref());
        let name = name.as_ref().map_or(ptr::null(), |n| n.as_ptr());
        // Safety: we know the pointer for the u8 slice is valid for buf.len() bytes
        unsafe {
            self.define_class_impl(
                name,
                loader.as_ref(),
                buf.as_ptr() as *const jbyte,
                buf.len(),
            )
        }
    }

    /// Look up a class by its fully-qualified "internal" JNI name, via JNI
    /// `FindClass`.
    ///
    /// **Note:** Only use this method if you _strictly_ need the JNI
    /// `FindClass` behavior exactly, otherwise, it is strongly recommended to
    /// use [Reference::lookup_class] (cached), [Env::load_class] or
    /// [LoaderContext::load_class].
    ///
    /// The `name` must be in the "internal" form, with slashes instead of dots,
    /// like `jni_str!("java/lang/IllegalArgumentException")` or an array
    /// descriptor like `jni_str!("[Ljava/lang/Number;")`.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_str, errors::Result, Env, objects::JClass};
    /// #
    /// # fn example<'local>(env: &mut Env<'local>) -> Result<()> {
    /// let class: JClass<'local> = env.find_class(jni_str!("java/lang/IllegalArgumentException"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// - Returns a reference to loaded class on success
    /// - Returns [`Error::NoClassDefFound`] if a `NoClassDefFoundError`
    ///   exception occurs, indicating that no definition for a requested class
    ///   or interface can be found (most common failure)
    /// - Returns [`Error::ClassFormatError`] if a `ClassFormatError` exception
    ///   occurs, indicating that the class data does not specify a valid class.
    /// - Returns [`Error::ClassCircularityError`] if a `ClassCircularityError`
    ///   exception occurs, indicating that a class or interface would be its
    ///   own superclass or superinterface.
    /// - Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an
    ///   `OutOfMemoryError` exception occurs, indicating that the JVM reports
    ///   running out of memory.
    /// - Returns [`Error::NullPtr`] for any other unknown error condition
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending
    /// exception).
    ///
    /// # Android
    ///
    /// On Android in particular it's notable that `FindClass` will not be able
    /// to find application classes when called from a native thread, outside of
    /// a native method call.
    ///
    /// [Env::load_class] can work on Android if a loader has been set up for
    /// the current thread (see: [JThread::set_context_class_loader]) and if you
    /// use the `android-activity` crate this may be done for you.
    pub fn find_class<S>(&mut self, name: S) -> Result<JClass<'local>>
    where
        S: AsRef<JNIStr>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let name = name.as_ref();
        // Safety:
        // FindClass is 1.1 API that must be valid
        // name is non-null
        unsafe {
            jni_call_with_catch_and_null_check!(
                catch |env| {
                    e: crate::exceptions::JNoClassDefFoundError => {
                        let cause: &JThrowable = &e.as_throwable();
                        let cause = env.new_global_ref(cause).ok();
                        Err(Error::NoClassDefFound {
                            requested: name.to_string(),
                            cause
                        })
                    },
                    // Although the JNI spec does not list `ClassNotFoundException` as a possible
                    // exception for `FindClass` we want to make sure that would be mapped to
                    // Error:NoClassDefFound, consistent with `LoaderContext::load_class`
                    e: crate::exceptions::JClassNotFoundException => {
                        let cause: &JThrowable = &e.as_throwable();
                        let cause = env.new_global_ref(cause).ok();
                        Err(Error::NoClassDefFound {
                            requested: name.to_string(),
                            cause
                        })
                    },
                    crate::exceptions::JClassFormatError =>
                        Err(Error::ClassFormatError),
                    crate::exceptions::JClassCircularityError =>
                        Err(Error::ClassCircularityError),

                    // Although the JNI spec doesn't say that `FindClass` may throw `LinkageErrors`,
                    // we want to make sure we would map them to `Error::LinkageError`, consistent
                    // with `LoaderContext::load_class`
                    e: crate::exceptions::JLinkageError => {
                        let cause: &JThrowable = &e.as_throwable();
                        let cause = env.new_global_ref(cause).ok();
                        Err(Error::LinkageError {
                            requested: name.to_string(),
                            cause
                        })
                    },
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::NullPtr("Unexpected exception")),
                },
                self, v1_1, FindClass, name.as_ptr())
            .map(|class| JClass::from_raw(self, class))
        }
    }

    /// Look up a class by its fully-qualified name, via `Class.forName(<name>,
    /// <initialize>, <loader>)` with a `FindClass` fallback.
    ///
    /// This is a convenience wrapper around [LoaderContext::load_class] with
    /// [LoaderContext::None].
    ///
    /// `name` should be a binary name like `jni_str!("java.lang.Number")` or an
    /// array descriptor like `jni_str!("[Ljava.lang.Number;")`.
    ///
    /// **Note:** that unlike [Env::find_class], the name uses **dots instead of
    /// slashes** and should conform to the format that `Class.getName()`
    /// returns and that `Class.forName()` expects.
    ///
    /// For example, lookup the class for
    /// `java.lang.IllegalArgumentException`like this:
    /// ```rust,no_run
    /// # use jni::{jni_str, errors::Result, Env, objects::JClass};
    /// #
    /// # fn example<'local>(env: &mut Env<'local>) -> Result<()> {
    /// let class: JClass<'local> = env.load_class(jni_str!("java.lang.IllegalArgumentException"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// - Returns a reference to loaded class on success
    /// - Returns [`Error::NoClassDefFound`] if a `NoClassDefFoundError` or
    ///   `ClassNotFoundException` exception occurs, indicating that no
    ///   definition for a requested class or interface can be found (most
    ///   common failure)
    /// - Returns [`Error::ClassFormatError`] if a `ClassFormatError` exception
    ///   occurs, indicating that the class data does not specify a valid class.
    /// - Returns [`Error::ClassCircularityError`] if a `ClassCircularityError`
    ///   exception occurs, indicating that a class or interface would be its
    ///   own superclass or superinterface.
    /// - May Return [`Error::LinkageError`] for various failures to link the
    ///   requested class.
    /// - May Return [`Error::ExceptionInInitializer`] if an exception occurs
    ///   during class initialization.
    /// - Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an
    ///   `OutOfMemoryError` exception occurs, indicating that the JVM reports
    ///   running out of memory.
    /// - Returns [`Error::NullPtr`] for any other unknown error condition
    ///
    /// *Note:* In order for this API to be consistent with
    /// [`jni::Env::find_class`], this API does _not_ directly return
    /// [`Error::ClassNotFound`] errors that may come directly from the loader.
    ///
    /// *Note:* Any [`Error::NoClassDefFound`], [`Error::LinkageError`] or
    /// [`Error::ExceptionInInitializer`] errors include the original `cause`
    /// exception if more details are needed.
    ///
    /// # Android
    ///
    /// This API will notably check the context class loader of the current
    /// thread, which makes it possible to associate a thread with an
    /// application class loader.
    ///
    /// This may be particularly useful for native applications on Android
    /// because native threads will not normally be able to find application
    /// classes through `FindClass`.
    ///
    /// If you use the `android-activity` crate, it may set up the context class
    /// loader for you.
    pub fn load_class<S>(&mut self, name: S) -> Result<JClass<'local>>
    where
        S: AsRef<JNIStr>,
    {
        let name = name.as_ref();
        LoaderContext::None.load_class(self, name, false)
    }

    /// Returns the superclass for a particular class. Returns None for `java.lang.Object` or
    /// an interface. As with [Self::find_class], takes a descriptor
    ///
    /// # Errors
    ///
    /// If a JNI call fails
    pub fn get_superclass<'other_local, T>(&mut self, class: T) -> Result<Option<JClass<'local>>>
    where
        T: Desc<'local, JClass<'other_local>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let class = class.lookup(self)?;
        let superclass = unsafe {
            JClass::from_raw(
                self,
                jni_call_no_post_check_ex!(self, v1_1, GetSuperclass, class.as_ref().as_raw())?,
            )
        };

        Ok((!superclass.is_null()).then_some(superclass))
    }

    // Like is_assignable_from but it doesn't need a mutable Env reference because it doesn't do any
    // descriptor lookups.
    fn is_assignable_from_class(&self, class1: &JClass, class2: &JClass) -> Result<bool> {
        let class1 = null_check!(class1, "is_assignable_from class1")?;
        let class2 = null_check!(class2, "is_assignable_from class2")?;

        // Safety:
        // - IsAssignableFrom is 1.1 API that must be valid
        // - We make sure class1 and class2 can't be null
        unsafe {
            Ok(jni_call_no_post_check_ex!(
                self,
                v1_1,
                IsAssignableFrom,
                class1.as_raw(), // MUST not be null
                class2.as_raw()  // MUST not be null
            )?)
        }
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Tests whether class1 is assignable from class2.
    pub fn is_assignable_from<'other_local_1, 'other_local_2, T, U>(
        &mut self,
        class1: T,
        class2: U,
    ) -> Result<bool>
    where
        T: Desc<'local, JClass<'other_local_1>>,
        U: Desc<'local, JClass<'other_local_2>>,
    {
        let class1 = class1.lookup(self)?;
        let class1 = null_check!(class1.as_ref(), "is_assignable_from class1")?;
        let class2 = class2.lookup(self)?;
        let class2 = null_check!(class2.as_ref(), "is_assignable_from class2")?;

        self.is_assignable_from_class(class1, class2)
    }

    /// Checks if an object can be cast to a specific reference type.
    pub(crate) fn is_instance_of_cast_type<To: Reference>(&self, obj: &JObject) -> Result<bool> {
        let class = match To::lookup_class(self, &LoaderContext::FromObject(obj)) {
            Ok(class) => class,
            Err(Error::ClassNotFound { name: _ }) => return Ok(false),
            Err(e) => return Err(e),
        };

        let class: &JClass = &class;
        self.is_instance_of_class(obj, class)
    }

    // An internal helper that implements is_instance_of except it doesn't take a
    // Desc for the class and doesn't need a mutable Env reference since it never
    // needs to allocate a new local reference.
    /// Returns true if the object reference can be cast to the given type.
    pub(crate) fn is_instance_of_class<'other_local_1, 'other_local_2, O, C>(
        &self,
        object: O,
        class: C,
    ) -> Result<bool>
    where
        O: AsRef<JObject<'other_local_1>>,
        C: AsRef<JClass<'other_local_2>>,
    {
        let class = null_check!(class.as_ref(), "is_instance_of class")?;

        // Safety:
        // - IsInstanceOf is 1.1 API that must be valid
        // - We make sure class can't be null
        unsafe {
            Ok(jni_call_no_post_check_ex!(
                self,
                v1_1,
                IsInstanceOf,
                object.as_ref().as_raw(), // may be null
                class.as_raw()            // MUST not be null
            )?)
        }
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Returns true if the object reference can be cast to the given type.
    ///
    /// _NB: Unlike the operator `instanceof`, function `IsInstanceOf` *returns `true`*
    /// for all classes *if `object` is `null`.*_
    ///
    /// See [JNI documentation](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#IsInstanceOf)
    /// for details.
    pub fn is_instance_of<'other_local_1, 'other_local_2, O, T>(
        &mut self,
        object: O,
        class: T,
    ) -> Result<bool>
    where
        O: AsRef<JObject<'other_local_1>>,
        T: Desc<'local, JClass<'other_local_2>>,
    {
        let class = class.lookup(self)?;
        self.is_instance_of_class(object, class)
    }

    /// Returns true if ref1 and ref2 refer to the same Java object, or are both `NULL`. Otherwise,
    /// returns false.
    ///
    /// Returns [`Error::JavaException`] if called while there is a pending Java exception
    pub fn is_same_object<'other_local_1, 'other_local_2, O, T>(
        &self,
        ref1: O,
        ref2: T,
    ) -> Result<bool>
    where
        O: AsRef<JObject<'other_local_1>>,
        T: AsRef<JObject<'other_local_2>>,
    {
        // Safety:
        // - IsSameObject is 1.1 API that must be valid
        // - the spec allows either object reference to be `null`
        unsafe {
            jni_call_no_post_check_ex!(
                self,
                v1_1,
                IsSameObject,
                ref1.as_ref().as_raw(), // may be null
                ref2.as_ref().as_raw()  // may be null
            )
        }
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JThrowable>::lookup`)
    //
    /// Raise an exception from an existing object. This will continue being thrown in java unless
    /// `exception_clear` is called.
    ///
    /// Returns [`Error::JavaException`] after throwing the exception.
    ///
    /// The '?' operator should typically used to ensure that the new pending exception is reported
    /// as an error to calling Rust code, allowing the stack to unwind until something decides to
    /// catch the exception (or it propagates back to the JVM).
    ///
    /// *Note:* that once there is a pending exception then most JNI calls (that are not exception
    /// safe) will return [`Error::JavaException`] until the exception is cleared.
    ///
    /// # Examples
    /// ```rust,no_run
    /// # use jni::{jni_str, errors::Result, Env};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// env.throw((jni_str!("java/lang/Exception"), jni_str!("something bad happened")))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Defaulting to "java/lang/Exception":
    ///
    /// ```rust,no_run
    /// # use jni::{jni_str, errors::Result, Env};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// env.throw(jni_str!("something bad happened"))?;
    /// # Ok(())
    /// # }
    /// ```
    pub fn throw<'other_local, E>(&mut self, obj: E) -> Result<()>
    where
        E: Desc<'local, JThrowable<'other_local>>,
    {
        let throwable = obj.lookup(self)?;

        // Safety:
        // Throw is 1.1 API that must be valid
        //
        // We are careful to ensure that we don't drop the reference
        // to `throwable` after converting to a raw pointer.
        let res: i32 =
            unsafe { jni_call_no_post_check_ex!(self, v1_1, Throw, throwable.as_ref().as_raw())? };

        // Ensure that `throwable` isn't dropped before the JNI call returns.
        drop(throwable);

        if res == 0 {
            Err(Error::JavaException)
        } else {
            Err(Error::ThrowFailed(res))
        }
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    fn throw_new_optional(&self, class: &JClass, msg: Option<&JNIStr>) -> Result<()> {
        let throwable_class = JThrowable::lookup_class(self, &LoaderContext::None)?;
        let throwable_class: &JClass = &throwable_class;

        if !self.is_assignable_from_class(class.as_ref(), throwable_class)? {
            return Err(Error::WrongObjectType);
        }
        let msg = msg.as_ref().map(|m| m.as_ref());

        // Safety:
        // ThrowNew is 1.1 API that must be valid
        //
        // We are careful to ensure that we don't drop the reference
        // to `class` or `msg` after converting to raw pointers.
        let res: i32 = unsafe {
            jni_call_no_post_check_ex!(
                self,
                v1_1,
                ThrowNew,
                class.as_raw(),
                msg.map(|m| m.as_ptr()).unwrap_or(std::ptr::null())
            )?
        };

        if res == 0 {
            Err(Error::JavaException)
        } else {
            Err(Error::ThrowFailed(res))
        }
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Create and throw a new exception from a class descriptor and an error message.
    ///
    /// The '?' operator should typically used to ensure that the new pending exception is reported
    /// as an error to calling Rust code, allowing the stack to unwind until something decides to
    /// catch the exception (or it propagates back to the JVM).
    ///
    /// *Note:* that once there is a pending exception then most JNI calls (that are not exception
    /// safe) will return [`Error::JavaException`] until the exception is cleared.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_str, errors::Result, Env};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// env.throw_new(jni_str!("java/lang/Exception"), jni_str!("something bad happened"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Alternatively, see [Env::throw_new_void] if you want to construct an exception with no
    /// message argument.
    pub fn throw_new<'other_local, S, T>(&mut self, class: T, msg: S) -> Result<()>
    where
        S: AsRef<JNIStr>,
        T: Desc<'local, JClass<'other_local>>,
    {
        let class = class.lookup(self)?;
        let msg: &JNIStr = msg.as_ref();
        self.throw_new_optional(class.as_ref(), Some(msg))
    }

    /// Create and throw a new exception from a class descriptor and no error message.
    ///
    /// The '?' operator should typically used to ensure that the new pending exception is reported
    /// as an error to calling Rust code, allowing the stack to unwind until something decides to
    /// catch the exception (or it propagates back to the JVM).
    ///
    /// *Note:* that once there is a pending exception then most JNI calls (that are not exception
    /// safe) will return [`Error::JavaException`] until the exception is cleared.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_str, errors::Result, Env};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// env.throw_new_void(jni_str!("java/lang/Exception"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// This will expect to find a constructor for the given `class` that takes no arguments.
    ///
    /// Alternatively, see [Env::throw_new] if you want to construct an exception with a message.
    pub fn throw_new_void<'other_local, T>(&mut self, class: T) -> Result<()>
    where
        T: Desc<'local, JClass<'other_local>>,
    {
        let class = class.lookup(self)?;
        self.throw_new_optional(class.as_ref(), None)
    }

    /// Returns true if an exception is currently in the process of being thrown.
    ///
    /// This doesn't need to create any local references
    #[inline]
    pub fn exception_check(&self) -> bool {
        // Safety: ExceptionCheck is 1.2 API, which we check for in `from_raw()`
        unsafe { ex_safe_jni_call_no_post_check_ex!(self, v1_2, ExceptionCheck) }
    }

    /// Check whether or not an exception is currently in the process of being
    /// thrown.
    ///
    /// An exception is in this state from the time it gets thrown and
    /// not caught in a java function until `exception_clear` is called.
    pub fn exception_occurred(&mut self) -> Option<JThrowable<'local>> {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let throwable =
            unsafe { ex_safe_jni_call_no_post_check_ex!(self, v1_1, ExceptionOccurred) };
        if throwable.is_null() {
            None
        } else {
            Some(unsafe { JThrowable::from_raw(self, throwable) })
        }
    }

    /// Print exception information to the console.
    pub fn exception_describe(&self) {
        // Safety: ExceptionDescribe is 1.1 API that must be valid
        unsafe { ex_safe_jni_call_no_post_check_ex!(self, v1_1, ExceptionDescribe) };
    }

    /// Clear an exception in the process of being thrown. If this is never
    /// called, the exception will continue being thrown when control is
    /// returned to java.
    pub fn exception_clear(&self) {
        // Safety: ExceptionClear is 1.1 API that must be valid
        unsafe { ex_safe_jni_call_no_post_check_ex!(self, v1_1, ExceptionClear) };
    }

    /// Catch a pending Java exception and convert it into a Rust
    /// [`Error::CaughtJavaException`].
    ///
    /// If a Java exception is pending, it will be cleared and converted into a
    /// Rust [`Error::CaughtJavaException`].
    pub fn exception_catch(&self) -> Result<()> {
        fn try_get_stack_trace(
            env: &mut jni::Env<'_>,
            throwable: &jni::objects::JThrowable,
        ) -> jni::errors::Result<String> {
            let stack_trace = throwable.get_stack_trace(env)?;
            let len = stack_trace.len(env)?;
            let mut trace = String::new();
            for i in 0..len {
                let element = stack_trace.get_element(env, i)?;
                let element_jstr = element.try_to_string(env)?;
                trace.push_str(&format!("{i}: {element_jstr}\n"));
            }
            Ok(trace)
        }

        if self.exception_check() {
            let result = self.with_local_frame(
                DEFAULT_LOCAL_FRAME_CAPACITY,
                |env| -> jni::errors::Result<jni::errors::Error> {
                    let Some(e) = env.exception_occurred() else {
                        // should only be called after receiving a JavaException Result
                        unreachable!(
                            "ExceptionOccurred returned None after ExceptionCheck returned true"
                        );
                    };
                    env.exception_clear();

                    assert!(!e.is_null(), "ExceptionOccurred returned a null throwable");

                    let e = env.new_global_ref(e)?;
                    // besides null pointer checks, there are no documented errors or exceptions
                    // for GetObjectClass or getName, so this should be infallible
                    let name = env.get_object_class(&e)?.get_name(env)?.to_string();
                    let msg = e.get_message(env)?.to_string();

                    let stack = match try_get_stack_trace(env, &e) {
                        Ok(stack_trace) => stack_trace,
                        Err(err) => format!("none: {err:?}"),
                    };

                    Ok(jni::errors::Error::CaughtJavaException {
                        exception: e,
                        name,
                        msg,
                        stack,
                    })
                },
            );

            match result {
                Ok(exception) => Err(exception),
                Err(err) => Err(jni::errors::Error::CaughtJavaException {
                    exception: Default::default(),
                    name: "UNKNOWN".to_string(),
                    msg: format!("Failed to query JThrowable: {err:?})"),
                    stack: "none".to_string(),
                }),
            }
        } else {
            Ok(())
        }
    }

    /// Abort the JVM with an error message.
    ///
    /// This method is guaranteed not to panic, and only calls `ExceptionClear`
    /// prior to calling [`FatalError`]. The `jni-rs` wrapper function does not
    /// perform any heap allocations (although `FatalError` might perform heap
    /// allocations of its own).
    ///
    /// In exchange for these strong guarantees, this method requires an error
    /// message to already be suitably encoded, as described in the
    /// documentation for the [`JNIStr`] type.
    ///
    /// The simplest way to use this is to convert a string to a [`JNIStr`] via
    /// [`jni::jni_str!`] like so:
    ///
    /// ```no_run
    /// # use jni::{jni_str, Env, strings::JNIStr};
    /// # let env: Env = unimplemented!();
    /// env.fatal_error(jni_str!("Game over!"))
    /// ```
    ///
    /// [`FatalError`]:
    ///     https://docs.oracle.com/en/java/javase/21/docs/specs/jni/functions.html#fatalerror
    /// [`ExceptionClear`]:
    ///     https://docs.oracle.com/en/java/javase/21/docs/specs/jni/functions.html#exceptionclear
    /// [Modified UTF-8]:
    ///     https://docs.oracle.com/en/java/javase/21/docs/specs/jni/types.html#modified-utf-8-strings
    pub fn fatal_error(&self, msg: &JNIStr) -> ! {
        // Since FatalError is not considered to safe to call with a pending exception
        // we have to first clear any pending exception.
        self.exception_clear();

        // Safety: FatalError is 1.1 API that must be valid
        //
        // Very little is specified about the implementation of FatalError but we still
        // currently consider this "safe", similar to how `abort()` is considered safe.
        // It won't give the application an opportunity to clean or save state but the
        // process will be terminated.
        //
        // Although FatalError is not exception safe, we call it as if it is because
        // we have guaranteed that there are no pending exceptions to check for.
        unsafe { ex_safe_jni_call_no_post_check_ex!(self, v1_1, FatalError, msg.as_ptr()) }
    }

    /// Create a new instance of a direct java.nio.ByteBuffer
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{errors::Result, Env};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let buf = vec![0; 1024 * 1024];
    /// let (addr, len) = { // (use buf.into_raw_parts() on nightly)
    ///     let buf = buf.leak();
    ///     (buf.as_mut_ptr(), buf.len())
    /// };
    /// let direct_buffer = unsafe { env.new_direct_byte_buffer(addr, len) }?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Safety
    ///
    /// Expects a valid (non-null) pointer and length
    ///
    /// Caller must ensure the lifetime of `data` extends to all uses of the returned
    /// `ByteBuffer`. The JVM may maintain references to the `ByteBuffer` beyond the lifetime
    /// of this `Env`.
    pub unsafe fn new_direct_byte_buffer(
        &mut self,
        data: *mut u8,
        len: usize,
    ) -> Result<JByteBuffer<'local>> {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let data = null_check!(data, "new_direct_byte_buffer data argument")?;
        // Safety: jni-rs requires JNI >= 1.4
        unsafe {
            let obj = jni_call_with_catch_and_null_check!(
                catch |env| {
                    crate::exceptions::JIllegalArgumentException =>
                        Err(Error::JniCall(JniError::InvalidArguments)),
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::NullPtr("Unexpected Exception")),
                },
                self,
                v1_4,
                NewDirectByteBuffer,
                data as *mut c_void,
                len as jlong
            )?;
            Ok(JByteBuffer::from_raw(self, obj))
        }
    }

    /// Returns the starting address of the memory of the direct
    /// java.nio.ByteBuffer.
    ///
    /// # Safety
    ///
    /// The caller must ensure the lifetime of `buf` extends to all uses of the
    /// returned pointer.
    pub fn get_direct_buffer_address(&self, buf: &JByteBuffer) -> Result<*mut u8> {
        let buf = null_check!(buf, "get_direct_buffer_address argument")?;
        // Safety: jni-rs requires JNI >= 1.4 and this is checked in `from_raw`
        unsafe {
            // GetDirectBufferAddress has no documented exceptions that it can throw
            let ptr =
                jni_call_only_check_null_ret!(self, v1_4, GetDirectBufferAddress, buf.as_raw())?;
            Ok(ptr as _)
        }
    }

    /// Returns the capacity (length) of the direct java.nio.ByteBuffer.
    ///
    /// # Terminology
    ///
    /// "capacity" here means the length that was passed to [`Self::new_direct_byte_buffer()`]
    /// which does not reflect the (potentially) larger size of the underlying allocation (unlike the `Vec`
    /// API).
    ///
    /// The terminology is simply kept from the original JNI API (`GetDirectBufferCapacity`).
    pub fn get_direct_buffer_capacity(&self, buf: &JByteBuffer) -> Result<usize> {
        let buf = null_check!(buf, "get_direct_buffer_capacity argument")?;
        // Safety: jni-rs requires JNI >= 1.4 and this is checked in `from_raw`
        unsafe {
            let capacity =
                jni_call_no_post_check_ex!(self, v1_4, GetDirectBufferCapacity, buf.as_raw())?;
            match capacity {
                -1 => Err(Error::JniCall(JniError::Unknown)),
                _ => Ok(capacity as usize),
            }
        }
    }

    /// Creates a new global reference to the Java object `obj`.
    ///
    /// Global references take more time to create or delete than ordinary
    /// local references do, but have several properties that make them useful
    /// in certain situations. See [`Global`] for more information.
    ///
    /// If you use this API to try and upgrade a [`Weak`] then it may return
    /// [`Error::ObjectFreed`] if the object has been garbage collected.
    pub fn new_global_ref<'any_local, O>(&self, obj: O) -> Result<Global<O::GlobalKind>>
    where
        O: Reference + AsRef<JObject<'any_local>>,
    {
        // Avoid passing null to `NewGlobalRef` so that we can recognise out-of-memory errors
        if obj.is_null() {
            return Ok(Global::null());
        }

        // Safety:
        // - the minimum supported JNI version is 1.4
        // - we can assume that `obj.raw()` is a valid reference
        // - we know there's no other wrapper for the reference passed to from_global_raw
        //   since we have just created it.
        let global_ref = unsafe {
            let global_ref = O::global_kind_from_raw(jni_call_no_post_check_ex!(
                self,
                v1_1,
                NewGlobalRef,
                obj.as_raw()
            )?);
            Global::new(self, global_ref)
        };

        // Per JNI spec, `NewGlobalRef` will return a null pointer if the object was GC'd
        // (which could happen if `obj` is a `Weak`):
        //
        //  > it is recommended that a (strong) local or global reference to the
        //  > underlying object be acquired using one of the JNI functions
        //  > NewLocalRef or NewGlobalRef. These functions will return NULL if
        //  > the object has been freed.
        //
        if global_ref.is_null() {
            // In this case it's ambiguous whether there has been an out-of-memory error or
            // the object has been garbage collected and so we now _explicitly_ check
            // whether the object has been garbage collected.
            if self.is_same_object(obj, JObject::null())? {
                Err(Error::ObjectFreed)
            } else {
                Err(Error::JniCall(JniError::NoMemory))
            }
        } else {
            Ok(global_ref)
        }
    }

    /// Creates a new global reference and casts it to a different type.
    ///
    /// This is a convenience method that combines [`Self::cast_global`] and
    /// [`Self::new_global_ref`].
    ///
    /// It first checks if the object is an instance of the target type, and if so it creates a new
    /// global reference with the target type.
    ///
    /// `obj` can be a local reference or a global reference.
    ///
    /// For upcasting (converting to a more general type), consider using the `AsRef` trait
    /// implementations instead, which don't require runtime checks.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let local_obj: JObject = env.new_object(jni_str!("java/lang/String"), jni_sig!("()V"), &[])?;
    /// let global_string = env.new_cast_global_ref::<JString>(local_obj)?;
    /// // global_string is now a `Global<JString>` that persists beyond local frames
    ///
    /// // For upcasting, the `AsRef` trait is more efficient:
    /// let as_obj_again: &JObject = global_string.as_ref(); // No runtime check needed
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Errors
    ///
    /// Returns [`Error::WrongObjectType`] if the object is not an instance of the target type.
    /// Returns [`Error::ClassNotFound`] if the target class cannot be found.
    pub fn new_cast_global_ref<'any_local, To>(
        &self,
        obj: impl Reference + AsRef<JObject<'any_local>>,
    ) -> Result<Global<To::GlobalKind>>
    where
        To: Reference,
    {
        if obj.is_null() {
            return Ok(Default::default());
        }

        if self.is_instance_of_cast_type::<To>(obj.as_ref())? {
            let new = self.new_global_ref(obj)?;
            // Safety:
            // - we have just checked that `new` is an instance of `To`
            unsafe {
                let cast = To::global_kind_from_raw(new.into_raw());
                Ok(Global::new(self, cast))
            }
        } else {
            Err(Error::WrongObjectType)
        }
    }

    /// Creates an auto-delete [`Global`] reference wrapper around a raw JNI `jobject` pointer.
    ///
    /// This may be useful if a third-party library gives you ownership over a JNI global reference
    /// pointer and you want to manage it safely in Rust.
    ///
    /// If you don't own the global reference you must not use this function and can instead
    /// use [`Env::as_cast_raw`] to temporarily wrap the raw pointer without taking ownership.
    ///
    /// # Safety
    /// - The `raw` pointer must be a valid JNI reference of the appropriate type `T`, or `null`.
    /// - The `raw` pointer must not have any other existing wrappers
    /// - You must logically own the global reference and be able to guarantee that it can not
    ///   be deleted elsewhere while the returned [`Global`] is alive.
    pub unsafe fn global_from_raw<T: Reference>(&self, raw: sys::jobject) -> Global<T::GlobalKind> {
        // Safety: the rules above cover the safety requirements
        unsafe { Global::new(self, T::global_kind_from_raw(raw)) }
    }

    /// Attempts to cast an owned global reference to a different type.
    ///
    /// This performs a runtime type check using `IsInstanceOf` and consumes the input reference.
    ///
    /// For upcasting (converting to a more general type), consider using the `AsRef` trait
    /// implementations instead, which don't require runtime checks.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let local_obj: JObject = env.new_object(jni_str!("java/lang/String"), jni_sig!("()V"), &[])?;
    /// let global_obj: Global<JObject<'static>> = env.new_global_ref(&local_obj)?;
    /// let global_string = env.cast_global::<JString>(global_obj)?;
    ///
    /// // For upcasting, the `AsRef` trait is more efficient:
    /// let as_obj_again: &JObject = global_string.as_ref(); // No runtime check needed
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Errors
    ///
    /// Returns [`Error::WrongObjectType`] if the object is not an instance of the target type.
    /// Returns [`Error::ClassNotFound`] if the target class cannot be found.
    pub fn cast_global<To>(
        &self,
        obj: Global<
            impl Into<JObject<'static>>
            + AsRef<JObject<'static>>
            + Default
            + Reference
            + Send
            + Sync
            + 'static,
        >,
    ) -> Result<Global<To::GlobalKind>>
    where
        To: Reference,
    {
        if obj.is_null() {
            return Ok(Default::default());
        }

        if self.is_instance_of_cast_type::<To>(obj.as_ref())? {
            // Safety:
            // - we have just checked that `obj` is an instance of `T`
            // - there won't be multiple wrappers since we are creating one from the other
            unsafe {
                let cast = To::global_kind_from_raw(obj.into_raw());
                Ok(Global::new(self, cast))
            }
        } else {
            Err(Error::WrongObjectType)
        }
    }

    /// Creates a new weak global reference.
    ///
    /// Weak global references are a special kind of Java object reference that
    /// doesn't prevent the Java object from being garbage collected. See
    /// [`Weak`] for more information.
    ///
    /// If you use this API to create a [`Weak`] from another [`Weak`]
    /// then it may return [`Error::ObjectFreed`] if the object has been garbage
    /// collected.
    ///
    /// Attempting to create a [`Weak`] for a `null` reference will return an
    /// [`Error::ObjectFreed`] error.
    pub fn new_weak_ref<O>(&self, obj: O) -> Result<Weak<O::GlobalKind>>
    where
        O: Reference,
    {
        // Check if the pointer is null *before* calling `NewWeakGlobalRef`.
        //
        // This avoids a bug in some JVM implementations which, contrary to the JNI specification,
        // will throw `java.lang.OutOfMemoryError: C heap space` from `NewWeakGlobalRef` if it is
        // passed a null pointer. (The specification says it will return a null pointer in that
        // situation, not throw an exception.)
        if obj.is_null() {
            return Err(Error::ObjectFreed);
        }

        // Safety:
        // - the minimum supported JNI version is 1.4
        // - we can assume that `obj.raw()` is a valid reference
        // - we know there's no other wrapper for the reference passed to from_global_raw
        //   since we have just created it.
        let weak_ref = unsafe {
            let weak = O::global_kind_from_raw(jni_call_with_catch!(
                catch |env| {
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_2,
                NewWeakGlobalRef,
                obj.as_raw()
            )?);
            Weak::new(self, weak)
        };

        // Unlike for NewLocalRef and NewGlobalRef, the JNI spec doesn't seem to
        // give the same guarantee that it will return null if the object has
        // already been freed, but it seems reasonable to assume it can.

        if weak_ref.is_null() {
            // Unlike for NewLocalRef and NewGlobalRef we can assume that NewWeakGlobalRef
            // will throw an out-of-memory exception (that we catch) instead of returning null
            Err(Error::ObjectFreed)
        } else {
            Ok(weak_ref)
        }
    }

    /// Creates an auto-delete [`Weak`] reference wrapper around a raw JNI `jobject` pointer.
    ///
    /// This may be useful if a third-party library gives you ownership over a JNI weak reference
    /// pointer and you want to manage it safely in Rust.
    ///
    /// If you don't own the weak reference you must not use this function and can instead
    /// use [`Env::as_cast_raw`] to temporarily wrap the raw pointer without taking ownership.
    ///
    /// # Safety
    /// - The `raw` pointer must be a valid JNI reference of the appropriate type `T`, or `null`.
    /// - The `raw` pointer must not have any other existing wrappers
    /// - You must logically own the weak reference and be able to guarantee that it can not
    ///   be deleted elsewhere while the returned [`Weak`] is alive.
    pub unsafe fn weak_from_raw<T: Reference>(&self, raw: sys::jobject) -> Weak<T::GlobalKind> {
        // Safety: the rules above cover the safety requirements
        unsafe { Weak::new(self, T::global_kind_from_raw(raw)) }
    }

    /// Create a new local reference to an object.
    ///
    /// Specifically, this calls the JNI function [`NewLocalRef`], which creates a reference in the
    /// current local reference frame, regardless of whether the original reference belongs to the
    /// same local reference frame, a different one, or is a [global reference][Global]. In Rust
    /// terms, this method accepts a JNI reference with any valid lifetime and produces a clone of
    /// that reference with the lifetime of this `Env`. The returned reference can outlive the
    /// original.
    ///
    /// This method is useful when you have a strong global reference and you can't prevent it from
    /// being dropped before you're finished with it. In that case, you can use this method to
    /// create a new local reference that's guaranteed to remain valid for the duration of the
    /// current local reference frame, regardless of what later happens to the original global
    /// reference.
    ///
    /// # Lifetimes
    ///
    /// `'local` is the lifetime of the local reference frame that this `Env` belongs to. This
    /// method creates a new local reference in that frame, with lifetime `'local`.
    ///
    /// `'other_local` is the lifetime of the original reference's frame. It can be any valid
    /// lifetime, even one that `'local` outlives or vice versa.
    ///
    /// Think of `'local` as meaning `'new` and `'other_local` as meaning `'original`. (It is
    /// unfortunately not possible to actually give these names to the two lifetimes because
    /// `'local` is a parameter to the `Env` type, not a parameter to this method.)
    ///
    /// # Example
    ///
    /// In the following example, the `ExampleError::extract_throwable` method uses
    /// `Env::new_local_ref` to create a new local reference that outlives the original global
    /// reference:
    ///
    /// ```no_run
    /// # use jni::{jni_sig, jni_str, Env, objects::*, strings::*};
    /// # use std::fmt::Display;
    /// #
    /// # type SomeOtherErrorType = Box<dyn Display>;
    /// #
    /// /// An error that may be caused by either a Java exception or something going wrong in Rust
    /// /// code.
    /// enum ExampleError {
    ///     /// This variant represents a Java exception.
    ///     ///
    ///     /// The enclosed `Global` points to a Java object of class `java.lang.Throwable`
    ///     /// (or one of its many subclasses).
    ///     Exception(Global<JObject<'static>>),
    ///
    ///     /// This variant represents an error in Rust code, not a Java exception.
    ///     Other(SomeOtherErrorType),
    /// }
    ///
    /// impl ExampleError {
    ///     /// Consumes this `ExampleError` and produces a `JThrowable`, suitable for throwing
    ///     /// back to Java code.
    ///     ///
    ///     /// If this is an `ExampleError::Exception`, then this extracts the enclosed Java
    ///     /// exception object. Otherwise, a new exception object is created to represent this
    ///     /// error.
    ///     fn extract_throwable<'local>(self, env: &mut Env<'local>) -> jni::errors::Result<JThrowable<'local>> {
    ///         let throwable: JObject = match self {
    ///             ExampleError::Exception(exception) => {
    ///                 // The error was caused by a Java exception.
    ///
    ///                 // Here, `exception` is a `Global` pointing to a Java `Throwable`. It
    ///                 // will be dropped at the end of this `match` arm. We'll use
    ///                 // `new_local_ref` to create a local reference that will outlive the
    ///                 // `Global`.
    ///
    ///                 env.new_local_ref(&exception)?
    ///             }
    ///
    ///             ExampleError::Other(error) => {
    ///                 // The error was caused by something that happened in Rust code. Create a
    ///                 // new `java.lang.Error` to represent it.
    ///
    ///                 let error_string = JString::from_str(env, error.to_string())?;
    ///
    ///                 env.new_object(
    ///                     jni_str!("java/lang/Error"),
    ///                     jni_sig!("(Ljava/lang/String;)V"),
    ///                     &[
    ///                         (&error_string).into(),
    ///                     ],
    ///                 )?
    ///             }
    ///         };
    ///         let throwable = env.cast_local::<JThrowable>(throwable)?;
    ///         Ok(throwable)
    ///     }
    /// }
    /// ```
    ///
    /// [`NewLocalRef`]: https://docs.oracle.com/en/java/javase/11/docs/specs/jni/functions.html#newlocalref
    pub fn new_local_ref<'any_local, O>(&mut self, obj: O) -> Result<O::Kind<'local>>
    where
        O: Reference + AsRef<JObject<'any_local>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();

        //let obj = obj.as_ref();

        // By checking for `null` before calling `NewLocalRef` we can recognise
        // that a `null` returned from `NewLocalRef` is from being out of memory.
        if obj.is_null() {
            return Ok(O::null());
        }

        // Safety:
        // - the minimum supported JNI version is 1.4
        // - we can assume that `obj.raw()` is a valid reference, or null
        // - we know there's no other wrapper for the reference passed to from_local_raw
        //   since we have just created it.
        let local = unsafe {
            O::kind_from_raw(jni_call_no_post_check_ex!(
                self,
                v1_2,
                NewLocalRef,
                obj.as_raw()
            )?)
        };

        // Per JNI spec, `NewLocalRef` will return a null pointer if the object was GC'd
        // (which could happen if `obj` is a `Weak`):
        //
        //  > it is recommended that a (strong) local or global reference to the
        //  > underlying object be acquired using one of the JNI functions
        //  > NewLocalRef or NewGlobalRef. These functions will return NULL if
        //  > the object has been freed.
        //
        if local.is_null() {
            // In this case it's ambiguous whether there has been an out-of-memory error or
            // the object has been garbage collected and so we now _explicitly_ check
            // whether the object has been garbage collected.
            if self.is_same_object(obj, JObject::null())? {
                Err(Error::ObjectFreed)
            } else {
                Err(Error::JniCall(JniError::NoMemory))
            }
        } else {
            Ok(local)
        }
    }

    /// Creates a new local reference and casts it to a different type.
    ///
    /// This is a convenience method that combines [`Self::cast_local`] and [`Self::new_local_ref`].
    ///
    /// This performs a runtime type check using `IsInstanceOf` and then creates a new local
    /// reference.
    ///
    /// `obj` can be a local reference or a global reference.
    ///
    /// For upcasting (converting to a more general type), consider using the `From` trait
    /// implementations instead, which don't require runtime checks.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let local_obj: JObject = env.new_object(jni_str!("java/lang/String"), jni_sig!("()V"), &[])?;
    /// let local_string = env.new_cast_local_ref::<JString>(&local_obj)?;
    /// // local_string is now a JString<'local> in the current frame
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Errors
    ///
    /// Returns [`Error::WrongObjectType`] if the object is not an instance of the target type.
    /// Returns [`Error::ClassNotFound`] if the target class cannot be found.
    pub fn new_cast_local_ref<'any_local, To>(
        &mut self,
        obj: impl Reference + AsRef<JObject<'any_local>>,
    ) -> Result<To::Kind<'local>>
    where
        To: Reference,
    {
        if obj.is_null() {
            return Ok(To::null());
        }

        if self.is_instance_of_cast_type::<To>(obj.as_ref())? {
            let new = self.new_local_ref(obj.as_ref())?;
            // Safety:
            // - we have just checked that `new` is an instance of `To`
            // - as it's a new reference, it's assigned the `'local` Env lifetime
            unsafe { Ok(To::kind_from_raw::<'local>(new.into_raw())) }
        } else {
            Err(Error::WrongObjectType)
        }
    }

    /// Attempts to cast an owned local reference to a different type.
    ///
    /// This performs a runtime type check using `IsInstanceOf` and consumes the input reference.
    /// For upcasting (converting to a more general type), consider using the `From` trait
    /// implementations instead, which don't require runtime checks.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let obj: JObject = env.new_object(jni_str!("java/lang/String"), jni_sig!("()V"), &[])?;
    /// let string: JString = env.cast_local::<JString>(obj)?;
    ///
    /// // For upcasting, From trait is more efficient:
    /// let obj_again: JObject = string.into(); // No runtime check needed
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Errors
    ///
    /// Returns [`Error::WrongObjectType`] if the object is not an instance of the target type.
    /// Returns [`Error::ClassNotFound`] if the target class cannot be found.
    pub fn cast_local<'any_local, To>(
        &self,
        obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>,
    ) -> Result<To::Kind<'any_local>>
    where
        To: Reference,
    {
        if obj.is_null() {
            return Ok(To::null::<'any_local>());
        }

        if self.is_instance_of_cast_type::<To>(obj.as_ref())? {
            let obj: JObject = obj.into();
            // Safety:
            // - we have just checked that `obj` is an instance of `T`
            // - it is associated with the same lifetime that it was created with
            unsafe { Ok(To::kind_from_raw::<'any_local>(obj.into_raw())) }
        } else {
            Err(Error::WrongObjectType)
        }
    }

    /// Attempts to cast a reference (local or global) to a different type
    /// without consuming it.
    ///
    /// This method borrows the input reference and returns a wrapper that
    /// derefs to the target type. The original reference remains valid and can
    /// be used after the cast operation.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let obj: JObject = env.new_object(jni_str!("java/lang/String"), jni_sig!("()V"), &[])?;
    /// let string_ref = env.as_cast::<JString>(&obj)?;
    /// // obj is still valid here
    /// let empty_string_contents = string_ref.to_string();
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Errors
    ///
    /// Returns [`Error::WrongObjectType`] if the object is not an instance of
    /// the target type. Returns [`Error::ClassNotFound`] if the target class
    /// cannot be found.
    pub fn as_cast<'from, 'any, To>(
        &self,
        obj: &'from (impl Reference + AsRef<JObject<'any>>),
    ) -> Result<Cast<'from, 'any, To>>
    where
        To: Reference,
        'any: 'from,
    {
        Cast::new(self, obj)
    }

    /// Cast a reference (local or global) to a different type without consuming
    /// it and without any runtime checks.
    ///
    /// This method borrows the input reference and returns a wrapper that
    /// derefs to the target type. The original reference remains valid and can
    /// be used after the cast operation.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::*};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let obj: JObject = env.new_object(jni_str!("java/lang/String"), jni_sig!("()V"), &[])?;
    /// let string_ref = unsafe { env.as_cast_unchecked::<JString>(&obj) };
    /// // obj is still valid here
    /// let empty_string_contents = string_ref.to_string();
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Safety
    ///
    /// The caller must ensure that `obj` is a valid instance of `To`, or
    /// `null`.
    pub unsafe fn as_cast_unchecked<'from, 'any, To>(
        &self,
        obj: &'from (impl Reference + AsRef<JObject<'any>>),
    ) -> Cast<'from, 'any, To>
    where
        To: Reference,
        'any: 'from,
    {
        // Safety: the caller must ensure that `obj` is a valid instance of `To`, or `null`.
        unsafe { Cast::new_unchecked(obj) }
    }

    /// Attempts to cast a raw [`jobject`] reference without taking ownership.
    ///
    /// This method borrows the input reference and returns a wrapper that
    /// derefs to the target type. The original reference remains valid and can
    /// continue to be used.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{errors::Result, Env, objects::*, sys::jobject};
    /// #
    /// # fn example(env: &mut Env, raw_global: jobject) -> Result<()> {
    /// // SAFETY: we know that raw_global is a valid java.lang.String reference
    /// let string_ref = unsafe { env.as_cast_raw::<Global<JString>>(&raw_global)? };
    /// let string_contents = string_ref.to_string();
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Errors
    ///
    /// Returns [`Error::WrongObjectType`] if the object is not an instance of
    /// the target type. Returns [`Error::ClassNotFound`] if the target class
    /// cannot be found.
    ///
    /// # Safety
    ///
    /// The caller must ensure that `from` is a valid reference (local or
    /// global) - which may be `null`.
    ///
    /// The caller must ensure the `from` reference will not be deleted while
    /// the `Cast` exists.
    ///
    /// Note: even though this API is `unsafe`, it will still do a runtime check
    /// that `from` is a valid instance of `To`, so you are not required to know
    /// this.
    ///
    /// Note: this API is agnostic about whether the reference is local or
    /// global because the returned [`Cast`] wrapper doesn't give ownership over
    /// the reference and so you can't accidentally attempt to delete it using
    /// the wrong JNI API.
    pub unsafe fn as_cast_raw<'from, To>(
        &self,
        from: &'from jobject,
    ) -> Result<Cast<'from, 'from, To>>
    where
        To: Reference,
    {
        // Safety: the rules in the doc comment cover the `from_raw` safety requirements
        unsafe { Cast::from_raw(self, from) }
    }

    /// Creates a new auto-deleted local reference.
    ///
    /// See also [`with_local_frame`](struct.Env.html#method.with_local_frame) method that
    /// can be more convenient when you create a _bounded_ number of local references
    /// but cannot rely on automatic de-allocation (e.g., in case of recursion, deep call stacks,
    /// [permanently-attached](struct.JavaVM.html#attaching-native-threads) native threads, etc.).
    #[deprecated = "Use '.auto()' from IntoAuto trait"]
    pub fn auto_local<O>(&self, obj: O) -> Auto<'local, O>
    where
        O: Into<JObject<'local>>,
    {
        Auto::new(obj)
    }

    /// Deletes a local reference early, before its JNI stack frame unwinds.
    ///
    /// Local references exist within a JNI stack frame, which would typically be created by the
    /// Java VM before making a native method call, and then unwound when your native method
    /// returns.
    ///
    /// New JNI stack frames may also be created via [`Self::with_local_frame`].
    ///
    /// Typically you don't have to worry about deleting local references since they are
    /// automatically freed when the JNI stack frame they were created in unwinds.
    ///
    /// But, each local reference takes memory and so you need to make sure to not excessively
    /// allocate local references.
    ///
    /// If you find that you are allocating a large number of local references in a single native
    /// method call, (e.g. while looping over a large collection), this API can be used to
    /// explicitly delete local references before the JNI stack frame unwinds.
    ///
    /// In most cases it is better to use [`Auto`] (see [`IntoAuto::auto`] method) or
    /// [`Self::with_local_frame`] instead of directly calling [`Self::delete_local_ref`].
    pub fn delete_local_ref<'other_local, O>(&self, obj: O)
    where
        O: Into<JObject<'other_local>>,
    {
        let obj = obj.into();
        let raw = obj.into_raw();

        // Safety:
        // `raw` may be `null`
        // DeleteLocalRef is safe to call while there may be a pending exception
        unsafe {
            ex_safe_jni_call_no_post_check_ex!(self, v1_1, DeleteLocalRef, raw);
        }
    }

    /// Creates a new local reference frame, in which at least a given number
    /// of local references can be created.
    ///
    /// Returns `Err` on failure, with a pending `OutOfMemoryError`.
    ///
    /// Prefer to use
    /// [`with_local_frame`](struct.Env.html#method.with_local_frame)
    /// instead of direct `push_local_frame`/`pop_local_frame` calls.
    ///
    /// # Safety
    ///
    /// The caller must guarantee that there will be a corresponding call to
    /// `pop_local_frame`
    ///
    /// The caller must ensure that a new `AttachGuard` is created before
    /// creating a new local frame and the local frame may only access
    /// a `Env` that is borrowed from this new guard (so that local
    /// references will be tied to lifetime of the new guard)
    unsafe fn push_local_frame(&self, capacity: i32) -> Result<()> {
        // Safety:
        // This method is safe to call in case of pending exceptions (see chapter 2 of the spec)
        // We check for JNI > 1.2 in `from_raw`
        let res =
            unsafe { ex_safe_jni_call_no_post_check_ex!(self, v1_2, PushLocalFrame, capacity) };
        jni_error_code_to_result(res)
    }

    /// Pops off the current local reference frame, frees all the local
    /// references allocated on the current stack frame, except the `result`,
    /// which is returned from this function and remains valid.
    ///
    /// The resulting `JObject` will be `NULL` iff `result` is `NULL`.
    ///
    /// This method allows direct control of local frames, but it can cause
    /// undefined behavior and is therefore unsafe. Prefer
    /// [`Env::with_local_frame`] instead.
    ///
    /// # Safety
    ///
    /// Any local references created after the most recent call to
    /// [`Env::push_local_frame`] (or the underlying JNI function) must not
    /// be used after calling this method.
    ///
    /// The `AttachGuard` created before calling `push_local_frame` must be
    /// dropped after calling `pop_local_frame`.
    unsafe fn pop_local_frame<'frame_local, T: Reference>(
        &self,
        result: T::Kind<'frame_local>,
    ) -> Result<T::Kind<'local>> {
        let result: JObject<'frame_local> = result.into();
        // Safety:
        // This method is safe to call in case of pending exceptions (see chapter 2 of the spec)
        // We check for JNI > 1.2 in `from_raw`
        unsafe {
            let raw =
                ex_safe_jni_call_no_post_check_ex!(self, v1_2, PopLocalFrame, result.into_raw());
            Ok(T::kind_from_raw(raw))
        }
    }

    /// Run a closure with a mutable [`Env`] associated with a new local reference frame.
    ///
    /// `capacity` is the minimum number of local references that can be created in the new frame.
    /// it can be seen as a hint to the JVM to pre-allocate space for that many local references
    /// but if more are created then the JVM will transparently allocate more space as needed.
    ///
    /// If a frame can't be allocated with the requested capacity for local references, returns
    /// `Err` with a pending `OutOfMemoryError`.
    ///
    /// Once this method returns, all references allocated in the frame are freed.
    ///
    /// If you need to return a reference from the frame to the caller then you should either
    /// use [`Self::with_local_frame_returning_local`] or else create a [`Global`] with
    /// [`Self::new_global_ref`].
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See top-level [Env] documentation for rules on limiting yourself to one
    /// [Env] reference per-scope to avoid exposing code to runtime checks for
    /// the top JNI frame (that can panic).
    pub fn with_local_frame<F, T, E>(&self, capacity: usize, f: F) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        let capacity: jni_sys::jint = capacity
            .try_into()
            .map_err(|_| Error::JniCall(JniError::InvalidArguments))?;

        unsafe {
            // Safety: by creating a new AttachGuard we ensure that the attach guard level
            // will be incremented in sync with the creation of a new JNI stack frame
            let mut guard = AttachGuard::from_unowned(self.get_raw());
            let env = guard.borrow_env_mut();
            self.push_local_frame(capacity)?;
            let ret = catch_unwind(AssertUnwindSafe(|| f(env)));
            self.pop_local_frame::<JObject>(JObject::null())?;
            drop(guard);

            match ret {
                Ok(ret) => ret,
                Err(payload) => {
                    resume_unwind(payload);
                }
            }
        }
    }

    /// Run a closure with a mutable [`Env`] associated with a new local reference frame, and return
    /// one local reference to the caller.
    ///
    /// `capacity` is the minimum number of local references that can be created in the new frame.
    /// it can be seen as a hint to the JVM to pre-allocate space for that many local references but
    /// if more are created then the JVM will transparently allocate more space as needed.
    ///
    /// If a frame can't be allocated with the requested capacity for local references, returns
    /// `Err` with a pending `OutOfMemoryError`.
    ///
    /// Once this method returns, all references allocated in the frame are freed, except the one
    /// that the function returns, which remains valid.
    ///
    /// Since the low-level JNI interface has support for passing back a single local reference from
    /// a local frame as special-case optimization, this alternative to `with_local_frame` exposes
    /// that capability to return a local reference without needing to create a temporary
    /// [`Global`].
    pub fn with_local_frame_returning_local<F, T, E>(
        &mut self,
        capacity: usize,
        f: F,
    ) -> std::result::Result<T::Kind<'local>, E>
    where
        F: for<'new_local> FnOnce(
            &mut Env<'new_local>,
        ) -> std::result::Result<T::Kind<'new_local>, E>,
        T: Reference,
        E: From<Error>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();

        let capacity: jni_sys::jint = capacity
            .try_into()
            .map_err(|_| Error::JniCall(JniError::InvalidArguments))?;

        unsafe {
            // Inner scope to ensure drop order: `Result` -> `env` -> `guard`, before we potentially call `resume_unwind`.
            let panic_payload = {
                // Safety: by creating a new AttachGuard we ensure that the attach guard level
                // will be incremented in sync with the creation of a new JNI stack frame
                let mut guard = AttachGuard::from_unowned(self.get_raw());
                let env = guard.borrow_env_mut();

                self.push_local_frame(capacity)?;

                match catch_unwind(AssertUnwindSafe(|| f(env))) {
                    Ok(Ok(obj)) => {
                        let obj = self.pop_local_frame::<T>(obj)?;
                        return Ok(obj);
                    }
                    Ok(Err(err)) => {
                        self.pop_local_frame::<T>(T::null())?;
                        return Err(err);
                    }
                    Err(payload) => {
                        self.pop_local_frame::<T>(T::null())?;
                        payload
                    }
                }
            };

            resume_unwind(panic_payload);
        }
    }

    /// Runs a closure with a mutable [`Env`] associated with the top local reference frame.
    ///
    /// Unlike [`Self::with_local_frame()`], this API does not push a new JNI stack frame and so new
    /// local references created in the closure will be associated with the existing local reference
    /// frame at the top of the stack.
    ///
    /// Most of the time this API should probably be avoided (see [`Self::with_local_frame`]).
    ///
    /// Only use if:
    /// - You're sure your code won't leak local references into the current stack frame.
    /// - OR, you're sure that the leaked references are acceptable because you know when the top
    ///   frame will unwind and release those references.
    ///
    /// This will have a slightly lower overhead than [`Self::with_local_frame()`] (since it doesn't
    /// need to push/pop a JNI stack frame), but the trade off is that you may leak local references
    /// into the top stack frame.
    ///
    /// Keep in mind that deleting local references individually is likely to have a higher cost
    /// than pushing/popping a JNI stack frame, so you should probably only use this API if you're
    /// OK with leaking a small number of local references into the top frame and waiting for it to
    /// unwind.
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See top-level [Env] documentation for rules on limiting yourself to one [Env] reference
    /// per-scope to avoid exposing code to runtime checks for the top JNI frame (that can panic).
    pub fn with_top_local_frame<F, T, E>(&self, f: F) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        unsafe {
            let mut guard = AttachGuard::from_unowned(self.get_raw());
            f(guard.borrow_env_mut())
        }
    }

    /// Allocates a new object from a class descriptor without running a
    /// constructor.
    ///
    /// - Returns [`Error::Instantiation`] if an `InstantiationException`
    ///   occurs, if the class is an interface or an abstract class.
    /// - Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an
    ///   `OutOfMemoryError` exception occurs.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    pub fn alloc_object<'other_local, T>(&mut self, class: T) -> Result<JObject<'local>>
    where
        T: Desc<'local, JClass<'other_local>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let class = class.lookup(self)?;
        let obj = unsafe {
            jni_call_with_catch_and_null_check!(
                catch |env| {
                    crate::exceptions::JInstantiationException =>
                        Err(Error::Instantiation),
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::NullPtr("Unexpected Exception")),
                },
                self, v1_1, AllocObject, class.as_ref().as_raw())?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        Ok(unsafe { JObject::from_raw(self, obj) })
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Look up a method by class descriptor, name, and signature.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::JMethodID};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let method_id: JMethodID =
    ///     env.get_method_id(jni_str!("java/lang/String"), jni_str!("substring"), jni_sig!("(II)Ljava/lang/String;"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Returns a valid method ID on success or [`Error::MethodNotFound`] on failure.
    ///
    /// Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an `OutOfMemoryError` is thrown by
    /// the JVM.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    pub fn get_method_id<'other_local, 'sig, 'sig_args, C, N, S>(
        &mut self,
        class: C,
        name: N,
        sig: S,
    ) -> Result<JMethodID>
    where
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<MethodSignature<'sig, 'sig_args>>,
    {
        let class = class.lookup(self)?;
        let ffi_name = name.as_ref();
        let sig = sig.as_ref();

        let method_id = unsafe {
            jni_call_with_catch!(
                catch |env| {
                    crate::exceptions::JNoSuchMethodError =>
                        Err(Error::MethodNotFound {
                            name: ffi_name.to_string(),
                            sig: sig.sig().to_string(),
                        }),
                    e: crate::exceptions::JExceptionInInitializerError => {
                        let exception = e.get_exception(env);
                        let exception = if let Ok(exception) = exception {
                            env.new_global_ref(exception).ok()
                        } else {
                            None
                        };
                        Err(Error::ExceptionInInitializer { exception })
                    },
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_1,
                GetMethodID,
                class.as_ref().as_raw(),
                ffi_name.as_ptr(),
                sig.as_ref().sig().as_ptr()
            )?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        assert!(
            !method_id.is_null(),
            "Spurious null method ID returned, without a pending exception"
        );

        Ok(unsafe { JMethodID::from_raw(method_id) })
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Look up a static method by class descriptor, name, and signature.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::JStaticMethodID};
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let method_id: JStaticMethodID =
    ///     env.get_static_method_id(jni_str!("java/lang/String"), jni_str!("valueOf"), jni_sig!("(I)Ljava/lang/String;"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Returns a valid method ID on success or [`Error::MethodNotFound`] on failure.
    ///
    /// Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an `OutOfMemoryError` is thrown by
    /// the JVM.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    pub fn get_static_method_id<'other_local, 'sig, 'sig_args, C, N, S>(
        &mut self,
        class: C,
        name: N,
        sig: S,
    ) -> Result<JStaticMethodID>
    where
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<MethodSignature<'sig, 'sig_args>>,
    {
        let class = class.lookup(self)?;
        let ffi_name = name.as_ref();
        let sig = sig.as_ref();

        let method_id = unsafe {
            jni_call_with_catch!(
                catch |env| {
                    crate::exceptions::JNoSuchMethodError =>
                        Err(Error::MethodNotFound {
                            name: ffi_name.to_string(),
                            sig: sig.sig().to_string(),
                        }),
                    e: crate::exceptions::JExceptionInInitializerError => {
                        let exception = e.get_exception(env);
                        let exception = if let Ok(exception) = exception {
                            env.new_global_ref(exception).ok()
                        } else {
                            None
                        };
                        Err(Error::ExceptionInInitializer { exception })
                    },
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_1,
                GetStaticMethodID,
                class.as_ref().as_raw(),
                ffi_name.as_ptr(),
                sig.as_ref().sig().as_ptr()
            )?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        assert!(
            !method_id.is_null(),
            "Spurious null method ID returned, without a pending exception"
        );

        Ok(unsafe { JStaticMethodID::from_raw(method_id) })
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Look up the field ID for a class/name/type combination.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::JFieldID};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let field_id: JFieldID = env.get_field_id(jni_str!("com/my/Class"), jni_str!("intField"), jni_sig!("I"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Returns a valid field ID on success or [`Error::FieldNotFound`] on failure.
    ///
    /// Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an `OutOfMemoryError` is thrown by
    /// the JVM.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    pub fn get_field_id<'other_local, 'sig, C, N, S>(
        &mut self,
        class: C,
        name: N,
        sig: S,
    ) -> Result<JFieldID>
    where
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<FieldSignature<'sig>>,
    {
        let class = class.lookup(self)?;
        let ffi_name = name.as_ref();
        let ffi_sig = sig.as_ref();

        let field_id = unsafe {
            jni_call_with_catch!(
                catch |env| {
                    crate::exceptions::JNoSuchFieldError =>
                        Err(Error::FieldNotFound {
                            name: ffi_name.to_string(),
                            sig: ffi_sig.sig().to_string(),
                        }),
                    e: crate::exceptions::JExceptionInInitializerError => {
                        let exception = e.get_exception(env);
                        let exception = if let Ok(exception) = exception {
                            env.new_global_ref(exception).ok()
                        } else {
                            None
                        };
                        Err(Error::ExceptionInInitializer { exception })
                    },
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_1,
                GetFieldID,
                class.as_ref().as_raw(),
                ffi_name.as_ptr(),
                ffi_sig.sig().as_ptr()
            )?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        assert!(
            !field_id.is_null(),
            "Spurious null field ID returned with no exception"
        );

        Ok(unsafe { JFieldID::from_raw(field_id) })
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Look up the static field ID for a class/name/type combination.
    ///
    /// # Example
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::JStaticFieldID};
    /// #
    /// # fn example(env: &mut Env) -> Result<()> {
    /// let field_id: JStaticFieldID = env.get_static_field_id(jni_str!("com/my/Class"), jni_str!("intField"), jni_sig!("I"))?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Returns a valid field ID on success or [`Error::FieldNotFound`] on failure.
    ///
    /// Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an `OutOfMemoryError` is thrown by
    /// the JVM.
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    pub fn get_static_field_id<'other_local, 'sig, T, U, V>(
        &mut self,
        class: T,
        name: U,
        sig: V,
    ) -> Result<JStaticFieldID>
    where
        T: Desc<'local, JClass<'other_local>>,
        U: AsRef<JNIStr>,
        V: AsRef<FieldSignature<'sig>>,
    {
        let class = class.lookup(self)?;
        let ffi_name = name.as_ref();
        let ffi_sig = sig.as_ref();

        let field_id = unsafe {
            jni_call_with_catch!(
                catch |env| {
                    crate::exceptions::JNoSuchFieldError =>
                        Err(Error::FieldNotFound {
                            name: ffi_name.to_string(),
                            sig: ffi_sig.sig().to_string(),
                        }),
                    e: crate::exceptions::JExceptionInInitializerError => {
                        let exception = e.get_exception(env);
                        let exception = if let Ok(exception) = exception {
                            env.new_global_ref(exception).ok()
                        } else {
                            None
                        };
                        Err(Error::ExceptionInInitializer { exception })
                    },
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_1,
                GetStaticFieldID,
                class.as_ref().as_raw(),
                ffi_name.as_ptr(),
                ffi_sig.sig().as_ptr()
            )?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        assert!(
            !field_id.is_null(),
            "Spurious null field ID returned with no exception"
        );

        Ok(unsafe { JStaticFieldID::from_raw(field_id) })
    }

    /// Get the class for an object.
    pub fn get_object_class<'other_local, O>(&mut self, obj: O) -> Result<JClass<'local>>
    where
        O: AsRef<JObject<'other_local>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let obj = obj.as_ref();
        let obj = null_check!(obj, "get_object_class")?;
        unsafe {
            Ok(JClass::from_raw(
                self,
                jni_call_no_post_check_ex!(self, v1_1, GetObjectClass, obj.as_raw())?,
            ))
        }
    }

    /// Call a static method in an unsafe manner. This does nothing to check
    /// whether the method is valid to call on the class, whether the return
    /// type is correct, or whether the number of args is valid for the method.
    ///
    /// Under the hood, this simply calls the `CallStatic<Type>MethodA` method
    /// with the provided arguments.
    ///
    /// # Safety
    ///
    /// The provided JMethodID must be valid, and match the types and number of arguments, and return type.
    /// If these are incorrect, the JVM may crash. The JMethodID must also match the passed type.
    pub unsafe fn call_static_method_unchecked<'other_local, T, U>(
        &mut self,
        class: T,
        method_id: U,
        ret: ReturnType,
        args: &[jvalue],
    ) -> Result<JValueOwned<'local>>
    where
        T: Desc<'local, JClass<'other_local>>,
        U: Desc<'local, JStaticMethodID>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        use super::signature::Primitive::{
            Boolean, Byte, Char, Double, Float, Int, Long, Short, Void,
        };
        use JavaType::{Array, Object, Primitive};

        let class = class.lookup(self)?;

        let method_id = method_id.lookup(self)?.as_ref().into_raw();

        let class_raw = class.as_ref().as_raw();
        let jni_args = args.as_ptr();

        macro_rules! invoke {
            ($call:ident -> $ret:ty) => {{
                let o: $ret =
                    jni_call_post_check_ex!(self, v1_1, $call, class_raw, method_id, jni_args)?;
                o
            }};
        }

        // Safety: we're sure we're calling the correct JNI method based on `ret` type
        let ret = unsafe {
            match ret {
                Object | Array => {
                    let obj = invoke!(CallStaticObjectMethodA -> jobject);
                    let obj = JObject::from_raw(self, obj);
                    JValueOwned::from(obj)
                }
                Primitive(Boolean) => invoke!(CallStaticBooleanMethodA -> bool).into(),
                Primitive(Char) => invoke!(CallStaticCharMethodA -> u16).into(),
                Primitive(Byte) => invoke!(CallStaticByteMethodA -> i8).into(),
                Primitive(Short) => invoke!(CallStaticShortMethodA -> i16).into(),
                Primitive(Int) => invoke!(CallStaticIntMethodA -> i32).into(),
                Primitive(Long) => invoke!(CallStaticLongMethodA -> i64).into(),
                Primitive(Float) => invoke!(CallStaticFloatMethodA -> f32).into(),
                Primitive(Double) => invoke!(CallStaticDoubleMethodA -> f64).into(),
                Primitive(Void) => {
                    jni_call_post_check_ex!(
                        self,
                        v1_1,
                        CallStaticVoidMethodA,
                        class_raw,
                        method_id,
                        jni_args
                    )?;
                    JValueOwned::Void
                }
            }
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        Ok(ret)
    }

    /// Call an object method in an unsafe manner. This does nothing to check
    /// whether the method is valid to call on the object, whether the return
    /// type is correct, or whether the number of args is valid for the method.
    ///
    /// Under the hood, this simply calls the `Call<Type>MethodA` method with
    /// the provided arguments.
    ///
    /// # Safety
    ///
    /// The provided JMethodID must be valid, and match the types and number of arguments, and return type.
    /// If these are incorrect, the JVM may crash. The JMethodID must also match the passed type.
    pub unsafe fn call_method_unchecked<'other_local, O, T>(
        &mut self,
        obj: O,
        method_id: T,
        ret_ty: ReturnType,
        args: &[jvalue],
    ) -> Result<JValueOwned<'local>>
    where
        O: AsRef<JObject<'other_local>>,
        T: Desc<'local, JMethodID>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        use super::signature::Primitive::{
            Boolean, Byte, Char, Double, Float, Int, Long, Short, Void,
        };
        use JavaType::{Array, Object, Primitive};

        let method_id = method_id.lookup(self)?.as_ref().into_raw();

        let obj = obj.as_ref().as_raw();

        let jni_args = args.as_ptr();

        macro_rules! invoke {
            ($call:ident -> $ret:ty) => {{
                let o: $ret = jni_call_post_check_ex!(self, v1_1, $call, obj, method_id, jni_args)?;
                o
            }};
        }

        // Safety: we're sure we're calling the correct JNI method based on `ret_ty` type
        let ret = unsafe {
            match ret_ty {
                Object | Array => {
                    let obj = invoke!(CallObjectMethodA -> jobject);
                    let obj = JObject::from_raw(self, obj);
                    JValueOwned::from(obj)
                }
                Primitive(Boolean) => invoke!(CallBooleanMethodA -> bool).into(),
                Primitive(Char) => invoke!(CallCharMethodA -> u16).into(),
                Primitive(Byte) => invoke!(CallByteMethodA -> i8).into(),
                Primitive(Short) => invoke!(CallShortMethodA -> i16).into(),
                Primitive(Int) => invoke!(CallIntMethodA -> i32).into(),
                Primitive(Long) => invoke!(CallLongMethodA -> i64).into(),
                Primitive(Float) => invoke!(CallFloatMethodA -> f32).into(),
                Primitive(Double) => invoke!(CallDoubleMethodA -> f64).into(),
                Primitive(Void) => {
                    jni_call_post_check_ex!(self, v1_1, CallVoidMethodA, obj, method_id, jni_args)?;
                    JValueOwned::Void
                }
            }
        };

        Ok(ret)
    }

    /// Call an non-virtual object method in an unsafe manner. This does nothing to check
    /// whether the method is valid to call on the object, whether the return
    /// type is correct, or whether the number of args is valid for the method.
    ///
    /// Under the hood, this simply calls the `CallNonvirtual<Type>MethodA` method with
    /// the provided arguments.
    ///
    /// # Safety
    ///
    /// The provided JClass, JMethodID must be valid, and match the types and number of arguments, and return type.
    /// If these are incorrect, the JVM may crash. The JMethodID must also match the passed type.
    pub unsafe fn call_nonvirtual_method_unchecked<'other_local, O, T, U>(
        &mut self,
        obj: O,
        class: T,
        method_id: U,
        ret_ty: ReturnType,
        args: &[jvalue],
    ) -> Result<JValueOwned<'local>>
    where
        O: AsRef<JObject<'other_local>>,
        T: Desc<'local, JClass<'other_local>>,
        U: Desc<'local, JMethodID>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        use super::signature::Primitive::{
            Boolean, Byte, Char, Double, Float, Int, Long, Short, Void,
        };
        use JavaType::{Array, Object, Primitive};

        let method_id = method_id.lookup(self)?.as_ref().into_raw();
        let class = class.lookup(self)?;

        let obj = obj.as_ref().as_raw();
        let class_raw = class.as_ref().as_raw();

        let jni_args = args.as_ptr();

        macro_rules! invoke {
            ($call:ident -> $ret:ty) => {{
                let o: $ret = jni_call_post_check_ex!(
                    self, v1_1, $call, obj, class_raw, method_id, jni_args
                )?;
                o
            }};
        }

        // Safety: we're sure we're calling the correct JNI method based on `ret_ty` type
        let ret = unsafe {
            match ret_ty {
                Object | Array => {
                    let obj = invoke!(CallNonvirtualObjectMethodA -> jobject);
                    let obj = JObject::from_raw(self, obj);
                    JValueOwned::from(obj)
                }
                Primitive(Boolean) => invoke!(CallNonvirtualBooleanMethodA -> bool).into(),
                Primitive(Char) => invoke!(CallNonvirtualCharMethodA -> u16).into(),
                Primitive(Byte) => invoke!(CallNonvirtualByteMethodA -> i8).into(),
                Primitive(Short) => invoke!(CallNonvirtualShortMethodA -> i16).into(),
                Primitive(Int) => invoke!(CallNonvirtualIntMethodA -> i32).into(),
                Primitive(Long) => invoke!(CallNonvirtualLongMethodA -> i64).into(),
                Primitive(Float) => invoke!(CallNonvirtualFloatMethodA -> f32).into(),
                Primitive(Double) => invoke!(CallNonvirtualDoubleMethodA -> f64).into(),
                Primitive(Void) => {
                    jni_call_post_check_ex!(
                        self,
                        v1_1,
                        CallNonvirtualVoidMethodA,
                        obj,
                        class_raw,
                        method_id,
                        jni_args
                    )?;
                    JValueOwned::Void
                }
            }
        };

        Ok(ret)
    }

    /// Calls an object method safely. This comes with a number of lookups/checks. It
    ///
    /// * Looks up the [JClass] for the given object.
    /// * Looks up the [JMethodID] for the object's class/name/signature combination
    /// * Ensures that the number/types of args matches the signature
    /// * Calls [`Self::call_method_unchecked`] with the verified safe arguments.
    ///
    /// Note: this may cause a Java exception if the arguments are the wrong type, in addition to if the method itself
    /// throws.
    ///
    /// # Example
    ///
    /// ```rust,no_run
    /// # use jni::{jni_sig, jni_str, errors::Result, Env, objects::JObject, JValue};
    /// # fn example<'local>(env: &mut Env<'local>, obj: &JObject<'local>) -> Result<()> {
    /// // Call a method with signature: String concat(String str)
    /// let arg = env.new_string("world")?;
    /// let result = env.call_method(
    ///     obj,
    ///     jni_str!("concat"),
    ///     jni_sig!((str: JString) -> JString),
    ///     &[JValue::Object(&arg)],
    /// )?;
    /// # Ok(())
    /// # }
    /// ```
    /// Note: a traditional, raw JNI signature like `"(Ljava/lang/String;)Ljava/lang/String;"` can also be used with
    /// [`jni_sig!`] but this demonstrates the more ergonomic syntax that the macro allows. See the [`jni_sig!`] macro
    /// documentation for more details and examples.
    pub fn call_method<'other_local, 'sig, 'sig_args, O, N, S>(
        &mut self,
        obj: O,
        name: N,
        sig: S,
        args: &[JValue],
    ) -> Result<JValueOwned<'local>>
    where
        O: AsRef<JObject<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<MethodSignature<'sig, 'sig_args>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let obj = obj.as_ref();
        let obj = null_check!(obj, "call_method obj argument")?;
        let sig = sig.as_ref();

        if sig.args().len() != args.len() {
            return Err(Error::InvalidArgList(sig.into()));
        }

        // check arguments types
        let base_types_match = sig
            .args()
            .iter()
            .zip(args.iter())
            .all(|(exp, act)| match exp {
                JavaType::Primitive(p) => act.primitive_type() == Some(*p),
                JavaType::Object | JavaType::Array => act.primitive_type().is_none(),
            });
        if !base_types_match {
            return Err(Error::InvalidArgList(sig.into()));
        }

        let class = self.get_object_class(obj)?.auto();

        let args: Vec<jvalue> = args.iter().map(|v| v.as_jni()).collect();

        // SAFETY: We've ensured that the Desc trait will be used to dynamically look up a valid
        // method ID for the object's class, method name, and signature (or else it will return an
        // Err). We've also validated the argument counts and types using the same type signature we
        // fetched the original method ID from.
        unsafe { self.call_method_unchecked(obj, (&class, name, sig), sig.ret(), &args) }
    }

    /// Calls a static method safely. This comes with a number of
    /// lookups/checks. It
    ///
    /// * Looks up the [JMethodID] for the class/name/signature combination
    /// * Ensures that the number/types of args matches the signature
    ///   * Cannot check an object's type - but primitive types are matched against each other (including Object)
    /// * Calls [`Self::call_static_method_unchecked`] with the verified safe arguments.
    ///
    /// Note: this may cause a Java exception if the arguments are the wrong
    /// type, in addition to if the method itself throws.
    pub fn call_static_method<'other_local, 'sig, 'sig_args, C, N, S>(
        &mut self,
        class: C,
        name: N,
        sig: S,
        args: &[JValue],
    ) -> Result<JValueOwned<'local>>
    where
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<MethodSignature<'sig, 'sig_args>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let sig = sig.as_ref();
        if sig.args().len() != args.len() {
            return Err(Error::InvalidArgList(sig.into()));
        }

        // check arguments types
        let base_types_match = sig
            .args()
            .iter()
            .zip(args.iter())
            .all(|(exp, act)| match exp {
                JavaType::Primitive(p) => act.primitive_type() == Some(*p),
                JavaType::Object | JavaType::Array => act.primitive_type().is_none(),
            });
        if !base_types_match {
            return Err(Error::InvalidArgList(sig.into()));
        }

        // go ahead and look up the class since we'll need that for the next call.
        let class = class.lookup(self)?;
        let class = class.as_ref();

        let args: Vec<jvalue> = args.iter().map(|v| v.as_jni()).collect();

        // SAFETY: We've obtained the method_id above, so it is valid for this class.
        // We've also validated the argument counts and types using the same type signature
        // we fetched the original method ID from.
        unsafe { self.call_static_method_unchecked(class, (class, name, sig), sig.ret(), &args) }
    }

    /// Calls a non-virtual method safely. This comes with a number of
    /// lookups/checks. It
    ///
    /// * Looks up the [JMethodID] for the class/name/signature combination
    /// * Ensures that the number/types of args matches the signature
    ///   * Cannot check an object's type - but primitive types are matched against each other (including Object)
    /// * Calls [`Self::call_nonvirtual_method_unchecked`] with the verified safe arguments.
    ///
    /// Note: this may cause a Java exception if the arguments are the wrong
    /// type, in addition to if the method itself throws.
    pub fn call_nonvirtual_method<'other_local, 'sig, 'sig_args, O, C, N, S>(
        &mut self,
        obj: O,
        class: C,
        name: N,
        sig: S,
        args: &[JValue],
    ) -> Result<JValueOwned<'local>>
    where
        O: AsRef<JObject<'other_local>>,
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<MethodSignature<'sig, 'sig_args>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let obj = obj.as_ref();
        let obj = null_check!(obj, "call_method obj argument")?;
        let sig = sig.as_ref();
        if sig.args().len() != args.len() {
            return Err(Error::InvalidArgList(sig.into()));
        }

        // check arguments types
        let base_types_match = sig
            .args()
            .iter()
            .zip(args.iter())
            .all(|(exp, act)| match exp {
                JavaType::Primitive(p) => act.primitive_type() == Some(*p),
                JavaType::Object | JavaType::Array => act.primitive_type().is_none(),
            });
        if !base_types_match {
            return Err(Error::InvalidArgList(sig.into()));
        }

        // go ahead and look up the class since we'll need that for the next call.
        let class = class.lookup(self)?;
        let class = class.as_ref();

        let args: Vec<jvalue> = args.iter().map(|v| v.as_jni()).collect();

        // SAFETY: We've obtained the method_id above, so it is valid for this class.
        // We've also validated the argument counts and types using the same type signature
        // we fetched the original method ID from.
        unsafe {
            self.call_nonvirtual_method_unchecked(obj, class, (class, name, sig), sig.ret(), &args)
        }
    }

    /// Create a new object using a constructor. This is done safely using
    /// checks similar to those in `call_static_method`.
    pub fn new_object<'other_local, 'sig, 'sig_args, C, S>(
        &mut self,
        class: C,
        ctor_sig: S,
        ctor_args: &[JValue],
    ) -> Result<JObject<'local>>
    where
        C: Desc<'local, JClass<'other_local>>,
        S: AsRef<MethodSignature<'sig, 'sig_args>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let ctor_sig = ctor_sig.as_ref();

        // check arguments length
        if ctor_sig.args().len() != ctor_args.len() {
            return Err(Error::InvalidArgList(ctor_sig.into()));
        }

        // check arguments types
        let base_types_match = ctor_sig
            .args()
            .iter()
            .zip(ctor_args.iter())
            .all(|(exp, act)| match exp {
                JavaType::Primitive(p) => act.primitive_type() == Some(*p),
                JavaType::Object | JavaType::Array => act.primitive_type().is_none(),
            });
        if !base_types_match {
            return Err(Error::InvalidArgList(ctor_sig.into()));
        }

        // check return value
        if ctor_sig.ret() != ReturnType::Primitive(Primitive::Void) {
            return Err(Error::InvalidCtorReturn);
        }

        // build strings
        let class = class.lookup(self)?;
        let class = class.as_ref();

        let method_id: JMethodID = Desc::<JMethodID>::lookup((class, ctor_sig), self)?;

        let ctor_args: Vec<jvalue> = ctor_args.iter().map(|v| v.as_jni()).collect();
        // SAFETY: We've obtained the method_id above, so it is valid for this class.
        // We've also validated the argument counts and types using the same type signature
        // we fetched the original method ID from.
        unsafe { self.new_object_unchecked(class, method_id, &ctor_args) }
    }

    /// Create a new object using a constructor. Arguments aren't checked
    /// because of the `JMethodID` usage.
    ///
    /// # Safety
    ///
    /// The provided JMethodID must be valid, and match the types and number of arguments, as well as return type
    /// (always an Object for a constructor). If these are incorrect, the JVM may crash.  The JMethodID must also match
    /// the passed type.
    pub unsafe fn new_object_unchecked<'other_local, C, M>(
        &mut self,
        class: C,
        ctor_id: M,
        ctor_args: &[jvalue],
    ) -> Result<JObject<'local>>
    where
        C: Desc<'local, JClass<'other_local>>,
        M: Desc<'local, JMethodID>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let class = class.lookup(self)?;

        let ctor_id: JMethodID = *ctor_id.lookup(self)?.as_ref();

        let jni_args = ctor_args.as_ptr();

        let obj = unsafe {
            jni_call_post_check_ex_and_null_ret!(
                self,
                v1_1,
                NewObjectA,
                class.as_ref().as_raw(),
                ctor_id.into_raw(),
                jni_args
            )
            .map(|obj| JObject::from_raw(self, obj))
        }?;

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        Ok(obj)
    }

    /// Cast a [JObject] to a [JList].
    ///
    /// Returns `Error::WrongObjectType` if the object is not a `java.util.List`.
    #[deprecated(
        since = "0.22.0",
        note = "use JList::cast_local instead or Env::new_cast_local_ref/cast_local/as_cast_local or Env::new_cast_global_ref/cast_global/as_cast_global"
    )]
    pub fn get_list<'any_local>(
        &mut self,
        obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>,
    ) -> Result<JList<'any_local>> {
        JList::cast_local(self, obj)
    }

    /// Cast a [JObject] to a [JMap].
    ///
    /// Returns `Error::WrongObjectType` if the object is not a `java.util.Map`.
    #[deprecated(
        since = "0.22.0",
        note = "use JMap::cast_local instead or Env::new_cast_local_ref/cast_local/as_cast_local or Env::new_cast_global_ref/cast_global/as_cast_global"
    )]
    pub fn get_map<'any_local>(
        &mut self,
        obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>,
    ) -> Result<JMap<'any_local>> {
        JMap::cast_local(self, obj)
    }

    /// Gets the contents of a Java string, in [modified UTF-8] encoding.
    ///
    /// The returned [MUTF8Chars] can be used to access the modified UTF-8 bytes,
    /// or to convert to a Rust string (which uses standard UTF-8 encoding).
    ///
    /// This entails calling the JNI function `GetStringUTFChars`.
    ///
    /// [modified UTF-8]: https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8
    ///
    /// # Errors
    ///
    /// Returns an error if `obj` is `null`.
    #[deprecated(
        since = "0.22.0",
        note = "use JString::mutf8_chars or JString::to_string instead; this method is redundant and does not perform unsafe operations"
    )]
    pub fn get_string_unchecked<'any_local, StringRef>(
        &mut self,
        obj: StringRef,
    ) -> Result<MUTF8Chars<'any_local, StringRef>>
    where
        StringRef: AsRef<JString<'any_local>> + Reference,
    {
        MUTF8Chars::from_get_string_utf_chars(self, obj)
    }

    /// Gets the contents of a Java string, in [modified UTF-8] encoding.
    ///
    /// The returned [MUTF8Chars] can be used to access the modified UTF-8 bytes,
    /// or to convert to a Rust string (which uses standard UTF-8 encoding).
    ///
    /// This entails calling the JNI function `GetStringUTFChars`.
    ///
    /// [modified UTF-8]: https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8
    ///
    /// # Errors
    ///
    /// Returns an error if `obj` is `null`.
    #[deprecated(
        since = "0.22.0",
        note = "use JString::mutf8_chars or JString::to_string instead"
    )]
    pub fn get_string<'any_local, StringRef>(
        &self,
        obj: StringRef,
    ) -> Result<MUTF8Chars<'any_local, StringRef>>
    where
        StringRef: AsRef<JString<'any_local>> + Reference,
    {
        MUTF8Chars::from_get_string_utf_chars(self, obj)
    }

    /// Create a new java string object from a rust string.
    ///
    /// This requires a re-encoding of rusts UTF-8 strings to JNI's modified UTF-8 format before
    /// calling the JNI function `NewStringUTF`.
    ///
    /// To avoid this intermediate copy + encoding, consider using [`JString::from_jni_str`], and
    /// using the [`crate::jni_str`] macro to encode string literals at compile time.
    pub fn new_string(&mut self, from: impl AsRef<str>) -> Result<JString<'local>> {
        JString::new(self, from)
    }

    /// Get the length of a [`JPrimitiveArray`] or [`JObjectArray`].
    #[deprecated(
        since = "0.22.0",
        note = "use JPrimitiveArray::len or JObjectArray::len instead. This method will be removed in a future version"
    )]
    pub fn get_array_length<'other_local, 'array>(
        &self,
        array: &'array impl AsJArrayRaw<'other_local>,
    ) -> Result<jsize> {
        let array = null_check!(array.as_jarray_raw(), "get_array_length array argument")?;
        let len: jsize = unsafe { jni_call_no_post_check_ex!(self, v1_1, GetArrayLength, array)? };
        Ok(len)
    }

    /// Construct a new array holding objects in class `element_class`.
    ///
    /// All elements are initially set to `initial_element`.
    ///
    /// It's recommended to use [JObjectArray::new] or [`Self::new_object_type_array`]
    /// instead, which both support element type parameterization.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_object_array<'other_local_1, 'other_local_2, T, U>(
        &mut self,
        length: jsize,
        element_class: T,
        initial_element: U,
    ) -> Result<JObjectArray<'local>>
    where
        T: Desc<'local, JClass<'other_local_2>>,
        U: AsRef<JObject<'other_local_1>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let class = element_class.lookup(self)?;

        let array = unsafe {
            jni_call_post_check_ex_and_null_ret!(
                self,
                v1_1,
                NewObjectArray,
                length,
                class.as_ref().as_raw(),
                initial_element.as_ref().as_raw()
            )
            .map(|array| JObjectArray::<JObject>::from_raw(self, array))?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        Ok(array)
    }

    /// Construct a new array holding objects with of type `E`.
    ///
    /// All elements are initially set to `initial_element`.
    ///
    /// It's recommended to use [JObjectArray::new] instead.
    pub fn new_object_type_array<'any_local, E>(
        &mut self,
        length: usize,
        initial_element: impl AsRef<E::Kind<'any_local>>,
    ) -> Result<JObjectArray<'local, E::Kind<'local>>>
    where
        E: Reference,
    {
        JObjectArray::<E>::new(self, length, initial_element)
    }

    /// Returns a local reference to an element of the [`JObjectArray`] `array`.
    #[deprecated(
        since = "0.22.0",
        note = "use JObjectArray::get_element instead. This method will be removed in a future version"
    )]
    pub fn get_object_array_element<'other_local, E: Reference + 'other_local>(
        &mut self,
        array: impl AsRef<JObjectArray<'other_local, E>>,
        index: usize,
    ) -> Result<E::Kind<'local>> {
        array.as_ref().get_element(self, index)
    }

    /// Sets an element of the [`JObjectArray`] `array`.
    #[deprecated(
        since = "0.22.0",
        note = "use JObjectArray::set_element instead. This method will be removed in a future version"
    )]
    pub fn set_object_array_element<'any_local_1, 'any_local_2, E: Reference + 'any_local_1>(
        &self,
        array: impl AsRef<JObjectArray<'any_local_1, E>>,
        index: usize,
        value: impl AsRef<E::Kind<'any_local_2>>,
    ) -> Result<()> {
        array.as_ref().set_element(self, index, value)
    }

    /// Create a new java byte array from a rust byte slice.
    pub fn byte_array_from_slice(&mut self, buf: &[u8]) -> Result<JByteArray<'local>> {
        let bytes = JByteArray::new(self, buf.len())?;
        // SAFETY: i8 and u8 are both plain, single-byte, data types with the same size and
        // alignment.
        //
        // We just want to forward the bitwise representation of the slice and it doesn't matter
        // whether we interpret the data as signed or unsigned.
        let buf: &[i8] = unsafe { std::mem::transmute(buf) };
        bytes.set_region(self, 0, buf)?;
        Ok(bytes)
    }

    /// Converts a java byte array to a rust vector of bytes.
    ///
    /// Returns [Error::IndexOutOfBounds] if one of the calculated indexes in the region is not
    /// valid (in case the array length is changed concurrently).
    ///
    /// This API catches exceptions internally and is not expected to return [`Error::JavaException`]
    pub fn convert_byte_array<'other_local>(
        &self,
        array: impl AsRef<JByteArray<'other_local>>,
    ) -> Result<Vec<u8>> {
        let array = array.as_ref().as_raw();
        let array = null_check!(array, "convert_byte_array array argument")?;

        // GetArrayLength is not documented to throw any exceptions
        let length = unsafe { jni_call_no_post_check_ex!(self, v1_1, GetArrayLength, array)? };
        let mut vec = vec![0u8; length as usize];
        unsafe {
            jni_call_with_catch!(
                catch |env| {
                    crate::exceptions::JArrayIndexOutOfBoundsException =>
                        Err(Error::IndexOutOfBounds),
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_1,
                GetByteArrayRegion,
                array,
                0,
                length,
                vec.as_mut_ptr() as *mut i8
            )?;
        }
        Ok(vec)
    }

    /// Create a new [JBooleanArray] of supplied length.
    ///
    /// It is recommended to use [JBooleanArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_boolean_array(&mut self, length: usize) -> Result<JBooleanArray<'local>> {
        JBooleanArray::new(self, length)
    }

    /// Create a new [JByteArray] of supplied length.
    ///
    /// It is recommended to use [JByteArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_byte_array(&mut self, length: usize) -> Result<JByteArray<'local>> {
        JByteArray::new(self, length)
    }

    /// Create a new [JCharArray] of supplied length.
    ///
    /// It is recommended to use [JCharArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_char_array(&mut self, length: usize) -> Result<JCharArray<'local>> {
        JCharArray::new(self, length)
    }

    /// Create a new [JShortArray] of supplied length.
    ///
    /// It is recommended to use [JShortArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_short_array(&mut self, length: usize) -> Result<JShortArray<'local>> {
        JShortArray::new(self, length)
    }

    /// Create a new [JIntArray] of supplied length.
    ///
    /// It is recommended to use [JIntArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_int_array(&mut self, length: usize) -> Result<JIntArray<'local>> {
        JIntArray::new(self, length)
    }

    /// Create a new [JLongArray] of supplied length.
    ///
    /// It is recommended to use [JLongArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_long_array(&mut self, length: usize) -> Result<JLongArray<'local>> {
        JLongArray::new(self, length)
    }

    /// Create a new [JFloatArray] of supplied length.
    ///
    /// It is recommended to use [JFloatArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_float_array(&mut self, length: usize) -> Result<JFloatArray<'local>> {
        JFloatArray::new(self, length)
    }

    /// Create a new [JDoubleArray] of supplied length.
    ///
    /// It is recommended to use [JDoubleArray::new] instead.
    ///
    /// This API may be deprecated and removed in a future version.
    pub fn new_double_array(&mut self, length: usize) -> Result<JDoubleArray<'local>> {
        JDoubleArray::new(self, length)
    }

    /// Copy elements of the java boolean array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JBooleanArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_boolean_array_region<'other_local>(
        &self,
        array: impl AsRef<JBooleanArray<'other_local>>,
        start: jsize,
        buf: &mut [jboolean],
    ) -> Result<()> {
        unsafe {
            <jboolean as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy elements of the java byte array from the `start` index to the `buf`
    /// slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JByteArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_byte_array_region<'other_local>(
        &self,
        array: impl AsRef<JByteArray<'other_local>>,
        start: jsize,
        buf: &mut [jbyte],
    ) -> Result<()> {
        unsafe { <jbyte as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy elements of the java char array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JCharArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_char_array_region<'other_local>(
        &self,
        array: impl AsRef<JCharArray<'other_local>>,
        start: jsize,
        buf: &mut [jchar],
    ) -> Result<()> {
        unsafe { <jchar as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy elements of the java short array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JShortArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_short_array_region<'other_local>(
        &self,
        array: impl AsRef<JShortArray<'other_local>>,
        start: jsize,
        buf: &mut [jshort],
    ) -> Result<()> {
        unsafe {
            <jshort as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy elements of the java int array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JIntArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_int_array_region<'other_local>(
        &self,
        array: impl AsRef<JIntArray<'other_local>>,
        start: jsize,
        buf: &mut [jint],
    ) -> Result<()> {
        unsafe { <jint as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy elements of the java long array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JLongArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_long_array_region<'other_local>(
        &self,
        array: impl AsRef<JLongArray<'other_local>>,
        start: jsize,
        buf: &mut [jlong],
    ) -> Result<()> {
        unsafe { <jlong as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy elements of the java float array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JFloatArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_float_array_region<'other_local>(
        &self,
        array: impl AsRef<JFloatArray<'other_local>>,
        start: jsize,
        buf: &mut [jfloat],
    ) -> Result<()> {
        unsafe {
            <jfloat as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy elements of the java double array from the `start` index to the
    /// `buf` slice. The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    /// If `start` is negative _or_ `start + buf.len()` is greater than [`array.length`]
    /// then no elements are copied, an `ArrayIndexOutOfBoundsException` is thrown,
    /// and `Err` is returned.
    ///
    /// [`array.length`]: struct.Env.html#method.get_array_length
    #[deprecated(
        since = "0.22.0",
        note = "use JDoubleArray::get_region instead; this method will be removed in a future version"
    )]
    pub fn get_double_array_region<'other_local>(
        &self,
        array: impl AsRef<JDoubleArray<'other_local>>,
        start: jsize,
        buf: &mut [jdouble],
    ) -> Result<()> {
        unsafe {
            <jdouble as TypeArraySealed>::get_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy the contents of the `buf` slice to the java boolean array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JBooleanArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_boolean_array_region<'other_local>(
        &self,
        array: impl AsRef<JBooleanArray<'other_local>>,
        start: jsize,
        buf: &[jboolean],
    ) -> Result<()> {
        unsafe {
            <jboolean as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy the contents of the `buf` slice to the java byte array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JByteArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_byte_array_region<'other_local>(
        &self,
        array: impl AsRef<JByteArray<'other_local>>,
        start: jsize,
        buf: &[jbyte],
    ) -> Result<()> {
        unsafe { <jbyte as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy the contents of the `buf` slice to the java char array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JCharArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_char_array_region<'other_local>(
        &self,
        array: impl AsRef<JCharArray<'other_local>>,
        start: jsize,
        buf: &[jchar],
    ) -> Result<()> {
        unsafe { <jchar as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy the contents of the `buf` slice to the java short array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JShortArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_short_array_region<'other_local>(
        &self,
        array: impl AsRef<JShortArray<'other_local>>,
        start: jsize,
        buf: &[jshort],
    ) -> Result<()> {
        unsafe {
            <jshort as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy the contents of the `buf` slice to the java int array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JIntArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_int_array_region<'other_local>(
        &self,
        array: impl AsRef<JIntArray<'other_local>>,
        start: jsize,
        buf: &[jint],
    ) -> Result<()> {
        unsafe { <jint as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy the contents of the `buf` slice to the java long array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JLongArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_long_array_region<'other_local>(
        &self,
        array: impl AsRef<JLongArray<'other_local>>,
        start: jsize,
        buf: &[jlong],
    ) -> Result<()> {
        unsafe { <jlong as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf) }
    }

    /// Copy the contents of the `buf` slice to the java float array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JFloatArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_float_array_region<'other_local>(
        &self,
        array: impl AsRef<JFloatArray<'other_local>>,
        start: jsize,
        buf: &[jfloat],
    ) -> Result<()> {
        unsafe {
            <jfloat as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Copy the contents of the `buf` slice to the java double array at the
    /// `start` index.
    #[deprecated(
        since = "0.22.0",
        note = "use JDoubleArray::set_region instead; this method will be removed in a future version"
    )]
    pub fn set_double_array_region<'other_local>(
        &self,
        array: impl AsRef<JDoubleArray<'other_local>>,
        start: jsize,
        buf: &[jdouble],
    ) -> Result<()> {
        unsafe {
            <jdouble as TypeArraySealed>::set_region(self, array.as_ref().as_raw(), start, buf)
        }
    }

    /// Convert a [`JMethodID`] into a [`JObject`] with the corresponding
    /// `java.lang.reflect.Method` or `java.lang.reflect.Constructor` instance.
    pub fn to_reflected_method<'other_local>(
        &mut self,
        class: impl Desc<'local, JClass<'other_local>>,
        method_id: impl Desc<'local, JMethodID>,
    ) -> Result<JObject<'local>> {
        // Safety: Rust type safety ensures that method_id is a JMethodID, while is_static is false
        unsafe { self.to_reflected_method_base(class, method_id, JMethodID::into_raw, false) }
    }

    /// Convert a [`JStaticMethodID`] into a [`JObject`] with the corresponding
    /// `java.lang.reflect.Method` instance.
    pub fn to_reflected_static_method<'other_local>(
        &mut self,
        class: impl Desc<'local, JClass<'other_local>>,
        method_id: impl Desc<'local, JStaticMethodID>,
    ) -> Result<JObject<'local>> {
        // Safety: Rust type safety ensures that method_id is a JStaticMethodID, while is_static is true
        unsafe { self.to_reflected_method_base(class, method_id, JStaticMethodID::into_raw, true) }
    }

    /// Convert a [`JMethodID`] or [`JStaticMethodID`] into a [`JObject`] with the
    /// corresponding `java.lang.reflect.Method` or `java.lang.reflect.Constructor`
    /// instance.
    ///
    /// The `to_jmethodid` function is used to convert the method ID type into
    /// a raw [`sys::jmethodID`].
    ///
    /// # Safety
    ///
    /// `is_static` must correctly indicate whether the method ID is for a static method. (The JNI
    /// spec does not define what happens if this is incorrect.)
    #[allow(clippy::wrong_self_convention)]
    unsafe fn to_reflected_method_base<'other_local, M>(
        &mut self,
        class: impl Desc<'local, JClass<'other_local>>,
        method_id: impl Desc<'local, M>,
        to_jmethodid: impl FnOnce(M) -> crate::sys::jmethodID,
        is_static: bool,
    ) -> Result<JObject<'local>>
    where
        M: Copy,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let class = class.lookup(self)?;

        let method_id = to_jmethodid(*method_id.lookup(self)?.as_ref());

        unsafe {
            jni_call_post_check_ex_and_null_ret!(
                self,
                v1_2,
                ToReflectedMethod,
                class.as_ref().as_raw(),
                method_id,
                is_static
            )
            .map(|jobject| JObject::from_raw(self, jobject))
        }
    }

    /// Get a field without checking the provided type against the actual field.
    ///
    /// # Safety
    ///
    /// - The `obj` must not be `null`
    /// - The `field` must be associated with the given `obj` (got from passing the `obj` to [Env::get_field_id])
    /// - The field must have the specified `ty` type.
    pub unsafe fn get_field_unchecked<'other_local, O, F>(
        &mut self,
        obj: O,
        field: F,
        ty: JavaType,
    ) -> Result<JValueOwned<'local>>
    where
        O: AsRef<JObject<'other_local>>,
        F: Desc<'local, JFieldID>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        use super::signature::Primitive::{
            Boolean, Byte, Char, Double, Float, Int, Long, Short, Void,
        };
        use JavaType::{Array, Object, Primitive};

        let obj = obj.as_ref();
        let obj = null_check!(obj, "get_field_typed obj argument")?;

        let field = field.lookup(self)?.as_ref().into_raw();
        let obj = obj.as_raw();

        macro_rules! field {
            ($get_field:ident) => {{
                // Safety: No exceptions are defined for Get*Field and we assume
                // the caller knows that the field is valid
                unsafe {
                    JValueOwned::from(jni_call_no_post_check_ex!(
                        self, v1_1, $get_field, obj, field
                    )?)
                }
            }};
        }

        match ty {
            Object | Array => {
                let obj = unsafe {
                    jni_call_post_check_ex!(self, v1_1, GetObjectField, obj, field)
                        .map(|obj| JObject::from_raw(self, obj))?
                };
                Ok(obj.into())
            }
            Primitive(Char) => Ok(field!(GetCharField)),
            Primitive(Boolean) => Ok(field!(GetBooleanField)),
            Primitive(Short) => Ok(field!(GetShortField)),
            Primitive(Int) => Ok(field!(GetIntField)),
            Primitive(Long) => Ok(field!(GetLongField)),
            Primitive(Float) => Ok(field!(GetFloatField)),
            Primitive(Double) => Ok(field!(GetDoubleField)),
            Primitive(Byte) => Ok(field!(GetByteField)),
            Primitive(Void) => Err(Error::WrongJValueType("void", "see java field")),
        }
    }

    /// Set a field without any type checking.
    ///
    /// # Safety
    ///
    /// - The `obj` must not be `null`
    /// - The `field` must be associated with the given `obj` (got from passing the `obj` to [Env::get_field_id])
    /// - The field type must match the given `value` type.
    pub unsafe fn set_field_unchecked<'other_local, O, F>(
        &mut self,
        obj: O,
        field: F,
        value: JValue,
    ) -> Result<()>
    where
        O: AsRef<JObject<'other_local>>,
        F: Desc<'local, JFieldID>,
    {
        if let JValue::Void = value {
            return Err(Error::WrongJValueType("void", "see java field"));
        }

        let obj = obj.as_ref();

        let field = field.lookup(self)?.as_ref().into_raw();
        let obj = obj.as_raw();

        macro_rules! set_field {
            ($set_field:ident($val:expr)) => {{ unsafe { jni_call_no_post_check_ex!(self, v1_1, $set_field, obj, field, $val) } }};
        }

        match value {
            JValue::Object(o) => set_field!(SetObjectField(o.as_raw()))?,
            JValue::Bool(b) => set_field!(SetBooleanField(b))?,
            JValue::Char(c) => set_field!(SetCharField(c))?,
            JValue::Short(s) => set_field!(SetShortField(s))?,
            JValue::Int(i) => set_field!(SetIntField(i))?,
            JValue::Long(l) => set_field!(SetLongField(l))?,
            JValue::Float(f) => set_field!(SetFloatField(f))?,
            JValue::Double(d) => set_field!(SetDoubleField(d))?,
            JValue::Byte(b) => set_field!(SetByteField(b))?,
            _ => (),
        };

        Ok(())
    }

    /// Get a field. Requires an object class lookup and a field id lookup
    /// internally.
    pub fn get_field<'other_local, 'sig, O, N, S>(
        &mut self,
        obj: O,
        name: N,
        sig: S,
    ) -> Result<JValueOwned<'local>>
    where
        O: AsRef<JObject<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<FieldSignature<'sig>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let obj = obj.as_ref();
        let obj = null_check!(obj, "get_field obj argument")?;
        let class = self.get_object_class(obj)?.auto();

        let sig = sig.as_ref();
        let field_ty = sig.ty();
        let field_id: JFieldID = Desc::<JFieldID>::lookup((&class, name, sig), self)?;

        // Safety: Since we have explicitly looked up the field ID based on the given
        // return type we have already validate that they match
        unsafe { self.get_field_unchecked(obj, field_id, field_ty) }
    }

    /// Set a field. Does the same lookups as `get_field` and ensures that the
    /// type matches the given value.
    pub fn set_field<'other_local, 'sig, O, N, S>(
        &mut self,
        obj: O,
        name: N,
        sig: S,
        value: JValue,
    ) -> Result<()>
    where
        O: AsRef<JObject<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<FieldSignature<'sig>>,
    {
        let obj = obj.as_ref();
        let obj = null_check!(obj, "set_field obj argument")?;
        let sig = sig.as_ref();
        let field_ty = sig.ty();

        if value.java_type() != field_ty {
            return Err(Error::WrongJValueType(value.type_name(), "see java field"));
        }

        let class = self.get_object_class(obj)?.auto();

        // Safety: We have explicitly checked that the field type matches
        // the value type and the field ID is going to be looked up dynamically
        // based on the class, name and signature (so it's safe to use)
        unsafe { self.set_field_unchecked(obj, (&class, name, sig), value) }
    }

    /// Get a static field without checking the provided type against the actual
    /// field.
    ///
    /// # Safety
    ///
    /// - The `class` must not be null
    /// - The `field` must be associated with the given `class` (got from passing the `class` to [Env::get_static_field_id])
    /// - The field must have the specified `ty` type.
    pub unsafe fn get_static_field_unchecked<'other_local, C, F>(
        &mut self,
        class: C,
        field: F,
        ty: JavaType,
    ) -> Result<JValueOwned<'local>>
    where
        C: Desc<'local, JClass<'other_local>>,
        F: Desc<'local, JStaticFieldID>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        use super::signature::Primitive::{
            Boolean, Byte, Char, Double, Float, Int, Long, Short, Void,
        };
        use JavaType::{Array, Object, Primitive};

        let class = class.lookup(self)?;
        let field = field.lookup(self)?;

        macro_rules! field {
            ($get_field:ident) => {{
                unsafe {
                    jni_call_post_check_ex!(
                        self,
                        v1_1,
                        $get_field,
                        class.as_ref().as_raw(),
                        field.as_ref().into_raw()
                    )?
                }
            }};
        }

        let ret = match ty {
            Primitive(Void) => Err(Error::WrongJValueType("void", "see java field")),
            Object | Array => {
                let obj = field!(GetStaticObjectField);
                let obj = unsafe { JObject::from_raw(self, obj) };
                Ok(JValueOwned::from(obj))
            }
            Primitive(Boolean) => Ok(field!(GetStaticBooleanField).into()),
            Primitive(Char) => Ok(field!(GetStaticCharField).into()),
            Primitive(Short) => Ok(field!(GetStaticShortField).into()),
            Primitive(Int) => Ok(field!(GetStaticIntField).into()),
            Primitive(Long) => Ok(field!(GetStaticLongField).into()),
            Primitive(Float) => Ok(field!(GetStaticFloatField).into()),
            Primitive(Double) => Ok(field!(GetStaticDoubleField).into()),
            Primitive(Byte) => Ok(field!(GetStaticByteField).into()),
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        ret
    }

    /// Set a static field. Requires a class lookup and a field id lookup internally.
    ///
    /// # Safety
    ///
    /// - The `class` must not be null
    /// - The `field` must be associated with the given `class` (got from passing the `class` to [Env::get_static_field_id])
    /// - The field type must match the given `value` type.
    pub unsafe fn set_static_field_unchecked<'other_local, C, F>(
        &mut self,
        class: C,
        field: F,
        value: JValue,
    ) -> Result<()>
    where
        C: Desc<'local, JClass<'other_local>>,
        F: Desc<'local, JStaticFieldID>,
    {
        let class = class.lookup(self)?;
        let field = field.lookup(self)?;

        macro_rules! set_field {
            ($set_field:ident($val:expr)) => {{
                unsafe {
                    jni_call_no_post_check_ex!(
                        self,
                        v1_1,
                        $set_field,
                        class.as_ref().as_raw(),
                        field.as_ref().into_raw(),
                        $val
                    )
                }
            }};
        }

        match value {
            JValue::Object(v) => set_field!(SetStaticObjectField(v.as_raw()))?,
            JValue::Byte(v) => set_field!(SetStaticByteField(v))?,
            JValue::Char(v) => set_field!(SetStaticCharField(v))?,
            JValue::Short(v) => set_field!(SetStaticShortField(v))?,
            JValue::Int(v) => set_field!(SetStaticIntField(v))?,
            JValue::Long(v) => set_field!(SetStaticLongField(v))?,
            JValue::Bool(v) => set_field!(SetStaticBooleanField(v))?,
            JValue::Float(v) => set_field!(SetStaticFloatField(v))?,
            JValue::Double(v) => set_field!(SetStaticDoubleField(v))?,
            _ => (),
        }

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        Ok(())
    }

    /// Get a static field. Requires a class lookup and a field id lookup
    /// internally.
    pub fn get_static_field<'other_local, 'sig, C, N, S>(
        &mut self,
        class: C,
        name: N,
        sig: S,
    ) -> Result<JValueOwned<'local>>
    where
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<FieldSignature<'sig>>,
    {
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        self.assert_top();
        let sig = sig.as_ref();
        let field_ty = sig.ty();

        // go ahead and look up the class since we'll need that for the next call.
        let class = class.lookup(self)?;

        // SAFETY: We have verified that `class`, `field`, `sig` and `ty` are valid
        unsafe {
            self.get_static_field_unchecked(class.as_ref(), (class.as_ref(), name, sig), field_ty)
        }
    }

    /// Set a static field. Requires a class lookup and a field id lookup internally.
    pub fn set_static_field<'other_local, 'sig, C, N, S>(
        &mut self,
        class: C,
        name: N,
        sig: S,
        value: JValue,
    ) -> Result<()>
    where
        C: Desc<'local, JClass<'other_local>>,
        N: AsRef<JNIStr>,
        S: AsRef<FieldSignature<'sig>>,
    {
        let sig = sig.as_ref();
        let field_ty = sig.ty();

        if value.java_type() != field_ty {
            return Err(Error::WrongJValueType(value.type_name(), "see java field"));
        }

        let class = class.lookup(self)?;

        // Safety: We have explicitly checked that the field type matches the value type.
        unsafe {
            self.set_static_field_unchecked(class.as_ref(), (class.as_ref(), name, sig), value)
        }
    }

    /// Looks up the field ID for the given field name and takes the monitor
    /// lock on the given object so the field can be updated without racing
    /// with other Java threads
    fn lock_rust_field<'other_local, O, S>(
        &self,
        obj: O,
        field: S,
    ) -> Result<(MonitorGuard<'local>, JFieldID)>
    where
        O: AsRef<JObject<'other_local>>,
        S: AsRef<JNIStr>,
    {
        // Note: although the returned Monitor is associated with a lifetime, this API doesn't need
        // a `&mut self` reference because we don't need to create and return a new local reference
        // (Monitors aren't JNI types that are owned by a JNI stack frame).

        // We also don't assert that `self.level == JavaVM::thread_attach_guard_level()` since the
        // returned monitor is associated with the current thread and there's no reason you can't
        // lock with a reference that's not from the top stack frame.

        // Since `Desc::lookup` may need to create a temporary local reference for the object class
        // (which we don't want to leak), we push a new stack frame that we can get a mutable
        // reference for.

        self.with_local_frame(DEFAULT_LOCAL_FRAME_CAPACITY, |env| {
            let obj = obj.as_ref();
            let class = env.get_object_class(obj)?;
            let field_id: JFieldID =
                Desc::<JFieldID>::lookup((&class, &field, jni_sig!("J")), env)?;
            let guard = self.lock_obj(obj)?;
            Ok((guard, field_id))
        })
    }

    /// Surrenders ownership of a Rust value to Java.
    ///
    /// This requires an object with a `long` field to store the pointer.
    ///
    /// In Java the property may look like:
    /// ```java
    /// private long myRustValueHandle = 0;
    /// ```
    ///
    /// Or, in Kotlin the property may look like:
    /// ```java
    /// private var myRustValueHandle: Long = 0
    /// ```
    ///
    /// _Note that `private` properties are accessible to JNI which may be
    /// preferable to avoid exposing the handles to more code than necessary
    /// (since the handles are usually only meaningful to Rust code)_.
    ///
    /// The Rust value will be implicitly wrapped in a `Box<Mutex<T>>`.
    ///
    /// The Java object will be locked while changing the field value.
    ///
    /// # Safety
    ///
    /// This will lead to undefined behaviour if the specified field
    /// doesn't have a type of `long`.
    ///
    /// It's important to note that using this API will leak memory if
    /// [`Self::take_rust_field`] is never called so that the Rust type may be
    /// dropped.
    ///
    /// One suggestion that may help ensure that a set Rust field will be
    /// cleaned up later is for the Java object to implement `Closeable` and let
    /// people use a `use` block (Kotlin) or `try-with-resources` (Java).
    ///
    /// **DO NOT** make a copy of the handle stored in one of these fields
    /// since that could lead to a use-after-free error if the Rust type is
    /// taken and dropped multiple times from Rust. If you need to copy an
    /// object with one of these fields then the field should be zero
    /// initialized in the copy.
    pub unsafe fn set_rust_field<'other_local, O, S, T>(
        &self,
        obj: O,
        field: S,
        rust_object: T,
    ) -> Result<()>
    where
        O: AsRef<JObject<'other_local>>,
        S: AsRef<JNIStr>,
        T: Send + 'static,
    {
        let (_guard, field_id) = self.lock_rust_field(&obj, &field)?;

        // It's OK that we don't push a new stack frame here since we know we are dealing with a
        // `jlong` field and since we have already looked up the field ID then we also know that
        // get_field_unchecked and set_field_unchecked don't need to create any local references.

        self.with_top_local_frame(|env| {
            // Safety: the requirement that the given field must be a `long` is
            // documented in the 'Safety' section of this function
            unsafe {
                let field_ptr = env
                    .get_field_unchecked(&obj, field_id, ReturnType::Primitive(Primitive::Long))?
                    .j()? as *mut Mutex<T>;
                if !field_ptr.is_null() {
                    return Err(Error::FieldAlreadySet(field.as_ref().to_str().into()));
                }
            }

            let mbox = Box::new(::std::sync::Mutex::new(rust_object));
            let ptr: *mut Mutex<T> = Box::into_raw(mbox);

            // Safety: the requirement that the given field must be a `long` is
            // documented in the 'Safety' section of this function
            unsafe { env.set_field_unchecked(obj, field_id, (ptr as crate::sys::jlong).into()) }
        })
    }

    /// Gets a lock on a Rust value that's been given to a Java object.
    ///
    /// Java still retains ownership and [`Self::take_rust_field`] will still
    /// need to be called at some point.
    ///
    /// The Java object will be locked before reading the field value but the
    /// Java object lock will be released after the Rust `Mutex` lock for the
    /// field value has been taken (i.e the Java object won't be locked once
    /// this function returns).
    ///
    /// # Safety
    ///
    /// This will lead to undefined behaviour if the specified field
    /// doesn't have a type of `long`.
    ///
    /// If the field contains a non-zero value then it is assumed to be a valid
    /// pointer that was set via `set_rust_field` and will lead to undefined
    /// behaviour if that is not true.
    pub unsafe fn get_rust_field<'other_local, O, S, T>(
        &self,
        obj: O,
        field: S,
    ) -> Result<MutexGuard<'local, T>>
    where
        O: AsRef<JObject<'other_local>>,
        S: AsRef<JNIStr>,
        T: Send + 'static,
    {
        let (_guard, field_id) = self.lock_rust_field(&obj, &field)?;

        // Reference Leaks:
        //
        // It's ok that we don't push a new stack frame here since we know we are dealing with a
        // `jlong` field and since we have already looked up the field ID then we also know that
        // get_field_unchecked doesn't need to create any local references.

        self.with_top_local_frame(|env| {
            // Safety: the requirement that the given field must be a `long` is
            // documented in the 'Safety' section of this function
            unsafe {
                let field_ptr = env
                    .get_field_unchecked(obj, field_id, ReturnType::Primitive(Primitive::Long))?
                    .j()? as *mut Mutex<T>;
                null_check!(field_ptr, "rust value from Java")?;
                // dereferencing is safe, because we checked it for null
                Ok((*field_ptr).lock().unwrap())
            }
        })
    }

    /// Take a Rust field back from Java.
    ///
    /// It sets the field to a null pointer to signal that it's empty.
    ///
    /// The Java object will be locked before taking the field value.
    ///
    /// # Safety
    ///
    /// This will lead to undefined behaviour if the specified field
    /// doesn't have a type of `long`.
    ///
    /// If the field contains a non-zero value then it is assumed to be a valid
    /// pointer that was set via `set_rust_field` and will lead to undefined
    /// behaviour if that is not true.
    pub unsafe fn take_rust_field<'other_local, O, S, T>(&self, obj: O, field: S) -> Result<T>
    where
        O: AsRef<JObject<'other_local>>,
        S: AsRef<JNIStr>,
        T: Send + 'static,
    {
        let (_guard, field_id) = self.lock_rust_field(&obj, &field)?;

        // Reference Leaks:
        //
        // It's ok that we don't push a new stack frame here since we know we are dealing with a
        // `jlong` field and since we have already looked up the field ID then we also know that
        // get_field_unchecked doesn't need to create any local references.

        self.with_top_local_frame(|env| {
            // Safety: the requirement that the given field must be a `long` is
            // documented in the 'Safety' section of this function
            let mbox = unsafe {
                let ptr = env
                    .get_field_unchecked(&obj, field_id, ReturnType::Primitive(Primitive::Long))?
                    .j()? as *mut Mutex<T>;

                null_check!(ptr, "rust value from Java")?;
                Box::from_raw(ptr)
            };

            // attempt to acquire the lock. This prevents us from consuming the
            // mutex if there's an outstanding lock. No one else will be able to
            // get a new one as long as we're in the guarded scope.
            drop(mbox.try_lock()?);

            // Safety: the requirement that the given field must be a `long` is
            // documented in the 'Safety' section of this function
            unsafe {
                env.set_field_unchecked(obj, field_id, (0 as sys::jlong).into())?;
            }

            Ok(mbox.into_inner().unwrap())
        })
    }

    /// Lock a Java object. The MonitorGuard that this returns is responsible
    /// for ensuring that it gets unlocked.
    pub fn lock_obj<'other_local, O>(&self, obj: O) -> Result<MonitorGuard<'local>>
    where
        O: AsRef<JObject<'other_local>>,
    {
        // Note: although the returned Monitor is associated with a lifetime, we
        // don't need a `&mut self` reference and we don't assert that
        // `self.level == JavaVM::thread_attach_guard_level()` since the
        // returned monitor is associated with the current thread and is not a
        // local reference.

        let inner = obj.as_ref().as_raw();
        let res = unsafe { jni_call_no_post_check_ex!(self, v1_1, MonitorEnter, inner)? };
        jni_error_code_to_result(res)?;

        Ok(MonitorGuard {
            obj: inner,
            life: Default::default(),
        })
    }

    /// Returns the Java VM interface.
    ///
    /// May return [`Error::JavaException`] if called while there is a pending
    /// exception.
    pub fn get_java_vm(&self) -> Result<JavaVM> {
        // This avoids calling JNI if we already know the VM pointer
        JavaVM::from_env(self)
    }

    /// Ensures that at least a given number of local references can be created
    /// in the current thread.
    pub fn ensure_local_capacity(&self, capacity: usize) -> Result<()> {
        let capacity: jint = capacity
            .try_into()
            .map_err(|_| Error::JniCall(JniError::InvalidArguments))?;
        // Safety:
        // - jni-rs required JNI_VERSION > 1.2
        // - we have ensured capacity is >= 0
        // - EnsureLocalCapacity has no documented exceptions that it throws
        let res = unsafe { jni_call_no_post_check_ex!(self, v1_2, EnsureLocalCapacity, capacity)? };
        jni_error_code_to_result(res)?;
        Ok(())
    }

    // FIXME: this API shouldn't need a `&mut self` reference since it doesn't return a local reference
    // (currently it just needs the `&mut self` for the sake of `Desc<JClass>::lookup`)
    //
    /// Bind function pointers to native methods of class according to method name and signature.
    ///
    /// For details see
    /// [documentation](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#RegisterNatives).
    ///
    /// # Safety
    ///
    /// The caller must ensure that the function signatures in `methods` have a `this` or `class`
    /// parameter as appropriate for instance or static methods, and that the function pointers are
    /// valid and match the signatures.
    ///
    /// Although `NativeMethod` has an `unsafe` constructor that requires the caller to ensure that
    /// the function pointer is valid and matches the signature, this API is still marked as
    /// `unsafe` to reflect the additional risk that the signature matches but the second parameter
    /// is not consistent with whether the method is static or instance.
    ///
    /// - **Static native methods** must have a `class: JClass<'local>` parameter as the second parameter.
    ///
    /// - **Instance native methods** must have a `this: JObject<'local>` or `this: MyType<'local>` parameter as the second parameter.
    ///
    /// # Throws
    ///
    /// - `NoSuchMethodError` - if a method could not be found (based on its name and signature) or
    ///   it was not a native method.
    ///
    /// **Note** that there is no way for the JVM to report errors if the function pointer is invalid or
    /// does not match the signature, so incorrect function pointers may lead to crashes or
    /// undefined behaviour.
    pub unsafe fn register_native_methods<'other_local, T>(
        &mut self,
        class: T,
        methods: &[NativeMethod],
    ) -> Result<()>
    where
        T: Desc<'local, JClass<'other_local>>,
    {
        let class = class.lookup(self)?;
        // Safety:
        //  - NativeMethod is a #[transparent] / repr(C)] wrapper around sys::JNINativeMethod
        //  - NativeMethod only has an `unsafe` constructor that requires the caller to ensure that
        //    the function pointer is valid and matches the signature
        let res = unsafe {
            jni_call_with_catch!(
                catch |env| {
                    e: crate::exceptions::JNoSuchMethodError => {
                        let msg = e.as_throwable().get_message(env).ok();
                        let msg = if let Some(msg) = msg { msg.to_string() } else { String::new() };
                        Err(Error::NoSuchMethod(msg))
                    },
                    else => Err(Error::JniCall(JniError::Unknown)),
                },
                self,
                v1_1,
                RegisterNatives,
                class.as_ref().as_raw(),
                methods.as_ptr() as *mut sys::JNINativeMethod,
                methods.len() as jint
            )?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        jni_error_code_to_result(res)
    }

    /// Unbind all native methods of class.
    pub fn unregister_native_methods<'other_local, T>(&mut self, class: T) -> Result<()>
    where
        T: Desc<'local, JClass<'other_local>>,
    {
        let class = class.lookup(self)?;
        let res = unsafe {
            jni_call_post_check_ex!(self, v1_1, UnregisterNatives, class.as_ref().as_raw())?
        };

        // Ensure that `class` isn't dropped before the JNI call returns.
        drop(class);

        jni_error_code_to_result(res)
    }

    /// Returns an [`AutoElements`] to access the elements of the given Java `array`.
    ///
    /// # Safety
    ///
    /// See: [JPrimitiveArray::get_elements] for more details
    #[deprecated(
        since = "0.22.0",
        note = "use JPrimitiveArray::get_elements instead. This API will be removed in a future version"
    )]
    pub unsafe fn get_array_elements<'array_local, T, TArrayRef>(
        &self,
        array: TArrayRef,
        mode: ReleaseMode,
    ) -> Result<AutoElements<'array_local, T, TArrayRef>>
    where
        T: TypeArray,
        TArrayRef: AsRef<JPrimitiveArray<'array_local, T>> + Reference,
    {
        AutoElements::new(self, array, mode)
    }

    /// Returns an [`AutoElementsCritical`] to access the elements of the given Java `array`.
    ///
    /// # Safety
    ///
    /// See: [JPrimitiveArray::get_elements_critical] for more details
    #[deprecated(
        since = "0.22.0",
        note = "use JPrimitiveArray::get_elements_critical instead. This API will be removed in a future version"
    )]
    pub unsafe fn get_array_elements_critical<'array_local, T, TArrayRef>(
        &self,
        array: TArrayRef,
        mode: ReleaseMode,
    ) -> Result<AutoElementsCritical<'array_local, T, TArrayRef>>
    where
        T: TypeArray,
        TArrayRef: AsRef<JPrimitiveArray<'array_local, T>> + Reference,
    {
        AutoElementsCritical::new(self, array, mode)
    }
}

/// The outcome of an operation that may succeed, fail with an error, or panic.
///
/// This enum is used to encapsulate the result of operations within native methods
/// where extra care is needed to handle errors and panics gracefully before
/// returning a value to the Java environment.
#[derive(Debug)]
pub enum Outcome<T, E> {
    /// Contains the success value
    Ok(T),
    /// Contains the error value
    Err(E),
    /// Contains the panic value
    Panic(Box<dyn std::any::Any + Send + 'static>),
}

/// An opaque wrapper around an [`Outcome<T, Error>`] that supports mapping
/// errors within native methods (via [`ErrorPolicy`]), with access to an
/// [`Env`] reference.
///
/// This is returned by [`EnvUnowned::with_env`] and is designed for use within
/// native method implementations where unwinding can't be caught by the JVM and
/// will abort the process.
///
/// Once you have an [`EnvOutcome`] you must resolve it to a value that can be
/// returned to the Java environment.
///
/// An [`EnvOutcome`] is resolved into a return value by calling
/// [`EnvOutcome::resolve`] or [`EnvOutcome::resolve_with`] with the help of an
/// [`ErrorPolicy`] that can customize how errors and panics are handled before
/// returning a value.
///
/// There are several built-in error policies in the `jni::errors` module, and
/// you can also implement your own by implementing the [`ErrorPolicy`] trait.
/// See:
///
/// - [`ThrowRuntimeExAndDefault`]: an `ErrorPolicy` that throws any error as a
///   `java.lang.RuntimeException` and returns a default value.
/// - [`LogErrorAndDefault`]: an `ErrorPolicy` that logs errors and returns a
///   default value.
/// - [`LogContextErrorAndDefault`]: an `ErrorPolicy` that logs errors, with a
///   given context string, and returns a default value.
///
/// # Examples
///
/// ## Map Rust errors and panics to Java exceptions:
/// ```rust,no_run
/// # use jni::objects::{JObject, JString};
/// # use jni::errors::ThrowRuntimeExAndDefault;
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_com_example_MyClass_myNativeMethod<'caller>(
///     mut unowned_env: jni::EnvUnowned<'caller>,
///     _this: JObject<'caller>,
///     arg: JString<'caller>,
/// ) -> JObject<'caller> {
///     unowned_env.with_env(|env| -> jni::errors::Result<_> {
///         // Use `env` to call Java methods or access fields.
///         Ok(JObject::null())
///     }).resolve::<ThrowRuntimeExAndDefault>()
/// }
/// ```
///
/// ## Log errors with a context string and return a default value:
/// ```rust,no_run
/// # use jni::objects::{JObject, JString};
/// # use jni::errors::LogContextErrorAndDefault;
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_com_example_MyClass_myNativeMethod<'caller>(
///    mut unowned_env: jni::EnvUnowned<'caller>,
///    _this: JObject<'caller>,
///    arg: JString<'caller>,
/// ) -> JObject<'caller> {
///    unowned_env.with_env(|env| -> jni::errors::Result<_> {
///       // Use `env` to call Java methods or access fields.
///       Ok(JObject::null())
///   }).resolve_with::<LogContextErrorAndDefault, _>(|| {
///     format!("in myNativeMethod with arg: {arg}")
///   })
/// }
/// ```
#[must_use = "The outcome must be resolved to a value that can be returned to Java. See ::resolve or ::resolve_with"]
#[derive(Debug)]
pub struct EnvOutcome<'local, T, E> {
    raw_env: *mut crate::sys::JNIEnv,
    outcome: Outcome<T, E>,
    _invariant: std::marker::PhantomData<&'local mut ()>, // !Send/!Sync + tie to frame
}

impl<'local, T, E> EnvOutcome<'local, T, E> {
    pub(crate) fn new(raw_env: *mut crate::sys::JNIEnv, outcome: Outcome<T, E>) -> Self {
        Self {
            raw_env,
            outcome,
            _invariant: Default::default(),
        }
    }

    /// No captures (fast path).
    pub fn resolve<'native_method, P>(self) -> T
    where
        P: ErrorPolicy<T, E, Captures<'local, 'native_method> = ()>,
        T: Default + 'native_method,
        'local: 'native_method,
    {
        self.resolve_with::<P, _>(|| ())
    }

    /// Builder can borrow locals (via 'm) and use the temporary Env<'cf>.
    pub fn resolve_with<'native_method, P, F>(self, capture: F) -> T
    where
        P: ErrorPolicy<T, E>,
        T: Default + 'native_method,
        F: FnOnce() -> <P as ErrorPolicy<T, E>>::Captures<'local, 'native_method>,
        'local: 'native_method,
    {
        // Rebuild Env<'cf> (AttachGuard hidden), then build captures and map once.
        // All calls are guarded with catch_unwind; fall back to last_resort on failure.
        self.resolve_inner::<P, F>(capture)
    }

    fn resolve_inner<'native_method, P, F>(self, capture: F) -> T
    where
        P: ErrorPolicy<T, E>,
        T: Default + 'native_method,
        F: FnOnce() -> <P as ErrorPolicy<T, E>>::Captures<'local, 'native_method>,
        'local: 'native_method,
    {
        unsafe {
            let mut guard: AttachGuard<'local> = AttachGuard::from_unowned(self.raw_env);
            let env = guard.borrow_env_mut();

            match self.outcome {
                Outcome::Ok(t) => t,
                Outcome::Err(e) => {
                    let mut cap = capture();
                    std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                        match P::on_error(env, &mut cap, e) {
                            Ok(t) => t,
                            Err(err) => P::on_internal_jni_error(&mut cap, err),
                        }
                    }))
                    .unwrap_or_else(|payload| P::on_internal_panic(&mut cap, payload))
                }
                Outcome::Panic(p) => {
                    let mut cap = capture();
                    std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                        match P::on_panic(env, &mut cap, p) {
                            Ok(t) => t,
                            Err(err) => P::on_internal_jni_error(&mut cap, err),
                        }
                    }))
                    .unwrap_or_else(|payload| P::on_internal_panic(&mut cap, payload))
                }
            }
        }
    }

    /// Consumes the `EnvOutcome`, returning the underlying `Outcome<T, E>`.
    pub fn into_outcome(self) -> Outcome<T, E> {
        self.outcome
    }
}

/// Represents an external (unowned) JNI stack frame and thread attachment that
/// was passed to a native method call.
///
/// This is an FFI safe wrapper around a [`crate::sys::JNIEnv`] pointer that has
/// been passed as the first argument to a native method call, and represents
/// an implicit JNI thread attachment.
///
/// For example, you can use it with a native method implementation like this:
/// ```rust,no_run
/// # use jni::objects::{JObject, JString};
/// # use jni::errors::ThrowRuntimeExAndDefault;
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_com_example_MyClass_myNativeMethod<'caller>(
///     mut unowned_env: jni::EnvUnowned<'caller>,
///     _this: JObject<'caller>,
///     arg: JString<'caller>,
/// ) -> JObject<'caller> {
///     unowned_env.with_env(|env| -> jni::errors::Result<_> {
///         // Use `env` to call Java methods or access fields.
///         Ok(JObject::null())
///     }).resolve::<ThrowRuntimeExAndDefault>()
/// }
/// ```
#[repr(transparent)]
#[derive(Debug)]
pub struct EnvUnowned<'local> {
    ptr: *mut jni_sys::JNIEnv,
    _lifetime: std::marker::PhantomData<&'local ()>,
}

impl<'local> EnvUnowned<'local> {
    /// Runs a closure with a [`Env`] based on an unowned JNI thread attachment
    /// associated with an external JNI stack frame.
    ///
    /// This API is specifically intended to be used within native/foreign Java
    /// method implementations in cases where you have named the lifetime for
    /// the caller's JNI stack frame.
    ///
    /// It returns an [`EnvOutcome`] that supports mapping errors with access to
    /// an [`Env`], so you may choose to throw errors as exceptions.
    ///
    /// To avoid the risk of unwinding into the JVM (which will abort the
    /// process) this API wraps the closure in a [`catch_unwind`] to catch any
    /// panics. Any panic can be handled via the [`ErrorPolicy`] given to
    /// [`EnvOutcome::resolve`] or handled directly via [`EnvOutcome::into_outcome`].
    ///
    /// Note: This API does not create a new JNI stack frame, since the JVM will
    /// clean up the JNI stack frame when the native method returns.
    ///
    /// Note: This API returns an [`EnvOutcome`]
    pub fn with_env<F, T, E>(&mut self, f: F) -> EnvOutcome<'local, T, E>
    where
        F: FnOnce(&mut Env<'local>) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        // Safety: we trust that self.ptr a valid, non-null pointer
        let mut guard: AttachGuard<'local> = unsafe { AttachGuard::from_unowned(self.ptr) };
        let env = guard.borrow_env_mut();
        let result = catch_unwind(AssertUnwindSafe(|| f(env)));
        let outcome = match result {
            Ok(ret) => match ret {
                Ok(t) => Outcome::Ok(t),
                Err(e) => Outcome::Err(e),
            },
            Err(payload) => Outcome::Panic(payload),
        };
        EnvOutcome::new(self.ptr, outcome)
    }

    /// Runs a closure with a [`Env`] based on an unowned JNI thread attachment
    /// associated with an external JNI stack frame.
    ///
    /// This API is specifically intended to be used within native/foreign Java
    /// method implementations in cases where you have named the lifetime for
    /// the caller's JNI stack frame.
    ///
    /// Since it would lead to undefined behaviour to allow Rust code to unwind
    /// across a native method call boundary, you probably want to use
    /// [`EnvUnowned::with_env`] instead, which will wrap the closure
    /// in a `catch_unwind` to catch any panics.
    ///
    /// Note: This API does not create a new JNI stack frame, which is normally
    /// what you want when implementing a native method, since the JVM will
    /// clean up the JNI stack frame when the native method returns.
    pub fn with_env_no_catch<F, T, E>(&mut self, f: F) -> EnvOutcome<'local, T, E>
    where
        F: FnOnce(&mut Env<'local>) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        // Safety: we trust that self.ptr a valid, non-null pointer
        let mut guard: AttachGuard<'local> = unsafe { AttachGuard::from_unowned(self.ptr) };
        let result = f(guard.borrow_env_mut());
        match result {
            Ok(t) => EnvOutcome::new(self.ptr, Outcome::Ok(t)),
            Err(e) => EnvOutcome::new(self.ptr, Outcome::Err(e)),
        }
    }

    /// Creates a new `EnvUnowned` from a raw [`crate::sys::JNIEnv`] pointer.
    ///
    /// It should be very uncommon to use this method directly, but could be
    /// useful if you are given a raw [`crate::sys::JNIEnv`] pointer that you
    /// know represents a valid JNI attachment for the current thread.
    ///
    /// If you are implementing a native method in Rust though, you should
    /// prefer to use the [`EnvUnowned`] type as the first argument to your
    /// native method and avoid the need to use a raw pointer.
    ///
    /// If you have a raw [`crate::sys::JNIEnv`] pointer, this API should be
    /// marginally safer than using [`crate::AttachGuard`] manually
    /// since the attach guard management will be hidden within the
    /// [`Self::with_env`] and [`Self::with_env_no_catch`] methods.
    ///
    /// Beware that [`Self::with_env`] and [`Self::with_env_no_catch`] will not
    /// create a new JNI stack frame, so if you are not implementing a native
    /// method with a JNI stack frame that will be cleaned up on return, you may
    /// need to consider the risk of leaking local references into the current
    /// stack frame (JNI references are only cleaned up when the stack frame is
    /// popped)
    ///
    /// # Safety
    ///
    /// The pointer must be a valid, non-null pointer to a `jni_sys::JNIEnv`
    /// that represents an attachment of the current thread to a Java VM.
    ///
    /// The assigned lifetime must not outlive the JNI stack frame that owns the
    /// `Env` pointer. For example it would _never_ be safe to use `'static`.
    pub unsafe fn from_raw(ptr: *mut jni_sys::JNIEnv) -> Self {
        assert!(!ptr.is_null(), "EnvUnowned pointer must not be null");
        Self {
            ptr,
            _lifetime: std::marker::PhantomData,
        }
    }

    /// Returns the raw pointer to the underlying [`crate::sys::JNIEnv`].
    pub fn as_raw(&self) -> *mut jni_sys::JNIEnv {
        self.ptr
    }

    /// Consumes the [`EnvUnowned`] and returns the raw pointer to the underlying [`crate::sys::JNIEnv`].
    pub fn into_raw(self) -> *mut jni_sys::JNIEnv {
        self.ptr
    }
}

#[repr(transparent)]
#[derive(Debug, Clone, Copy)]
/// Native method descriptor for use with [`Env::register_native_methods`].
///
/// This is a `#[repr(transparent)]` / `repr(C)` wrapper around the [`crate::sys::JNINativeMethod`]
/// struct but with the additional constraint that it can only be constructed via an `unsafe`
/// [`Self::from_raw_parts`] method that requires the caller to ensure that the function pointer is
/// valid and matches the signature.
pub struct NativeMethod<'desc> {
    desc: JNINativeMethod,
    _phantom: PhantomData<&'desc JNIStr>,
}

impl<'desc> NativeMethod<'desc> {
    /// Creates a new `NativeMethod` descriptor.
    ///
    /// # Safety
    ///
    /// The `fn_ptr` must be a valid pointer to a function that matches the
    /// signature specified by `sig`.
    ///
    /// For example, with the signature `"(I)Ljava/lang/String;"`...
    ///
    /// A **static method** must point to a function with the signature:
    ///
    /// `fn<'local>(unowned_env: EnvUnowned<'local>, class: JClass<'local>, arg0: jint) -> JString<'local>`.
    ///
    /// An **instance method** must point to a function with the signature:
    ///
    /// `fn<'local>(unowned_env: EnvUnowned<'local>, this: JObject<'local>, arg0: jint) -> JString<'local>`.
    ///
    /// or some specific `this` type like:
    ///
    /// `fn<'local>(unowned_env: EnvUnowned<'local>, this: MyType<'local>, arg0: jint) -> JString<'local>`.
    pub const unsafe fn from_raw_parts(
        name: &'desc JNIStr,
        sig: &'desc JNIStr,
        fn_ptr: *mut c_void,
    ) -> NativeMethod<'desc> {
        NativeMethod {
            desc: JNINativeMethod {
                name: name.as_ptr() as *mut c_char,
                signature: sig.as_ptr() as *mut c_char,
                fnPtr: fn_ptr as *mut c_void,
            },
            _phantom: PhantomData,
        }
    }
}

/// Guard for a lock on a java object. This gets returned from the `lock_obj`
/// method.
#[derive(Debug)]
#[must_use]
pub struct MonitorGuard<'local> {
    obj: sys::jobject,
    life: PhantomData<&'local ()>,
}

impl Drop for MonitorGuard<'_> {
    fn drop(&mut self) {
        // Panics:
        //
        // The first `.expect()` is OK because the `&self` reference is enough to prove that
        // `JavaVM::singleton` must have been initialized and won't panic. (A MonitorGuard can only
        // be created with a Env reference)
        //
        // The second `.expect()` is OK because the guard is associated with a Env lifetime, so
        // it logically shouldn't be possible for the thread to become detached before the monitor
        // is dropped.
        JavaVM::singleton()
            .expect("JavaVM singleton must be initialized")
            .with_top_local_frame(|env| -> crate::errors::Result<()> {
                // Safety:
                //
                // This relies on `MonitorGuard` not being `Send` to maintain the
                // invariant that "The current thread must be the owner of the monitor
                // associated with the underlying Java object referred to by obj"
                //
                // This also means we can assume the `IllegalMonitorStateException`
                // exception can't be thrown due to the current thread not owning
                // the monitor.
                let res =
                    unsafe { ex_safe_jni_call_no_post_check_ex!(env, v1_1, MonitorExit, self.obj) };
                if let Err(err) = jni_error_code_to_result(res) {
                    log::error!("error releasing java monitor: {err}");
                }

                Ok(())
            })
            .expect("MonitorGuard dropped on detached thread");
    }
}

#[cfg(test)]
static_assertions::assert_not_impl_any!(MonitorGuard: Send);
