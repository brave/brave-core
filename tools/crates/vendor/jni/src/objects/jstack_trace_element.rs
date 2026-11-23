crate::bind_java_type! {
    pub JStackTraceElement => "java.lang.StackTraceElement",
    methods {
        /// Get the class name of the stack trace element.
        fn get_class_name() -> JString,
        /// Get the file name of the stack trace element, if available.
        fn get_file_name() -> JString,
        /// Get the line number of the stack trace element.
        fn get_line_number() -> jint,
        /// Get the method name of the stack trace element.
        fn get_method_name() -> JString,
        /// Check if the stack trace element corresponds with a native method.
        fn is_native_method() -> bool,
        /// Returns a string representation of this stack trace element.
        fn try_to_string {
            name = "toString",
            sig = () -> JString,
        }
    }
}

impl<'local> JStackTraceElement<'local> {
    // In jni 0.22.0 and 0.22.1 we were incorrectly trying to lookup an isNative
    // method and although it was impossible to call (because this API binding
    // would fail to initialize) we also exported a public `is_native()` method
    // that code could potentially have linked against.
    #[doc(hidden)]
    #[deprecated(since = "0.22.1", note = "Use `is_native_method` instead")]
    pub fn is_native<'env_local>(
        &self,
        env: &::jni::Env<'env_local>,
    ) -> ::jni::errors::Result<bool> {
        self.is_native_method(env)
    }
}
