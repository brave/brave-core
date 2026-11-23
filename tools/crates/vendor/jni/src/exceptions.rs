#[cfg(doc)]
use crate::errors::Error;

macro_rules! bind_exception {
    ($rust_type:ident => $java_type:literal $($rest:tt)*) => {
        $crate::bind_java_type! {
            pub $rust_type => $java_type,
            is_instance_of = {
                throwable: JThrowable
            }
            $($rest)*
        }

        impl<'local> $rust_type<'local> {
            /// Checks if the given throwable is an instance of this exception type.
            ///
            #[doc = concat!("Returns `Some(Cast<", stringify!($rust_type), ">)` if the throwable is an instance of this exception type")]
            /// or returns `None` if it is not.
            ///
            /// Returns [Error::NullPtr] if the throwable is null.
            pub fn matches<'any, 'from>(
                env: &crate::Env,
                throwable: &'from crate::objects::JThrowable<'any>,
            ) -> $crate::errors::Result<Option<$crate::refs::Cast<'any, 'from, $rust_type<'any>>>>
            {
                if throwable.is_null() {
                    return Err($crate::errors::Error::NullPtr("Invalid null Throwable"));
                }
                let class = <$rust_type as $crate::refs::Reference>::lookup_class(
                    env,
                    &Default::default(),
                )?;
                let class: &$crate::objects::JClass = &class;
                if env.is_instance_of_class(throwable, class)? {
                    return Ok(Some(unsafe {
                        $crate::refs::Cast::new_unchecked(throwable)
                    }));
                } else {
                    return Ok(None);
                }
            }
        }
    };
}

/// Binds a simple exception that just has a void constructor and message constructor
macro_rules! bind_basic_exception {
    ($rust_type:ident => $java_type:literal $($rest:tt)*) => {
        bind_exception! {
            $rust_type => $java_type,
            constructors {
                /// Construct without any message
                fn new_null(),
                /// Construct with a message
                fn new(msg: JString),
            }
            $($rest)*
        }
    };
}

bind_exception! {
    JArrayIndexOutOfBoundsException => "java.lang.ArrayIndexOutOfBoundsException",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message that indicates the illegal index
        fn new_for_index(index: jint),
    }
}
bind_basic_exception! { JArrayStoreException => "java.lang.ArrayStoreException" }
bind_basic_exception! { JClassCircularityError => "java.lang.ClassCircularityError" }
bind_basic_exception! { JClassFormatError => "java.lang.ClassFormatError" }
bind_exception! {
    JExceptionInInitializerError => "java.lang.ExceptionInInitializerError",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with only an exception cause and a `null` message
        fn new_with_exception(cause: JThrowable)
    },
    methods {
        /// Returns the exception that was thrown during static initialization.
        fn get_cause() -> JThrowable,

        /// Returns the exception that was thrown during static initialization.
        fn get_exception() -> JThrowable
    }
}
bind_basic_exception! {
    JClassNotFoundException => "java.lang.ClassNotFoundException",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message and a cause
        fn new_with_cause(msg: JString, cause: JThrowable),
    },
    methods {
        /// Returns the exception that was raised if an error occurred while attempting to load the class (the cause)
        fn get_cause() -> JThrowable,
    }
}
bind_exception! {
    JIllegalArgumentException => "java.lang.IllegalArgumentException",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message and a cause
        fn new_with_cause(msg: JString, cause: JThrowable),
        /// Construct with only a cause
        ///
        /// The message will be `null` if the cause is `null` or the message
        /// will come from the cause.
        fn new_with_only_cause(cause: JThrowable)
    }
}
bind_basic_exception! { JIllegalMonitorStateException => "java.lang.IllegalMonitorStateException" }
bind_basic_exception! { JInstantiationException => "java.lang.InstantiationException" }
bind_exception! {
    JLinkageError => "java.lang.LinkageError",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message and a cause
        fn new_with_cause(msg: JString, cause: JThrowable),
    },
}
bind_basic_exception! { JNoClassDefFoundError => "java.lang.NoClassDefFoundError" }
bind_basic_exception! { JNoSuchFieldError => "java.lang.NoSuchFieldError" }
bind_basic_exception! { JNoSuchMethodError => "java.lang.NoSuchMethodError" }
bind_basic_exception! { JNumberFormatException => "java.lang.NumberFormatException" }
bind_basic_exception! { JOutOfMemoryError => "java.lang.OutOfMemoryError" }
bind_exception! {
    JRuntimeException => "java.lang.RuntimeException",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message and a cause
        fn new_with_cause(msg: JString, cause: JThrowable)
    }
}
bind_exception! {
    JSecurityException => "java.lang.SecurityException",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message and a cause
        fn new_with_cause(msg: JString, cause: JThrowable),
        /// Construct with only a cause
        ///
        /// The message will be `null` if the cause is `null` or the message
        /// will come from the cause.
        fn new_with_only_cause(cause: JThrowable)
    }
}
bind_exception! {
    JStringIndexOutOfBoundsException => "java.lang.StringIndexOutOfBoundsException",
    constructors {
        /// Construct without any message
        fn new_null(),
        /// Construct with a message
        fn new(msg: JString),
        /// Construct with a message that indicates the illegal index
        fn new_for_index(index: jint),
    }
}
