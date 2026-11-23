use std::{borrow::Cow, marker::PhantomData, ops::Deref};

use jni_sys::jobject;

use crate::{
    env::Env,
    errors::{Error, Result},
    objects::{Global, JClass, JObject, LoaderContext, Reference},
    strings::JNIStr,
};

/// Represents a runtime checked (via `IsInstanceOf`) cast of a reference from one type to another
///
/// This borrows a reference and implements `Deref` for the target type.
///
/// This can be used to cast global or local references.
///
/// See: [Env::as_cast]
///
#[repr(transparent)]
#[derive(Debug)]
pub struct Cast<'any, 'from, To: Reference> {
    _from: PhantomData<&'from JObject<'any>>,

    // SAFETY: We know that this hidden wrapper has no `Drop` side effects,
    // since that's a pre-condition for implementing `Reference`
    to: To::Kind<'any>,
}

impl<'any, 'from, To: Reference> Cast<'any, 'from, To> {
    /// Creates a new cast from one object type to another.
    ///
    /// This can be used to cast global or local references.
    ///
    /// Returns [Error::WrongObjectType] if the object is not of the expected type.
    pub(crate) fn new<'env_local, From: Reference + AsRef<JObject<'any>>>(
        env: &Env<'env_local>,
        from: &'from From,
    ) -> Result<Self>
    where
        'any: 'from,
    {
        let from: &JObject = from.as_ref();
        if from.is_null() {
            return Ok(Self {
                _from: PhantomData,
                to: To::null(),
            });
        }

        if env.is_instance_of_cast_type::<To>(from)? {
            // Safety:
            // - We have just checked that `from` is an instance of `T`
            // - Although we are creating a second wrapper for the raw reference, we will be
            //   borrowing the original wrapper (so the caller won't own two wrappers around the
            //   same reference) and this wrapper will be hidden.
            // - A pre-condition of `Reference` is that `T::Kind` must not have any `Drop` side
            //   effects so we don't have to worry that creating a second wrapper could lead to a
            //   double free when dropped.
            // - We're allowed to potentially create a `JObject::Kind` wrapper for a `'static`
            //   global reference in this situation where we're not giving ownership of the cast
            //   wrapper and we're borrowing from the original reference.
            unsafe {
                Ok(Self {
                    _from: PhantomData,
                    to: To::kind_from_raw::<'any>(from.as_raw()),
                })
            }
        } else {
            Err(Error::WrongObjectType)
        }
    }

    /// Creates a [`Cast`] from a raw JNI reference pointer
    ///
    /// Returns [Error::WrongObjectType] if the object is not of the expected type.
    ///
    /// # Safety
    ///
    /// The caller must ensure that `from` is a valid reference (local or global) - which may be
    /// `null`.
    ///
    /// The caller must ensure the `from` reference will not be deleted while the `Cast` exists.
    ///
    /// Note: even though this API is `unsafe`, it will still do a runtime check that `from` is a
    /// valid instance of `To`, so you are not required to know this.
    ///
    /// Note: this API is agnostic about whether the reference is local or global because the `Cast`
    /// wrapper doesn't give ownership over the reference and so you can't accidentally attempt to
    /// delete it using the wrong JNI API.
    pub unsafe fn from_raw<'env_local>(env: &Env<'env_local>, from: &'from jobject) -> Result<Self>
    where
        'any: 'from,
    {
        let from = unsafe { JObject::from_raw(env, *from) };
        let from = &from; // make it clear, we don't own `from`

        // Note we can't just chain up to Cast::new since the from lifetime would be incorrect.

        if from.is_null() {
            return Ok(Self {
                _from: PhantomData,
                to: To::null(),
            });
        }

        if env.is_instance_of_cast_type::<To>(from)? {
            // Safety:
            // - We have just checked that `from` is an instance of `T`
            // - Although we are creating a second wrapper for the raw reference, we will be
            //   borrowing the original wrapper (so the caller won't own two wrappers around the
            //   same reference) and this wrapper will be hidden.
            // - A pre-condition of `Reference` is that `T::Kind` must not have any `Drop` side
            //   effects so we don't have to worry that creating a second wrapper could lead to a
            //   double free when dropped.
            // - We're allowed to potentially create a `JObject::Kind` wrapper for a `'static`
            //   global reference in this situation where we're not giving ownership of the cast
            //   wrapper and we're borrowing from the original reference.
            unsafe {
                Ok(Self {
                    _from: PhantomData,
                    to: To::kind_from_raw::<'any>(from.as_raw()),
                })
            }
        } else {
            Err(Error::WrongObjectType)
        }
    }

    /// Creates a new cast from one object type to another without a runtime check.
    ///
    /// # Safety
    ///
    /// The caller must ensure that `from` is an instance of `To`, or null.
    pub unsafe fn new_unchecked<From: Reference + AsRef<JObject<'any>>>(from: &'from From) -> Self
    where
        'any: 'from,
    {
        // Safety:
        // - The caller has promised that `from` is an instance of `T`, or null
        // - Although we are creating a second wrapper for the raw reference, we will be
        //   borrowing the original wrapper (so the caller won't own two wrappers around the
        //   same reference) and this wrapper will be hidden.
        // - A pre-condition of `Reference` is that `T::Kind` must not have any `Drop` side
        //   effects so we don't have to worry that creating a second wrapper could lead to a
        //   double free when dropped.
        // - We're allowed to potentially create a `JObject::Kind` wrapper for a `'static`
        //   global reference in this situation where we're not giving ownership of the cast
        //   wrapper and we're borrowing from the original reference.
        unsafe {
            Self {
                _from: PhantomData,
                to: To::kind_from_raw::<'any>(from.as_raw()),
            }
        }
    }

    /// Creates a [`Cast`] from a raw JNI object reference without a runtime check.
    ///
    /// # Safety
    ///
    /// The caller must ensure that `from` is a valid JNI reference to an instance of `To` (or
    /// null).
    pub unsafe fn from_raw_unchecked(from: &'from jobject) -> Self
    where
        'any: 'from,
    {
        // Safety:
        // - The caller has promised that `from` is an instance of `T`, or null
        // - Although we are creating a second wrapper for the raw reference, we will be
        //   borrowing the original wrapper (so the caller won't own two wrappers around the
        //   same reference) and this wrapper will be hidden.
        // - A pre-condition of `Reference` is that `T::Kind` must not have any `Drop` side
        //   effects so we don't have to worry that creating a second wrapper could lead to a
        //   double free when dropped.
        // - We're allowed to potentially create a `JObject::Kind` wrapper for a `'static`
        //   global reference in this situation where we're not giving ownership of the cast
        //   wrapper and we're borrowing from the original reference.
        unsafe {
            Self {
                _from: PhantomData,
                to: To::kind_from_raw::<'any>(*from),
            }
        }
    }
}

impl<'local, 'from, To: Reference> Deref for Cast<'local, 'from, To> {
    type Target = To::Kind<'local>;

    fn deref(&self) -> &Self::Target {
        &self.to
    }
}

impl<'local, 'from, To: Reference> AsRef<To::Kind<'local>> for Cast<'local, 'from, To> {
    fn as_ref(&self) -> &To::Kind<'local> {
        &self.to
    }
}

unsafe impl<'any, 'from, To: Reference> Reference for Cast<'any, 'from, To> {
    type Kind<'local> = To::Kind<'local>;
    type GlobalKind = To::GlobalKind;

    fn as_raw(&self) -> jobject {
        self.to.as_raw()
    }

    fn class_name() -> Cow<'static, JNIStr> {
        To::class_name()
    }

    fn lookup_class<'caller>(
        env: &Env<'_>,
        loader_context: &LoaderContext,
    ) -> crate::errors::Result<impl Deref<Target = Global<JClass<'static>>> + 'caller> {
        To::lookup_class(env, loader_context)
    }

    unsafe fn kind_from_raw<'env>(local_ref: jobject) -> Self::Kind<'env> {
        unsafe { To::kind_from_raw(local_ref) }
    }

    unsafe fn global_kind_from_raw(global_ref: jobject) -> Self::GlobalKind {
        unsafe { To::global_kind_from_raw(global_ref) }
    }
}
