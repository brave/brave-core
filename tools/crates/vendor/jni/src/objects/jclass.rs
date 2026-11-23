use crate::jni_str;

crate::bind_java_type! {
    pub JClass => "java.lang.Class",
    __jni_core = true,
    __sys_type = jclass,
    hooks {
        load_class = |env, _loader_context, _initialize| {
            // As a special-case; we ignore loader_context and use `env.find_class` just to be clear
            // that there's no risk of recursion. (`LoaderContext::load_class` depends on the
            // `JClassAPI`)
            env.find_class(const { jni_str!("java/lang/Class") })
        }
    },
    methods {
        /// Returns the name of the class.
        ///
        /// This returns a binary name, such as `java.lang.String`.
        ///
        /// If the class represents a primitive type then this will return the same letter used for
        /// that primitive in a JNI type signature - such as `I` for `int`.
        ///
        /// Similarly; if the class represents an array type the name will have a prefix of one or
        /// more "[" characters, followed by the binary name of the element type.
        fn get_name() -> JString,

        /// Finds a class by its fully-qualified binary name or array descriptor.
        ///
        /// This is a method binding for `java.lang.Class.forName(String)`
        ///
        /// This method is used to locate a class by its name, which may be either a fully-qualified
        /// binary name (e.g., `java.lang.String`) or an array descriptor (e.g., `[Ljava.lang.String;`).
        ///
        /// Note: that unlike `FindClass` the names use dot (`.`) notation instead of slash (`/`) notation.
        ///
        /// # Throws
        ///
        /// This method may throw a `ClassNotFoundException` if the class cannot be found.
        static fn for_name(name: JString) -> JClass,

        /// Finds a class by its fully-qualified binary name or array descriptor.
        ///
        /// This is a method binding for `java.lang.Class.forName(String, boolean, ClassLoader)`
        ///
        /// This method is used to locate a class by its name (via the ClassLoader) which may be either
        /// a fully-qualified binary name (e.g., `java.lang.String`) or an array descriptor (e.g.,
        /// `[Ljava.lang.String;`).
        ///
        /// Note: that unlike `FindClass` the names use dot (`.`) notation instead of slash (`/`) notation.
        ///
        /// If initialized is true, the class will be initialized before it is returned.
        ///
        /// # Throws
        ///
        /// This method may throw a `ClassNotFoundException` if the class cannot be found.
        static fn for_name_with_loader {
            name = "forName",
            sig = (name: JString, initialize: bool, loader: JClassLoader) -> JClass,
        },

        /// Returns the class loader for this class.
        ///
        /// This is used to find the class loader that was responsible for loading this class.
        ///
        /// It may return null for bootstrap classes or objects representing primitive types not associated with a class loader.
        ///
        /// # Throws
        ///
        /// `SecurityException` if the class loader cannot be accessed.
        fn get_class_loader() -> JClassLoader
    }
}
