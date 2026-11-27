use crate::jni_str;

crate::bind_java_type! {
    pub JThread => "java.lang.Thread",
    hooks {
        load_class = |env, _loader_context, _initialize| {
            // As a special-case; we ignore loader_context and use `env.find_class` just to be clear that there's no risk of
            // recursion. (`LoaderContext::load_class` depends on the `JThreadAPI`)
            env.find_class(const { jni_str!("java/lang/Thread") })
        }
    },
    methods {
        /// Returns a reference to the currently executing thread object.
        ///
        /// This is a Java method binding for `java.lang.Thread.currentThread()`.
        static fn current_thread() -> JThread,
        /// Get the name of this thread.
        fn get_name() -> JString,
        /// Sets the name of this thread.
        ///
        /// # Throws
        ///
        /// - `SecurityException` if the current thread is not allowed to modify this thread's name
        fn set_name(name: JString) -> (),
        /// Gets the ID of this thread.
        fn get_id() -> jlong,
        /// Gets the context class loader for this thread.
        ///
        /// This is a Java method binding for `java.lang.Thread#getContextClassLoader()`.
        ///
        /// # Throws
        ///
        /// - `SecurityException` if the current thread can't access its context class loader.
        fn get_context_class_loader() -> JClassLoader,
        /// Sets the context class loader for this thread.
        ///
        /// The `loader` may be `null` to indicate the system class loader.
        /// This is a Java method binding for `java.lang.Thread#setContextClassLoader(java.lang.ClassLoader)`.
        ///
        /// # Throws
        ///
        /// - `SecurityException` if the current thread can't access its context class loader.
        fn set_context_class_loader(loader: JClassLoader) -> (),
    }
}
