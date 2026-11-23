use std::{borrow::Cow, ops::Deref};

use jni_sys::jobject;
use log::{debug, warn};

use crate::{
    AttachConfig, JavaVM,
    env::Env,
    errors::{Error, Result},
    objects::{Global, JClass, JObject, LoaderContext},
    strings::JNIStr,
    sys,
};

use super::Reference;

// Note: `Weak` must not implement `Into<JObject>`! If it did, then it would be possible to
// wrap it in `Auto`, which would cause undefined behavior upon drop as a result of calling
// the wrong JNI function to delete the reference.

/// A global reference to a Java object that does *not* prevent it from being
/// garbage collected.
///
/// <dfn>Weak global references</dfn> have the same properties as [ordinary
/// “strong” global references][Global], with one exception: a weak global
/// reference does not prevent the referenced Java object from being garbage
/// collected. In other words, the Java object can be garbage collected even if
/// there is a weak global reference to it.
///
///
/// # Upgrading
///
/// Because the Java object referred to by a weak global reference may be
/// garbage collected at any moment, it cannot be directly used (such as
/// calling methods on the referenced Java object). Instead, it must first be
/// <dfn>upgraded</dfn> to a local or strong global reference, using the
/// [`Weak::upgrade_local`] or [`Weak::upgrade_global`] method,
/// respectively.
///
/// Both upgrade methods return an [`Option`]. If, when the upgrade method is
/// called, the Java object has not yet been garbage collected, then the
/// `Option` will be [`Some`] containing a newly created strong reference that
/// can be used as normal. If not, the `Option` will be [`None`].
///
/// Upgrading a weak global reference does not delete it. It is only deleted
/// when the `Weak` is dropped, and it can be upgraded more than once.
///
///
/// # Creating and Deleting
///
/// To create a weak global reference, use the [`Env::new_weak_ref`] method.
/// To delete it, simply drop the `Weak` (but be sure to do so on an attached
/// thread if possible; see the warning below).
///
/// It is also possible to create a new JNI weak global reference from an
/// existing one. To do that, use the [`Weak::clone_in_jvm`] method.
///
///
/// # Warning: Drop On an Attached Thread If Possible
///
/// When a `Weak` is dropped, a JNI call is made to delete the global
/// reference. If this frequently happens on a thread that is not already
/// attached to the JVM, the thread will be temporarily attached using
/// [`JavaVM::attach_current_thread_for_scope`], causing a severe performance
/// penalty.
///
/// To avoid this performance penalty, ensure that `Weak`s are only dropped
/// on a thread that is already attached (or never dropped at all).
///
/// In the event that a global reference is dropped on an unattached thread, a
/// message is [logged][log] at [`log::Level::Warn`].
#[repr(transparent)]
#[derive(Debug)]
pub struct Weak<T>
where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync,
{
    obj: T,
}

/// A temporary type alias to sign post that `WeakRef` has been renamed to `Weak`.
#[deprecated(
    since = "0.22.0",
    note = r#"Since 0.22, `WeakRef` has been renamed to `Weak`."#
)]
pub type WeakRef<T> = Weak<T>;

unsafe impl<T> Send for Weak<T> where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync
{
}

unsafe impl<T> Sync for Weak<T> where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync
{
}

impl<T> Default for Weak<T>
where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync,
{
    fn default() -> Self {
        Self::null()
    }
}

impl<T, U> AsRef<U> for Weak<T>
where
    T: AsRef<U>
        + Into<JObject<'static>>
        + AsRef<JObject<'static>>
        + Default
        + Reference
        + Send
        + Sync,
{
    fn as_ref(&self) -> &U {
        self.obj.as_ref()
    }
}

impl<T> Deref for Weak<T>
where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.obj
    }
}

impl<T> Weak<T>
where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync,
{
    /// Creates a new auto-delete wrapper for the `'static` weak global reference
    ///
    /// Note: It's more likely that you want to look at the [`Env::new_weak_ref`] API instead
    /// of this, since you can't get `'static` reference types through safe APIs.
    ///
    /// The [`Env`] reference here serves as proof that the current thread is attached, which
    /// implies [`JavaVM::singleton()`] is initialized, which is required by the `Drop`
    /// implementation.
    ///
    /// # Safety
    ///
    /// If the given reference is non-null, it must represent a weak global JNI reference.
    pub(crate) unsafe fn new(_env: &Env, obj: T) -> Self {
        Self { obj }
    }

    /// Creates a [`Global`] wrapper for a `null` reference
    ///
    /// This is equivalent [`Weak::default()`]
    ///
    /// A `null` [`Weak`] acts as-if the object has been garbage collected
    /// ([`Self::is_garbage_collected()`] will return `true`).
    pub fn null() -> Self {
        Self { obj: T::default() }
    }

    /// Returns the raw JNI weak reference.
    pub fn as_raw(&self) -> sys::jweak {
        self.obj.as_raw()
    }

    /// Creates a new local reference to this object.
    ///
    /// This returns `None` if the object has already been garbage collected, otherwise it returns
    /// `Some(new_local_reference)`.
    ///
    /// If this method returns `Some(r)`, it is guaranteed that the object will not be garbage
    /// collected at least until `r` is deleted or becomes invalid.
    pub fn upgrade_local<'local>(&self, env: &mut Env<'local>) -> Result<Option<T::Kind<'local>>> {
        match env.new_local_ref(self) {
            Ok(local_ref) => Ok(Some(local_ref)),
            Err(Error::ObjectFreed) => Ok(None),
            Err(err) => Err(err),
        }
    }

    /// Creates a new strong global reference to this object.
    ///
    /// This returns `None` if the object has already been garbage collected, otherwise it returns
    /// `Some(new_local_reference)`.
    ///
    /// If this method returns `Some(r)`, it is guaranteed that the object will not be garbage
    /// collected at least until `r` is deleted or becomes invalid.
    pub fn upgrade_global(&self, env: &Env) -> Result<Option<Global<T::GlobalKind>>> {
        match env.new_global_ref(self) {
            Err(Error::ObjectFreed) => Ok(None),
            Err(err) => Err(err),
            Ok(global_ref) => Ok(Some(global_ref)),
        }
    }

    /// Checks if the object referred to by this `Weak` has been garbage collected.
    ///
    /// Note that garbage collection can happen at any moment, so a return of `Ok(true)` from this
    /// method does not guarantee that [`Weak::upgrade_local`] or [`Weak::upgrade_global`]
    /// will succeed.
    ///
    /// This is equivalent to
    /// <code>self.[is_same_object][Weak::is_same_object](env, [JObject::null]\())</code>.
    ///
    /// This may return [`Error::JavaException`] if called while there is a pending exception.
    pub fn is_garbage_collected(&self, env: &Env) -> Result<bool> {
        env.is_same_object(self, JObject::null())
    }

    /// Returns true if this weak reference refers to the given object. Otherwise returns false.
    ///
    /// If `object` is [null][JObject::null], then this method is equivalent to
    /// [`Weak::is_garbage_collected`]: it returns true if the object referred to by this
    /// `Weak` has been garbage collected, or false if the object has not yet been garbage
    /// collected.
    #[deprecated = "Use Env::is_same_object"]
    pub fn is_same_object<'local, O>(&self, env: &Env<'local>, object: O) -> Result<bool>
    where
        O: AsRef<JObject<'local>>,
    {
        env.is_same_object(self, object)
    }

    /// Returns true if this weak reference refers to the same object as another weak reference.
    /// Otherwise returns false.
    ///
    /// This method will also return true if both weak references refer to an object that has been
    /// garbage collected.
    #[deprecated = "Use Env::is_same_object"]
    pub fn is_weak_ref_to_same_object(&self, env: &Env, other: &Self) -> Result<bool> {
        env.is_same_object(self, other)
    }

    /// Creates a new weak reference to the same object that this one refers to.
    ///
    /// This method returns `None` if the object has already been garbage collected.
    pub fn clone_in_jvm(&self, env: &mut Env<'_>) -> Result<Option<Weak<T::GlobalKind>>> {
        match env.new_weak_ref(self) {
            Err(Error::ObjectFreed) => Ok(None),
            Err(err) => Err(err),
            Ok(weak_ref) => Ok(Some(weak_ref)),
        }
    }
}

impl<T> Drop for Weak<T>
where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync,
{
    fn drop(&mut self) {
        let obj = std::mem::take(&mut self.obj);

        // It's redundant to explicitly call DeleteWeakGlobalRef with a null pointer and we don't
        // assume that a JavaVM has been initialized if we only wrap a 'static null pointer
        if !obj.is_null() {
            // Panic: If we have a non-null reference, we know JavaVM::singleton() must have been
            // initialized (and can't return an error) because ::new() takes a Env reference.
            let vm = JavaVM::singleton().expect("JavaVM singleton uninitialized");

            // Note: we use `attach_current_thread_guard` and avoid using
            // `AttachGuard::detach_with_catch` because we need to be exception safe and must
            // not interfere with any currently-pending exception
            let exception_safe_delete = || -> jni::errors::Result<()> {
                let mut scope = jni::ScopeToken::default();
                let mut guard = unsafe {
                    vm.attach_current_thread_guard(|| AttachConfig::new().scoped(true), &mut scope)?
                };
                let env = guard.borrow_env_mut();

                // If the Env is borrowing from an AttachGuard that owns the current thread
                // attachment that means the thread was not already attached
                if env.owns_attachment() {
                    warn!(
                        "Dropping a Weak in a detached thread. Fix your code if this message appears frequently (see the Weak docs)."
                    );
                }

                // Safety: This method is safe to call in case of pending exceptions (see chapter 2 of the spec)
                // jni-rs requires JNI_VERSION > 1.2
                unsafe {
                    ex_safe_jni_call_no_post_check_ex!(
                        env,
                        v1_2,
                        DeleteWeakGlobalRef,
                        obj.as_raw()
                    );
                }
                Ok(())
            };

            if let Err(err) = exception_safe_delete() {
                debug!("error dropping weak ref: {:#?}", err);
            }
        }
    }
}

// SAFETY: Kind and GlobalKind are implicitly transparent wrappers if T is
// implemented correctly / safely.
unsafe impl<T> Reference for Weak<T>
where
    T: Into<JObject<'static>> + AsRef<JObject<'static>> + Default + Reference + Send + Sync,
{
    type Kind<'env> = T::Kind<'env>;
    type GlobalKind = T::GlobalKind;

    fn as_raw(&self) -> jobject {
        self.obj.as_raw()
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

#[test]
fn test_weak_ref_send() {
    fn assert_send<T: Send>() {}
    assert_send::<Weak<JObject<'static>>>();
}

#[test]
fn test_weak_ref_sync() {
    fn assert_sync<T: Sync>() {}
    assert_sync::<Weak<JObject<'static>>>();
}
