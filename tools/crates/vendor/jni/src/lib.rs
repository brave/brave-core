#![warn(missing_docs)]
#![allow(clippy::upper_case_acronyms)]
// TODO: https://github.com/jni-rs/jni-rs/issues/348
#![allow(clippy::not_unsafe_ptr_arg_deref)]
#![deny(missing_debug_implementations)]

//! # Safe JNI Bindings in Rust
//!
//! This crate provides a (mostly) safe way to interface with Java and Kotlin
//! using the Java Native Interface (JNI).
//!
//! ## Getting Started by implementing a native method
//!
//! Lets write some Java code that defines a native method and then implement it
//! in Rust.
//!
//! ### The Java side
//!
//! First, you need a Java class definition. `HelloWorld.java`:
//!
//! ```java
//! class HelloWorld {
//!     // This declares that the static `hello` method will be provided
//!     // by a native library.
//!     private static native String hello(String input);
//!
//!     static {
//!         // This actually loads the shared object that we'll be creating.
//!         // The actual location of the .so or .dll may differ based on your
//!         // platform.
//!         System.loadLibrary("mylib");
//!     }
//!
//!     // The rest is just regular ol' Java!
//!     public static void main(String[] args) {
//!         String output = HelloWorld.hello("World");
//!         System.out.println(output);
//!     }
//! }
//! ```
//!
//! Compile this to a class file with `javac HelloWorld.java`.
//!
//! Trying to run it now will give us the error `Exception in thread "main"
//! java.lang.UnsatisfiedLinkError: no mylib in java.library.path` since we
//! haven't written our native code yet.
//!
//! ### The Rust side
//!
//! There are two approaches to implementing a native method in Rust.
//!
//! In either case you first need to implement an `extern "system"` function
//! that accepts the appropriate arguments and has a matching return type.
//!
//! And then to make the implementation visible to the JVM you can either:
//! 1. Export the function from a shared library with a mangled name that the
//!    JVM can look up.
//! 2. You can use the JNI API to explicitly register the implementation with
//!    the JVM at runtime using [`Env::register_native_methods`] (in this case
//!    the function name doesn't need to be mangled or exported from a shared
//!    library)
//!
//! The arguments and return type for a native method implementation are
//! documented in the JNI specification under "Design" -> "Native Method
//! Arguments"
//!
//! <https://docs.oracle.com/en/java/javase/11/docs/specs/jni/design.html#native-method-arguments>
//!
//! which can be summarized as:
//!
//! - The first argument is always an implicit JNI environment pointer that
//!   represents the current thread's attachment to the Java VM
//!     - Use [`jni::EnvUnowned`] to capture this
//! - Next, if the native method is static, the second argument is a reference
//!   to the class that owns the method.
//!     - Use [`jni::objects::JClass`] in this case
//!     - Alternatively, use [`jni::sys::jclass`] if you want to work with the
//!       raw JNI type.
//! - Else, if the native method is non-static, the second argument is a `this`
//!   object reference
//!     - Use [`jni::objects::JObject`] or a more specific reference type like
//!       [`jni::objects::JString`] in this case
//!     - Alternatively, use [`jni::sys::jobject`] if you want to work with the
//!       raw JNI type.
//! - The rest of the arguments are the Java arguments passed to the method:
//!     - Java primitive types are passed as their corresponding JNI types (e.g.
//!       [`jni::sys::jint`] for `int`)
//!     - Java object types are passed as their corresponding JNI reference
//!       types (e.g. `jni::objects::JString` for `java.lang.String`)
//!     - Alternatively you can use the raw [`jni::sys::jobject`] type for Java
//!       object arguments if you want to work with the raw JNI types.
//! - The return type is the Java return type mapped in the same way as the
//!   arguments
//!
//! #### Exporting a native method from a shared library
//!
//! Create your crate with `cargo new mylib`. This will create a directory
//! `mylib` that has everything needed to build a basic crate with `cargo`. We
//! need to make a couple of changes to `Cargo.toml` before we do anything else.
//!
//! * Under `[dependencies]`, add `jni = "0.22"`
//! * Add a new `[lib]` section and under it, `crate-type = ["cdylib"]`.
//!
//! Now, if you run `cargo build` from inside the crate directory, you should
//! see a `libmylib.so` (if you're on Linux) or a `libmylib.dylib` (if you are
//! on macOS) in the `target/debug` directory.
//!
//! The last thing we need to do is implement our native method.
//!
//! As an illustration, we'll avoid using any of the helper macros (like
//! [`jni::jni_mangle`] or [`jni::native_method`]) so it's clear how the
//! underlying API works.
//!
//! Add this to your crate's `src/lib.rs`:
//!
//! ```rust,no_run
//! // As we are going to implement a native method in Rust, the JVM is going
//! // to pass us a `jni_sys::JNIEnv` pointer that implicitly represents
//! // an attachment of the current thread to the Java VM.
//! //
//! // `EnvUnowned` is an FFI-safe type that lets us capture the pointer and also
//! // associate the caller's JNI stack frame with a lifetime.
//! //
//! // IMPORTANT: The first thing a native method must do is to upgrade this into
//! // an `Env`, which has a side effect of ensuring the `jni-rs` crate is initialized
//! // before any other JNI calls are made.
//! use jni::EnvUnowned;
//!
//! // This is the JNI environment interface we'll call the majority of our methods on.
//! use jni::Env;
//!
//! // These objects are what you should use as arguments to your native
//! // function. They carry extra lifetime information to prevent them escaping
//! // this context and getting used after being GC'd.
//! use jni::objects::{JClass, JString};
//!
//! // This type represents an owned, JNI-compatible string.
//! use jni::strings::JNIString;
//!
//! // This keeps Rust from "mangling" the name and making it unique for this crate.
//! #[unsafe(no_mangle)]
//! pub extern "system" fn Java_HelloWorld_hello<'caller>(
//!     // This captures the raw `jni_sys::JNIEnv` pointer and associates it with the
//!     // caller's JNI stack frame lifetime.
//!     mut unowned_env: EnvUnowned<'caller>,
//!     // This is the class that owns our static method. It's not going to be used,
//!     // but still must be present to match the expected signature of a static
//!     // native method.
//!     //
//!     // If we were implementing a non-static native method then this would be
//!     // a `this: JObject<'caller>`, serving as a reference to the instance.
//!     class: JClass<'caller>,
//!     input: JString<'caller>)
//!     -> JString<'caller>
//! {
//!     // Before we can access JNI, jni-rs needs to know that the thread is
//!     // attached to the Java VM.
//!     //
//!     // Within a native method we can assume the JVM attaches the thread
//!     // before calling our implementation, and this is represented
//!     // by the EnvUnowned type.
//!     //
//!     // We use `with_env` to upgrade the EnvUnowned to a Env, which gives us
//!     // access to the full JNI API.
//!     //
//!     // IMPORTANT: Don't use `jni-rs` in any other way within a native method
//!     // until you call `unowned_env.with_env` or `AttachGuard::from_unowned`.
//!     //
//!     // Internally `with_env()` creates a hidden `AttachGuard`` to track the thread
//!     // attachment explicitly and this will also wrap the given closure with
//!     // `catch_unwind` to ensure that your code can't panic and unwind across
//!     // FFI boundaries.
//!     let outcome = unowned_env.with_env(|env| -> Result<_, jni::errors::Error> {
//!         // First, we have to get the string out of Java. Check out the `strings`
//!         // module for more info on how this works.
//!         let input: String = input.to_string();
//!         // Then we have to create a new Java string to return. Again, more info
//!         // in the `strings` module.
//!         JString::from_str(env, format!("Hello, {}!", input))
//!     });
//!
//!    // Finally, we have to resolve the `EnvOutcome` into a concrete return value.
//!    //
//!    // Our code above may have failed with a JNI error, or some other
//!    // application-specific error, or it may have panicked. None of these things
//!    // can pass over the FFI boundary back into the JVM.
//!    //
//!    // Mapping of errors and panics is done according to the selected
//!    // `ErrorPolicy` trait implementation which is able to use JNI for throwing
//!    // Java exceptions if necessary.
//!    //
//!    // This design lets you encapsulate your own approach to forwarding errors
//!    // and panics to Java code and then easily reuse it across multiple native
//!    // methods.
//!    //
//!    // In this case we use a built-in policy that throws a Java
//!    // `RuntimeException` with a message containing the error/panic details.
//!     outcome.resolve::<jni::errors::ThrowRuntimeExAndDefault>()
//! }
//! ```
//!
//! The `Java_HelloWorld_hello` function name was mangled according to the JNI
//! Specification, under "Design" -> "Resolving Native Method Names"
//!
//! <https://docs.oracle.com/en/java/javase/11/docs/specs/jni/design.html#resolving-native-method-names>
//!
//! To avoid having to manually mangle the function name, the
//! [`jni::jni_mangle`], [`jni::native_method`] and [`jni::bind_java_type`]
//! macros can all handle this for you.
//!
//! ### Final steps
//!
//! That's it! Build your crate and try to run your Java class again.
//!
//! ... Same error as before you say? Well that's because JVM is looking for
//! `mylib` in all the wrong places. To tell it where to look you can pass this
//! argument to `java`: `-Djava.library.path=/path/to/mylib/target/debug`. Now,
//! you should get the expected output `Hello, World!` from your Java class.
//!
//! For reference you can find this example in the
//! [`mylib-example`][jni-rs-mylib-example] directory of this repository:
//!
//! ## Launching JVM from Rust
//!
//! It is possible to launch a JVM from a native process using the [Invocation
//! API], provided by [`JavaVM`].
//!
//! ## See Also
//!
//! ### Examples
//! - The [examples directory][jni-rs-examples] in this repository contains a
//!   variety of examples that illustrate the utility macros provided by this
//!   crate.
//! - [Example mylib project][jni-rs-mylib-example] (as illustrated above)
//! - Our [integration tests][jni-rs-its] and [benchmarks][jni-rs-benches]
//!
//! ### Macros
//! - [`jni_str!`] for compile-time encoding of JNI strings
//! - [`jni_sig!`] for compile-time encoding of JNI type signatures
//! - [`bind_java_type`] for generating full Rust bindings for Java types
//! - [`native_method`] for individual native method bindings
//! - [`jni_mangle`] for mangling native method names
//!
//! ### JNI Documentation
//! - [Java Native Interface Specification][jni-spec]
//! - [JNI tips][jni-tips] — general tips on JNI development and some
//!   Android-specific
//!
//! ### Open-Source Users
//! - The Servo browser engine Android [port][users-servo]
//! - The Exonum framework [Java Binding][users-ejb]
//! - MaidSafe [Java Binding][users-maidsafe]
//!
//! ### Other Projects Simplifying Java and Rust Communication
//! - Consider [JNR][projects-jnr] if you just need to use a native library with
//!   C interface
//! - Watch OpenJDK [Project Panama][projects-panama] which aims to enable using
//!   native libraries with no JNI code
//! - Consider [GraalVM][projects-graalvm] — a recently released VM that gives
//!   zero-cost interoperability between various languages (including Java and
//!   [Rust][graalvm-rust] compiled into LLVM-bitcode)
//! - See a [plugin](https://github.com/questdb/rust-maven-plugin/) for invoking
//!   cargo from Java Maven builds
//!
//! [Invocation API]:
//!     https://docs.oracle.com/en/java/javase/21/docs/specs/jni/invocation.html
//! [jni-spec]:
//!     https://docs.oracle.com/en/java/javase/21/docs/specs/jni/index.html
//! [jni-tips]: https://developer.android.com/training/articles/perf-jni
//! [jni-rs-examples]:
//!     https://github.com/jni-rs/jni-rs/tree/master/examples
//! [jni-rs-mylib-example]:
//!     https://github.com/jni-rs/jni-rs/tree/master/mylib-example
//! [jni-rs-its]: https://github.com/jni-rs/jni-rs/tree/master/tests
//! [jni-rs-benches]: https://github.com/jni-rs/jni-rs/tree/master/benches
//! [users-servo]:
//!     https://github.com/servo/servo/tree/main/ports/servoshell/egl/android
//! [users-ejb]:
//!     https://github.com/exonum/exonum-java-binding/tree/master/exonum-java-binding/core/rust
//! [users-maidsafe]:
//!     https://github.com/maidsafe/safe_client_libs/tree/master/safe_app_jni
//! [projects-jnr]: https://github.com/jnr/jnr-ffi/
//! [projects-graalvm]: http://www.graalvm.org/docs/why-graal/#for-java-programs
//! [graalvm-rust]:
//!     http://www.graalvm.org/docs/reference-manual/languages/llvm/#running-rust
//! [projects-panama]: https://jdk.java.net/panama/

#[doc = include_str!("../docs/0.22-MIGRATION.md")]
#[cfg(doctest)]
pub struct MigrationDoctests;

/// `jni-sys` re-exports
pub use jni_sys as sys;

mod version;
pub use self::version::*;

#[cfg(windows)]
mod windows_sys;

#[macro_use]
mod macros;

/// Error types and utilities.
pub mod errors;

/// Parser for java type signatures.
pub mod signature;

/// Descriptors for classes and method IDs.
pub mod descriptors;

// Wrappers around jni value types that add lifetimes and other functionality.
mod jvalue;
pub use jvalue::{JValue, JValueOwned};

#[doc(hidden)]
#[deprecated(
    since = "0.22.0",
    note = "Please use `jni::string::{char_from_*, char_to_*}` instead of `jni::{char_from_*, char_to_*}`"
)]
pub use jvalue::{char_from_java, char_from_java_int, char_to_java, char_to_java_int};

/// JNI identifiers for methods and fields
pub mod ids;

/// Base support for reference types and auto-release wrappers like [`refs::Auto`] and [`refs::Global`].
pub mod refs;

/// Reference type implementations and API bindings for various `java.lang` Objects
pub mod objects;

/// Reference type implementations and API bindings for various `java.lang` Exceptions
pub mod exceptions;

/// Helpers for accessing array elements
pub mod elements;

/// Handling of strings in Java's [modified UTF-8] encoding, including
/// conversion to and from Rust strings (which use standard UTF-8).
///
/// [modified UTF-8]: https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8
pub mod strings;

mod env;
pub use env::*;

#[deprecated(
    since = "0.22.0",
    note = r#"Since 0.22, `JNIEnv` has been split into `Env` and `EnvUnowned`

- `EnvUnowned` is FFI safe and can be accepted as an argument for native methods
- `Env` is what provides the full JNI environment API and is not FFI safe.

To remain safe by default, `jni::JNIEnv` is now an alias for `EnvUnowned` (which is FFI safe).

Use `EnvUnowned` to capture a raw `jni_sys::JNIEnv` pointer in native methods, like:
`pub extern "system" fn Java_HelloWorld_hello<'frame>(unowned_env: EnvUnowned<'frame>, ...)`
Then use `unowned_env.with_env()` to upgrade it to an `Env` reference.

Most of the time you should temporarily acquire a `Env` reference using:
- `JavaVM::attach_current_thread` (preferred) or `JavaVM::attach_current_thread_for_scope`
- `JavaVM::with_env` (if certain that the thread is already attached)
- `UnownedEnv::with_env()` in native methods

When migrating to 0.22, if you have a mixture of FFI/non-FFI usage of JNIEnv then err on the side
of renaming `JNIEnv` to `EnvUnowned` because that's safe for FFI and for non-FFI usage there will be
clear compiler errors if trying to access the real `Env` API through the `EnvUnowned` type.
"#
)]
/// An FFI safe alias for `EnvUnowned` for (safer) compatibility with
/// existing code.
///
/// Since 0.22 ([#570](https://github.com/jni-rs/jni-rs/pull/570)) the
/// `JNIEnv` type was renamed to [Env], which is no longer an FFI safe
/// pointer wrapper.
///
/// FFI usage of `JNIEnv`, within native method arguments, should be
/// migrated to [EnvUnowned], followed by [`EnvUnowned::with_env`].
///
/// To help make this clear and sign post how to safely migrate to the [Env]
/// and [EnvUnowned] types, we export this deprecated alias with a warning.
pub type JNIEnv<'frame> = EnvUnowned<'frame>;

/// Java VM interface.
pub mod vm;

#[doc(hidden)]
#[deprecated(
    since = "0.22.0",
    note = "Please use `jni::vm::JavaVM` instead of `jni::JavaVM`."
)]
pub use self::vm::*;

// workaround proc-macro-crate limitations so we can use jni-macros from the jni crate itself
// ref: https://github.com/bkchr/proc-macro-crate/issues/11
extern crate self as jni;

#[doc(inline)]
pub use jni_macros::jni_cstr;
#[doc = include_str!("../docs/macros/jni_str.md")]
pub use jni_macros::jni_str;

#[doc = include_str!("../docs/macros/jni_mangle.md")]
pub use jni_macros::jni_mangle;

#[doc = include_str!("../docs/macros/jni_sig.md")]
pub use jni_macros::jni_sig;
#[doc(inline)]
pub use jni_macros::jni_sig_cstr;
#[doc(inline)]
pub use jni_macros::jni_sig_jstr;
#[doc(inline)]
pub use jni_macros::jni_sig_str;

#[doc = include_str!("../docs/macros/bind_java_type_0_overview.md")]
#[doc = include_str!("../docs/macros/bind_java_type_1_properties.md")]
#[doc = include_str!("../docs/macros/bind_java_type_2_examples.md")]
#[doc = include_str!("../docs/macros/bind_java_type_3_advanced.md")]
pub use jni_macros::bind_java_type;

#[doc = include_str!("../docs/macros/native_method.md")]
pub use jni_macros::native_method;

// For internal jni call macros to ensure their results are checked
#[doc(hidden)]
#[must_use]
pub fn __must_use<T>(v: T) -> T {
    v
}

// Provides a way to validate all our object bindings in a unit test, considering
// that the `J<Foo>API::get` functions are only exported with `pub(crate)` visibility.
//
// If any typos in method names or incorrect method or field signatures will result
// in a panic here when the binding initialization fails.
#[doc(hidden)]
pub fn __test_bindings_init(env: &crate::Env, loader: &crate::refs::LoaderContext) {
    objects::JByteBufferAPI::get(env, loader)
        .expect("Failed to initialize JByteBufferAPI bindings");
    objects::JClassLoaderAPI::get(env, loader)
        .expect("Failed to initialize JClassLoaderAPI bindings");
    objects::JClassAPI::get(env, loader).expect("Failed to initialize JClassAPI bindings");
    objects::JCharSequenceAPI::get(env, loader)
        .expect("Failed to initialize JCharSequenceAPI bindings");
    objects::JCollectionAPI::get(env, loader)
        .expect("Failed to initialize JCollectionAPI bindings");
    objects::JIteratorAPI::get(env, loader).expect("Failed to initialize JIteratorAPI bindings");
    objects::JListAPI::get(env, loader).expect("Failed to initialize JListAPI bindings");
    objects::JMapAPI::get(env, loader).expect("Failed to initialize JMapAPI bindings");
    objects::JMapEntryAPI::get(env, loader).expect("Failed to initialize JMapEntryAPI bindings");
    objects::JObjectArrayAPI::<objects::JString>::get(env, loader)
        .expect("Failed to initialize JObjectArrayAPI<JString> bindings");
    objects::JObjectAPI::get(env).expect("Failed to initialize JObjectAPI bindings");
    objects::JPrimitiveArrayAPI_jboolean::get(env, loader)
        .expect("Failed to initialize JPrimitiveArrayAPI_jboolean bindings");
    objects::JSetAPI::get(env, loader).expect("Failed to initialize JSetAPI bindings");
    objects::JStackTraceElementAPI::get(env, loader)
        .expect("Failed to initialize JStackTraceElementAPI bindings");
    objects::JStringAPI::get(env, loader).expect("Failed to initialize JStringAPI bindings");
    objects::JThreadAPI::get(env, loader).expect("Failed to initialize JThreadAPI bindings");
    objects::JThrowableAPI::get(env, loader).expect("Failed to initialize JThrowableAPI bindings");

    exceptions::JArrayIndexOutOfBoundsExceptionAPI::get(env, loader)
        .expect("Failed to initialize JArrayIndexOutOfBoundsException bindings");
    exceptions::JArrayStoreExceptionAPI::get(env, loader)
        .expect("Failed to initialize JArrayStoreException bindings");
    exceptions::JClassCircularityErrorAPI::get(env, loader)
        .expect("Failed to initialize JClassCircularityError bindings");
    exceptions::JClassFormatErrorAPI::get(env, loader)
        .expect("Failed to initialize JClassFormatError bindings");
    exceptions::JExceptionInInitializerErrorAPI::get(env, loader)
        .expect("Failed to initialize JExceptionInInitializerError bindings");
    exceptions::JIllegalArgumentExceptionAPI::get(env, loader)
        .expect("Failed to initialize JIllegalArgumentException bindings");
    exceptions::JIllegalMonitorStateExceptionAPI::get(env, loader)
        .expect("Failed to initialize JIllegalMonitorStateException bindings");
    exceptions::JInstantiationExceptionAPI::get(env, loader)
        .expect("Failed to initialize JInstantiationException bindings");
    exceptions::JNoClassDefFoundErrorAPI::get(env, loader)
        .expect("Failed to initialize JNoClassDefFoundError bindings");
    exceptions::JNoSuchFieldErrorAPI::get(env, loader)
        .expect("Failed to initialize JNoSuchFieldError bindings");
    exceptions::JNoSuchMethodErrorAPI::get(env, loader)
        .expect("Failed to initialize JNoSuchMethodError bindings");
    exceptions::JOutOfMemoryErrorAPI::get(env, loader)
        .expect("Failed to initialize JOutOfMemoryError bindings");
    exceptions::JSecurityExceptionAPI::get(env, loader)
        .expect("Failed to initialize JSecurityException bindings");
    exceptions::JStringIndexOutOfBoundsExceptionAPI::get(env, loader)
        .expect("Failed to initialize JStringIndexOutOfBoundsException bindings");
}
