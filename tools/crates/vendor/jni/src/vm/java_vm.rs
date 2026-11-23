use std::{
    cell::Cell,
    os::raw::c_char,
    ptr,
    sync::atomic::{AtomicUsize, Ordering},
    thread::{Thread, current},
};

use log::debug;

use crate::{
    JNIVersion,
    env::Env,
    errors::*,
    objects::{Global, JObject, JThrowable, Reference},
    strings::{JNIStr, JNIString},
    sys,
};

#[cfg(all(feature = "invocation", not(target_os = "android")))]
use std::{os::raw::c_void, path::PathBuf};
#[cfg(feature = "invocation")]
use {crate::InitArgs, std::ffi::OsStr};

#[cfg(use_fls_attach_guard)]
use super::fls_attach_guard;
#[cfg(use_tls_attach_guard)]
use super::tls_attach_guard;

#[cfg(doc)]
use {
    crate::env::{self, EnvUnowned},
    crate::objects,
};

/// The capacity of local frames, allocated for attached threads by default. Same as the default
/// value Hotspot uses when calling native Java methods.
pub const DEFAULT_LOCAL_FRAME_CAPACITY: usize = 32;

/// The `jni-rs` crate makes the assumption that it's not possible to create more than one Java VM
/// per-process, or even re-initialize a JavaVM that is "destroyed".
///
/// This allows us to save a global pointer for the JavaVM.
///
/// We also guarantee that if you currently have an [`AttachGuard`] thread attachment (or a `Env`
/// reference), that implies that [`JavaVM::singleton()`] has been initialized and will return a
/// valid [`JavaVM`].
///
/// For example, this guarantee is relied on internally to avoid redundantly saving JavaVM pointers
/// if know we can assume that `JavaVM::singleton()` will return a `JavaVM` when needed.
static JAVA_VM_SINGLETON: std::sync::OnceLock<JavaVM> = std::sync::OnceLock::new();

/// The Java VM API, including (optional) [Invocation API][invocation-api] support.
///
/// An existing JavaVM can be obtained either via [`JavaVM::singleton`], or [`Env::get_java_vm`]
/// in an already attached thread, or a new VM can be [launched](#launching-jvm-from-rust) via
/// [`JavaVM::new`].
///
/// ## Minimum supported JNI version is 1.4
///
/// The implementation of this crate assumes your Java VM supports at least JNI >= 1.4
///
/// The implementation wouldn't be able to call `ExceptionCheck` without JNI 1.2 and requiring >=
/// 1.4 means we don't need any runtime version checks for the direct byte buffer APIs.
///
/// Since `GetVersion` requires a [`Env`] and is not one of the JNI APIs that is safe to use with
/// pending exceptions then the implementation is not always able to explicitly assert the supported
/// version.
///
/// ## Attaching Native Threads
///
/// Your application always needs to explicitly attach `jni-rs` to the current thread before it can
/// access the [`Env`] API (most of the interesting APIs are under [`Env`]).
///
/// If you're implementing a native/foreign method then JNI will pass a thread attachment (in the
/// form of a raw [`sys::JNIEnv`] pointer that should be captured using the
/// [`env::EnvUnowned`] type) and converted into a [`Env`] reference via
/// [`env::EnvUnowned::with_env`])
///
/// Note: [`Env`] is not a `#[transparent]` wrapper over a [`sys::JNIEnv`] pointer.
///
/// The attachment of the current thread is always represented via an [`AttachGuard`] which blocks
/// the thread from being detached and acts as a marker for a JNI stack frame.
///
/// [`Env`] is only ever exposed in the public API by-reference, and will always borrow from an
/// [`AttachGuard`] such that it's lifetime is tied to a fixed JNI stack frame.
///
/// Unless you are using `unsafe` APIs though, the [`AttachGuard`] itself will usually be hidden,
/// and you will get a [`Env`] reference (that borrows from a hidden [`AttachGuard`]).
///
/// This library supports these modes of attachment:
/// * A permanent attachment with [`JavaVM::attach_current_thread`]. The thread will automatically
///   detach itself before it terminates (recommended for attaching in native threads).
/// * A scoped attachment with [`JavaVM::attach_current_thread_for_scope`]. The thread will
///   automatically detach itself once your given closure returns (and the hidden [`AttachGuard`]
///   is dropped).
/// * Implicit ("unowned") attachments, for use in native methods. The thread will never get
///   explicitly detached by `jni-rs` if we have implicit attachment.
///
/// ### Local Reference Management
///
/// Remember that the native thread attached to the VM must manage local references
/// carefully, i.e., do not allocate an excessive number of references and release them promptly when
/// they are no longer needed to enable the GC to collect them.
///
/// A common approach is to push appropriately-sized local frames for larger
/// code fragments (see [`Env::with_local_frame`] or [`JavaVM::with_local_frame`])
/// and [`objects::Auto`] for temporary references in loops.
///
/// See also the [JNI specification][spec-references] for details on referencing Java objects.
///
/// ## Launching JVM from Rust
///
/// To [launch][launch-vm] a JVM from a native process, enable the `invocation` feature in the
/// Cargo.toml:
///
/// ```toml
/// jni = { version = "0.22.1", features = ["invocation"] }
/// ```
///
/// The application will be able to use [`JavaVM::new`] which will dynamically load a `jvm` library
/// (which is distributed with the JVM) at runtime:
///
/// ```rust,ignore-aarch64-linux-android
/// # use jni::errors;
/// # //
/// # // Ignore this test without invocation feature, so that simple `cargo test` works
/// # #[cfg(feature = "invocation")]
/// # fn main() -> errors::StartJvmResult<()> {
/// # use jni::{jni_sig, jni_str, AttachGuard, objects::JValue, InitArgsBuilder, Env, JNIVersion, JavaVM, sys::jint};
/// # //
/// // Build the VM properties
/// let jvm_args = InitArgsBuilder::new()
///           // Pass the JNI API version (default is 8)
///           .version(JNIVersion::V1_8)
///           // You can additionally pass any JVM options (standard, like a system property,
///           // or VM-specific).
///           // Here we enable some extra JNI checks useful during development
///           .option("-Xcheck:jni")
///           .build()
///           .unwrap();
///
/// // Create a new VM
/// let jvm = JavaVM::new(jvm_args)?;
///
/// // Attach the current thread to call to the JavaVM
/// jvm.attach_current_thread(|env| -> errors::Result<()> {
///     // Call Java Math#abs(-10)
///     let x = JValue::from(-10);
///     let val: jint = env.call_static_method(jni_str!("java/lang/Math"), jni_str!("abs"), jni_sig!("(I)I"), &[x])?
///         .i()?;
///
///     assert_eq!(val, 10);
///     Ok(())
/// });
///
/// # Ok(()) }
/// #
/// # // This is a stub that gets run instead if the invocation feature is not built
/// # #[cfg(not(feature = "invocation"))]
/// # fn main() {}
/// ```
///
/// At runtime, the JVM installation path is determined via the [java-locator] crate:
/// 1. By the `JAVA_HOME` environment variable, if it is set.
/// 2. Otherwise — from `java` output.
///
/// It is recommended to set `JAVA_HOME`
///
/// For the operating system to correctly load the `jvm` library it may also be necessary to update
/// the path that the OS uses to find dependencies of the `jvm` library.
/// * On **Windows**, append the path to `$JAVA_HOME/bin` to the `PATH` environment variable.
/// * On **MacOS**, append the path to `libjvm.dylib` to `LD_LIBRARY_PATH` environment variable.
/// * On **Linux**, append the path to `libjvm.so` to `LD_LIBRARY_PATH` environment variable.
///
/// The exact relative path to `jvm` library is version-specific.
///
/// [invocation-api]: https://docs.oracle.com/en/java/javase/12/docs/specs/jni/invocation.html
/// [get-vm]: struct.Env.html#method.get_java_vm
/// [launch-vm]: struct.JavaVM.html#method.new
/// [act]: struct.JavaVM.html#method.attach_current_thread
/// [actp]: struct.JavaVM.html#method.attach_current_thread_permanently
/// [spec-references]:
///     https://docs.oracle.com/en/java/javase/12/docs/specs/jni/design.html#referencing-java-objects
/// [java-locator]: https://crates.io/crates/java-locator
#[derive(Debug, Clone)]
pub struct JavaVM(*mut sys::JavaVM);

unsafe impl Send for JavaVM {}
unsafe impl Sync for JavaVM {}

impl JavaVM {
    /// Launch a new JavaVM using the provided init args.
    ///
    /// Unlike original JNI API, the main thread (the thread from which this method is called) will
    /// not be attached to JVM. You must explicitly use `attach_current_thread…` methods (refer
    /// to [Attaching Native Threads section](#attaching-native-threads)).
    ///
    /// *This API requires the "invocation" feature to be enabled,
    /// see ["Launching JVM from Rust"](struct.JavaVM.html#launching-jvm-from-rust).*
    ///
    /// This will attempt to locate a JVM using
    /// [java-locator], if the JVM has not already been loaded. Use the
    /// [`with_libjvm`][Self::with_libjvm] method to give an explicit location for the JVM shared
    /// library (`jvm.dll`, `libjvm.so`, or `libjvm.dylib`, depending on the platform).
    #[cfg(feature = "invocation")]
    pub fn new(args: InitArgs) -> StartJvmResult<Self> {
        #[cfg(not(target_os = "android"))]
        {
            Self::with_libjvm(args, || {
                Ok([
                    java_locator::locate_jvm_dyn_library()
                        .map_err(StartJvmError::NotFound)?
                        .as_str(),
                    java_locator::get_jvm_dyn_lib_file_name(),
                ]
                .iter()
                .collect::<PathBuf>())
            })
        }

        #[cfg(target_os = "android")]
        {
            let _a = args;
            Err(StartJvmError::Unsupported)
        }
    }

    /// Get a [`JavaVM`] for the global Java VM
    ///
    /// If no [`JavaVM`] has been initialized, this will return [`Error::UninitializedJavaVM`].
    ///
    /// If a [`JavaVM`] has previously been created, either via [`JavaVM::new()`] or
    /// [`JavaVM::from_raw`] then that [`JavaVM`] will be accessible as a global singleton.
    ///
    /// This is possible because JNI does not support fully destroying a Java VM and then
    /// initializing a new one and so as soon as we have seen a Java VM pointer once, we know it's
    /// the only VM that will ever exist and it will always be valid in safe code.
    ///
    /// If your code observes a [`Env`] reference or an [`AttachGuard`] (from this crate version)
    /// then you can assume that [`JavaVM::singleton()`] has been initialized.
    ///
    /// Beware that the observation of reference types (such as [`crate::objects::JObject`]) only
    /// imply that [`JavaVM::singleton()`] has been initialized if the references are non-null.
    ///
    /// One other caveat is that native methods may capture reference type arguments, such as
    /// [`JObject`], where their lifetime is _not_ tied to a real `Env`. (And so at the start of
    /// a native method, [`JavaVM::singleton()`] may not be initialized even though we can observe
    /// reference types).
    ///
    /// In practice though, you can usually assume [`JavaVM::singleton()`] has been initialized
    /// if you observe non-null reference types, based on the assumption that:
    ///
    /// - Before any other `jni-rs` API is used, a native method is expected to use
    ///   [`env::EnvUnowned::with_env`] to get a `Env` reference, which will initialize
    ///   [`JavaVM::singleton()`].
    /// - For any native method implementation to be safe, it must use `catch_unwind` (e.g. via
    ///   [`env::EnvUnowned::with_env`]) to ensure that panics can't unwind over an FFI boundary
    ///   (at least rendering an early miss-use of `JavaVM::singleton()` "safe").
    ///
    /// Note: that other versions of `jni-rs` within the same application aren't able to share this
    /// singleton state. So you should not make assumptions about this being initialized as a side
    /// effect of other dependencies using `jni-rs` (unless you are using a re-exported version of
    /// `jni-rs` from that dependency). For example the `android-activity` crate will initialize a
    /// [JavaVM] before `android_main()` is called, but unless you are using the same version of
    /// `jni-rs` as `android-activity` you can't immediately assume there is a [JavaVM] singleton.
    pub fn singleton() -> Result<Self> {
        JAVA_VM_SINGLETON
            .get()
            .cloned()
            .ok_or(Error::UninitializedJavaVM)
    }

    #[cfg(all(feature = "invocation", not(target_os = "android")))]
    unsafe fn with_create_fn_ptr(
        args: InitArgs,
        create_fn_ptr: unsafe extern "system" fn(
            pvm: *mut *mut sys::JavaVM,
            penv: *mut *mut c_void,
            args: *mut c_void,
        ) -> sys::jint,
    ) -> Result<Self> {
        let mut ptr: *mut sys::JavaVM = ::std::ptr::null_mut();
        let mut env: *mut sys::JNIEnv = ::std::ptr::null_mut();

        unsafe {
            jni_error_code_to_result(create_fn_ptr(
                &mut ptr as *mut _,
                &mut env as *mut *mut sys::JNIEnv as *mut *mut c_void,
                args.inner_ptr(),
            ))?;
        }

        let vm = unsafe { Self::from_raw(ptr) };

        // JNI_CreateJavaVM will implicitly attach the calling thread to the JVM.
        //
        // Since the JVM may attribute this thread with special significance as a "main" thread, we
        // avoid detaching it.
        //
        // Instead we take ownership of that attachment by creating a `TLSAttachGuard` for it.
        //
        // Note: This will make a redundant `AttachCurrentThread` call via
        // `sys_attach_current_thread` and the `Default` `config` is benign here because it will be
        // ignored while the thread is already attached.
        //
        // This will track the new attachment in TLS/FLS
        #[cfg(use_tls_attach_guard)]
        unsafe {
            tls_attach_guard::tls_attach_current_thread(&vm, &Default::default())?
        };
        #[cfg(use_fls_attach_guard)]
        unsafe {
            fls_attach_guard::fls_attach_current_thread(&vm, &Default::default())?
        };

        Ok(vm)
    }

    /// Launch a new JavaVM using the provided init args, loading it from the given shared library file if it's not already loaded.
    ///
    /// Unlike original JNI API, the main thread (the thread from which this method is called) will
    /// not be attached to JVM. You must explicitly use `attach_current_thread…` methods (refer
    /// to [Attaching Native Threads section](#attaching-native-threads)).
    ///
    /// *This API requires the "invocation" feature to be enabled,
    /// see ["Launching JVM from Rust"](struct.JavaVM.html#launching-jvm-from-rust).*
    ///
    /// The `libjvm_path` parameter takes a *closure* which returns the path to the JVM shared
    /// library. The closure is only called if the JVM is not already loaded. Any work that needs
    /// to be done to locate the JVM shared library should be done inside that closure.
    #[cfg(all(feature = "invocation", not(target_os = "android")))]
    fn impl_with_libjvm<P: AsRef<OsStr>>(
        args: InitArgs,
        libjvm_path: impl FnOnce() -> StartJvmResult<P>,
    ) -> StartJvmResult<Self> {
        if let Some(jvm) = JAVA_VM_SINGLETON.get() {
            Ok(jvm.clone())
        } else {
            // Determine the path to the shared library.
            let libjvm_path = libjvm_path()?;
            let libjvm_path_string = libjvm_path.as_ref().to_string_lossy().into_owned();

            // Try to load it.
            let libjvm = match unsafe { libloading::Library::new(libjvm_path.as_ref()) } {
                Ok(ok) => ok,
                Err(error) => return Err(StartJvmError::LoadError(libjvm_path_string, error)),
            };

            let result = unsafe {
                // Try to find the `JNI_CreateJavaVM` function in the loaded library.
                let create_fn = libjvm.get(b"JNI_CreateJavaVM\0").map_err(|error| {
                    StartJvmError::LoadError(libjvm_path_string.to_owned(), error)
                })?;

                // Create the JVM.
                Self::with_create_fn_ptr(args, *create_fn).map_err(StartJvmError::Create)
            };

            if result.is_ok() {
                // Prevent libjvm from being unloaded.
                //
                // If libjvm is unloaded while the JVM is running, the program will crash as soon as it
                // tries to execute any JVM code, including the many threads that the JVM automatically
                // creates.
                //
                // For reasons unknown, HotSpot seems to somehow prevent itself from being unloaded, so it
                // will work even if this `forget` call isn't here, but there's no guarantee that other JVM
                // implementations will also prevent themselves from being unloaded.
                //
                // Note: `jni-rs` makes the assumption that there can only ever be a single `JavaVM`
                // per-process and it's never possible to full destroy and unload a JVM once it's been
                // created. Calling `DestroyJavaVM` is only expected to release some resources and
                // leave the JVM in a poorly-defined limbo state that doesn't allow unloading.
                // Ref: https://github.com/jni-rs/jni-rs/issues/567
                //
                // See discussion at: https://github.com/jni-rs/jni-rs/issues/550
                std::mem::forget(libjvm);
            }

            result
        }
    }

    /// Launch a new JavaVM using the provided init args, loading it from the given shared library file if it's not already loaded.
    ///
    /// Unlike original JNI API, the main thread (the thread from which this method is called) will
    /// not be attached to JVM. You must explicitly use `attach_current_thread…` methods (refer
    /// to [Attaching Native Threads section](#attaching-native-threads)).
    ///
    /// *This API requires the "invocation" feature to be enabled,
    /// see ["Launching JVM from Rust"](struct.JavaVM.html#launching-jvm-from-rust).*
    ///
    /// The `libjvm_path` parameter takes a *closure* which returns the path to the JVM shared
    /// library. The closure is only called if the JVM is not already loaded. Any work that needs
    /// to be done to locate the JVM shared library should be done inside that closure.
    #[cfg(feature = "invocation")]
    pub fn with_libjvm<P: AsRef<OsStr>>(
        args: InitArgs,
        libjvm_path: impl FnOnce() -> StartJvmResult<P>,
    ) -> StartJvmResult<Self> {
        #[cfg(not(target_os = "android"))]
        {
            Self::impl_with_libjvm(args, libjvm_path)
        }

        #[cfg(target_os = "android")]
        {
            let _args = args;
            let _libjvm_path = libjvm_path;
            Err(StartJvmError::Unsupported)
        }
    }

    /// Create a JavaVM from a raw pointer.
    ///
    /// # Safety
    ///
    /// Expects a valid, non-null JavaVM pointer that supports JNI version >= 1.4.
    ///
    /// Only does a `null` check.
    pub unsafe fn from_raw(ptr: *mut sys::JavaVM) -> Self {
        assert!(!ptr.is_null());
        JAVA_VM_SINGLETON.get_or_init(|| JavaVM(ptr)).clone()
    }

    /// Returns underlying [`sys::JavaVM`] interface.
    pub fn get_raw(&self) -> *mut sys::JavaVM {
        self.0
    }

    /// Create a JavaVM from an Env reference.
    ///
    /// This may return [`Error::JavaException`] if called while there is a pending exception.
    pub(crate) fn from_env(env: &Env) -> Result<Self> {
        // Don't use `.get_or_init()` here because it would deadlock if calling `JavaVM::from_raw`
        // which also uses `.get_or_init()`
        if let Some(jvm) = JAVA_VM_SINGLETON.get() {
            Ok(jvm.clone())
        } else {
            let mut raw = ptr::null_mut();
            let res = unsafe { jni_call_no_post_check_ex!(env, v1_1, GetJavaVM, &mut raw)? };
            jni_error_code_to_result(res)?;

            // Safety: The pointer from GetJavaVM should be valid
            unsafe { Ok(JavaVM::from_raw(raw)) }
        }
    }

    /// Attaches the current thread to the Java VM and calls the provided callback with a mutable
    /// [`Env`] reference.
    ///
    /// If the thread was not already attached then a new attachment is made which will be
    /// automatically detached when the current thread terminates.
    ///
    /// Calling this in a thread that is already attached is cheap since it will only need to check
    /// thread local storage without making a JNI call.
    ///
    /// This API requests to permanently attach the current thread but since pre-existing
    /// attachments aren't affected by this API, it should not be assumed that the thread will
    /// definitely remain attached until it exits - that is only a request. For example if something
    /// higher on the stack has created a scoped attachment then that will take precedence and the
    /// thread will not be permanently attached.
    ///
    /// You can safely assume that the thread will remain attached for the duration of the callback.
    ///
    /// If you're not sure whether to use [`Self::attach_current_thread`] or
    /// [`Self::attach_current_thread_for_scope`], then you should probably use this API because it
    /// increases the chance that future attachment calls will be cheap.
    ///
    /// # Exception Handling
    ///
    /// If the thread is already attached and has a pending exception (before the callback is run),
    /// the exception will be stashed and re-thrown after the callback completes to avoid side
    /// effects and to also ensure your callback can run un-affected by the pending exception. (this
    /// never affects the return value)
    ///
    /// If your callback throws an exception this will be caught and returned as an
    /// [`Error::CaughtJavaException`].
    ///
    /// The exception handling policy can be changed via [`AttachConfig::exceptions_policy`] and
    /// using [`JavaVM::attach_current_thread_with_config`].
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See [Env] documentation for rules on limiting yourself to one [Env] reference per-scope to
    /// avoid exposing code to runtime checks for the top JNI frame that can panic.
    pub fn attach_current_thread<F, T, E>(&self, callback: F) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        self.attach_current_thread_with_config(
            AttachConfig::default,
            Some(DEFAULT_LOCAL_FRAME_CAPACITY),
            callback,
        )
    }

    /// Attaches the current thread to the Java VM and calls the provided callback with a mutable
    /// [`Env`] reference.
    ///
    /// If the thread was not already attached, the thread will be detached when the callback
    /// returns.
    ///
    /// Calling this in a thread that is already attached is cheap since it will only need to check
    /// thread local storage without making a JNI call.
    ///
    /// Attaching a thread is an expensive operation if it was not already attached, so it's
    /// generally recommended that you should use [`Self::attach_current_thread()`] (requesting to
    /// attach the thread permanently) instead of using a scoped attachment. Using this API may
    /// increase the chance that you incur the cost of repeatedly attaching and detaching the same
    /// thread.
    ///
    /// # Exception Handling
    ///
    /// If the thread is already attached and has a pending exception (before the callback is run),
    /// the exception will be stashed and re-thrown after the callback completes to avoid side
    /// effects and to also ensure your callback can run un-affected by the pending exception. (this
    /// never affects the return value)
    ///
    /// If your callback throws an exception this will be caught and returned as an
    /// [`Error::CaughtJavaException`].
    ///
    /// The exception handling policy can be changed via [`AttachConfig::exceptions_policy`] and
    /// using [`JavaVM::attach_current_thread_with_config`].
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See [Env] documentation for rules on limiting yourself to one [Env] reference per-scope to
    /// avoid exposing code to runtime checks for the top JNI frame that can panic.
    pub fn attach_current_thread_for_scope<F, T, E>(&self, callback: F) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        self.attach_current_thread_with_config(
            || AttachConfig::new().scoped(true),
            Some(DEFAULT_LOCAL_FRAME_CAPACITY),
            callback,
        )
    }

    /// Attaches the current thread to the Java VM and calls the provided callback with a mutable
    /// [`Env`] reference.
    ///
    /// This function allows you to customize the attachment process and choose whether to create
    /// a new local frame (with a given `Some(capacity)`) or use the current one (`None`).
    ///
    /// Most of the time you should prefer to use [`Self::attach_current_thread()`] or
    /// [`Self::attach_current_thread_for_scope()`] instead of this function.
    ///
    /// The semantics of [`Self::attach_current_thread`] are equivalent to:
    /// ```rust,ignore-aarch64-linux-android
    /// # use jni::{JavaVM, AttachConfig, DEFAULT_LOCAL_FRAME_CAPACITY};
    /// # fn jni_example(vm: &JavaVM) -> jni::errors::Result<()> {
    ///      vm.attach_current_thread_with_config(AttachConfig::default, Some(DEFAULT_LOCAL_FRAME_CAPACITY), |env| {
    ///          // Use the Env reference
    ///          Ok(())
    ///      })
    /// # }
    /// ```
    ///
    /// The semantics of [`Self::attach_current_thread_for_scope`] are equivalent to:
    /// ```rust,ignore-aarch64-linux-android
    /// # use jni::{JavaVM, AttachConfig, DEFAULT_LOCAL_FRAME_CAPACITY};
    /// # fn jni_example(vm: &JavaVM) -> jni::errors::Result<()> {
    ///      vm.attach_current_thread_with_config(|| AttachConfig::default().scoped(true), Some(DEFAULT_LOCAL_FRAME_CAPACITY), |env| {
    ///          // Use the Env reference
    ///          Ok(())
    ///      })
    /// # }
    /// ```
    ///
    /// See [`Self::attach_current_thread`], [`Self::attach_current_thread_for_scope`] and [`AttachConfig`] for more details.
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See [Env] documentation for rules on limiting yourself to one [Env] reference per-scope to
    /// avoid exposing code to runtime checks for the top JNI frame that can panic.
    pub fn attach_current_thread_with_config<'config, F, C, T, E>(
        &self,
        config: C,
        capacity: Option<usize>,
        callback: F,
    ) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
        C: FnOnce() -> AttachConfig<'config>,
    {
        let config = config();
        let exceptions_policy = config.exceptions_policy;
        let mut scope = ScopeToken::default();
        // Safety: The ScopeToken and guard will remain fixed on the stack by keeping the guard
        // private to this function.
        let mut guard = unsafe { self.attach_current_thread_guard(|| config, &mut scope)? };
        let env = guard.borrow_env_mut();

        let pre_pending_exception = match exceptions_policy {
            AttachmentExceptionPolicy::Ignore => None,
            AttachmentExceptionPolicy::PreCheckPostCatch => {
                // In case the thread was already attached we must not go any further if there is a pending exception,
                // otherwise this API would have the surprising side effect of clearing pending exceptions. In other
                // words we behave as if this API is not exception safe and immediately return `JavaException` to
                // indicate there is a pending exception that must be handled.
                if env.exception_check() {
                    return Err(Error::JavaException.into());
                } else {
                    None
                }
            }
            AttachmentExceptionPolicy::PreReThrowPostCatch => {
                if let Some(e) = env.exception_occurred() {
                    env.exception_clear();
                    Some(e)
                } else {
                    None
                }
            }
        };

        let ret = if let Some(capacity) = capacity {
            env.with_local_frame(capacity, callback)
        } else {
            callback(env)
        };

        // Explicitly detach the guard (instead of relying on Drop) so we can
        // return any error from the detachment process.
        //
        // Any left-over pending exception from the `callback` closure will be
        // returned as `Error::CaughtJavaException` here.
        //
        // Any exception that was caught before running the `callback` closure
        // will be re-thrown.
        //
        // Note: any error here will also have a higher precedence than the
        // result from the closure which is probably reasonable because a
        // detachment error would probably indicate a serious issue and an
        // exception should typically imply that the closure has returned a
        // `JavaException` error.
        if exceptions_policy == AttachmentExceptionPolicy::Ignore {
            guard.detach()?;
        } else {
            guard.detach_with_catch_and_throw(pre_pending_exception)?;
        }

        ret
    }

    /// Attaches the current thread to the Java VM and returns an [`AttachGuard`] for the
    /// attachment.
    ///
    /// This is a low-level (unsafe) building block for [`Self::attach_current_thread`],
    /// [`Self::attach_current_thread_for_scope`] and [`Self::attach_current_thread_with_config`]
    /// that allows for more fine-grained control over the attachment process and how you borrow an
    /// `Env` reference from the guard.
    ///
    /// The given `config` callback is only lazily called if the thread was not already attached and
    /// returns a [`AttachConfig`] that you can use to customize the attachment.
    ///
    /// For example, this can be used to implement your own equivalent to
    /// [`Self::attach_current_thread`] like:
    ///
    /// ```rust,ignore-aarch64-linux-android
    /// # use jni::{JavaVM, AttachConfig, DEFAULT_LOCAL_FRAME_CAPACITY};
    /// struct Executor {
    ///     vm: JavaVM,
    /// }
    /// impl Executor {
    ///     fn new(vm: JavaVM) -> Self {
    ///         Self { vm }
    ///     }
    ///
    ///     pub fn my_attach_current_thread<F, T, E>(&self, callback: F) -> std::result::Result<T, E>
    ///     where
    ///         F: FnOnce(&mut jni::Env) -> std::result::Result<T, E>,
    ///         E: From<jni::errors::Error>,
    ///     {
    ///         let mut scope = jni::ScopeToken::default();
    ///         // Safety: The scope + guard will remain fixed on the stack by keeping the guard
    ///         // private to this function.
    ///         let mut guard = unsafe { self.vm.attach_current_thread_guard(AttachConfig::default, &mut scope)? };
    ///         let ret = guard.borrow_env_mut().with_local_frame(DEFAULT_LOCAL_FRAME_CAPACITY, callback);
    ///         guard.detach_with_catch()?; // return pending exceptions as Error::CaughtJavaException
    ///         ret
    ///     }
    /// }
    /// ```
    ///
    /// See [`Self::attach_current_thread`], [`Self::attach_current_thread_for_scope`],
    /// [`AttachConfig`] and [`AttachGuard`] for more details.
    ///
    /// Consider using [`Self::attach_current_thread_with_config`] before resorting to this (unsafe)
    /// API.
    ///
    /// # Safety
    ///
    /// The returned guard must be managed according to the general [`AttachGuard`] "Safety" rules.
    ///
    /// **IMPORTANTLY:** Never give the returned guard a `'static` lifetime by creating a `'static`
    /// [`ScopeToken`]; the guard must be treated as an immovable value on the stack and not be
    /// re-ordered relative to other guards on the stack. See the 'Safety' rules for [`AttachGuard`]
    ///
    /// Since the `'local` lifetime for the [Env] reference will come from the `&'scope ScopeToken`
    /// reference, you must ensure that the `scope` is dropped in sync with the guard (to ensure
    /// no local reference types may outlive the attachment).
    ///
    /// This can be achieved by hiding the scope and guard from safe code, only exposing a temporary [`Env`]
    /// reference (as in the example above).
    ///
    pub unsafe fn attach_current_thread_guard<'config, 'scope, F>(
        &self,
        config: F,
        _scope: &'scope mut ScopeToken,
    ) -> Result<AttachGuard<'scope>>
    where
        F: FnOnce() -> AttachConfig<'config>,
    {
        // Safety:
        // - The minimum supported JNI version is >= 1.4
        // - The caller is responsible for managing the returned guard safely
        let guard = unsafe {
            // Note: Even Windows (where we track permanent attachments via FLS) we use TLS to track
            // active AttachGuards for the current thread because we assume that fibers using JNI must
            // not be switched out or freed while they have active AttachGuards.
            if let Some(guard) = Self::try_get_nested_env_attach_guard::<'scope>() {
                guard
            } else {
                // If we don't have an active AttachGuard we _always_ check the real JNI GetEnv
                // state even if we have a permanent attachment recorded in TLS/FLS. We want to be
                // resilient to external code detaching the thread, so long as there are no active
                // AttachGuards.
                match self.sys_get_env_attachment() {
                    Ok(guard) => guard,
                    Err(Error::JniCall(JniError::ThreadDetached)) => {
                        let config = config();
                        if config.scoped {
                            let jni = sys_attach_current_thread(self, &config, &current(), true)?;
                            AttachGuard::from_owned(jni)
                        } else {
                            #[cfg(use_tls_attach_guard)]
                            {
                                tls_attach_guard::tls_attach_current_thread(self, &config)?
                            }
                            #[cfg(use_fls_attach_guard)]
                            {
                                fls_attach_guard::fls_attach_current_thread(self, &config)?
                            }
                        }
                    }
                    Err(err) => Err(err)?,
                }
            }
        };

        Ok(guard)
    }

    /// Explicitly detaches the current thread from the JVM, **IFF** it was previously attached
    /// using [`JavaVM::attach_current_thread`] **AND** if there is no [`AttachGuard`] also keeping
    /// the current thread attached (I.e. you have no [`Env`] reference in scope).
    ///
    /// This will always return an error if there are currently any active [`AttachGuard`]s
    /// (detaching the thread in this case would effectively turn guards into invalid, dangling
    /// pointers).
    ///
    /// Detaching a non-attached thread is a no-op that won't return an error (assuming there's no
    /// active [`AttachGuard`] as noted above).
    ///
    /// This API has no effect on thread attachments that were created outside of this crate's API
    /// (such as other JNI language bindings).
    ///
    /// This API is not applicable to scoped attachments because it's an error to call while there
    /// are active [`AttachGuard`]s.
    ///
    /// **Windows Note**: On Windows, in the unlikely event that you have multiple fibers scheduled
    /// on the same thread, this API only has an effect within the context of a fiber that previously
    /// attached the thread to the JVM via [`JavaVM::attach_current_thread`].
    ///
    /// _**Note**: It's _rarely_ necessary to use this API because a thread that is attached via
    /// [`JavaVM::attach_current_thread`] will automatically detach when that thread terminates.
    ///
    /// Explicitly detaching the thread could lead to overheads later if the same thread needs to
    /// get re-attached.
    ///
    /// If there is a need to detach a thread before it terminates, then it's possible that a scoped
    /// attachment via [`JavaVM::attach_current_thread_for_scope`] could be used so that the
    /// detachment would happen automatically.
    pub fn detach_current_thread(&self) -> Result<()> {
        if THREAD_GUARD_NEST_LEVEL.get() != 0 {
            return Err(Error::ThreadAttachmentGuarded);
        }

        #[cfg(use_tls_attach_guard)]
        {
            tls_attach_guard::tls_detach_current_thread()
        }

        #[cfg(use_fls_attach_guard)]
        {
            fls_attach_guard::fls_detach_current_thread()
        }
    }

    /// Returns the current number of threads attached to the JVM.
    ///
    /// This only counts the thread attachments made via this library's API.
    ///
    /// This method is provided mostly for diagnostic purposes / unit tests.
    ///
    /// Check the docs for `ATTACHED_THREADS` for more details - including caveats for Windows with FLS.
    #[doc(hidden)]
    pub fn threads_attached(&self) -> usize {
        ATTACHED_THREADS.load(Ordering::SeqCst)
    }

    /// Returns the current nesting level for [`AttachGuard`]s
    ///
    /// This is only really public since it's useful for unit tests
    #[doc(hidden)]
    pub fn thread_attach_guard_level() -> usize {
        THREAD_GUARD_NEST_LEVEL.get()
    }

    /// Get an [`AttachGuard`] for the [`Env`] associated with the current thread or, if JNI is
    /// not attached to the Java VM, this will return [`Error::JniCall`] with
    /// [`JniError::ThreadDetached`].
    ///
    /// Note: jni-rs is implemented based on an assumption that all real-world implementations of
    /// JNI `GetEnv` will return the same pointer for any given version (so long as the version is
    /// supported).
    ///
    /// Hypothetically the JNI spec allows for the possibility for an implementation to return a
    /// different Env pointer that nulls out functions that aren't valid for that version (or
    /// dispatches calls differently).
    ///
    /// If we ever find a JVM implementation that in fact returns a different pointer then we could
    /// just repeat the GetEnv call with the maximum supported version after querying the version.
    ///
    /// # Safety
    ///
    /// You must know that the [`JavaVM`] supports at least JNI >= 1.2 (we require >= 1.4 but
    /// we couldn't even call GetEnv without 1.2)
    ///
    /// See the 'Safety' rules for [`AttachGuard`]
    pub(crate) unsafe fn sys_get_env_attachment<'local>(&self) -> Result<AttachGuard<'local>> {
        unsafe {
            let mut ptr = ptr::null_mut();
            let res =
                java_vm_call_unchecked!(self, v1_2, GetEnv, &mut ptr, JNIVersion::V1_4.into());
            jni_error_code_to_result(res)?;
            let jni = ptr as *mut sys::JNIEnv;
            Ok(AttachGuard::from_unowned(jni))
        }
    }

    /// Returns `true` if the current thread is attached to a Java VM.
    ///
    /// Since this calls [`sys::JNIInvokeInterface__1_2::GetEnv`], it will also recognize thread
    /// attachments that made without using this crate (such as other JNI language bindings).
    pub fn is_thread_attached(&self) -> Result<bool> {
        // Safety: we aren't materializing an attachment guard while we already have access to one
        unsafe {
            self.sys_get_env_attachment()
                .map(|_| true)
                .or_else(|jni_err| match jni_err {
                    Error::JniCall(JniError::ThreadDetached) => Ok(false),
                    _ => Err(jni_err),
                })
        }
    }

    /// Returns an [`AttachGuard`] for the [`Env`] associated with the current thread, if there is
    /// already at least one `AttachGuard` on the stack for the current thread.
    ///
    /// Returns None if there is no `AttachGuard` on the stack for the current thread.
    ///
    /// This serves a similar purpose to [`sys::JNIInvokeInterface__1_2::GetEnv`] in that it
    /// provides access to the current thread's JNI environment if JNI is attached.
    ///
    /// This API will only recognize attachments made by this crate (including uses of
    /// [`AttachGuard::from_unowned`]). I.e. the implementation only checks crate-specific thread
    /// local storage and will not actually call [`sys::JNIInvokeInterface__1_2::GetEnv`].
    ///
    /// More-specifically, this will only recognise attachments while there is an `AttachGuard` on
    /// the stack for the current thread. Once all `AttachGuard`s for the current thread have been
    /// dropped, this API will return `None` even if a permanent attachment exists, that is being
    /// tracked via TLS/FLS outside of an `AttachGuard`.
    ///
    /// This API can be used speculatively before calling sys_get_env_attachment to avoid making a
    /// JNI call to query the current JNI environment.
    ///
    /// # Safety
    ///
    /// See the 'Safety' rules for [`AttachGuard`]
    pub(crate) unsafe fn try_get_nested_env_attach_guard<'local>() -> Option<AttachGuard<'local>> {
        let env_ptr = THREAD_ATTACHMENT.get();
        if env_ptr.is_null() {
            None
        } else {
            // Safety: we can assume any THREAD_ATTACHMENT pointer is valid
            unsafe { Some(AttachGuard::from_unowned(env_ptr)) }
        }
    }

    /// Returns an [`AttachGuard`] for top JNI stack frame attached to the current thread.
    ///
    /// If the current thread is not attached to the Java VM, this will return [`Error::JniCall`]
    /// with [`JniError::ThreadDetached`].
    ///
    /// This serves a similar purpose to [`sys::JNIInvokeInterface__1_2::GetEnv`] in that it
    /// provides access to the current thread's JNI environment if JNI is attached.
    ///
    /// This API can recognize attachments made by other JNI language bindings but will first check
    /// crate-specific thread local storage for an attachment before calling
    /// [`sys::JNIInvokeInterface__1_2::GetEnv`].
    ///
    /// Consider using [`Self::with_local_frame`] or [`Self::with_top_local_frame`] as safe
    /// alternatives for running code against the currently attached JNI environment.
    ///
    /// # Safety
    ///
    /// See the 'Safety' rules for [`AttachGuard`]
    pub unsafe fn get_env_attachment<'scope>(
        &self,
        _scope: &'scope mut ScopeToken,
    ) -> Result<AttachGuard<'scope>> {
        unsafe {
            match Self::try_get_nested_env_attach_guard() {
                Some(guard) => Ok(guard),
                None => self.sys_get_env_attachment(),
            }
        }
    }

    #[doc(hidden)]
    #[deprecated(
        since = "0.22.0",
        note = "use JavaVM::with_top_local_frame, JavaVM::attach_current_thread or JavaVM::get_env_attachment instead"
    )]
    pub unsafe fn get_env<'scope>(
        &self,
        _scope: &'scope mut ScopeToken,
    ) -> Result<AttachGuard<'scope>> {
        unsafe { self.get_env_attachment(_scope) }
    }

    /// Runs a closure within a new JNI stack frame that will be unwound to release all local
    /// references created within the given closure.
    ///
    /// If the current thread is not attached to the Java VM, this will return [`Error::JniCall`]
    /// with [`JniError::ThreadDetached`].
    ///
    /// This API can recognize attachments made by other JNI language bindings but will first check
    /// crate-specific thread local storage for an attachment before calling
    /// [`sys::JNIInvokeInterface__1_2::GetEnv`].
    ///
    /// Internally this calls [`sys::JNINativeInterface__1_2::PushLocalFrame`] with the given
    /// `capacity`, to create a new JNI stack frame, and calls
    /// [`sys::JNINativeInterface__1_2::PopLocalFrame`] after the closure is executed.
    ///
    /// If you're not sure that the current thread is attached to the Java VM, consider using
    /// [Self::attach_current_thread] instead.
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See [Env] documentation for rules on limiting yourself to one [Env] reference per-scope to
    /// avoid exposing code to runtime checks for the top JNI frame (that can panic).
    pub fn with_local_frame<F, T, E>(&self, capacity: usize, f: F) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        unsafe {
            let mut scope = ScopeToken::default();
            let mut guard = self.get_env_attachment(&mut scope)?;
            guard.borrow_env_mut().with_local_frame(capacity, f)
        }
    }

    /// Runs a closure with an [`Env`] reference associated with the top JNI stack frame.
    ///
    /// Unlike [`Self::with_local_frame()`], this API does not push a new JNI stack frame.
    ///
    /// Most of the time this API should probably be avoided (see [`Self::with_local_frame`] or
    /// [`Self::attach_current_thread`] instead) unless you're sure your code won't leak local
    /// references into the current stack frame (or you're sure that the leaked references are
    /// acceptable because you know when the top frame will unwind and release those references).
    ///
    /// This will have a slightly lower overhead than [`Self::with_local_frame()`] (since it doesn't
    /// need to push/pop a JNI stack frame), but the trade off is that you may leak local references
    /// into the top stack frame.
    ///
    /// Keep in mind that deleting local references individually is likely to have a higher cost
    /// than pushing/popping a JNI stack frame, so you should probably only use this API if you're
    /// OK with leaking a small number of local references into the top frame.
    ///
    /// # Runtime Top Frame Checks
    ///
    /// See [Env] documentation for rules on limiting yourself to one [Env] reference per-scope to
    /// avoid exposing code to runtime checks for the top JNI frame (that can panic).
    pub fn with_top_local_frame<F, T, E>(&self, f: F) -> std::result::Result<T, E>
    where
        F: FnOnce(&mut Env) -> std::result::Result<T, E>,
        E: From<Error>,
    {
        unsafe {
            let mut scope = ScopeToken::default();
            let mut guard = self.get_env_attachment(&mut scope)?;
            f(guard.borrow_env_mut())
        }
    }

    /// Unloads the JavaVM and frees all it's associated resources
    ///
    /// Firstly if this thread is not already attached to the `JavaVM` then it
    /// will be attached.
    ///
    /// This thread will then wait until there are no other non-daemon threads
    /// attached to the `JavaVM` before unloading it (including threads spawned
    /// by Java and those that are attached via JNI)
    ///
    /// # Safety
    ///
    /// IF YOU ARE USING DAEMON THREADS THIS MAY BE DIFFICULT TO USE SAFELY!
    ///
    /// ## Daemon thread rules
    ///
    /// Since the JNI spec makes it clear that [`DestroyJavaVM`][destroy] will
    /// not wait for attached daemon threads to exit, this also means that if
    /// you do have any attached daemon threads it is your responsibility to
    /// ensure that they don't try and use JNI after the `JavaVM` is destroyed
    /// and you won't be able to detach them after the `JavaVM` has been
    /// destroyed.
    ///
    /// This creates a very unsafe hazard if using `jni-rs` due to the various
    /// RAII types that will automatically make JNI calls within their `Drop`
    /// implementation.
    ///
    /// For this reason `jni-rs` doesn't directly support attaching or detaching
    /// 'daemon' threads and it's assumed you will manage their safety yourself
    /// if you're using them.
    ///
    /// Note: [`JavaVM::detach_current_thread()`] is a no-op for daemon threads
    /// because it will only detach threads that were attached via `jni-rs` APIs.
    ///
    /// ## Don't call from a Java native function
    ///
    /// There must be no Java methods on the call stack when `JavaVM::destroy()`
    /// is called.
    ///
    /// ## Drop all JNI state, including auto-release types before calling `JavaVM::destroy()`
    ///
    /// There is currently no `'vm` lifetime associated with a `JavaVM` that
    /// would allow the borrow checker to enforce that all `jni` resources
    /// associated with the `JavaVM` have been released.
    ///
    /// Since these JNI resources could lead to undefined behaviour through any
    /// use after the `JavaVM` has been destroyed then it is your responsibility
    /// to release these resources.
    ///
    /// In particular, there are numerous auto-release types in the `jni` API
    /// that will automatically make JNI calls within their `Drop`
    /// implementation. All such types _must_ be dropped before `destroy()` is
    /// called to avoid undefined behaviour.
    ///
    /// Here is an non-exhaustive list of auto-release types to consider:
    /// - `AttachGuard`
    /// - `AutoElements`
    /// - `AutoElementsCritical`
    /// - `Auto`
    /// - `Global`
    /// - `Weak`
    /// - `MUTF8Chars`
    /// - `JMap`
    ///
    /// ## Invalid `JavaVM` on return
    ///
    /// After `destroy()` returns then the `JavaVM` will be in an undefined
    /// state and must be dropped (e.g. via `std::mem::drop()`) to avoid
    /// undefined behaviour.
    ///
    /// This method doesn't take ownership of the `JavaVM` before it is
    /// destroyed because the `JavaVM` may have been shared (E.g. via an `Arc`)
    /// between all the threads that have not yet necessarily exited before this
    /// is called.
    ///
    /// So although the `JavaVM` won't necessarily be solely owned by this
    /// thread when `destroy()` is first called it will conceptually own the
    /// `JavaVM` before `destroy()` returns.
    ///
    /// [destroy]:
    ///     https://docs.oracle.com/en/java/javase/12/docs/specs/jni/invocation.html#unloading-the-vm
    pub unsafe fn destroy(&self) -> Result<()> {
        unsafe {
            let res = java_vm_call_unchecked!(self, v1_1, DestroyJavaVM);
            jni_error_code_to_result(res)
        }
    }
}

/// The policy for how exceptions are handled before and after running
/// an attachment closure.
///
/// This policy affects the closure-based thread attachment APIs, including:
/// - [`JavaVM::attach_current_thread`]
/// - [`JavaVM::attach_current_thread_for_scope`]
/// - [`JavaVM::attach_current_thread_with_config`]
///
#[derive(Debug, Default, Copy, Clone, PartialEq, Eq)]
pub enum AttachmentExceptionPolicy {
    /// Pending exceptions are not checked before or after any attachment
    /// closure
    ///
    /// This will expose your closure to any pending exception which it should
    /// be prepared to handle.
    ///
    /// For example this could be used to make purely exception-safe JNI calls.
    ///
    /// Note: It should be very rare to use this, since attachment closures are
    /// normally written as orthogonal code that is not expected to handle the
    /// side effects of external code.
    Ignore,

    /// Pending exceptions are caught before running any attachment closure and
    /// re-thrown afterwards, returning [`Error::JavaException`]
    ///
    /// If an exception is thrown by the attachment closure then this will
    /// _always_ be caught and returned via [`Error::CaughtJavaException`].
    ///
    /// If a pending exception was caught **and** an exception is thrown by the
    /// attachment closure then:
    /// 1. The new exception (from the closure) will be caught
    /// 2. The original exception will be re-thrown
    /// 3. The new exception will be returned via [`Error::CaughtJavaException`]
    ///
    /// Beware: that in this case the original exception will be pending after
    /// returning, but this will not be indicated via a [`Error::JavaException`]
    /// return value. The assumption in this case is that the caller is already
    /// aware of this pending exception.
    ///
    /// This is intended to make the best default trade off between avoiding
    /// side effects and consistently running orthogonal code within an
    /// attachment closure.
    ///
    /// This prioritizes consistency for the attachment closure:
    /// - It will *always run*, regardless of unexpected pending exceptions
    ///   associated with the current thread.
    /// - Any new exception thrown by the closure will be *always be caught* and
    ///   returned via [`Error::CaughtJavaException`] - regardless of whether
    ///   some other exception was previously pending.
    ///
    /// A pre-existing exception _may_ represent an oversight with handling an
    /// exception in code that is unrelated to the attachment closure, **or** it
    /// could also easily arise if `Drop` is used to release resources via JNI
    /// because these may automatically run while unwinding a stack (perhaps due
    /// to an [`Error::JavaException`] that the caller knows about).
    #[default]
    PreReThrowPostCatch,

    /// [`Error::JavaException`] is immediately returned if there is a pending
    /// exception and the attachment closure is not run.
    ///
    /// If any exception is thrown by the attachment closure then this will be
    /// caught and returned via [`Error::CaughtJavaException`]
    ///
    /// This is similar to the behaviour of other `jni` APIs that are not
    /// exception-safe and will immediately return with an error if called
    /// while there is a pending exception.
    PreCheckPostCatch,
}

/// Configuration options for attaching the current thread to a Java VM.
#[derive(Debug, Default)]
pub struct AttachConfig<'a> {
    scoped: bool,
    exceptions_policy: AttachmentExceptionPolicy,
    deprecated_thread_name: Option<JNIString>,
    thread_name: Option<&'a JNIStr>,
    group: Option<&'a Global<JObject<'static>>>,
}

impl<'a> AttachConfig<'a> {
    /// Creates a new `AttachConfig` with default values.
    pub fn new() -> Self {
        Self::default()
    }

    /// Sets whether the attachment should be owned by the current scope, such
    /// that the thread will be automatically detached when the attachment guard
    /// is dropped.
    ///
    /// The default is `false`, so the thread will be attached permanently.
    ///
    /// It is normally best to attach permanently because it can reduce the cost
    /// of repeatedly attaching and detaching threads.
    pub fn scoped(mut self, scoped: bool) -> Self {
        self.scoped = scoped;
        self
    }

    /// Sets the policy for handling exceptions during thread attachment.
    ///
    /// The default is [`AttachmentExceptionPolicy::PreReThrowPostCatch`] which
    /// is recommended to avoid the side effects of clearing (existing) pending
    /// exceptions and will return any new exception thrown by the closure via
    /// [`Error::CaughtJavaException`].
    ///
    /// For example, the default policy should be suitable for code that needs
    /// to use JNI from within a `Drop` implementation, without being influenced
    /// by external pending exceptions but also ensuring that external pending
    /// exception continue to propagate.
    pub fn exceptions_policy(mut self, policy: AttachmentExceptionPolicy) -> Self {
        self.exceptions_policy = policy;
        self
    }

    /// Sets the name of the thread as seen by the JVM and operating system.
    #[deprecated(since = "0.22.2", note = "Use `thread_name` instead")]
    pub fn name<S: AsRef<str>>(mut self, name: S) -> Self {
        self.deprecated_thread_name = Some(JNIString::from(name.as_ref()));
        self
    }

    /// Sets the name of the thread as seen by the JVM and operating system.
    pub fn thread_name(mut self, name: &'a JNIStr) -> Self {
        self.thread_name = Some(name);
        self
    }

    /// Specifies a global reference to a `ThreadGroup` that the thread should
    /// be associated with.
    pub fn group(mut self, group: &'a Global<JObject<'static>>) -> Self {
        self.group = Some(group);
        self
    }
}

/// Tracks the number of logical attachments managed by this crate.
///
/// This is unconditionally incremented for each call to `sys_attach_current_thread` and decremented
/// for each call to `sys_detach_current_thread`.
///
/// **Windows with FLS caveats:**
///
/// On Windows with FLS, this still typically corresponds to the number of OS threads attached,
/// because only one fiber per-OS-thread will be responsible for attaching to the JVM (others would
/// observe that something has already attached the OS thread).
///
/// In the unlikely situation where JNI is manually used to detach an OS thread (allowed if there
/// are no active AttachGuards), then multiple fibers could separately attach the same OS thread and
/// both increment this counter. They will also both decrement the counter when they are freed (or
/// if detach_current_thread() is called from each fiber).
pub(super) static ATTACHED_THREADS: AtomicUsize = AtomicUsize::new(0);

pub(super) unsafe fn sys_attach_current_thread(
    vm: &JavaVM,
    config: &AttachConfig,
    thread: &Thread,
    inc_attached_count: bool,
) -> Result<*mut sys::JNIEnv> {
    assert_eq!(JavaVM::thread_attach_guard_level(), 0);
    let mut env_ptr = ptr::null_mut();

    let mut args = sys::JavaVMAttachArgs {
        version: JNIVersion::V1_4.into(),
        name: config
            .thread_name
            .map(|s| s.as_ptr() as *mut c_char)
            .unwrap_or_else(|| {
                config
                    .deprecated_thread_name
                    .as_ref()
                    .map(|s| s.as_ptr() as *mut c_char)
                    .unwrap_or(ptr::null_mut())
            }),
        group: config
            .group
            .as_ref()
            .map(|g| g.as_raw())
            .unwrap_or(ptr::null_mut()),
    };
    let res = unsafe {
        java_vm_call_unchecked!(
            vm,
            v1_1,
            AttachCurrentThread,
            &mut env_ptr,
            &mut args as *mut sys::JavaVMAttachArgs as *mut core::ffi::c_void
        )
    };
    jni_error_code_to_result(res)?;

    // Increment the attached thread count only if requested.
    //
    // This is used to avoid double-counting when re-attaching a permanently-attached thread that
    // got manually detached outside of this crate's APIs (e.g., via raw JNI calls).
    //
    // I.e. the only time this should be false is when we are re-attaching via
    // tls_attach_current_thread or fls_attach_current_thread after detecting that the thread
    // already had a permanent attachment.
    if inc_attached_count {
        ATTACHED_THREADS.fetch_add(1, Ordering::SeqCst);
    }

    debug!(
        "Attached thread {} ({:?}). {} threads attached",
        thread.name().unwrap_or_default(),
        thread.id(),
        ATTACHED_THREADS.load(Ordering::SeqCst)
    );

    Ok(env_ptr as *mut sys::JNIEnv)
}

/// Check if the process is currently shutting down.
///
/// Note: RtlDllShutdownInProgress is available on Windows 10 + but not part of
/// the Windows SDK (not available via windows-bindgen), so we link to it
/// manually.
#[cfg(target_os = "windows")]
fn windows_is_shutdown_in_progress() -> bool {
    #[link(name = "ntdll")]
    unsafe extern "system" {
        fn RtlDllShutdownInProgress() -> u8; // BOOLEAN
    }
    unsafe { RtlDllShutdownInProgress() != 0 }
}

/// Detach a thread from the JVM.
///
/// If `cross_check_env` is provided, this function will verify that the env pointer matches the
/// current thread's attachment before detaching. This provides a defensive check that we "own" the
/// current attachment. We only use this when detaching scoped attachments.
///
/// `cross_check_env` is not provided for permanent TLS/FLS attachments since we allow external
/// code to detach these early (as long as there are no AttachGuards currently on the stack).
///
/// The `ATTACHED_THREADS` count is always decremented when this is called, even if the thread
/// was already detached by other means (e.g., manual `DetachCurrentThread` call via raw JNI API,
/// or by another fiber on Windows). This maintains correct logical accounting - we track how many
/// times *we* attached, not the actual JVM attachment state.
pub(super) unsafe fn sys_detach_current_thread(
    cross_check_env: Option<*mut jni_sys::JNIEnv>,
    thread: &Thread,
) -> Result<()> {
    assert_eq!(JavaVM::thread_attach_guard_level(), 0);

    let vm = JavaVM::singleton()?;

    // On Windows we don't ever attempt to explicitly detach a thread from the JVM (or even query if
    // we have a valid attachment) if we know the process is already shutting down.
    //
    // On Windows, when the process is shutting down there's a risk of deadlocks if we attempt to
    // call into the JVM while the thread holds the system loader lock. This is because the JVM may
    // need to reach a safepoint, which also needs to acquire the system loader lock.
    //
    // In practice we need this check on Windows because we register fiber local storage callbacks
    // that may either be called when a fiber/thread terminates _OR_ when the process is shutting
    // down, and we only really want to auto-detach threads in the former case.
    //
    // Note: we don't want to entirely skip the call to `sys_detach_current_thread` during shutdown
    // because we still want to maintain correct logical bookkeeping.
    //
    // Ref: https://github.com/jni-rs/jni-rs/issues/701
    #[cfg(target_os = "windows")]
    let allow_jni = !windows_is_shutdown_in_progress();
    #[cfg(not(target_os = "windows"))]
    let allow_jni = true;

    if allow_jni {
        // Check if thread is actually attached before attempting to detach
        let was_attached = match unsafe { vm.sys_get_env_attachment() } {
            Ok(mut guard) => {
                // Optional defensive cross-check: verify env matches current thread
                if let Some(expected_env) = cross_check_env {
                    // If this check fails it would imply we're holding an
                    // invalid pointer and would be completely unsound
                    assert_eq!(
                        guard.env.raw, expected_env,
                        "BUG: Something meddled with the JNI attachment behind our back"
                    );
                }

                // Clear Pending Exceptions
                //
                // Unlike for later versions; the JNI spec for Java 8 doesn't explicitly state that
                // DetachCurrentThread is safe to call while there are pending exceptions.
                //
                // Note: we don't use `.exception_catch()` to get the details of any exception since
                // we assume that will be done via `AttachGuard::detach_with_catch` if needed.

                guard.borrow_env_mut().exception_clear();

                drop(guard);
                true
            }
            Err(Error::JniCall(JniError::ThreadDetached)) => {
                // If this assertion fails it implies we have been left with a potentially invalid
                // pointer for an attachment that no longer exists.
                assert!(
                    cross_check_env.is_none(),
                    "BUG: Thread was detached by external code"
                );
                false
            }
            Err(e) => return Err(e),
        };

        if was_attached {
            // Only actually detach if thread is still attached
            unsafe {
                java_vm_call_unchecked!(vm, v1_1, DetachCurrentThread);
            }
            debug!(
                "Detached thread {} ({:?})",
                thread.name().unwrap_or_default(),
                thread.id()
            );
        } else {
            // Thread already detached (e.g., by another fiber on Windows, or manual DetachCurrentThread)
            debug!(
                "Thread {} ({:?}) already detached (skipping DetachCurrentThread)",
                thread.name().unwrap_or_default(),
                thread.id()
            );
        }
    }

    // ALWAYS decrement the logical count, even if thread was already detached.
    // This maintains correct bookkeeping:
    // - On Windows with FLS: each fiber attachment increments this counter
    // - On other platforms with TLS: each thread attachment increments this counter
    ATTACHED_THREADS.fetch_sub(1, Ordering::SeqCst);

    debug!(
        "{} attachments remain (logical count)",
        ATTACHED_THREADS.load(Ordering::SeqCst)
    );

    Ok(())
}

// There's a false-positive Clippy bug: https://github.com/rust-lang/rust-clippy/issues/13422
thread_local! {
    #[cfg_attr(target_os = "android", allow(clippy::missing_const_for_thread_local))]
    static THREAD_ATTACHMENT: Cell<*mut jni_sys::JNIEnv> = const { Cell::new(std::ptr::null_mut()) };
    #[cfg_attr(target_os = "android", allow(clippy::missing_const_for_thread_local))]
    pub(super) static THREAD_GUARD_NEST_LEVEL: Cell<usize> = const { Cell::new(0) };
}

/// Represents a JNI attachment of the current thread to a Java VM, which is
/// required before you can access the [`Env`] API.
///
/// [`AttachGuard`] is an `unsafe`, low-level building block for the safe thread
/// attachment APIs provided by [`JavaVM`]. Before considering this API you
/// should look at:
///
///   - [`JavaVM::attach_current_thread()`]
///   - [`JavaVM::attach_current_thread_for_scope()`]
///   - [`JavaVM::attach_current_thread_with_config()`]
///   - [`JavaVM::with_local_frame()`]
///   - [`JavaVM::with_top_local_frame()`]
///
/// More-specifically, an [`AttachGuard`] only represents an attachment for a
/// single JNI stack frame or local reference frame.
///
/// The lifetime for an [`AttachGuard`] effectively names the scope of a single
/// JNI stack frame. The lifetime of the guard is used to ensure that any local
/// references created via the [`Env`] API are only accessible within the
/// lifetime of the guard, which ensures that local references can't be used
/// outside of the JNI stack frame that owns them.
///
/// If the [`AttachGuard`] "owns" the underlying JNI thread attachment, that
/// means the guard will automatically detach the current thread from the Java
/// VM when the guard is dropped.
///
/// If you're implementing a JNI native method which is passed a raw
/// [`crate::sys::JNIEnv`] pointer, then you can get a corresponding guard via
/// [`AttachGuard::from_unowned`].
///
/// If you're implementing some JNI utility code that doesn't already have a raw
/// [`crate::sys::JNIEnv`] pointer you could use
/// [`JavaVM::attach_current_thread_guard`] to get an attachment guard.
///
/// # JavaVM::singleton() guarantee
///
/// If you know that at least one [`AttachGuard`] has ever existed (which is
/// implied if you have a [`Env`] reference) you can assume that
/// [`JavaVM::singleton()`] will return `Some(JavaVM)`.
///
/// This can be useful if you need to access JNI from something like a Drop
/// implementation, without needing to explicitly copy `JavaVM` pointers. In
/// these cases you can often prove that at least one `AttachGuard` must have
/// existed in order to construct the object that will later be dropped.
///
/// # Safety
///
/// [`AttachGuard`] management is considered `unsafe` because there are some
/// safety rules that can't be automatically guaranteed through the Rust type
/// system alone...
///
/// 1. You must treat a guard as an immovable type that needs to live on the
///    stack and can't be re-ordered relative to other guards on the stack.
///
/// 2. Following from (1), an [`AttachGuard`] _MUST NOT_ be given a `'static`
///    lifetime (e.g. by boxing or moving into a `static` variable).
///
/// 3. Any low-level JNI code that creates new local references must
///    runtime-assert that it has exclusive access to the top-most
///    [`AttachGuard`] on the stack.
///
///    If you're only using the safe [`Env`] API then this is handled for you,
///    but if you're implementing low-level JNI code that somehow creates local
///    references without involving a checked `Env` API you must add fail-safe
///    runtime assertions. (See [`Env::assert_top`])
///
/// # Safety Guidance
///
/// If you do need to use an [`AttachGuard`] directly, then it's recommended
/// that the guard should be kept private to a function that uses so it isn't
/// ever exposed to safe code.
///
/// It's very important that [`AttachGuard`]s are never given a `'static`
/// lifetime or re-ordered relative to other guards on the stack and keeping the
/// guard private to a function is a good way to ensure that it remains fixed on
/// the stack.
///
/// Beware of making it too easy to repeatedly materialize access to a mutable
/// [`Env`] and increasing the risk that safe code could attempt to create new
/// local references with an [`Env`] reference that is not at the top of the
/// stack. If this happens the API will panic at runtime to avoid undefined
/// behavior.
///
/// # Panics
///
///    The `Drop` implementation will `panic` if a guard is not dropped in the
///    same order that it was created, relative to other guards (LIFO order).
#[derive(Debug)]
pub struct AttachGuard<'local> {
    env: Env<'local>,
}

/// Increments the thread guard level, returning the new level.
fn thread_guard_level_push(env: *mut jni_sys::JNIEnv) -> usize {
    THREAD_GUARD_NEST_LEVEL.with(|cell| {
        let level = cell.get();
        if level == 0 {
            THREAD_ATTACHMENT.set(env);
        }
        cell.set(level + 1);
        level + 1
    })
}

/// Decrements the thread guard level, returning the new level.
fn thread_guard_level_pop() -> usize {
    let level = THREAD_GUARD_NEST_LEVEL.with(|cell| {
        let level = cell.get();
        assert_ne!(
            level, 0,
            "Spuriously dropped more AttachGuards than were known to exist"
        );
        cell.set(level - 1);
        level - 1
    });

    if level == 0 {
        THREAD_ATTACHMENT.set(std::ptr::null_mut());
    }

    level
}

impl<'local> AttachGuard<'local> {
    /// Wrap a raw [`sys::JNIEnv`] pointer in an `AttachGuard` that will detach
    /// the current thread on drop.
    ///
    /// An owned `AttachGuard` can be used to implement "for_scope" thread
    /// attachments.
    ///
    /// # Safety
    ///
    /// The pointer must be non-null and correspond to a valid [`Env`] pointer
    /// that is attached to the current thread.
    ///
    /// The returned guard must be managed according to the general
    /// [`AttachGuard`] "Safety" rules.
    ///
    /// **IMPORTANTLY:** Never give the returned guard a `'static` lifetime; the
    /// guard must be treated as an immovable value on the stack and not be
    /// re-ordered relative to other guards on the stack.
    unsafe fn from_owned(env: *mut sys::JNIEnv) -> Self {
        let level = thread_guard_level_push(env);

        let guard = Self {
            env: unsafe { Env::new(env, level, true) },
        };

        // Guarantee that if you have an `AttachGuard` then
        // `JavaVM::singleton()` will always return `Some(JavaVM)`
        let _vm = guard.env.get_java_vm();

        guard
    }

    /// Wrap a raw [`sys::JNIEnv`] pointer in an [`AttachGuard`] that does not
    /// own the underlying thread attachment and so it will **NOT** detach the
    /// current thread on drop.
    ///
    /// This can be use when implementing native JNI methods (that are passed an
    /// attached [`sys::JNIEnv`] pointer) as a way to access the [`Env`] API.
    ///
    /// It is recommended to use [`EnvUnowned`] instead of directly using this API
    /// since that will make it clearer that the guard is associated with the
    /// JNI stack frame lifetime that was passed to the native method.
    ///
    /// # Safety
    ///
    /// The pointer must be non-null and correspond to a valid [`Env`]
    /// pointer that is attached to the current thread.
    ///
    /// The returned guard must be managed according to the general
    /// [`AttachGuard`] "Safety" rules.
    ///
    /// **IMPORTANTLY:** Never give the returned guard a `'static` lifetime; the
    /// guard must be treated as an immovable value on the stack and not be
    /// re-ordered relative to other guards on the stack.
    pub unsafe fn from_unowned(env: *mut sys::JNIEnv) -> Self {
        let level = thread_guard_level_push(env);
        let guard = Self {
            env: unsafe { Env::new(env, level, false) },
        };

        // Guarantee that if you have an `AttachGuard` then
        // `JavaVM::singleton()` will always return `Some(JavaVM)`
        let _vm = guard.env.get_java_vm();

        guard
    }

    /// Returns true if the guard represents a scoped attachment that will also
    /// detach the thread when it is dropped.
    ///
    /// Note that not all scoped guards from
    /// [`JavaVM::attach_current_thread_for_scope`] will own the attachment,
    /// since the scope may be nested under some other guard, lower on the stack
    /// that has already attached the thread.
    pub fn owns_attachment(&self) -> bool {
        self.env.owns_attachment()
    }

    /// Borrows a mutable reference to the [`Env`] associated with this
    /// [`AttachGuard`], after asserting that this is the top-most
    /// guard on the stack.
    ///
    /// # Panic
    ///
    /// This will panic if the `AttachGuard` does not currently represent the
    /// top JNI stack frame.
    pub fn borrow_env_mut(&mut self) -> &mut Env<'local> {
        self.env.assert_top();
        &mut self.env
    }

    /// Handles detaching the current thread if the guards owns the attachment
    ///
    /// # Safety
    ///
    /// Since this is used as part of the `Drop` implementation then you must
    /// not allow the `Drop` implementation to run if this is called explicitly
    ///
    /// Even though this only takes a reference, the implementation assumes that
    /// the guard is going to be dropped.
    unsafe fn detach_impl(
        &mut self,
        catch_exceptions: bool,
        throw: Option<JThrowable<'local>>,
    ) -> Result<()> {
        let caught_exception = {
            let env = self.borrow_env_mut();
            let caught_exception = if catch_exceptions {
                // note: we don't want to return errors here immediately because it's more important
                // that we do the book-keeping for the guard level and possibly detaching the thread
                env.exception_catch()
            } else {
                Ok(())
            };

            if let Some(throwable) = throw {
                let _ = env.throw(throwable);
            }
            caught_exception
        };

        let new_level = thread_guard_level_pop();
        assert_eq!(
            new_level + 1,
            self.env.level,
            "AttachGuard was dropped out-of-order with respect to other guards"
        );

        let detach_result = if self.owns_attachment() {
            assert_eq!(
                new_level, 0,
                "Spurious AttachGuard that owns its attachment but is nested under another guard"
            );
            unsafe { sys_detach_current_thread(Some(self.env.raw), &std::thread::current()) }
        } else {
            Ok(())
        };

        match detach_result {
            Err(err) => Err(err),
            Ok(()) => caught_exception,
        }
    }

    /// Drop a guard explicitly and detach the current thread if the guard owns
    /// the current attachment.
    ///
    /// Unlike [`AttachGuard::drop`] this returns a `Result` that can indicate
    /// potential JNI errors from attempting to detach the thread.
    ///
    /// # Panics
    ///
    /// This will panic if a guard is dropped out-of-order, with respect to
    /// other guards. Each [`AttachGuard`] created may be nested with respect
    /// to other guards and must be dropped or detached in LIFO order.
    pub fn detach(mut self) -> Result<()> {
        // Safety: we're going to 'forget' the guard afterwards to ensure the
        // `Drop` implementation isn't run too.
        let res = unsafe { self.detach_impl(false, None) };

        // We've effectively dropped the guard manually (so we can also get a `Result`)
        // but that means we shouldn't allow the `Drop` implementation to run too.
        std::mem::forget(self);

        res
    }

    /// Catch any exceptions and then Drop the guard + detach thread (if owned).
    ///
    /// This is similar to [`AttachGuard::detach`], but will also catch (clear)
    /// any pending Java exceptions and return them as an
    /// [`Error::CaughtJavaException`].
    ///
    /// Pending exceptions are caught, regardless of whether this is an "owned"
    /// attachment or not.
    ///
    /// # Panics
    ///
    /// This will panic if a guard is dropped out-of-order, with respect to
    /// other guards. Each [`AttachGuard`] created may be nested with respect
    /// to other guards and must be dropped or detached in LIFO order.
    pub fn detach_with_catch(mut self) -> Result<()> {
        // Safety: we're going to 'forget' the guard afterwards to ensure the
        // `Drop` implementation isn't run too.
        let res = unsafe { self.detach_impl(true, None) };

        // We've effectively dropped the guard manually (so we can also get a `Result`)
        // but that means we shouldn't allow the `Drop` implementation to run too.
        std::mem::forget(self);

        res
    }

    /// Catch any exceptions, throw `throwable` and then Drop the guard + detach
    /// thread (if owned).
    ///
    /// This is similar to [`AttachGuard::detach`], but will also catch (clear)
    /// any pending Java exceptions and then throw the provided `throwable`. Any
    /// caught exception will be returned as an [`Error::CaughtJavaException`].
    ///
    /// This API can be used in case you have temporarily caught/cleared an
    /// exception to run JNI code and you then want to catch any exceptions for
    /// the run code before re-throwing the exception saved earlier.
    ///
    /// # Panics
    ///
    /// This will panic if a guard is dropped out-of-order, with respect to
    /// other guards. Each [`AttachGuard`] created may be nested with respect
    /// to other guards and must be dropped or detached in LIFO order.
    pub fn detach_with_catch_and_throw(
        mut self,
        throwable: Option<JThrowable<'local>>,
    ) -> Result<()> {
        // Safety: we're going to 'forget' the guard afterwards to ensure the
        // `Drop` implementation isn't run too.
        let res = unsafe { self.detach_impl(true, throwable) };

        // We've effectively dropped the guard manually (so we can also get a `Result`)
        // but that means we shouldn't allow the `Drop` implementation to run too.
        std::mem::forget(self);

        res
    }
}

impl Drop for AttachGuard<'_> {
    fn drop(&mut self) {
        if let Err(err) = unsafe { self.detach_impl(false, None) } {
            // This probably means that something `unsafe` happened to detach the thread already
            log::error!("Failed to detach current JNI thread: {err}");
        }
    }
}

/// A non-Send, non-Sync token, with no `const` constructor, representing a
/// local scope when attaching the current thread to a Java VM.
///
/// This gives us something for an [`AttachGuard`] to borrow that's not likely
/// to be accidentally made `'static`.
///
/// This is only relevant for `unsafe` code that is manually creating
/// [`AttachGuard`]s.
///
/// See: [JavaVM::attach_current_thread_guard] and [JavaVM::get_env_attachment].
#[derive(Debug, Default)]
pub struct ScopeToken {
    _non_send_sync: std::marker::PhantomData<*const ()>,
}

#[cfg(test)]
mod test {
    use crate::{AttachGuard, ScopeToken};
    static_assertions::assert_not_impl_any!(ScopeToken: Send);
    static_assertions::assert_not_impl_any!(ScopeToken: Sync);

    static_assertions::assert_not_impl_any!(AttachGuard: Send);
    static_assertions::assert_not_impl_any!(AttachGuard: Sync);
}
