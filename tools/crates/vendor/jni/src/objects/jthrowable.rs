crate::bind_java_type! {
    pub JThrowable => "java.lang.Throwable",
    __jni_core = true,
    __sys_type = jthrowable,
    methods {
        /// Get the message of the throwable by calling the `getMessage` method.
        fn get_message() -> JString,

        /// Get the cause of the throwable by calling the `getCause` method.
        fn get_cause() -> JThrowable,

        /// Gets the stack trace of the throwable by calling the `getStackTrace` method.
        fn get_stack_trace() -> JStackTraceElement[],

        /// Associate a suppressed throwable with this throwable by calling the `addSuppressed`
        /// method.
        ///
        /// A suppressed exception is one that was thrown but not propagated because another
        /// exception was thrown with a higher precedence. This is distinct from the "cause" of the
        /// exception because it's not assumed to be the direct cause of the higher-precedence
        /// exception.
        fn add_suppressed(throwable: JThrowable),

        /// Get the list of throwables that were suppressed by this throwable.
        fn get_suppressed() -> JThrowable[],
    }
}
