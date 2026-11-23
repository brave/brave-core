use std::panic::{AssertUnwindSafe, catch_unwind};

use crate::Env;

#[cfg(doc)]
use crate::{EnvOutcome, Outcome, errors::Error};

/// A policy for handling [`EnvOutcome`] errors and panics that may occur within a native method.
///
/// This trait allows customization of error handling strategies, such as throwing Java exceptions,
/// logging errors, or returning error codes. Implementors can define how to respond to errors and
/// panics, providing flexibility in managing native method outcomes.
///
/// Specify a policy by using the `resolve` or `resolve_with` methods on [`EnvOutcome`].
///
/// Some standard policies are provided in this crate, such as [`ThrowRuntimeExAndDefault`] which throws
/// a Java exception for any Rust error or panic, returning `null` or `0` as a default value.
///
/// For example use like:
/// ```rust,no_run
/// # use jni::{Env, EnvUnowned, EnvOutcome};
/// # use jni::objects::JObject;
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_HelloWorld_hello<'local>(mut unowned_env: EnvUnowned<'local>) -> JObject<'local> {
///     unowned_env.with_env(|env| -> jni::errors::Result<JObject> {
///         // do stuff that might fail or panic
///         Ok(JObject::null()) // placeholder
///     }).resolve::<jni::errors::ThrowRuntimeExAndDefault>()
/// }
/// ```
///
/// In some situations your error or panic handling may need to capture some state from the native
/// method, including references associated with the local reference frame.
///
/// To capture state your policy can define the associated type `Captures` to be some type
/// that can borrow from the JNI local reference frame and from the native method scope itself.
///
/// If your policy needs to capture state then you would use `resolve_with` to provide a closure
/// that builds the captures.
///
/// For example, implement a policy that captures state like:
/// ```rust,no_run
/// # use jni::{Env, EnvUnowned, EnvOutcome};
/// # use jni::objects::JObject;
/// # use jni::errors::{Error, ErrorPolicy};
/// struct CustomPolicyCaptures<'local, 'native_method>
/// where
///     'local: 'native_method,
/// {
///     context: &'native_method JObject<'local>, // capture a local reference
/// }
///
/// struct CustomPolicy;
///
/// impl<T: Default, E: std::error::Error> jni::errors::ErrorPolicy<T, E> for CustomPolicy {
///     type Captures<'unowned_env_local: 'native_method, 'native_method> = CustomPolicyCaptures<'unowned_env_local, 'native_method>;
///     fn on_error<'unowned_env_local: 'native_method, 'native_method>(
///         env: &mut Env<'unowned_env_local>,
///         cap: &mut CustomPolicyCaptures<'unowned_env_local, 'native_method>,
///         err: E,
///     ) -> jni::errors::Result<T> {
///         // Handle the error, possibly throwing a Java exception
///         eprintln!("Error: {:?}", err);
///         // Return a default value or take other appropriate action
///         Ok(T::default())
///     }
///     fn on_panic<'unowned_env_local: 'native_method, 'native_method>(
///         env: &mut Env<'unowned_env_local>,
///         cap: &mut CustomPolicyCaptures<'unowned_env_local, 'native_method>,
///         payload: Box<dyn std::any::Any + Send + 'static>,
///     ) -> jni::errors::Result<T> {
///         // Handle the panic, possibly throwing a Java exception
///         eprintln!("Panic: {:?}", payload);
///         // Return a default value or take other appropriate action
///         Ok(T::default())
///     }
/// }
///
/// // Then use the policy in a native method, capturing state like:
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_HelloWorld_hello<'local>(
///     mut unowned_env: EnvUnowned<'local>,
///     _this: JObject<'local>,
///     context: JObject<'local>,
/// ) -> JObject<'local> {
///    unowned_env.with_env(|env| -> jni::errors::Result<JObject> {
///       // do stuff that might fail or panic
///       Ok(JObject::null()) // placeholder
///    }).resolve_with::<CustomPolicy, _>(|| {
///       // capture state from the native method scope or from the JNI local reference frame
///       CustomPolicyCaptures::<'local, '_> {
///           context: &context, // capture a local reference
///       }
///   })
/// }
/// ```
pub trait ErrorPolicy<T, E> {
    /// Per-call captures; may borrow from the JNI local reference frame
    /// associated with the native method and from the native method scope
    /// itself.
    type Captures<'unowned_env_local: 'native_method, 'native_method>;

    /// Runs for any [`Outcome::Err`].
    ///
    /// This must map the error to some value that can be returned by the native
    /// method and may be used to throw a Java exception or log the error.
    ///
    /// If this returns an `Err` then `on_internal_jni_error` will be called.
    ///
    /// If this panics then `on_internal_panic` will be called.
    fn on_error<'unowned_env_local: 'native_method, 'native_method>(
        env: &mut Env<'unowned_env_local>,
        cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        err: E,
    ) -> crate::errors::Result<T>;

    /// Runs for any [`Outcome::Panic`].
    ///
    /// This must return some value that can be returned by the native
    /// method and can be used to throw a Java exception or log the panic
    ///
    /// If this returns an `Err` then `on_internal_jni_error` will be called.
    ///
    /// If this panics then `on_internal_panic` will be called.
    fn on_panic<'unowned_env_local: 'native_method, 'native_method>(
        env: &mut Env<'unowned_env_local>,
        captures: &mut Self::Captures<'unowned_env_local, 'native_method>,
        payload: Box<dyn std::any::Any + Send + 'static>,
    ) -> crate::errors::Result<T>;

    /// Runs if a JNI error occurs within `on_error` or `on_panic`.
    fn on_internal_jni_error<'unowned_env_local: 'native_method, 'native_method>(
        _captures: &mut Self::Captures<'unowned_env_local, 'native_method>,
        err: crate::errors::Error,
    ) -> T
    where
        T: Default,
    {
        log::error!(
            "Secondary failure while handling error or panic in native method: {:?}",
            err
        );
        T::default()
    }

    /// Runs if we panic within `on_error`, `on_panic` or `on_internal_jni_error`
    fn on_internal_panic<'unowned_env_local: 'native_method, 'native_method>(
        _captures: &mut Self::Captures<'unowned_env_local, 'native_method>,
        _payload: Box<dyn std::any::Any + Send + 'static>,
    ) -> T
    where
        T: Default,
    {
        log::error!("Last resort: panic while handling error or panic in native method");
        T::default()
    }
}

/// An error policy that throws `java.lang.RuntimeException` for any Rust error
/// or panic, returning `null` or `0` as a default value.
///
/// If an exception is already pending when an error or panic occurs then that
/// takes precedence and no new exception will be thrown and a default value
/// will be returned.
///
/// Note: pending exceptions are determined by calling [`Env::exception_check`],
/// and not by checking the error type since this is generic over all error
/// types and has no way to downcast to check for [`Error::JavaException`].
///
/// For example use like:
/// ```rust,no_run
/// # use jni::{Env, EnvUnowned, EnvOutcome};
/// # use jni::objects::JObject;
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_HelloWorld_hello<'local>(
///     mut unowned_env: EnvUnowned<'local>,
///     _this: JObject<'local>,
/// ) -> JObject<'local> {
///     unowned_env.with_env(|env| -> jni::errors::Result<JObject> {
///         // do stuff that might fail or panic
///         Ok(JObject::null()) // placeholder
///     }).resolve::<jni::errors::ThrowRuntimeExAndDefault>()
/// }
/// ```
#[derive(Debug, Default)]
pub struct ThrowRuntimeExAndDefault;

impl<T: Default, E: std::error::Error> ErrorPolicy<T, E> for ThrowRuntimeExAndDefault {
    type Captures<'unowned_env_local: 'native_method, 'native_method> = (); // no captures

    fn on_error<'unowned_env_local: 'native_method, 'native_method>(
        env: &mut Env<'unowned_env_local>,
        _cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        err: E,
    ) -> crate::errors::Result<T> {
        if env.exception_check() {
            return Ok(T::default()); // already thrown
        }
        let err_string = format!("Rust error: {}", err);
        // Note: `env.throw()` will return `Err(Error::JavaException)` after throwing but in this case
        // (where we are going to be letting the exception propagate to Java), we want to ensure we
        // don't return that as an error
        let _ = env.throw(err_string);
        Ok(T::default())
    }

    fn on_panic<'unowned_env_local: 'native_method, 'native_method>(
        env: &mut Env<'unowned_env_local>,
        _cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        payload: Box<dyn std::any::Any + Send + 'static>,
    ) -> crate::errors::Result<T> {
        let panic_string = match payload.downcast::<&'static str>() {
            Ok(s) => (*s).to_string(),
            Err(payload) => {
                // Since it's possible that dropping a panic payload may itself panic,
                // we catch any panic and fallback to forgetting/leaking the payload.
                if let Err(drop_panic) = catch_unwind(AssertUnwindSafe(|| drop(payload))) {
                    log::error!("Panic while dropping panic payload: {:?}", drop_panic);
                    std::mem::forget(drop_panic);
                }
                "non-string panic payload".to_string()
            }
        };

        // Note: `env.throw()` will return `Err(Error::JavaException)` after throwing but in this case
        // (where we are going to be letting the exception propagate to Java), we want to ensure we
        // don't return that as an error
        let _ = env.throw(format!("Rust panic: {}", panic_string));
        Ok(T::default())
    }
}

/// An error policy that logs errors and panics before returning a default value.
///
/// Error logs and panic messages are formatted like: "Rust error: {message}" or
/// "Rust panic: {message}" before returning a default value.
#[derive(Debug, Default)]
pub struct LogErrorAndDefault;

impl<T: Default, E: std::error::Error> ErrorPolicy<T, E> for LogErrorAndDefault {
    type Captures<'unowned_env_local: 'native_method, 'native_method> = (); // no captures

    fn on_error<'unowned_env_local: 'native_method, 'native_method>(
        _env: &mut Env<'unowned_env_local>,
        _cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        err: E,
    ) -> crate::errors::Result<T> {
        log::error!("Rust error: {}", err);
        Ok(T::default())
    }

    fn on_panic<'unowned_env_local: 'native_method, 'native_method>(
        _env: &mut Env<'unowned_env_local>,
        _cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        payload: Box<dyn std::any::Any + Send + 'static>,
    ) -> crate::errors::Result<T> {
        let panic_string = match payload.downcast::<&'static str>() {
            Ok(s) => (*s).to_string(),
            Err(payload) => {
                // Since it's possible that dropping a panic payload may itself panic,
                // we catch any panic and fallback to forgetting/leaking the payload.
                if let Err(drop_panic) = catch_unwind(AssertUnwindSafe(|| drop(payload))) {
                    log::error!("Panic while dropping panic payload: {:?}", drop_panic);
                    std::mem::forget(drop_panic);
                }
                "non-string panic payload".to_string()
            }
        };
        log::error!("Rust panic: {}", panic_string);
        Ok(T::default())
    }
}

/// An error policy that logs errors and panics along with a context string
/// before returning a default value.
///
/// For example it can be used like:
/// ```rust,no_run
/// # use jni::{Env, EnvUnowned, EnvOutcome};
/// # use jni::objects::{JClass, JObject, JString};
/// # use jni::errors::{Error, ErrorPolicy};
/// #[unsafe(no_mangle)]
/// pub extern "system" fn Java_HelloWorld_logErrorWithContextString<'local>(
///     mut unowned_env: EnvUnowned<'local>,
///     _this: JObject<'local>,
///     context: JObject<'local>,
///     arg: JString<'local>,
/// ) -> JClass<'local> {
///    unowned_env.with_env(|env| -> jni::errors::Result<JClass> {
///       // do stuff that might fail or panic
///       let class = env.get_object_class(&context)?;
///       Ok(class) // placeholder
///    }).resolve_with::<jni::errors::LogContextErrorAndDefault, _>(|| {
///       format!("In 'logErrorWithContextString' with arg: {arg}")
///   })
/// }
/// ```
///
/// Error logs and panic messages are formatted like: "{context}: {message}"
/// before returning a default value.
#[derive(Debug, Default)]
pub struct LogContextErrorAndDefault;

impl<T: Default, E: std::error::Error> ErrorPolicy<T, E> for LogContextErrorAndDefault {
    type Captures<'unowned_env_local: 'native_method, 'native_method> = String;

    fn on_error<'unowned_env_local: 'native_method, 'native_method>(
        _env: &mut Env<'unowned_env_local>,
        cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        err: E,
    ) -> crate::errors::Result<T> {
        log::error!("{cap}: {err}");
        Ok(T::default())
    }

    fn on_panic<'unowned_env_local: 'native_method, 'native_method>(
        _env: &mut Env<'unowned_env_local>,
        cap: &mut Self::Captures<'unowned_env_local, 'native_method>,
        payload: Box<dyn std::any::Any + Send + 'static>,
    ) -> crate::errors::Result<T> {
        let panic_string = match payload.downcast::<&'static str>() {
            Ok(s) => (*s).to_string(),
            Err(payload) => {
                // Since it's possible that dropping a panic payload may itself panic,
                // we catch any panic and fallback to forgetting/leaking the payload.
                if let Err(drop_panic) = catch_unwind(AssertUnwindSafe(|| drop(payload))) {
                    log::error!("Panic while dropping panic payload: {:?}", drop_panic);
                    std::mem::forget(drop_panic);
                }
                "non-string panic payload".to_string()
            }
        };
        log::error!("{cap}: {panic_string}");
        Ok(T::default())
    }
}

// Smoke test implementation for a custom policy that captures a local reference
#[cfg(test)]
mod tests {
    use crate::{
        EnvUnowned,
        objects::{JClass, JObject, JString},
    };

    use super::*;
    struct TestCustomPolicyCaptures<'local, 'native_method>
    where
        'local: 'native_method,
    {
        context: &'native_method JObject<'local>, // capture a local reference
    }

    struct TestCustomPolicy;

    impl<T: Default, E: std::error::Error> ErrorPolicy<T, E> for TestCustomPolicy {
        type Captures<'unowned_env_local: 'native_method, 'native_method> =
            TestCustomPolicyCaptures<'unowned_env_local, 'native_method>;

        fn on_error<'unowned_env_local: 'native_method, 'native_method>(
            _env: &mut Env<'unowned_env_local>,
            cap: &mut TestCustomPolicyCaptures<'unowned_env_local, 'native_method>,
            err: E,
        ) -> crate::errors::Result<T> {
            // Handle the error, possibly throwing a Java exception
            eprintln!("Error: {:?}, context: {:?}", err, cap.context);
            // You can access cap.context here
            Ok(T::default())
        }

        fn on_panic<'unowned_env_local: 'native_method, 'native_method>(
            _env: &mut Env<'unowned_env_local>,
            _cap: &mut TestCustomPolicyCaptures<'unowned_env_local, 'native_method>,
            payload: Box<dyn std::any::Any + Send + 'static>,
        ) -> crate::errors::Result<T> {
            // Handle the panic, possibly throwing a Java exception
            eprintln!("Panic: {:?}", payload);
            // You can access cap.context here
            Ok(T::default())
        }
    }

    #[unsafe(no_mangle)]
    pub extern "system" fn Java_HelloWorld_test<'local>(
        mut unowned_env: EnvUnowned<'local>,
        _this: JObject<'local>,
        context: JObject<'local>,
    ) -> JClass<'local> {
        unowned_env
            .with_env(|env| -> crate::errors::Result<_> {
                // do stuff that might fail or panic
                // for the sake of testing, use the context that will also be captured
                let class = env.get_object_class(&context)?;
                Ok(class)
            })
            .resolve_with::<TestCustomPolicy, _>(|| {
                // capture state from the native method scope or from the JNI local reference frame
                TestCustomPolicyCaptures::<'local, '_> {
                    context: &context, // capture a local reference
                }
            })
    }

    #[unsafe(no_mangle)]
    pub extern "system" fn Java_HelloWorld_logErrorWithContextString<'local>(
        mut unowned_env: EnvUnowned<'local>,
        _this: JObject<'local>,
        context: JObject<'local>,
        arg: JString<'local>,
    ) -> JClass<'local> {
        unowned_env
            .with_env(|env| -> crate::errors::Result<_> {
                // do stuff that might fail or panic
                // for the sake of testing, use the context that will also be captured
                let class = env.get_object_class(&context)?;
                Ok(class)
            })
            .resolve_with::<LogContextErrorAndDefault, _>(|| {
                format!("In 'logErrorWithContextString' with arg: {arg}")
            })
    }
}
