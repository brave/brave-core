# Examples

This chapter provides practical examples covering common use cases for the `bind_java_type!` macro.

## Basic Binding

The simplest binding just creates a wrapper type for a Java class:

```rust
use jni::bind_java_type;

bind_java_type! { pub MyType => com.example.MyClass }
```

This generates a `MyType<'local>` wrapper and `MyTypeAPI` struct, but no constructors or methods.

## Constructors

Bind Java constructors to create instances from Rust:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;

bind_java_type! {
    pub Counter => com.example.Counter,
    constructors {
        fn new(),
        fn with_initial(value: jint),
        fn with_name_and_value(name: JString, value: jint),
    },
}

# fn create_counters(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs: bindings need to be generated from actual Java classes
    // No-argument constructor
    let counter1 = Counter::new(env)?;

    // With single argument
    let counter2 = Counter::with_initial(env, 42)?;

    // With multiple arguments
    let name = JString::from_str(env, "my-counter")?;
    let counter3 = Counter::with_name_and_value(env, &name, 100)?;

    Ok(())
}
```

## Instance Methods

Call Java instance methods through the Rust wrapper:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;

bind_java_type! {
    pub Counter => com.example.Counter,
    constructors {
        fn new(),
    },
    methods {
        fn increment(),
        fn decrement(),
        fn get_value() -> jint,
        fn set_value(value: jint),
        fn add(amount: jint),
        fn get_name() -> JString,
    },
}

# fn use_custom_types(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    let counter = Counter::new(env)?;

    counter.increment(env)?;
    counter.add(env, 10)?;

    let value = counter.get_value(env)?;
    println!("Counter value: {}", value);

    let name = counter.get_name(env)?;
    println!("Counter name: {}", name.try_to_string(env)?);

    Ok(())
}
```

## Static Methods

Bind static Java methods:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;
use jni::sys::jint;

# bind_java_type! { pub Counter => com.example.Counter }
bind_java_type! {
    pub Utils => com.example.Utils,
    type_map = {
        Counter => com.example.Counter,
    },
    methods {
        static fn get_version() -> JString,
        static fn add(a: jint, b: jint) -> jint,
        static fn create_default_counter() -> Counter,
    },
}

# fn use_static_methods(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    let version = Utils::get_version(env)?;
    println!("Version: {}", version.try_to_string(env)?);

    let sum = Utils::add(env, 10, 20)?;
    println!("Sum: {}", sum);

    let counter = Utils::create_default_counter(env)?;

    Ok(())
}
```

## Fields

Bind Java fields with getter and setter methods:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;

bind_java_type! {
    pub Person => com.example.Person,
    constructors {
        fn new(),
    },
    fields {
        // Instance fields
        age: jint,
        name: JString,
        active: jboolean,

        // Static fields
        static counter: jint,
        static default_name: JString,
    },
}

# fn use_fields(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    let person = Person::new(env)?;

    // Instance fields
    person.set_age(env, 30)?;
    let age = person.age(env)?;

    let name = JString::from_str(env, "Alice")?;
    person.set_name(env, &name)?;
    let retrieved_name = person.name(env)?;

    person.set_active(env, true)?;
    let is_active = person.active(env)?;

    // Static fields
    Person::set_counter(env, 100)?;
    let count = Person::counter(env)?;

    let default = Person::default_name(env)?;

    Ok(())
}
```

## Arrays

Work with primitive and reference arrays:

```rust,ignore
use jni::bind_java_type;
use jni::Env;
use jni::objects::{JIntArray, JObjectArray, JString};

bind_java_type! {
    pub ArrayUtils => com.example.ArrayUtils,
    methods {
        // 1D primitive array
        static fn sum_array(values: jint[]) -> jint,
        static fn create_array(size: jint, value: jint) -> jint[],

        // 2D primitive array
        static fn transpose(matrix: jint[][]) -> jint[][],

        // 1D reference array
        static fn concat_strings(strings: JString[]) -> JString,

        // 2D reference array
        static fn flatten_strings(matrix: JString[][]) -> JString[],
    },
    fields {
        values: jint[],
        names: JString[],
        matrix: jint[][],
    },
}

# fn use_arrays(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    // Create int array
    let int_array = env.new_int_array(5)?;
    let sum = ArrayUtils::sum_array(env, &int_array)?;

    // Create and use string array
    let str1 = JString::from_str(env, "Hello")?;
    let str2 = JString::from_str(env, "World")?;
    let string_array = env.new_object_array(2, "java/lang/String", &JString::null())?;
    string_array.set_element(env, 0, &str1)?;
    string_array.set_element(env, 1, &str2)?;

    let result = ArrayUtils::concat_strings(env, &string_array)?;

    Ok(())
}
```

## Type Mappings

Use custom types in signatures via `type_map`:

```rust
use jni::bind_java_type;
use jni::Env;

// First, define bindings for custom types
bind_java_type! { pub CustomType => com.example.CustomType, constructors { fn new() } }
bind_java_type! { pub OtherType => com.example.OtherType }

// Then use them in type_map
bind_java_type! {
    pub Container => com.example.Container,
    type_map = {
        CustomType => com.example.CustomType,
        OtherType => com.example.OtherType,
    },
    constructors {
        fn new(),
    },
    methods {
        fn set_custom(value: CustomType),
        fn get_custom() -> CustomType,
        fn process(input: OtherType) -> CustomType,
    },
    fields {
        custom_field: CustomType,
        other_field: OtherType,
    },
}

fn use_custom_types(env: &mut Env) -> jni::errors::Result<()> {
    let container = Container::new(env)?;
    let custom = CustomType::new(env)?;

    container.set_custom(env, &custom)?;
    let retrieved = container.get_custom(env)?;

    Ok(())
}
```

## Native Methods

Implement Java native methods in Rust:

```rust
use jni::{Env, bind_java_type};
use jni::objects::{JClass, JString};
use jni::sys::jint;

bind_java_type! {
    pub Calculator => com.example.Calculator,
    constructors {
        fn new(),
    },
    native_methods {
        // Instance native method
        extern fn native_add(a: jint, b: jint) -> jint,

        // Static native method
        static extern fn native_multiply(a: jint, b: jint) -> jint,

        // With error handling
        extern fn native_divide(a: jint, b: jint) -> jint,
    },
}

// Implement the generated trait
impl CalculatorNativeInterface for CalculatorAPI {
    type Error = jni::errors::Error;

    fn native_add<'local>(
        _env: &mut Env<'local>,
        _this: Calculator<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, Self::Error> {
        Ok(a + b)
    }

    fn native_multiply<'local>(
        _env: &mut Env<'local>,
        _class: JClass<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, Self::Error> {
        Ok(a * b)
    }

    fn native_divide<'local>(
        _env: &mut Env<'local>,
        _this: Calculator<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, Self::Error> {
        if b == 0 {
            Err(jni::errors::Error::JniCall(jni::errors::JniError::Unknown))
        } else {
            Ok(a / b)
        }
    }
}

# fn use_native_methods(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    use jni::refs::LoaderContext;

    // Register native methods by getting the API
    let _api = CalculatorAPI::get(env, &LoaderContext::default())?;

    // Now Java code can call the native methods
    let calc = Calculator::new(env)?;

    Ok(())
}
```

## Raw Native Methods

For performance-critical code, use raw native methods that receive `EnvUnowned` directly:

```rust
use jni::{Env, EnvUnowned, bind_java_type};
use jni::sys::jint;

bind_java_type! {
    pub FastCalc => com.example.FastCalc,
    native_methods {
        // Raw method - no catch_unwind, no error handling
        raw extern fn fast_add(a: jint, b: jint) -> jint,
    },
}

impl FastCalcNativeInterface for FastCalcAPI {
    type Error = jni::errors::Error;

    fn fast_add<'local>(
        _env: EnvUnowned<'local>,
        _this: FastCalc<'local>,
        a: jint,
        b: jint,
    ) -> jint {
        // Direct return, no Result
        a + b
    }
}
```

## Direct Function Implementation

Bypass the trait and provide a direct function for native methods:

```rust
use jni::{Env, bind_java_type};
use jni::sys::jint;

bind_java_type! {
    pub DirectCalc => com.example.DirectCalc,
    native_methods {
        fn native_square {
            sig = (value: jint) -> jint,
            fn = square_impl,
        },
    },
}

// Direct implementation function
# fn square_impl<'local>(
#     _env: &mut Env<'local>,
#     _this: DirectCalc<'local>,
#     value: jint,
# ) -> Result<jint, jni::errors::Error> {
#     Ok(value * value)
# }
```

## Class Hierarchies with `is_instance_of`

Declare class inheritance relationships for safe casting:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JObject;

bind_java_type! {
    pub BaseClass => com.example.Base,
    constructors { fn new() },
}

bind_java_type! {
    pub DerivedClass => com.example.Derived,
    type_map = {
        BaseClass => com.example.Base,
    },
    constructors { fn new() },
    is_instance_of = {
        // With stem: generates as_base() method
        base: BaseClass,
    },
}

# fn use_casting(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    let derived = DerivedClass::new(env)?;

    // Use generated as_base() method (performs runtime check)
    let as_base = derived.as_base();

    // Automatic conversion via From trait
    let as_object: JObject = derived.into();

    Ok(())
}
```

## Private Methods

Create private method bindings for internal use:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;

bind_java_type! {
    pub MyType => com.example.MyType,
    constructors {
        fn new(),
    },
    methods {
        // Private binding with underscore prefix
        priv fn _internal_get_data() -> JString,
    },
}

// Public wrapper in your own impl block
impl<'local> MyType<'local> {
    /// Public API that wraps the private Java method
    pub fn get_data(&self, env: &mut Env<'local>) -> jni::errors::Result<String> {
        let data = self._internal_get_data(env)?;
        data.try_to_string(env)
    }
}
```

## Custom Field Names and Accessors

Override field names and customize getter/setter names:

```rust
use jni::bind_java_type;
use jni::objects::JString;

bind_java_type! {
    pub Config => com.example.Config,
    fields {
        // Custom Java field name
        internal_value {
            sig = jint,
            name = "mInternalValue",  // Java field uses Hungarian notation
        },

        // Read-only field (no setter)
        static VERSION {
            sig = jint,
            get = VERSION,
        },

        // Custom getter/setter names
        data {
            sig = JString,
            get = get_data,
            set = update_data,
        },
    },
}
```

## Documentation Comments

Add documentation to generated methods and fields:

```rust
use jni::bind_java_type;
use jni::objects::JString;

bind_java_type! {
    pub Counter => com.example.Counter,
    constructors {
        /// Creates a new counter with initial value 0
        fn new(),

        /// Creates a counter with the specified initial value
        fn with_initial(value: jint),
    },
    methods {
        /// Increments the counter by 1
        fn increment(),

        /// Returns the current counter value
        fn get_value() -> jint,
    },
    fields {
        /// The current counter value
        value: jint,

        name {
            sig = JString,
            /// Gets the counter name
            get = name,
            /// Sets the counter name
            set = set_name,
        },
    },
}
```

## Null Validation

Use `non_null` to validate that methods and fields never return null, converting null values into errors:

```rust,no_run
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;
use jni::refs::Reference;

bind_java_type! {
    pub UserProfile => com.example.UserProfile,
    constructors {
        fn new(),
    },
    methods {
        // Shorthand syntax - non_null qualifier
        non_null fn get_username() -> JString,
        non_null fn get_email() -> JString,

        // Optional field that can be null
        fn get_nickname() -> JString,

        // Block syntax with explicit non_null property
        fn get_display_name {
            sig = () -> JString,
            non_null = true,
        },
    },
    fields {
        // Shorthand syntax - non_null qualifier for fields
        non_null username: JString,
        non_null email: JString,

        // Optional field that can be null
        nickname: JString,

        // Block syntax
        display_name {
            sig = JString,
            non_null = true,
        },
    },
}

fn validate_non_null(env: &mut Env) -> jni::errors::Result<()> {
    let profile = UserProfile::new(env)?;

    // These will return Error::NullPtr("Null Object") if the Java method returns null
    let username = profile.get_username(env)?;  // non_null method
    assert!(!username.is_null());
    let email_field = profile.username(env)?;   // non_null field getter
    assert!(!email_field.is_null());

    // This can return null without error
    let nickname = profile.get_nickname(env)?;
    if !nickname.is_null() {
        println!("Nickname: {}", nickname.try_to_string(env)?);
    }

    // Setting a non_null field to null will also return an error
    let null_string = JString::null();
    let res = profile.set_username(env, &null_string);
    assert!(res.is_err()); // Error::NullPtr("Null Object")

    Ok(())
}
```

**When to use `non_null`:**

- APIs where null is logically an error but may not throw an exception
- Required fields that should always have a value
- Methods that are documented to never return null under normal circumstances

**Note:** `Error::JavaException` always takes precedence - if a Java exception is thrown, that will be returned regardless of the `non_null` setting.

## Complete Example

A comprehensive example combining multiple features:

```rust,ignore
use jni::{Env, bind_java_type};
use jni::objects::{JClass, JString};
use jni::sys::jint;
use jni::refs::LoaderContext;

// Define related types
bind_java_type! { pub CustomData => com.example.Data }

bind_java_type! {
    pub CompleteExample => com.example.CompleteExample,

    type_map = {
        CustomData => com.example.Data,
    },

    constructors {
        /// Creates a new instance with default values
        fn new(),

        /// Creates an instance with initial data
        fn with_data(data: CustomData),
    },

    methods {
        /// Returns the current value
        fn get_value() -> jint,

        /// Updates the value
        fn set_value(value: jint),

        /// Processes the data
        fn process(data: CustomData) -> JString,

        /// Creates a default instance (static factory)
        static fn create_default() -> com.example.CompleteExample,
    },

    fields {
        /// The internal value
        value: jint,

        /// The associated data
        data: CustomData,

        /// Global instance counter
        static instance_count: jint,
    },

    native_methods {
        /// Adds two numbers (native implementation)
        extern fn native_add(a: jint, b: jint) -> jint,

        /// Static native helper
        static extern fn native_format(value: jint) -> JString,
    },
}

// Implement native methods
impl CompleteExampleNativeInterface for CompleteExampleAPI {
    type Error = jni::errors::Error;

    fn native_add<'local>(
        _env: &mut Env<'local>,
        _this: CompleteExample<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, Self::Error> {
        Ok(a + b)
    }

    fn native_format<'local>(
        env: &mut Env<'local>,
        _class: JClass<'local>,
        value: jint,
    ) -> Result<JString<'local>, Self::Error> {
        JString::from_str(env, format!("Value: {}", value))
    }
}

// Usage
# fn complete_example(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden in docs
    // Initialize API (loads class and registers native methods)
    // (doesn't need to be called explicitly if using any other API method)
    let _api = CompleteExampleAPI::get(env, &LoaderContext::default())?;

    // Create instances
    let example = CompleteExample::new(env)?;
    let data = CustomData::new(env)?;
    let example2 = CompleteExample::with_data(env, &data)?;

    // Use methods
    example.set_value(env, 42)?;
    let value = example.get_value(env)?;

    let result = example.process(env, &data)?;

    // Use fields
    example.set_data(env, &data)?;
    let retrieved_data = example.data(env)?;

    // Static methods and fields
    let default = CompleteExample::create_default(env)?;
    let count = CompleteExample::instance_count(env)?;

    // Cast to base type
    let as_object = example.as_object(env)?;

    Ok(())
}
```
