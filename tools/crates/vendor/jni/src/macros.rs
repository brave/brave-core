//! Various macros for making low-level JNI calls more ergonomic
//!
//! Note: all macros must avoid un-hygienic / hidden control flow like `return`
//! or `?`

/// Directly calls an exception-safe Env FFI function, nothing else
///
/// # Safety
///
/// This may only be used with a JNI function that is considered to be safe
/// to call with a pending exception. From the JNI design spec, this includes:
///
/// - `ExceptionOccurred()`
/// - `ExceptionDescribe()`
/// - `ExceptionClear()`
/// - `ExceptionCheck()`
/// - `ReleaseStringChars()`
/// - `ReleaseStringUTFChars()`
/// - `ReleaseStringCritical()`
/// - `Release<Type>ArrayElements()`
/// - `ReleasePrimitiveArrayCritical()`
/// - `DeleteLocalRef()`
/// - `DeleteGlobalRef()`
/// - `DeleteWeakGlobalRef()`
/// - `MonitorExit()`
/// - `PushLocalFrame()`
/// - `PopLocalFrame()`
/// - `DetachCurrentThread()`
///
/// You must also ensure that the arguments you pass are valid for the
/// particular JNI function you are calling.
///
/// When calling any function added after JNI 1.1 you must know that it's valid
/// for the current JNI version.
macro_rules! ex_safe_jni_call_no_post_check_ex {
    ( $jnienv:expr, $version:tt, $name:ident $(, $args:expr )*) => {{
        // Safety: we know that the Env pointer can't be null, since that's
        // checked in `from_raw()`
        let env: *mut jni_sys::JNIEnv = $jnienv.get_raw();
        let interface: *const jni_sys::JNINativeInterface_ = *env;

        ((*interface).$version.$name)(env $(, $args)*)
    }};
}

/// Pre-checks for a pending exception error and then calls an Env FFI function
///
/// Since most JNI functions may trigger undefined behaviour if they are called
/// with a pending exception, this macro will explicitly check for a pending
/// exception (and return a `Err(crate::errors::Error::JavaException)` if one is
/// found) before calling the JNI function.
///
/// # Safety
///
/// You must also ensure that the arguments you pass are valid for the
/// particular JNI function you are calling.
///
/// When calling any function added after JNI 1.1 you must know that it's valid
/// for the current JNI version.
macro_rules! jni_call_no_post_check_ex {
    ( $jnienv:expr, $version:tt, $name:ident $(, $args:expr )*) => {{
        $crate::__must_use(if $jnienv.exception_check() {
            Err(crate::errors::Error::JavaException)
        } else {
            // Safety: we know that the Env pointer can't be null, since that's
            // checked in `from_raw()`
            let env: *mut jni_sys::JNIEnv = $jnienv.get_raw();
            let interface: *const jni_sys::JNINativeInterface_ = *env;

            Ok(((*interface).$version.$name)(env $(, $args)*))
        })
    }};
}

/// Calls a Env function, then checks for a pending exception
///
/// This will do a pre-check for pending exceptions to avoid undefined behaviour when calling JNI
/// functions that are not exception safe.
///
/// Note: After the JNI call, this only _checks_ for an exception, it doesn't clear the exception
/// and so the exception must still be handled (or left to be handled in Java by returning control
/// to the JVM).
///
/// Returns `Err(Error::JavaException)` if there is a pending exception after the call.
///
/// Carefully consider whether to use `jni_call_with_catch` instead of this because if the function
/// throws any exceptions they will not be caught/cleared by this macro (it will just return an
/// `Err(Error::JavaException)` to notify you of the pending exception that must be handled).
macro_rules! jni_call_post_check_ex {
    ( $jnienv:expr, $version:tt, $name:ident $(, $args:expr )* ) => ({
        jni_call_no_post_check_ex!($jnienv, $version, $name $(, $args)*).and_then(|ret| {
            if $jnienv.exception_check() {
                Err(crate::errors::Error::JavaException)
            } else {
                Ok(ret)
            }
        })
    })
}

/// Calls a Env function, then checks for a pending exception, then checks for a `null` return value
///
/// This will do a pre-check for pending exceptions to avoid undefined behaviour when calling JNI
/// functions that are not exception safe.
///
/// Note: After the JNI call, this only _checks_ for an exception, it doesn't clear the exception
/// and so the exception must still be handled (or left to be handled in Java by returning control
/// to the JVM).
///
/// Returns `Err(Error::JavaException)` if there is a pending exception after the call. Returns
/// `Err(Error::NullPtr)` if the JNI function returns `null`
///
/// Carefully consider whether to use `jni_call_with_catch` instead of this because if the function
/// throws any exceptions they will not be caught/cleared by this macro (it will just return an
/// `Err(Error::JavaException)` to notify you of the pending exception that must be handled).
macro_rules! jni_call_post_check_ex_and_null_ret {
    ( $jnienv:expr, $version:tt, $name:ident $(, $args:expr )* ) => ({
        jni_call_post_check_ex!($jnienv, $version, $name $(, $args)*).and_then(|ret| {
            if ret.is_null() {
                Err($crate::errors::Error::NullPtr(concat!(stringify!($name), " result")))
            } else {
                Ok(ret)
            }
        })
    })
}

/// Calls a Env function, with no post check for exceptions, then checks for a
/// `null` return value
///
/// This will do a pre-check for pending exceptions to avoid undefined behaviour
/// when calling JNI functions that are not exception safe.
///
/// Returns `Err(Error::NullPtr)` if the JNI function returns `null`
///
/// Carefully consider whether to use `jni_call_with_catch` instead of this because if the function
/// throws any exceptions they will not be caught/cleared by this macro (it will just return an
/// `Err(Error::JavaException)` to notify you of the pending exception that must be handled).
macro_rules! jni_call_only_check_null_ret {
    ( $jnienv:expr, $version:tt, $name:ident $(, $args:expr )* ) => ({
        jni_call_no_post_check_ex!($jnienv, $version, $name $(, $args)*).and_then(|ret| {
            if ret.is_null() {
                Err($crate::errors::Error::NullPtr(concat!(stringify!($name), " result")))
            } else {
                Ok(ret)
            }
        })
    })
}

/// Helper for building an exception-handler chain based on `catch {}` patterns
///
/// Each entry has the form `<path> => <expr>`, matched in order via
/// `<ExType>::matches()`. A required terminal `else => <expr>` entry acts as
/// a catch-all: its expression is evaluated unconditionally for any exception that
/// was not claimed by an earlier handler.
macro_rules! __jni_match_exceptions {
    // Empty catch block
    ($env:expr, $ex:expr $(,)?) => {
        compile_error!("Missing 'else' expression in catch block")
    };
    // `else => expr` (with optional trailing comma) - unconditional
    // fallback. Must be checked before the `$ex_type:path` arms so `else`
    // isn't just matched as a `:path`
    ($env:expr, $ex:expr, else => $ex_result:expr $(,)?) => {
        $ex_result
    };
    // Single typed handler (with optional trailing comma) - implies a missing
    // 'else' expression in the catch block
    ($env:expr, $ex:expr, $ex_type:path => $ex_result:expr $(,)?) => {
        compile_error!("Missing 'else' expression in catch block")
    };
    // Check the next exception type (with an unbound '_' name for the `Cast`
    // exception) and then recursively check remaining patterns in the `catch
    // {}` block (terminating at `else`)
    ($env:expr, $ex:expr, $ex_type:path => $ex_result:expr, $($rest:tt)+) => {
        if let Some(_) = <$ex_type>::matches($env, &$ex)? {
            $ex_result
        } else {
            __jni_match_exceptions!($env, $ex, $($rest)+)
        }
    };
    // Check the next exception type (with a given name for the `Cast`
    // exception) and then recursively check remaining patterns in the `catch
    // {}` block (terminating at `else`)
    ($env:expr, $ex:expr, $ex_name:ident: $ex_type:path => $ex_result:expr, $($rest:tt)+) => {
        if let Some($ex_name) = <$ex_type>::matches($env, &$ex)? {
            $ex_result
        } else {
            __jni_match_exceptions!($env, $ex, $($rest)+)
        }
    };
}

/// Calls a Env function, then catches (checks + clears) any pending exception
///
/// This macro takes a `catch |env| {}` block that specifies how all possible
/// exceptions should be mapped to `Result` values:
///
/// ```ignore
/// jni_call_with_catch!(
///     catch |env| {
///         ExceptionType1 => Err(Error::Type1),
///         exception: ExceptionType2 => {
///             let cause = exception.get_cause(env)?;
///             let cause = env.new_global_ref(cause)?;
///             Err(Error::Type2(cause))
///         },
///         // required catch-all
///         else => Err(Error::JniCall(JniError::Unknown)),
///     },
///     env, version, FunctionName, args...
/// )
/// ```
/// The exception is always cleared before matching or evaluating any handler
/// expression.
///
/// Any exception thrown by the JNI function is matched against the exception
/// types in order (via its `::matches` method, which will also `Cast` the
/// exception if it does match).
///
/// The expression of the first matching handler is returned as the `Result`
/// value.
///
/// A required `else => expr` entry at the end acts as a catch-all for any
/// exception not claimed by a typed handler.
///
/// When an exception is matched then it will also be `Cast` to the matched
/// exception type and can be given a name that is accessible to right-hand
/// expression (e.g. `exception_name: ExceptionType => { ... }`).
///
/// Note: The catch expressions always have access to an `env: &mut Env`
/// reference (regardless of whether the initial `Env` reference passed to the
/// macro was mutable) - since the mapping is always done within a
/// `with_local_frame` closure. This `env` reference is named based on the
/// `catch |env|` syntax in the macro invocation.
macro_rules! jni_call_with_catch {
    ( catch |$env_mut:ident| { $($ex_handlers:tt)* }, $jnienv:expr, $version:tt, $name:ident $(, $args:expr )* ) => ({
        // Wrap everything in a closure so we can use '?' without causing a
        // surprising return from the _callers_ function.
        $crate::__must_use((|| -> $crate::errors::Result<_> {
            // The only `Err` we expect here is `Error::JavaException` in case
            // there is already a pending exception
            let ret = jni_call_no_post_check_ex!($jnienv, $version, $name $(, $args)*) ?;
            if $jnienv.exception_check() {
                // Get a `&mut Env` we can use while mapping the exception to a `Result`
                $jnienv.with_local_frame(
                    $crate::DEFAULT_LOCAL_FRAME_CAPACITY,
                    |$env_mut| -> $crate::errors::Result<_> {
                        let e = $env_mut
                            .exception_occurred()
                            .expect("Expected an exception after ExceptionCheck");
                        $env_mut.exception_clear();
                        __jni_match_exceptions!($env_mut, e, $($ex_handlers)*)
                    },
                )
            } else {
                Ok(ret)
            }
        })())
    });
}

/// Calls a Env function, then catches (checks + clears) any pending exception, then checks for null
///
/// This behaves the same as `jni_call_with_catch!`, but additionally checks if the result is null
/// and returns `Err(Error::NullPtr)` in that case.
///
/// Use this when a `null` return value represents an error condition and `null` may be returned
/// without any exception being thrown.
macro_rules! jni_call_with_catch_and_null_check {
    ( catch |$env_mut:ident| { $($ex_handlers:tt)* }, $jnienv:expr, $version:tt, $name:ident $(, $args:expr )* ) => ({
        jni_call_with_catch! {
            catch |$env_mut| { $($ex_handlers)* },
            $jnienv,
            $version,
            $name $(, $args)*
        }.and_then(|ret| {
            if ret.is_null() {
                Err($crate::errors::Error::NullPtr(concat!(stringify!($name), " result")))
            } else {
                Ok(ret)
            }
        })
    });
}

/// Wrap a block of JNI code into a closure and then catch and handle exceptions
/// after running
///
/// This macro takes a try block of code to run followed by a `catch |env| {}`
/// block that specifies how all possible exceptions should be mapped to
/// `Result` values:
///
/// ```ignore
/// jni_try!(
///     (env) -> ResultType {
///         // JNI code here
///     },
///     catch |env| {
///         ExceptionType1 => Err(Error::Type1),
///         exception: ExceptionType2 => {
///             let cause = exception.get_cause(env)?;
///             let cause = env.new_global_ref(cause)?;
///             Err(Error::Type2(cause))
///         },
///         // required catch-all
///         else => Err(Error::JniCall(JniError::Unknown)),
///     },
/// )
/// ```
///
/// The exception is always cleared before matching or evaluating any handler
/// expression.
///
/// Any exception thrown is matched against the exception types in order (via
/// its `::matches` method, which will also `Cast` the exception if it does
/// match).
///
/// The expression of the first matching handler is returned as the `Result`
/// value.
///
/// A required `else => expr` entry at the end acts as a catch-all for any
/// exception not claimed by a typed handler.
///
/// When an exception is matched then it will also be `Cast` to the matched
/// exception type and can be given a name that is accessible to right-hand
/// expressions (e.g. `exception_name: ExceptionType => { ... }`).
///
/// Note: The catch expressions always have access to an `env: &mut Env`
/// reference (regardless of whether the initial `Env` reference passed to the
/// macro was mutable) - since the mapping is always done within a
/// `with_local_frame` closure. This `env` reference is named based on the
/// `catch |env|` syntax in the macro invocation.
macro_rules! jni_try {
    ( ($jnienv:expr) -> $ret_ty:ty { $($jni_code:tt)* } catch |$env_mut:ident| { $($ex_handlers:tt)* }) => ({
        // Wrap everything in a closure so any use of '?' can't return from the encompassing function.
        let ret = (|| -> $ret_ty {$($jni_code)*})();
        if $jnienv.exception_check() {
            // Get a `&mut Env` we can use while mapping the exception to a `Result`
            $jnienv.with_local_frame(
                $crate::DEFAULT_LOCAL_FRAME_CAPACITY,
                |$env_mut| -> $ret_ty {
                    let e = $env_mut
                        .exception_occurred()
                        .expect("Expected an exception after ExceptionCheck");
                    $env_mut.exception_clear();
                    __jni_match_exceptions!($env_mut, e, $($ex_handlers)*)
                },
            )
        } else {
            ret
        }
    });
}

/// Maps a pointer to either Ok(ptr) or Err(Error::NullPtr)
///
/// This makes it reasonably ergonomic to use `?` to early-exit with an `Err` in
/// case of `null` pointer arguments.
///
/// Unlike earlier macros this avoids using `return`, since that can result in
/// surprising control flow if the caller doesn't realize that a macro might
/// explicitly return from the current function.
macro_rules! null_check {
    ( $obj:expr, $ctx:expr ) => {
        if $obj.is_null() {
            Err($crate::errors::Error::NullPtr($ctx))
        } else {
            Ok($obj)
        }
    };
}

/// Directly calls a JavaVM function, nothing else
///
/// # Safety
///
/// You must ensure that the arguments you pass are valid for the particular JNI
/// function you are calling.
macro_rules! java_vm_call_unchecked {
    ( $jvm:expr, $version:tt, $name:ident $(, $args:expr )*) => {{
        // Safety: we know that the pointer can't be null, since that's
        // checked in `from_raw()`
        let jvm: *mut jni_sys::JavaVM = $jvm.get_raw();
        ((*(*jvm)).$version.$name)(jvm $(, $args)*)
    }};
}

#[cfg(test)]
mod tests {
    use crate::{
        errors::{Error, JniError},
        objects::JThrowable,
    };

    /// Smoke test that some example usage of `jni_call_with_catch_and_null_check!` compiles.
    ///
    /// `name` and `sig` represent the method/field name and descriptor that would be in scope when
    /// calling a lookup JNI function.
    ///
    /// This tries to test plausible mappings for all the exception types we have bindings for.
    fn _compile_test_jni_call_with_catch_and_null_check(
        env: &mut crate::Env<'_>,
        name: &crate::strings::JNIStr,
        sig: &crate::strings::JNIStr,
    ) -> crate::errors::Result<crate::sys::jclass> {
        unsafe {
            jni_call_with_catch_and_null_check!(
                catch |env| {
                    crate::exceptions::JOutOfMemoryError =>
                        Err(Error::JniCall(JniError::NoMemory)),

                    crate::exceptions::JClassFormatError =>
                        Err(Error::ClassFormatError),

                    crate::exceptions::JClassCircularityError =>
                        Err(Error::ClassCircularityError),

                    e: crate::exceptions::JClassNotFoundException => {
                        let cause: &JThrowable = &e.as_throwable();
                        let cause = env.new_global_ref(cause).ok();
                        Err(Error::NoClassDefFound {
                            requested: name.to_string(),
                            cause
                        })
                    },
                    e: crate::exceptions::JNoClassDefFoundError => {
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

                    crate::exceptions::JNoSuchMethodError =>
                        Err(Error::MethodNotFound {
                            name: name.to_string(),
                            sig: sig.to_string(),
                        }),

                    crate::exceptions::JNoSuchFieldError =>
                        Err(Error::FieldNotFound {
                            name: name.to_string(),
                            sig: sig.to_string(),
                        }),

                    crate::exceptions::JArrayStoreException =>
                        Err(Error::WrongObjectType),

                    crate::exceptions::JIllegalArgumentException =>
                        Err(Error::JniCall(JniError::InvalidArguments)),

                    crate::exceptions::JIllegalMonitorStateException =>
                        Err(Error::IllegalMonitorState),

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

                    crate::exceptions::JArrayIndexOutOfBoundsException =>
                        Err(Error::IndexOutOfBounds),

                    crate::exceptions::JStringIndexOutOfBoundsException =>
                        Err(Error::IndexOutOfBounds),

                    crate::exceptions::JInstantiationException =>
                        Err(Error::Instantiation),

                    crate::exceptions::JNumberFormatException =>
                        Err(Error::ParseFailed(String::new())),

                    else => Err(Error::NullPtr("Unexpected Exception"))
                    //else => Err(Error::JniCall(JniError::Unknown))
                },
                env,
                v1_1,
                FindClass,
                core::ptr::null()
            )
        }
    }

    fn _compile_test_jni_try(env: &mut crate::Env<'_>) -> crate::errors::Result<()> {
        let _res = jni_try! {
            (env) -> crate::errors::Result<crate::objects::JString> {
                let s = env.new_string("String")?;

                {
                    let msg = env.new_string("Test")?;
                    let e = crate::exceptions::JNumberFormatException::new(env, msg)?;
                    let e: crate::objects::JThrowable = e.into();
                    env.throw(e)?;
                }

                Ok(s)
            } catch |env| {
                crate::exceptions::JNumberFormatException => Err(Error::ParseFailed(String::new())),
                else => Err(Error::NullPtr("Test"))
            }
        }?;

        Ok(())
    }
}
