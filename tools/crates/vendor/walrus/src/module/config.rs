use crate::error::Result;
use crate::ir::InstrLocId;
use crate::module::Module;
use crate::parse::IndicesToIds;
use std::fmt;
use std::path::Path;
use wasmparser::WasmFeatures;

/// Configuration for a `Module` which currently affects parsing.
#[derive(Default)]
pub struct ModuleConfig {
    pub(crate) generate_dwarf: bool,
    pub(crate) generate_synthetic_names_for_anonymous_items: bool,
    pub(crate) only_stable_features: bool,
    pub(crate) skip_strict_validate: bool,
    pub(crate) skip_producers_section: bool,
    pub(crate) skip_name_section: bool,
    pub(crate) preserve_code_transform: bool,
    pub(crate) on_parse:
        Option<Box<dyn Fn(&mut Module, &IndicesToIds) -> Result<()> + Sync + Send + 'static>>,
    pub(crate) on_instr_loc: Option<Box<dyn Fn(&usize) -> InstrLocId + Sync + Send + 'static>>,
}

impl Clone for ModuleConfig {
    fn clone(&self) -> ModuleConfig {
        ModuleConfig {
            // These are all cloned...
            generate_dwarf: self.generate_dwarf,
            generate_synthetic_names_for_anonymous_items: self
                .generate_synthetic_names_for_anonymous_items,
            only_stable_features: self.only_stable_features,
            skip_strict_validate: self.skip_strict_validate,
            skip_producers_section: self.skip_producers_section,
            skip_name_section: self.skip_name_section,
            preserve_code_transform: self.preserve_code_transform,

            // ... and this is left empty.
            on_parse: None,
            on_instr_loc: None,
        }
    }
}

impl fmt::Debug for ModuleConfig {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // Destructure `self` so that we get compilation errors if we forget to
        // add new fields to the debug here.
        let ModuleConfig {
            ref generate_dwarf,
            ref generate_synthetic_names_for_anonymous_items,
            ref only_stable_features,
            ref skip_strict_validate,
            ref skip_producers_section,
            ref skip_name_section,
            ref preserve_code_transform,
            ref on_parse,
            ref on_instr_loc,
        } = self;

        f.debug_struct("ModuleConfig")
            .field("generate_dwarf", generate_dwarf)
            .field(
                "generate_synthetic_names_for_anonymous_items",
                generate_synthetic_names_for_anonymous_items,
            )
            .field("only_stable_features", only_stable_features)
            .field("skip_strict_validate", skip_strict_validate)
            .field("skip_producers_section", skip_producers_section)
            .field("skip_name_section", skip_name_section)
            .field("preserve_code_transform", preserve_code_transform)
            .field("on_parse", &on_parse.as_ref().map(|_| ".."))
            .field("on_instr_loc", &on_instr_loc.as_ref().map(|_| ".."))
            .finish()
    }
}

impl ModuleConfig {
    /// Creates a fresh new configuration with default settings.
    pub fn new() -> ModuleConfig {
        ModuleConfig::default()
    }

    /// Sets a flag to whether DWARF debug sections are generated for this
    /// module.
    ///
    /// By default this flag is `false`. Note that any emitted DWARF is
    /// currently wildly incorrect and buggy, and is also larger than the wasm
    /// itself!
    pub fn generate_dwarf(&mut self, generate: bool) -> &mut ModuleConfig {
        self.generate_dwarf = generate;
        // generate_dwarf implies preserve_code_transform
        self.preserve_code_transform = generate || self.preserve_code_transform;
        self
    }

    /// Sets a flag to whether the custom "name" section is generated for this
    /// module.
    ///
    /// The "name" section contains symbol names for the module, functions,
    /// locals, types, memories, tables, data, elements and globals.
    /// When enabled, stack traces will use these names, instead of
    /// `wasm-function[123]`.
    ///
    /// By default this flag is `true`.
    pub fn generate_name_section(&mut self, generate: bool) -> &mut ModuleConfig {
        self.skip_name_section = !generate;
        self
    }

    /// Sets a flag to whether synthetic debugging names are generated for
    /// anonymous locals/functions/etc when parsing and running passes for this
    /// module.
    ///
    /// By default this flag is `false`, and it will generate quite a few names
    /// if enabled!
    pub fn generate_synthetic_names_for_anonymous_items(
        &mut self,
        generate: bool,
    ) -> &mut ModuleConfig {
        self.generate_synthetic_names_for_anonymous_items = generate;
        self
    }

    /// Indicates whether the module, after parsing, performs strict validation
    /// of the wasm module to adhere with the current version of the wasm
    /// specification.
    ///
    /// This can be expensive for some modules and strictly isn't required to
    /// create a `Module` from a wasm file. This includes checks such as "atomic
    /// instructions require a shared memory".
    ///
    /// By default this flag is `true`
    pub fn strict_validate(&mut self, strict: bool) -> &mut ModuleConfig {
        self.skip_strict_validate = !strict;
        self
    }

    /// Indicates whether the module will have the "producers" custom section
    /// which preserves the original producers and also includes `walrus`.
    ///
    /// This is generally used for telemetry in browsers, but for otherwise tiny
    /// wasm binaries can add some size to the binary.
    ///
    /// By default this flag is `true`
    pub fn generate_producers_section(&mut self, generate: bool) -> &mut ModuleConfig {
        self.skip_producers_section = !generate;
        self
    }

    /// Indicates whether this module is allowed to use only stable WebAssembly
    /// features or not.
    ///
    /// This is currently used to disable some validity checks required by the
    /// WebAssembly specification. It's not religiously adhered to throughout
    /// the codebase, even if set to `true` some unstable features may still be
    /// allowed.
    ///
    /// By default this flag is `false`.
    pub fn only_stable_features(&mut self, only: bool) -> &mut ModuleConfig {
        self.only_stable_features = only;
        self
    }

    /// Returns a `wasmparser::WasmFeatures` based on the enabled proposals
    /// which should be used for `wasmparser::Parser`` and `wasmparser::Validator`.
    pub(crate) fn get_wasmparser_wasm_features(&self) -> WasmFeatures {
        // Start from empty so that we explicitly control what is enabled.
        let mut features = WasmFeatures::empty();
        // This is not a proposal.
        features.insert(WasmFeatures::FLOATS);
        // Always enable [finished proposals](https://github.com/WebAssembly/proposals/blob/main/finished-proposals.md).
        features.insert(WasmFeatures::MUTABLE_GLOBAL);
        features.insert(WasmFeatures::SATURATING_FLOAT_TO_INT);
        features.insert(WasmFeatures::SIGN_EXTENSION);
        features.insert(WasmFeatures::MULTI_VALUE);
        features.insert(WasmFeatures::REFERENCE_TYPES);
        features.insert(WasmFeatures::BULK_MEMORY);
        features.insert(WasmFeatures::SIMD);
        features.insert(WasmFeatures::RELAXED_SIMD);
        features.insert(WasmFeatures::TAIL_CALL);
        // Enable supported active proposals.
        if !self.only_stable_features {
            // # Fully supported proposals.
            features.insert(WasmFeatures::MULTI_MEMORY);
            features.insert(WasmFeatures::MEMORY64);
            // # Partially supported proposals.
            // ## threads
            // spec-tests/proposals/threads still fail
            // round_trip tests already require this feature, so we can't disable it by default.
            features.insert(WasmFeatures::THREADS);
        }
        features
    }

    /// Provide a function that is invoked after successfully parsing a module,
    /// and gets access to data structures that only exist at parse time, such
    /// as the map from indices in the original Wasm to the new walrus IDs.
    ///
    /// This is a good place to parse custom sections that reference things by
    /// index.
    ///
    /// This will never be invoked for modules that are created from scratch,
    /// and are not parsed from an existing Wasm binary.
    ///
    /// Note that only one `on_parse` function may be registered and subsequent
    /// registrations will override the old ones.
    ///
    /// Note that cloning a `ModuleConfig` will result in a config that does not
    /// have an `on_parse` function, even if the original did.
    pub fn on_parse<F>(&mut self, f: F) -> &mut ModuleConfig
    where
        F: Fn(&mut Module, &IndicesToIds) -> Result<()> + Send + Sync + 'static,
    {
        self.on_parse = Some(Box::new(f) as _);
        self
    }

    /// Provide a function that is invoked on source location ID step.
    ///
    /// Note that cloning a `ModuleConfig` will result in a config that does not
    /// have an `on_instr_loc` function, even if the original did.
    pub fn on_instr_loc<F>(&mut self, f: F) -> &mut ModuleConfig
    where
        F: Fn(&usize) -> InstrLocId + Send + Sync + 'static,
    {
        self.on_instr_loc = Some(Box::new(f) as _);
        self
    }

    /// Sets a flag to whether code transform is preverved during parsing.
    ///
    /// By default this flag is `false`.
    pub fn preserve_code_transform(&mut self, preserve: bool) -> &mut ModuleConfig {
        self.preserve_code_transform = preserve;
        self
    }

    /// Parses an in-memory WebAssembly file into a `Module` using this
    /// configuration.
    pub fn parse(&self, wasm: &[u8]) -> Result<Module> {
        Module::parse(wasm, self)
    }

    /// Parses a WebAssembly file into a `Module` using this configuration.
    pub fn parse_file<P>(&self, path: P) -> Result<Module>
    where
        P: AsRef<Path>,
    {
        Module::from_file_with_config(path, self)
    }
}
