use std::borrow::Cow;
use std::marker::PhantomData;
use std::ops::Deref;

use crate::{
    env::Env,
    errors::Result,
    objects::{
        AutoElements, AutoElementsCritical, Global, JClass, JObject, LoaderContext, Reference,
        ReleaseMode,
    },
    strings::JNIStr,
    sys::{jarray, jobject},
};

use super::TypeArray;

#[cfg(doc)]
use crate::{MonitorGuard, errors::Error, objects::JObjectArray};

/// A primitive array reference, tied to a JNI local reference frame.
///
/// This is a `#[transparent]` reference wrapper for primitive array types like `boolean[]` or
/// `int[]`.
///
/// For convenience it's recommended to use one of the type aliases like:
/// - [JBooleanArray]
/// - [JByteArray]
/// - [JCharArray]
/// - [JShortArray]
/// - [JIntArray]
/// - [JLongArray]
/// - [JFloatArray]
/// - [JDoubleArray]
///
/// See [JObjectArray] for non-primitive object arrays.
///
/// See the [`JObject`] documentation for more information about reference wrappers, how to cast
/// them, and local reference frame lifetimes.
///
#[repr(transparent)]
#[derive(Debug)]
pub struct JPrimitiveArray<'local, T: TypeArray> {
    obj: JObject<'local>,
    _marker: PhantomData<T>,
}

impl<'local, T: TypeArray> AsRef<JPrimitiveArray<'local, T>> for JPrimitiveArray<'local, T> {
    fn as_ref(&self) -> &JPrimitiveArray<'local, T> {
        self
    }
}

impl<'local, T: TypeArray> AsMut<JPrimitiveArray<'local, T>> for JPrimitiveArray<'local, T> {
    fn as_mut(&mut self) -> &mut JPrimitiveArray<'local, T> {
        self
    }
}

impl<'local, T: TypeArray> AsRef<JObject<'local>> for JPrimitiveArray<'local, T> {
    fn as_ref(&self) -> &JObject<'local> {
        &self.obj
    }
}

impl<'local, T: TypeArray> ::std::ops::Deref for JPrimitiveArray<'local, T> {
    type Target = JObject<'local>;

    fn deref(&self) -> &Self::Target {
        &self.obj
    }
}

impl<'local, T: TypeArray> From<JPrimitiveArray<'local, T>> for JObject<'local> {
    fn from(other: JPrimitiveArray<'local, T>) -> JObject<'local> {
        other.obj
    }
}

impl<T: TypeArray> std::default::Default for JPrimitiveArray<'_, T> {
    fn default() -> Self {
        Self {
            obj: JObject::null(),
            _marker: PhantomData,
        }
    }
}

impl<'local, T: TypeArray> JPrimitiveArray<'local, T> {
    /// Creates a new [`JPrimitiveArray`] of the given `length`.
    pub fn new<'env_local>(
        env: &mut Env<'env_local>,
        length: usize,
    ) -> Result<JPrimitiveArray<'env_local, T>> {
        if length > crate::sys::jsize::MAX as usize {
            return Err(crate::errors::Error::JniCall(
                crate::errors::JniError::InvalidArguments,
            ));
        }
        // Runtime check that the 'local reference lifetime will be tied to
        // Env lifetime for the top JNI stack frame
        env.assert_top();
        let raw_array = unsafe { T::new_array(env, length as crate::sys::jsize)? };
        unsafe { Ok(JPrimitiveArray::from_raw(env, raw_array)) }
    }

    /// Creates a [`JPrimitiveArray`] that wraps the given `raw` [`jarray`]
    ///
    /// # Safety
    ///
    /// - `raw` must be a valid raw JNI local reference (or `null`).
    /// - `raw` must be an instance of the appropriate primitive array type, e.g. `int[]`.
    /// - There must not be any other owning [`Reference`] wrapper for the same reference.
    /// - The local reference must belong to the current thread and not outlive the
    ///   JNI stack frame associated with the [Env] `'env_local` lifetime.
    pub unsafe fn from_raw<'env_local>(
        env: &Env<'env_local>,
        raw: jarray,
    ) -> JPrimitiveArray<'env_local, T> {
        JPrimitiveArray {
            obj: unsafe { JObject::from_raw(env, raw as jobject) },
            _marker: PhantomData,
        }
    }

    /// Creates a new null reference.
    ///
    /// Null references are always valid and do not belong to a local reference frame. Therefore,
    /// the returned `JPrimitiveArray` always has the `'static` lifetime.
    pub const fn null() -> JPrimitiveArray<'static, T> {
        JPrimitiveArray {
            obj: JObject::null(),
            _marker: PhantomData,
        }
    }

    /// Unwrap to the raw jni type.
    pub const fn into_raw(self) -> jarray {
        self.obj.into_raw() as jarray
    }

    /// Returns the length of the array.
    pub fn len(&self, env: &Env) -> Result<usize> {
        let array = null_check!(self.as_raw(), "JPrimitiveArray::len self argument")?;
        let len = unsafe { jni_call_no_post_check_ex!(env, v1_1, GetArrayLength, array)? } as usize;
        Ok(len)
    }

    /// Returns an [`AutoElements`] to access the elements of the given Java `array`.
    ///
    /// The elements are accessible until the returned auto-release guard is dropped.
    ///
    /// The returned array may be a copy of the Java array and changes made to
    /// the returned array will not necessarily be reflected in the original
    /// array until the [`AutoElements`] guard is dropped.
    ///
    /// If you know in advance that you will only be reading from the array then
    /// pass [`ReleaseMode::NoCopyBack`] so that the JNI implementation knows
    /// that it's not necessary to copy any data back to the original Java array
    /// when the [`AutoElements`] guard is dropped.
    ///
    /// Since the returned array may be a copy of the Java array, changes made to the
    /// returned array will not necessarily be reflected in the original array until
    /// the corresponding `Release*ArrayElements` JNI method is called.
    /// [`AutoElements`] has a commit() method, to force a copy back of pending
    /// array changes if needed (and without releasing it).
    ///
    /// # Safety
    ///
    /// ## No data races
    ///
    /// This API has no built-in synchronization that ensures there won't be any data
    /// races while accessing the array elements.
    ///
    /// To avoid undefined behaviour it is the caller's responsibility to ensure there
    /// will be no data races between other Rust or Java threads trying to access the
    /// same array.
    ///
    /// Acquiring a [`MonitorGuard`] lock for the `array` could be one way of ensuring
    /// mutual exclusion between Rust and Java threads, so long as the Java threads
    /// also acquire the same lock via `synchronized(array) {}`.
    ///
    /// ## No aliasing
    ///
    /// Callers must not create more than one [`AutoElements`] or
    /// [`AutoElementsCritical`] per Java array at the same time - even if
    /// there is no risk of a data race.
    ///
    /// The reason for this restriction is that [`AutoElements`] and
    /// [`AutoElementsCritical`] implement `DerefMut` which can provide a
    /// mutable `&mut [T]` slice reference for the elements and it would
    /// constitute undefined behaviour to allow there to be more than one
    /// mutable reference that points to the same memory.
    ///
    /// # jboolean elements
    ///
    /// Keep in mind that arrays of `jboolean` values should only ever hold
    /// values of `0` or `1` because any other value could lead to undefined
    /// behaviour within the JVM.
    ///
    /// Also see
    /// [`Env::get_array_elements_critical`] which
    /// imposes additional restrictions that make it less likely to incur the
    /// cost of copying the array elements.
    pub unsafe fn get_elements(
        &self,
        env: &Env,
        mode: ReleaseMode,
    ) -> Result<AutoElements<'local, T, &Self>> {
        AutoElements::new(env, self, mode)
    }

    /// Returns an [`AutoElementsCritical`] to access the elements of this
    /// array.
    ///
    /// The elements are accessible during the critical section that exists
    /// until the returned auto-release guard is dropped.
    ///
    /// This API imposes some strict restrictions that help the JNI
    /// implementation avoid any need to copy the underlying array elements
    /// before making them accessible to native code:
    ///
    /// 1. No other use of JNI calls are allowed (on the same thread) within the
    ///    critical section that exists while holding the
    ///    [`AutoElementsCritical`] guard.
    /// 2. No system calls can be made (Such as `read`) that may depend on a
    ///    result from another Java thread.
    ///
    /// The JNI spec does not specify what will happen if these rules aren't
    /// adhered to but it should be assumed it will lead to undefined behaviour,
    /// likely deadlock and possible program termination.
    ///
    /// Even with these restrictions the returned array may still be a copy of
    /// the Java array and changes made to the returned array will not
    /// necessarily be reflected in the original array until the
    /// [`AutoElementsCritical`] guard is dropped.
    ///
    /// If you know in advance that you will only be reading from the array then
    /// pass [`ReleaseMode::NoCopyBack`] so that the JNI implementation knows
    /// that it's not necessary to copy any data back to the original Java array
    /// when the [`AutoElementsCritical`] guard is dropped.
    ///
    /// A nested scope or explicit use of `std::mem::drop` can be used to
    /// control when the returned [`AutoElementsCritical`] is dropped to
    /// minimize the length of the critical section.
    ///
    /// If this array is `null`, an [`Error::NullPtr`] is returned.
    ///
    /// # Safety
    ///
    /// ## Critical Section Restrictions
    ///
    /// This API is marked as `unsafe` due to the complex, far-reaching nature
    /// of the critical-section restrictions imposed here that can't be enforced
    /// through Rust's borrow checker rules.
    ///
    /// The rules above about JNI usage and system calls _must_ be adhered to.
    ///
    /// Using this API implies:
    ///
    /// 1. All garbage collection will likely be paused during the critical
    ///    section
    /// 2. Any use of JNI in other threads may block if they need to allocate
    ///    memory (due to the garbage collector being paused)
    /// 3. Any use of system calls that will wait for a result from another Java
    ///    thread could deadlock if that other thread is blocked by a paused
    ///    garbage collector.
    ///
    /// A failure to adhere to the critical section rules could lead to any
    /// undefined behaviour, including aborting the program.
    ///
    /// ## No data races
    ///
    /// This API has no built-in synchronization that ensures there won't be any
    /// data races while accessing the array elements.
    ///
    /// To avoid undefined behaviour it is the caller's responsibility to ensure
    /// there will be no data races between other Rust or Java threads trying to
    /// access the same array.
    ///
    /// Acquiring a [`MonitorGuard`] lock for this array could be one way of
    /// ensuring mutual exclusion between Rust and Java threads, so long as the
    /// Java threads also acquire the same lock via `synchronized(array) {}`.
    ///
    /// ## No aliasing
    ///
    /// Callers must not create more than one [`AutoElements`] or
    /// [`AutoElementsCritical`] per Java array at the same time - even if there
    /// is no risk of a data race.
    ///
    /// The reason for this restriction is that [`AutoElements`] and
    /// [`AutoElementsCritical`] implement `DerefMut` which can provide a
    /// mutable `&mut [T]` slice reference for the elements and it would
    /// constitute undefined behaviour to allow there to be more than one
    /// mutable reference that points to the same memory.
    ///
    /// ## jboolean elements
    ///
    /// Keep in mind that arrays of `jboolean` values should only ever hold
    /// values of `0` or `1` because any other value could lead to undefined
    /// behaviour within the JVM.
    ///
    /// Also see [`get_elements`](Self::get_elements) which has
    /// fewer restrictions, but is more likely to incur a cost from copying
    /// the array elements.
    pub unsafe fn get_elements_critical(
        &self,
        env: &Env<'_>,
        mode: ReleaseMode,
    ) -> Result<AutoElementsCritical<'local, T, &Self>> {
        AutoElementsCritical::new(env, self, mode)
    }

    /// Copy elements of the array from the `start` index to the `buf` slice.
    /// The number of copied elements is equal to the `buf` length.
    ///
    /// # Errors
    ///
    /// Returns [Error::IndexOutOfBounds] if `start` is negative _or_ `start +
    /// buf.len()` is greater than [`Self::len`].
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending
    /// exception).
    pub fn get_region(&self, env: &Env, start: crate::sys::jsize, buf: &mut [T]) -> Result<()> {
        unsafe { T::get_region(env, self.as_raw() as jarray, start, buf) }
    }

    /// Copy the contents of the `buf` slice to the java byte array at the
    /// `start` index.
    ///
    /// # Errors
    ///
    /// Returns [Error::IndexOutOfBounds] if `start` is negative _or_ `start +
    /// buf.len()` is greater than [`Self::len`].
    ///
    /// This API catches exceptions internally and is not expected to return
    /// [`Error::JavaException`] (unless called while there is a pending exception).
    pub fn set_region(&self, env: &Env, start: crate::sys::jsize, buf: &[T]) -> Result<()> {
        unsafe { T::set_region(env, self.as_raw() as jarray, start, buf) }
    }
}

/// A `boolean[]` array reference, tied to a JNI local reference frame
pub type JBooleanArray<'local> = JPrimitiveArray<'local, crate::sys::jboolean>;

/// A `byte[]` array reference, tied to a JNI local reference frame
pub type JByteArray<'local> = JPrimitiveArray<'local, crate::sys::jbyte>;

/// A `char[]` array reference, tied to a JNI local reference frame
pub type JCharArray<'local> = JPrimitiveArray<'local, crate::sys::jchar>;

/// A `short[]` array reference, tied to a JNI local reference frame
pub type JShortArray<'local> = JPrimitiveArray<'local, crate::sys::jshort>;

/// A `int[]` array reference, tied to a JNI local reference frame
pub type JIntArray<'local> = JPrimitiveArray<'local, crate::sys::jint>;

/// A `long[]` array reference, tied to a JNI local reference frame
pub type JLongArray<'local> = JPrimitiveArray<'local, crate::sys::jlong>;

/// A `float[]` array reference, tied to a JNI local reference frame
pub type JFloatArray<'local> = JPrimitiveArray<'local, crate::sys::jfloat>;

/// A `double[]` array reference, tied to a JNI local reference frame
pub type JDoubleArray<'local> = JPrimitiveArray<'local, crate::sys::jdouble>;

/// Trait to access the raw `jarray` pointer for types that wrap an array reference
///
/// # Safety
///
/// Implementing this trait will allow a type to be passed to [`Env::get_array_length()`]
/// or other JNI APIs that only work with a valid reference to an array (or `null`)
///
pub unsafe trait AsJArrayRaw<'local>: AsRef<JObject<'local>> {
    /// Returns the raw JNI pointer as a `jarray`
    fn as_jarray_raw(&self) -> jarray {
        self.as_ref().as_raw() as jarray
    }
}

unsafe impl<'local, T: TypeArray> AsJArrayRaw<'local> for JPrimitiveArray<'local, T> {}

macro_rules! impl_ref_for_jprimitive_array {
    ($type:ident, $class_name:expr, $api_type:ident) => {
        #[allow(non_camel_case_types)]
        pub(crate) struct $api_type {
            class: Global<JClass<'static>>,
        }

        impl $api_type {
            pub(crate) fn get<'any_local>(
                env: &Env<'_>,
                loader_context: &LoaderContext<'any_local, '_>,
            ) -> Result<&'static Self> {
                static API: std::sync::OnceLock<$api_type> = std::sync::OnceLock::new();
                // Fast path
                if let Some(api) = API.get() {
                    return Ok(api);
                }

                // Lookup class and cache

                // Note: we don't mind racing here, and follow the general pattern of avoiding
                // locks while looking up classes and initializing APIs, because in the more
                // general case it can lead to deadlocks via class initialization dependencies.

                let api = env.with_local_frame(4, |env| -> Result<_> {
                    let class = loader_context
                        .load_class_for_type::<JPrimitiveArray<crate::sys::$type>>(env, false)?;
                    let class = env.new_global_ref(&class).unwrap();
                    Ok(Self { class })
                })?;
                let _ = API.set(api);
                Ok(API.get().unwrap())
            }
        }

        impl JPrimitiveArray<'_, crate::sys::$type> {
            /// Cast a local reference to a [`JPrimitiveArray<T>`]
            ///
            /// This will do a runtime (`IsInstanceOf`) check that the object is an instance of `T[]`.
            ///
            /// Also see these other options for casting local or global references to a [`JPrimitiveArray<T>`]:
            /// - [Env::as_cast]
            /// - [Env::new_cast_local_ref]
            /// - [Env::cast_local]
            /// - [Env::new_cast_global_ref]
            /// - [Env::cast_global]
            ///
            /// # Errors
            ///
            /// Returns [Error::WrongObjectType] if the `IsInstanceOf` check fails.
            pub fn cast_local<'any_local>(
                env: &mut Env<'_>,
                obj: impl Reference + Into<JObject<'any_local>> + AsRef<JObject<'any_local>>,
            ) -> Result<
                <JPrimitiveArray<'any_local, crate::sys::$type> as Reference>::Kind<'any_local>,
            > {
                env.cast_local::<JPrimitiveArray<crate::sys::$type>>(obj)
            }
        }

        // SAFETY: JPrimitiveArray is a transparent JObject wrapper with no Drop side effects
        unsafe impl Reference for JPrimitiveArray<'_, crate::sys::$type> {
            type Kind<'env> = JPrimitiveArray<'env, crate::sys::$type>;
            type GlobalKind = JPrimitiveArray<'static, crate::sys::$type>;

            fn as_raw(&self) -> jobject {
                self.obj.as_raw()
            }

            fn class_name() -> Cow<'static, JNIStr> {
                Cow::Borrowed($crate::jni_str!($class_name))
            }

            fn lookup_class<'caller>(
                env: &Env<'_>,
                loader_context: &LoaderContext,
            ) -> crate::errors::Result<impl Deref<Target = Global<JClass<'static>>> + 'caller> {
                let api = $api_type::get(env, &loader_context)?;
                Ok(&api.class)
            }

            unsafe fn kind_from_raw<'env>(local_ref: jobject) -> Self::Kind<'env> {
                JPrimitiveArray {
                    obj: unsafe { JObject::kind_from_raw(local_ref) },
                    _marker: PhantomData,
                }
            }

            unsafe fn global_kind_from_raw(global_ref: jobject) -> Self::GlobalKind {
                JPrimitiveArray {
                    obj: unsafe { JObject::global_kind_from_raw(global_ref) },
                    _marker: PhantomData,
                }
            }
        }
    };
}

impl_ref_for_jprimitive_array!(jboolean, "[Z", JPrimitiveArrayAPI_jboolean);
impl_ref_for_jprimitive_array!(jbyte, "[B", JPrimitiveArrayAPI_jbyte);
impl_ref_for_jprimitive_array!(jchar, "[C", JPrimitiveArrayAPI_jchar);
impl_ref_for_jprimitive_array!(jshort, "[S", JPrimitiveArrayAPI_jshort);
impl_ref_for_jprimitive_array!(jint, "[I", JPrimitiveArrayAPI_jint);
impl_ref_for_jprimitive_array!(jlong, "[J", JPrimitiveArrayAPI_jlong);
impl_ref_for_jprimitive_array!(jfloat, "[F", JPrimitiveArrayAPI_jfloat);
impl_ref_for_jprimitive_array!(jdouble, "[D", JPrimitiveArrayAPI_jdouble);
