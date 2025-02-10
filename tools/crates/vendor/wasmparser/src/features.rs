use bitflags::bitflags;

macro_rules! define_wasm_features {
    (
        $(#[$outer:meta])*
        pub struct WasmFeatures: $repr:ty {
            $(
                $(#[$inner:ident $($args:tt)*])*
                pub $field:ident: $const:ident($flag:expr) = $default:expr;
            )*
        }
    ) => {
        bitflags! {
            $(#[$outer])*
            pub struct WasmFeatures: $repr {
                $(
                    $(#[$inner $($args)*])*
                    #[doc = "\nDefaults to `"]
                    #[doc = stringify!($default)]
                    #[doc = "`.\n"]
                    const $const = $flag;
                )*
            }
        }

        impl Default for WasmFeatures {
            #[inline]
            fn default() -> Self {
                let mut features = WasmFeatures::empty();
                $(
                    features.set(WasmFeatures::$const, $default);
                )*
                features
            }
        }

        impl WasmFeatures {
            /// Construct a bit-packed `WasmFeatures` from the inflated struct version.
            #[inline]
            pub fn from_inflated(inflated: WasmFeaturesInflated) -> Self {
                let mut features = WasmFeatures::empty();
                $(
                    features.set(WasmFeatures::$const, inflated.$field);
                )*
                features
            }

            /// Inflate these bit-packed features into a struct with a field per
            /// feature.
            ///
            /// Although the inflated struct takes up much more memory than the
            /// bit-packed version, its fields can be exhaustively matched
            /// upon. This makes it useful for temporarily checking against,
            /// while keeping the bit-packed version as the method of storing
            /// the features for longer periods of time.
            #[inline]
            pub fn inflate(&self) -> WasmFeaturesInflated {
                WasmFeaturesInflated {
                    $(
                        $field: self.$field(),
                    )*
                }
            }

            $(
                /// Returns whether this feature is enabled in this feature set.
                #[inline]
                pub fn $field(&self) -> bool {
                    self.contains(WasmFeatures::$const)
                }
            )*
        }

        /// Inflated version of [`WasmFeatures`][crate::WasmFeatures] that
        /// allows for exhaustive matching on fields.
        pub struct WasmFeaturesInflated {
            $(
                $(#[$inner $($args)*])*
                #[doc = "\nDefaults to `"]
                #[doc = stringify!($default)]
                #[doc = "`.\n"]
                pub $field: bool,
            )*
        }
    };
}

define_wasm_features! {
    /// Flags for features that are enabled for validation.
    #[derive(Hash, Debug, Copy, Clone)]
    pub struct WasmFeatures: u32 {
        /// The WebAssembly `mutable-global` proposal.
        pub mutable_global: MUTABLE_GLOBAL(1) = true;
        /// The WebAssembly `saturating-float-to-int` proposal.
        pub saturating_float_to_int: SATURATING_FLOAT_TO_INT(1 << 1) = true;
        /// The WebAssembly `sign-extension-ops` proposal.
        pub sign_extension: SIGN_EXTENSION(1 << 2) = true;
        /// The WebAssembly reference types proposal.
        pub reference_types: REFERENCE_TYPES(1 << 3) = true;
        /// The WebAssembly multi-value proposal.
        pub multi_value: MULTI_VALUE(1 << 4) = true;
        /// The WebAssembly bulk memory operations proposal.
        pub bulk_memory: BULK_MEMORY(1 << 5) = true;
        /// The WebAssembly SIMD proposal.
        pub simd: SIMD(1 << 6) = true;
        /// The WebAssembly Relaxed SIMD proposal.
        pub relaxed_simd: RELAXED_SIMD(1 << 7) = true;
        /// The WebAssembly threads proposal.
        pub threads: THREADS(1 << 8) = true;
        /// The WebAssembly shared-everything-threads proposal; includes new
        /// component model built-ins.
        pub shared_everything_threads: SHARED_EVERYTHING_THREADS(1 << 9) = false;
        /// The WebAssembly tail-call proposal.
        pub tail_call: TAIL_CALL(1 << 10) = true;
        /// Whether or not floating-point instructions are enabled.
        ///
        /// This is enabled by default can be used to disallow floating-point
        /// operators and types.
        ///
        /// This does not correspond to a WebAssembly proposal but is instead
        /// intended for embeddings which have stricter-than-usual requirements
        /// about execution. Floats in WebAssembly can have different NaN patterns
        /// across hosts which can lead to host-dependent execution which some
        /// runtimes may not desire.
        pub floats: FLOATS(1 << 11) = true;
        /// The WebAssembly multi memory proposal.
        pub multi_memory: MULTI_MEMORY(1 << 12) = true;
        /// The WebAssembly exception handling proposal.
        pub exceptions: EXCEPTIONS(1 << 13) = false;
        /// The WebAssembly memory64 proposal.
        pub memory64: MEMORY64(1 << 14) = false;
        /// The WebAssembly extended_const proposal.
        pub extended_const: EXTENDED_CONST(1 << 15) = true;
        /// The WebAssembly component model proposal.
        pub component_model: COMPONENT_MODEL(1 << 16) = true;
        /// The WebAssembly typed function references proposal.
        pub function_references: FUNCTION_REFERENCES(1 << 17) = false;
        /// The WebAssembly memory control proposal.
        pub memory_control: MEMORY_CONTROL(1 << 18) = false;
        /// The WebAssembly gc proposal.
        pub gc: GC(1 << 19) = false;
        /// The WebAssembly [custom-page-sizes
        /// proposal](https://github.com/WebAssembly/custom-page-sizes).
        pub custom_page_sizes: CUSTOM_PAGE_SIZES(1 << 20) = false;
        /// Support for the `value` type in the component model proposal.
        pub component_model_values: COMPONENT_MODEL_VALUES(1 << 21) = false;
        /// Support for the nested namespaces and projects in component model names.
        pub component_model_nested_names: COMPONENT_MODEL_NESTED_NAMES(1 << 22) = false;
        /// Support for more than 32 flags per-type in the component model.
        pub component_model_more_flags: COMPONENT_MODEL_MORE_FLAGS(1 << 23) = false;
        /// Support for multiple return values in a component model function.
        pub component_model_multiple_returns: COMPONENT_MODEL_MULTIPLE_RETURNS(1 << 24) = false;
        /// The WebAssembly legacy exception handling proposal (phase 1)
        ///
        /// # Note
        ///
        /// Support this feature as long as all leading browsers also support it
        /// https://github.com/WebAssembly/exception-handling/blob/main/proposals/exception-handling/legacy/Exceptions.md
        pub legacy_exceptions: LEGACY_EXCEPTIONS(1 << 25) = false;
    }
}

impl From<WasmFeaturesInflated> for WasmFeatures {
    #[inline]
    fn from(inflated: WasmFeaturesInflated) -> Self {
        Self::from_inflated(inflated)
    }
}

impl From<WasmFeatures> for WasmFeaturesInflated {
    #[inline]
    fn from(features: WasmFeatures) -> Self {
        features.inflate()
    }
}
