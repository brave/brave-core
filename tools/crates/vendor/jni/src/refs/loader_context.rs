use std::{borrow::Cow, ffi::CString};

use crate::{
    errors::{Error, JniError},
    objects::{JClass, JClassLoader, JObject, JString, JThread, JThrowable},
    refs::{IntoAuto as _, Reference},
    strings::{JNIStr, JNIString},
};

/// Represents the context that influences how a class may be loaded.
#[derive(Debug, Default)]
pub enum LoaderContext<'any_local, 'a> {
    /// There's no extra context that influences how the class should be loaded, and a default
    /// strategy will be used:
    ///
    /// 1. The Thread context will be used to find a ClassLoader to check via Class.forName
    /// 2. FindClass will be called
    #[default]
    None,
    /// A direct reference to the class loader that should be used (with no fallback to FindClass)
    Loader(&'a JClassLoader<'any_local>),
    /// In case we don't have a direct reference, to a `ClassLoader`, the ClassLoader associated
    /// with this object's Class may be checked
    ///
    /// This is used when downcasting, where we can speculate that the object being
    /// downcast _should_ be associated with the correct `ClassLoader`.
    ///
    /// The search strategy will be:
    /// 1. The Thread context will be used to find a ClassLoader to check via Class.forName
    /// 2. The ClassLoader associated with the object being downcast will be used
    /// 3. FindClass will be called
    FromObject(&'a JObject<'any_local>),
}

impl<'a, 'any_local> LoaderContext<'a, 'any_local> {
    /// Loads the class with the given name using the loader context.
    ///
    /// `name` should be a binary name like `"java.lang.String"` or an array descriptor like
    /// `"[Ljava.lang.String;"`.
    ///
    /// **Note:** that unlike [crate::env::Env::find_class], the name uses **dots instead of
    /// slashes** and should conform to the format that `Class.getName()` returns and that
    /// `Class.forName()` expects.
    ///
    /// **Note**: see [Self::find_class] if you need to load a class by its internal name, with
    /// slashes, instead of dots (compatible with [crate::env::Env::find_class]). If you have a
    /// choice, then prefer using [Self::load_class] because in the common case `Class.forName` will
    /// be called first, and that requires a **binary** name (so you can avoid a format conversion).
    ///
    /// `initialize` indicates whether a newly loaded class should be initialized (has no effect on
    /// already initialized classes).
    ///
    /// - Returns a reference to loaded class on success
    /// - Returns [`Error::NoClassDefFound`] if a `NoClassDefFoundError` or `ClassNotFoundException`
    ///   exception occurs, indicating that no definition for a requested class or interface can be
    ///   found (most common failure)
    /// - Returns [`Error::ClassFormatError`] if a `ClassFormatError` exception occurs, indicating
    ///   that the class data does not specify a valid class.
    /// - Returns [`Error::ClassCircularityError`] if a `ClassCircularityError` exception occurs,
    ///   indicating that a class or interface would be its own superclass or superinterface.
    /// - May Return [`Error::LinkageError`] for various failures to link the requested class.
    /// - May Return [`Error::ExceptionInInitializer`] if an exception occurs during class
    ///   initialization.
    /// - Returns [`Error::JniCall`] + [`JniError::NoMemory`] if an `OutOfMemoryError` exception
    ///   occurs, indicating that the JVM reports running out of memory.
    /// - Returns [`Error::NullPtr`] for any other unknown error condition
    ///
    /// *Note:* In order for this API to be consistent with [`jni::Env::find_class`], this API does
    /// _not_ directly return [`Error::ClassNotFound`] errors that may come directly from the
    /// loader.
    ///
    /// *Note:* Any [`Error::NoClassDefFound`], [`Error::LinkageError`] or
    /// [`Error::ExceptionInInitializer`] errors include the original `cause` exception if more
    /// details are needed.
    ///
    ///
    /// The strategy for loading the class depends on the loader context (See [Self]).
    pub fn load_class<'env_local>(
        &self,
        env: &mut crate::env::Env<'env_local>,
        name: impl AsRef<JNIStr>,
        initialize: bool,
    ) -> crate::errors::Result<JClass<'env_local>> {
        /// Convert a binary name or array descriptor (like `"java.lang.String"` or
        /// `"[Ljava.lang.String;"`) into an internal name like `"java/lang/String"` or
        /// `"[Ljava/lang/String;"` that can be passed to `FindClass`.
        fn internal_find_class_name<'name>(binary_name: &'name JNIStr) -> Cow<'name, JNIStr> {
            let bytes = binary_name.to_bytes();
            if !bytes.contains(&b'.') {
                Cow::Borrowed(binary_name)
            } else {
                // Convert from dot-notation to slash-notation
                let owned: Vec<u8> = bytes
                    .iter()
                    .map(|&b| if b == b'.' { b'/' } else { b })
                    .collect();
                let cstring = CString::new(owned).unwrap();
                let jni_string = unsafe { JNIString::from_cstring(cstring) };
                Cow::Owned(jni_string)
            }
        }

        fn lookup_tccl_with_catch<'local>(
            env: &mut crate::env::Env<'local>,
        ) -> crate::errors::Result<Option<JClassLoader<'local>>> {
            jni_try!(
                (env) -> crate::errors::Result<Option<JClassLoader<'local>>> {
                    let current_thread = JThread::current_thread(env)?;
                    let tccl = current_thread.get_context_class_loader(env)?;
                    Ok(Some(tccl))
                }
                catch |env| {
                    crate::exceptions::JSecurityException => Ok(None),
                    else => Ok(None)
                }
            )
        }

        fn load_class_with_catch<'any_loader, 'any_local>(
            env: &mut crate::env::Env<'any_local>,
            name: &JNIStr,
            initialize: bool,
            loader: &JClassLoader<'any_loader>,
        ) -> crate::errors::Result<JClass<'any_local>> {
            jni_try!(
                (env) -> crate::errors::Result<JClass<'any_local>> {
                    let name_ref = JString::from_jni_str(env, name)?.auto();
                    JClass::for_name_with_loader(env, name_ref, initialize, loader)
                }
                catch |env| {
                    // For consistency with `Env::find_class` we internally map
                    // `ClassNotFoundException` into `Error::NoClassDefFound`
                    // (especially considering that `load_class` may fall back
                    // to calling `::find_class()`)
                    e: crate::exceptions::JClassNotFoundException => {
                        let cause: &JThrowable = &e.as_throwable();
                        let cause = env.new_global_ref(cause).ok();
                        Err(Error::NoClassDefFound {
                            requested: name.to_string(),
                            cause
                        })
                    },
                    e: crate::exceptions::JExceptionInInitializerError => {
                        let exception = e.get_exception(env);
                        let exception = if let Ok(exception) = exception {
                            env.new_global_ref(exception).ok()
                        } else {
                            None
                        };
                        Err(Error::ExceptionInInitializer { exception })
                    },
                    e: crate::exceptions::JLinkageError => {
                        let cause: &JThrowable = &e.as_throwable();
                        let cause = env.new_global_ref(cause).ok();
                        Err(Error::LinkageError {
                            requested: name.to_string(),
                            cause
                        })
                    },
                    crate::exceptions::JSecurityException =>
                        Err(Error::SecurityViolation),
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),
                    else => Err(Error::NullPtr("Unexpected Exception")),
                }
            )
        }

        fn find_class<'local>(
            env: &mut crate::env::Env<'local>,
            name: &JNIStr,
        ) -> crate::errors::Result<JClass<'local>> {
            let internal_name = internal_find_class_name(name);
            env.find_class(&internal_name)
        }

        fn lookup_class_with_fallbacks<'local>(
            env: &mut crate::env::Env<'local>,
            name: &JNIStr,
            initialize: bool,
            candidate: Option<&JObject>,
        ) -> crate::errors::Result<JClass<'local>> {
            if let Some(tccl) = lookup_tccl_with_catch(env)? {
                match load_class_with_catch(env, name, initialize, &tccl) {
                    Ok(class) => return Ok(class),
                    Err(Error::NoClassDefFound { .. }) => {
                        // Try the next fallback
                    }
                    Err(e) => return Err(e),
                }
            }

            if let Some(candidate) = candidate {
                let candidate_class = env.get_object_class(candidate)?;
                let loader = jni_try!(
                    (env) -> crate::errors::Result<Option<JClassLoader<'local>>> {
                        Ok(Some(candidate_class.get_class_loader(env)?))
                    }
                    catch |env| {
                        crate::exceptions::JSecurityException => Ok(None),
                        else => Ok(None)
                    }
                )?;
                if let Some(loader) = loader {
                    match load_class_with_catch(env, name, initialize, &loader) {
                        Ok(class) => return Ok(class),
                        Err(Error::NoClassDefFound { .. }) => {
                            // Try the next fallback
                        }
                        Err(e) => return Err(e),
                    }
                }
            }

            find_class(env, name)
        }

        let name = name.as_ref();
        match self {
            LoaderContext::None => env.with_local_frame_returning_local::<_, JClass, _>(5, |env| {
                lookup_class_with_fallbacks(env, name, initialize, None)
            }),
            LoaderContext::FromObject(candidate) => env
                .with_local_frame_returning_local::<_, JClass, _>(5, |env| {
                    lookup_class_with_fallbacks(env, name, initialize, Some(candidate))
                }),
            LoaderContext::Loader(loader) => load_class_with_catch(env, name, initialize, loader),
        }
    }

    /// Loads the class with the given "internal" name using the loader context.
    ///
    /// This behaves the same as [Self::load_class] except that it uses the so-called "internal"
    /// name format that `FindClass` or [`crate::Env::find_class`] accepts, with slashes instead of
    /// dots.
    ///
    /// If possible, prefer to use [Self::load_class] to avoid a redundant name format conversion.
    pub fn find_class<'env_local>(
        &self,
        env: &mut crate::env::Env<'env_local>,
        name: impl AsRef<JNIStr>,
        initialize: bool,
    ) -> crate::errors::Result<JClass<'env_local>> {
        /// Convert an internal name or array descriptor (like `"java/lang/String"` or
        /// `"[Ljava/lang/String;"`) into a binary name like `"java.lang.String"` or
        /// `"[Ljava.lang.String;"` that can be passed to [LoaderContext::load_class].
        fn internal_to_binary_class_name<'name>(internal: &'name JNIStr) -> Cow<'name, JNIStr> {
            let bytes = internal.to_bytes();
            if !bytes.contains(&b'/') {
                Cow::Borrowed(internal)
            } else {
                // Convert from slash-notation to dot-notation
                let owned: Vec<u8> = bytes
                    .iter()
                    .map(|&b| if b == b'/' { b'.' } else { b })
                    .collect();
                let cstring = CString::new(owned).unwrap();
                let jni_string = unsafe { JNIString::from_cstring(cstring) };
                Cow::Owned(jni_string)
            }
        }
        let internal_name = internal_to_binary_class_name(name.as_ref());
        self.load_class(env, &internal_name, initialize)
    }

    /// Loads the class associated with the `JObjectRef` type `T`, using the given loader context.
    ///
    /// `initialize` indicates whether a newly loaded class should be initialized (has no effect
    /// on already initialized classes).
    ///
    /// Returns a local reference to the loaded class, or a [`Error::ClassNotFound`] error if the
    /// class could not be found.
    ///
    /// The strategy for loading the class depends on the loader context (See [Self]).
    pub fn load_class_for_type<'env_local, T: Reference>(
        &self,
        env: &mut crate::env::Env<'env_local>,
        initialize: bool,
    ) -> crate::errors::Result<JClass<'env_local>> {
        let class_name = T::class_name();
        self.load_class(env, &class_name, initialize)
    }
}
