use crate::jni_str;

crate::bind_java_type! {
    pub JClassLoader => "java.lang.ClassLoader",
    hooks {
        load_class = |env, _loader_context, _initialize| {
            // As a special-case; we ignore loader_context and use `env.find_class` just to be clear that there's no risk of
            // recursion. (`LoaderContext::load_class` depends on the `JClassLoaderAPI`)
            env.find_class(const { jni_str!("java/lang/ClassLoader") })
        }
    },
    methods {
        /// Gets the system class loader.
        ///
        /// This is a method binding for `java.lang.ClassLoader.getSystemClassLoader()`.
        ///
        /// # Throws
        ///
        /// - `SecurityException` if a security manager doesn't allow access to the system class loader.
        /// - `IllegalStateException` if called recursively while the system class loader is being initialized.
        /// - `Error` if the system class loader could not be created according to the system property "java.system.class.loader".
        static fn get_system_class_loader() -> JClassLoader,
        /// Loads a class by name using this class loader.
        ///
        /// This is a Java method binding for `java.lang.ClassLoader.loadClass(String)`.
        ///
        /// # Throws
        ///
        /// `ClassNotFoundException` if the class cannot be found.
        fn load_class(name: JString) -> JClass,
    }
}
