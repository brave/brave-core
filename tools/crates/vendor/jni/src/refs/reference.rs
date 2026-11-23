use std::{borrow::Cow, ops::Deref};

use jni_sys::jobject;

use crate::{
    Env,
    objects::{Global, JClass, JObject},
    refs::LoaderContext,
    strings::JNIStr,
};

#[cfg(doc)]
use crate::objects::{Auto, JString};

/// A trait for types that represents a JNI reference (could be local, global or weak global as well
/// as wrapper types like [`Auto`] and [`Global`])
///
/// This trait unifies the behavior of all JNI reference types, allowing generic code to work with
/// both local and global references as well as smart-pointer-like wrappers around them.
///
/// For example, this makes it possible for APIs like [`Env::new_global_ref`] to be given a
/// non-static local reference type like [`JString<'local>`] (or an [`Auto`] wrapper) and return a
/// [`Global`] that is instead parameterized by [`JString<'static>`].
///
/// # Custom Java Type Bindings
///
/// All the built-in `jni::objects` types implement this trait, such as [`JObject`], [`JClass`] and
/// [`JString`]. You can also implement this trait for your own Java type bindings.
///
/// In addition to implementing the `Reference` trait, this is the recommended structure for custom
/// Java type bindings:
///
/// - Implement a `#[repr(transparent)]` struct wrapper around a [`JObject`].
/// - Derive `Default` for your type so that `Default::default()` returns a `null` reference
/// - Implement a corresponding `struct YourTypeAPI` that will cache a `Global<JClass>` and any
///   method/field IDs needed for your type's API (type should be `Send + Sync` as it will be stored
///   in a static cache)
/// - Implement a `YourTypeAPI::get()` method that takes a [`LoaderContext`] and returns a `'static`
///   singleton reference to the API cache, lazily loading the class and caching method/field IDs as
///   needed
/// - Implement `Reference` for your type
/// - Implement conventional reference-type APIs for `<YourType>` including: `from_raw()`,
///   `into_raw()`, `null()`, and `cast_local()`
/// - Implement constructors and methods for your type, based on `YourTypeAPI::get()` to access
///   cached class and method/field IDs
/// - Implement `AsRef<YourType<'local>>` for your type, since it's conventional for method
///   parameters to accept `impl AsRef<YourType>`
/// - Implement `Deref<Target = JObject<'local>>` so that all types can be treated as [`JObject`]s
/// - Implement `From<YourType<'local>> for IsInstanceOfType<'local>` and
///   `AsRef<IsInstanceOfType<'local>>` for any super-types (most importantly [`JObject`])
///
/// # Safety
///
/// The associated `Kind` and `GlobalKind` types must be FFI-safe, transparent wrappers around the
/// underlying JNI object reference types (such as [`jni::sys::jobject`] or [`jni::sys::jclass`])
/// and must not have any `Drop` side effects.
///
/// # Example
///
/// It would generally be recommended to use a macro to encapsulate the boilerplate of implementing
/// custom reference types, but for illustration purposes, a full implementation (including class +
/// method ID caching) could look like this:
///
/// ```rust,no_run
/// # use std::borrow::Cow;
/// # use jni::{jni_str, jni_sig};
/// # use jni::objects::{JObject, JClass, JString, Global};
/// # use jni::refs::{Reference, LoaderContext};
/// # use jni::Env;
/// # use jni::JValue;
/// # use jni::objects::JMethodID;
/// # use jni::errors::Result;
/// # use jni::sys::jobject;
///
/// /// A custom reference type for a `com.example.MyType` Java class
/// #[derive(Default, Debug)]
/// #[repr(transparent)]
/// struct MyType<'local>(JObject<'local>);
///
/// /// API cache for MyType
/// struct MyTypeAPI {
///     class: Global<JClass<'static>>,
///     my_method_id: jni::objects::JMethodID,
/// }
/// unsafe impl Send for MyTypeAPI {}
/// unsafe impl Sync for MyTypeAPI {}
///
/// impl MyTypeAPI {
///     pub fn get<'env>(
///         env: &Env<'env>,
///         loader_context: &LoaderContext,
///     ) -> Result<&'static Self> {
///         static API: std::sync::OnceLock<MyTypeAPI> = std::sync::OnceLock::new();
///         if let Some(api) = API.get() {
///             return Ok(api);
///         }
///         // Lazily load class and cache method IDs
///         // Note: we allow racing here to avoid deadlocks (e.g. through class init re-entry)
///         let api = env.with_local_frame(4, |env|  ->  jni::errors::Result<MyTypeAPI> {
///             let class: JClass = loader_context.load_class_for_type::<MyType>(env,false)?;
///             Ok(Self {
///                 class: env.new_global_ref(&class)?,
///                 my_method_id: env.get_method_id(
///                     &class, jni_str!("myMethod"), jni_sig!((JString) -> JString))?,
///             })
///         })?;
///         let _ = API.set(api);
///         Ok(API.get().unwrap())
///    }
/// }
///
/// impl<'local> MyType<'local> {
///     pub fn new(env: &mut Env<'local>) -> Result<Self> {
///         let api = MyTypeAPI::get(env, &LoaderContext::default())?;
///         let obj = env.new_object(&api.class, jni_sig!("()V"), &[])?;
///         Ok(MyType(obj))
///     }
///     pub unsafe fn from_raw<'env>(env: &Env<'env>, raw: jobject) -> MyType<'env> {
///         unsafe { MyType(JObject::from_raw(env, raw)) }
///     }
///     pub fn into_raw(self) -> jobject {
///        self.0.into_raw()
///     }
///     pub const fn null() -> MyType<'static> {
///         MyType(JObject::null())
///     }
///     pub fn cast_local<'any_local>(
///         env: &mut Env<'_>,
///         obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>
///     ) -> Result<MyType<'any_local>> {
///         env.cast_local::<MyType>(obj)
///     }
///     pub fn my_method<'env, 'any_local>(
///         &mut self,
///         env: &mut Env<'env>,
///         arg: impl AsRef<JString<'any_local>>
///     ) -> Result<JString<'env>> {
///        let api = MyTypeAPI::get(env, &LoaderContext::default())?;
///        let ret = unsafe {
///             env.call_method_unchecked(
///                 &self.0,
///                 api.my_method_id,
///                 jni::signature::ReturnType::Object,
///                 &[JValue::Object(arg.as_ref()).as_jni()]
///             )?.l()?
///        };
///        JString::cast_local(env, ret)
///     }
/// }
///
/// /// Safety: MyType::Kind + MyType::GlobalKind are transparent wrappers around
/// /// JObject with no Drop side effects
/// unsafe impl Reference for MyType<'_> {
///    type Kind<'local> = MyType<'local>;
///    type GlobalKind = MyType<'static>;
///
///     fn as_raw(&self) -> jni::sys::jobject {
///         self.0.as_raw()
///     }
///     fn class_name() -> std::borrow::Cow<'static, jni::strings::JNIStr> {
///         Cow::Borrowed(jni_str!("com.example.MyType"))
///     }
///     fn lookup_class<'caller>(
///         env: &jni::Env<'_>,
///         loader_context: &jni::refs::LoaderContext,
///     ) -> jni::errors::Result<
///         impl std::ops::Deref<Target = Global<JClass<'static>>> + 'caller,
///     > {
///         let api = MyTypeAPI::get(env, loader_context)?;
///         Ok(&api.class)
///     }
/// }
///
/// impl<'local> AsRef<MyType<'local>> for MyType<'local> {
///     fn as_ref(&self) -> &MyType<'local> {
///         self
///     }
/// }
/// impl<'local> std::ops::Deref for MyType<'local> {
///     type Target = JObject<'local>;
///     fn deref(&self) -> &Self::Target {
///         &self.0
///     }
/// }
/// impl<'local> AsRef<JObject<'local>> for MyType<'local> {
///     fn as_ref(&self) -> &JObject<'local> {
///         self
///     }
/// }
/// impl<'local> From<MyType<'local>> for JObject<'local> {
///     fn from(other: MyType<'local>) -> JObject<'local> {
///         other.0
///     }
/// }
/// ```
pub unsafe trait Reference: Sized {
    /// The generic associated [`Self::Kind`] type corresponds to the underlying
    /// class type (such as [`JObject`] or [`JString`]), parameterized by the
    /// lifetime that indicates whether the type holds a global reference
    /// (`'static`) or a local reference that's tied to a JNI stack frame.
    ///
    /// # Safety
    ///
    /// This must be a transparent `JObject` or `jobject` wrapper type that
    /// has no `Drop` side effects.
    type Kind<'local>: Reference + Default + Into<JObject<'local>> + AsRef<JObject<'local>> + 'local;
    // XXX: the compiler blows up if we try and specify a Send + Sync bound
    // here: "overflow evaluating the requirement..."
    //where
    //    Self::Kind<'static>: Send + Sync;
    //
    // As a workaround, we have a separate associated type

    /// The associated `GlobalKind` type should be equivalent to
    /// `Kind<'static>`, with the additional bound that ensures the type is
    /// `Send + Sync`
    ///
    /// # Safety
    ///
    /// This must be a transparent `JObject` or `jobject` wrapper type that
    /// has no `Drop` side effects.
    type GlobalKind: Reference
        + Default
        + Into<JObject<'static>>
        + AsRef<JObject<'static>>
        + Send
        + Sync
        + 'static;

    /// Returns the underlying, raw [`crate::sys::jobject`] reference.
    fn as_raw(&self) -> jobject;

    /// Returns `true` if this is a `null` object reference
    fn is_null(&self) -> bool {
        self.as_raw().is_null()
    }

    /// Returns `null` reference based on [`Self::Kind`]
    fn null<'any>() -> Self::Kind<'any> {
        Self::Kind::default()
    }

    /// The fully qualified class name of the Java class represented by this
    /// reference.
    ///
    /// The class name is expected to be dot-separated, in the same format as
    /// `Class.getName()` and suitable for passing to `Class.forName()`
    ///
    /// For example: `"com.example.MyClass"`
    ///
    /// Note: this format is very similar to the FindClass naming conventions,
    /// except for the use of dots instead of slashes.
    ///
    /// An array of objects would look like: "[Ljava.lang.Object;" An array of
    /// integers would look like: "[I"
    ///
    /// This returns a `Cow` so that in the common case a `&'static JNIStr`
    /// literal can be returned but for Array types they may compose the name
    /// dynamically.
    ///
    /// There's no guarantee that the name is interned / cached, so it's not
    /// recommended to call this in any fast path, it's mainly intended for use
    /// when first loading a class, or for debugging.
    ///
    /// # Example
    ///
    /// ```rust,no_run
    /// # use std::borrow::Cow;
    /// # use jni::jni_str;
    /// # fn example_class_name() -> Cow<'static, jni::strings::JNIStr> {
    /// Cow::Borrowed(jni_str!("com.example.MyClass"))
    /// # }
    /// ```
    fn class_name() -> Cow<'static, JNIStr>;

    /// Looks up a global reference to the [`JClass`] associated with this
    /// reference.
    ///
    /// Whenever you need the class associated with some reference wrapper type
    /// (e.g. [`JObject`], [`JClass`], [`JString`] etc), prefer using this
    /// method instead of [Env::find_class], [Env::load_class] or
    /// [LoaderContext::load_class].
    ///
    /// This [`Reference`] trait is implemented for all reference wrapper types
    /// like [`JObject`], [`JString`], [`JClass`] etc.
    ///
    /// All implementations will maintain a static cache holding a
    /// `Global<JClass>` that is cheap to lookup and doesn't require a JNI call
    /// or creating any new references.
    ///
    /// Note that in order to avoid creating a new JNI reference, this API
    /// will return a guard that derefs to a `Global<JClass>`, rather than a
    /// `JClass` directly.
    ///
    /// For example, lookup the class for `java.lang.String` / [JString] like
    /// this:
    /// ```rust,no_run
    /// # use jni::{errors::Result, Env, objects::JClass};
    /// #
    /// # fn example<'local>(env: &mut Env<'local>) -> Result<()> {
    /// use jni::objects::{JString, Reference as _, LoaderContext};
    /// let string_class = JString::lookup_class(env, &LoaderContext::None)?;
    /// let string_class_ref: &JClass = string_class.as_ref();
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// # Implementation notes
    ///
    /// This is used as part of casting checks to do a cached lookup of
    /// associated class references - avoiding the cost of repeated class
    /// lookups.
    ///
    /// The implementation is expected to use [`std::sync::OnceLock`] (or
    /// similar) to lookup cached API state, including a `Global<JClass>`.
    ///
    /// In case no class reference is already cached then use
    /// [`LoaderContext::load_class`] (not [Env::find_class]) to lookup a class
    /// reference.
    ///
    /// A bare-bones implementation could look like this:
    ///
    /// ```rust,no_run
    /// # use std::sync::OnceLock;
    /// # use std::ops::Deref;
    /// # use jni::{Env, objects::{JClass, Global}, refs::Reference as _, refs::LoaderContext};
    /// # type MyType<'local> = jni::objects::JObject<'local>;
    /// # fn lookup_class_impl<'caller>(env: &Env<'_>, loader_context: &LoaderContext) -> jni::errors::Result<impl Deref<Target = Global<JClass<'static>>> + 'caller> {
    /// static CLASS: OnceLock<Global<JClass>> = OnceLock::new();
    ///
    /// let class = if let Some(class) = CLASS.get() {
    ///     class
    /// } else {
    ///     env.with_local_frame(4, |env| -> jni::errors::Result<_> {
    ///         let class: jni::objects::JClass =
    ///             loader_context.load_class_for_type::<MyType>(env, false)?;
    ///         let global_class = env.new_global_ref(&class)?;
    ///         let _ = CLASS.set(global_class);
    ///         Ok(CLASS.get().unwrap())
    ///     })?
    /// };
    ///
    /// Ok(class)
    /// # }
    /// ```
    ///
    /// Note that it's recommended to allow the slow path to race, without
    /// locking to avoid any risk of deadlocks, for example through class
    /// initialization re-entry.
    fn lookup_class<'caller>(
        env: &Env<'_>,
        loader_context: &LoaderContext,
    ) -> crate::errors::Result<impl Deref<Target = Global<JClass<'static>>> + 'caller>;

    /// Returns a new reference type based on [`Self::Kind`] for the given `reference` that is tied
    /// to the specified lifetime.
    ///
    /// This method in generally only expected to be used internally or when implementing your own
    /// reference wrapper types.
    ///
    /// You should always prefer to use wrapper-provided `::from_raw()` methods (Such as
    /// `JString::from_raw()`) for wrapping raw local references because those will guarantee that
    /// the returned reference has a lifetime that's tied to a valid local reference frame.
    ///
    /// # Safety
    ///
    /// There must not be no other owning wrapper for the given `reference` (unless it is `null`)
    /// (as an exception it can be OK to create a temporary, hidden wrapper while borrowing an
    /// original, owning wrapper - e.g. as part of a type cast)
    ///
    /// Local references must have a lifetime that's associated with an AttachGuard or a Env that
    /// limits them to a single JNI stack frame.
    ///
    /// This can also be used to create a borrowed view of a global reference (e.g. as part of a
    /// type cast), which may be associated with a `'static` lifetime only so long as the lifetime
    /// of the view is limited by borrowing from the original global wrapper.
    ///
    /// You are responsible to knowing that `Self::Kind` is a suitable wrapper type for the given
    /// `reference`. E.g. because the `reference` came from an `into_raw` call from the same type.
    ///
    /// The default implementation simply uses `std::mem::transmute_copy`, based on the invariant
    /// that `Self::Kind` is required to be a transparent wrapper around a `jobject` JNI reference
    /// (this is a `Safety` requirement of the `Reference` trait).
    ///
    unsafe fn kind_from_raw<'env>(reference: jobject) -> Self::Kind<'env> {
        unsafe { std::mem::transmute_copy(&reference) }
    }

    /// Returns a (`'static`) reference type based on [`Self::GlobalKind`] for the given
    /// `global_ref`.
    ///
    /// # Safety
    ///
    /// `global_ref` must be a valid global reference, or `null`.
    ///
    /// There must not be no other wrapper for the given `global_ref` reference (unless it is
    /// `null`)
    ///
    /// You are responsible to knowing that `Self::GlobalKind` is a suitable wrapper type for the
    /// given `global_ref` reference. E.g. because the `global_ref` came from an `into_raw` call
    /// from the same type.
    ///
    /// The default implementation simply uses `std::mem::transmute_copy`, based on the invariant
    /// that `Self::GlobalKind` is required to be a transparent wrapper around a `jobject` JNI
    /// reference (this is a `Safety` requirement of the `Reference` trait).
    ///
    unsafe fn global_kind_from_raw(global_ref: jobject) -> Self::GlobalKind {
        unsafe { std::mem::transmute_copy(&global_ref) }
    }
}

mod transparent_reference {
    pub trait True {}
    #[derive(Debug)]
    pub struct AssertEq<A, B>(std::marker::PhantomData<(A, B)>);
    impl<T> True for AssertEq<T, T> {}
}

/// A marker trait for Reference types that are FFI-safe transparent wrappers that match their
/// associated Kind type
pub trait TransparentReference: Reference {}
impl<'local, T> TransparentReference for T
where
    T: Reference + 'local,
    transparent_reference::AssertEq<<T as Reference>::Kind<'local>, T>: transparent_reference::True,
{
}

// SAFETY: Kind and GlobalKind are implicitly transparent wrappers if T is
// implemented correctly / safely.
unsafe impl<T> Reference for &T
where
    T: Reference,
{
    type Kind<'local> = T::Kind<'local>;
    type GlobalKind = T::GlobalKind;

    fn as_raw(&self) -> jobject {
        (*self).as_raw()
    }

    fn class_name() -> Cow<'static, JNIStr> {
        T::class_name()
    }

    fn lookup_class<'caller>(
        env: &Env<'_>,
        loader_context: &LoaderContext,
    ) -> crate::errors::Result<impl Deref<Target = Global<JClass<'static>>> + 'caller> {
        T::lookup_class(env, loader_context)
    }

    unsafe fn kind_from_raw<'env>(local_ref: jobject) -> Self::Kind<'env> {
        unsafe { T::kind_from_raw(local_ref) }
    }

    unsafe fn global_kind_from_raw(global_ref: jobject) -> Self::GlobalKind {
        unsafe { T::global_kind_from_raw(global_ref) }
    }
}
