//! Direct bindings to `wasm-opt`.
//!
//! These are bindings to `wasm-opt`,
//! as built by the [`wasm-opt-sys`] crate.
//! The bindings are created by the [`cxx`] crate,
//! and all go through a custom C++ shim layer
//! that provides a `cxx`-compatible C++ API.
//!
//! Most users will not want to use this crate directly,
//! but instead the [`wasm-opt`] crate.
//!
//! [`wasm-opt-sys`]: https://docs.rs/wasm-opt-sys
//! [`cxx`]: https://docs.rs/cxx
//! [`wasm-opt`]: https://docs.rs/wasm-opt
//!
//! The version of `cxx` used by these bindings is
//! reexported here.

pub use cxx;

// Establish linking with wasm_opt_sys, which contains no Rust code.
extern crate wasm_opt_sys;

#[cxx::bridge(namespace = "Colors")]
pub mod colors {
    unsafe extern "C++" {
        include!("shims.h");

        fn setEnabled(enabled: bool);
    }
}

#[cxx::bridge(namespace = "wasm_shims")]
pub mod wasm {
    unsafe extern "C++" {
        include!("shims.h");
    }

    unsafe extern "C++" {
        type Module;

        fn newModule() -> UniquePtr<Module>;

        fn validateWasm(wasm: Pin<&mut Module>) -> bool;
    }

    unsafe extern "C++" {
        type ModuleReader;

        fn newModuleReader() -> UniquePtr<ModuleReader>;

        fn setDebugInfo(self: Pin<&mut Self>, debug: bool);

        fn setDwarf(self: Pin<&mut Self>, dwarf: bool);

        fn readText(
            self: Pin<&mut Self>,
            filename: Pin<&mut CxxString>,
            wasm: Pin<&mut Module>,
        ) -> Result<()>;

        fn readBinary(
            self: Pin<&mut Self>,
            filename: Pin<&mut CxxString>,
            wasm: Pin<&mut Module>,
            sourceMapFilename: Pin<&mut CxxString>,
        ) -> Result<()>;

        fn read(
            self: Pin<&mut Self>,
            filename: Pin<&mut CxxString>,
            wasm: Pin<&mut Module>,
            sourceMapFilename: Pin<&mut CxxString>,
        ) -> Result<()>;
    }

    unsafe extern "C++" {
        type ModuleWriter;

        fn newModuleWriter() -> UniquePtr<ModuleWriter>;

        fn setDebugInfo(self: Pin<&mut Self>, debug: bool);

        fn setSourceMapFilename(self: Pin<&mut Self>, source_map_filename: Pin<&mut CxxString>);

        fn setSourceMapUrl(self: Pin<&mut Self>, source_map_url: Pin<&mut CxxString>);

        fn writeText(
            self: Pin<&mut Self>,
            wasm: Pin<&mut Module>,
            filename: Pin<&mut CxxString>,
        ) -> Result<()>;

        fn writeBinary(
            self: Pin<&mut Self>,
            wasm: Pin<&mut Module>,
            filename: Pin<&mut CxxString>,
        ) -> Result<()>;
    }

    unsafe extern "C++" {
        fn getRegisteredNames() -> UniquePtr<CxxVector<CxxString>>;

        fn getPassDescription(name: Pin<&mut CxxString>) -> UniquePtr<CxxString>;

        fn isPassHidden(name: Pin<&mut CxxString>) -> bool;
    }

    unsafe extern "C++" {
        type InliningOptions;

        fn newInliningOptions() -> UniquePtr<InliningOptions>;

        fn setAlwaysInlineMaxSize(self: Pin<&mut Self>, size: u32);

        fn setOneCallerInlineMaxSize(self: Pin<&mut Self>, size: u32);

        fn setFlexibleInlineMaxSize(self: Pin<&mut Self>, size: u32);

        fn setAllowFunctionsWithLoops(self: Pin<&mut Self>, allow: bool);

        fn setPartialInliningIfs(self: Pin<&mut Self>, number: u32);
    }

    unsafe extern "C++" {
        type PassOptions;

        fn newPassOptions() -> UniquePtr<PassOptions>;

        fn setValidate(self: Pin<&mut Self>, validate: bool);

        fn setValidateGlobally(self: Pin<&mut Self>, validate: bool);

        fn setOptimizeLevel(self: Pin<&mut Self>, level: i32);

        fn setShrinkLevel(self: Pin<&mut Self>, level: i32);

        fn setInliningOptions(self: Pin<&mut Self>, inlining: UniquePtr<InliningOptions>);

        fn setTrapsNeverHappen(self: Pin<&mut Self>, ignoreTraps: bool);

        fn setLowMemoryUnused(self: Pin<&mut Self>, memoryUnused: bool);

        fn setFastMath(self: Pin<&mut Self>, fastMath: bool);

        fn setZeroFilledMemory(self: Pin<&mut Self>, zeroFilledMemory: bool);

        fn setDebugInfo(self: Pin<&mut Self>, debugInfo: bool);

        fn setArguments(self: Pin<&mut Self>, key: Pin<&mut CxxString>, value: Pin<&mut CxxString>);
    }

    unsafe extern "C++" {
        type WasmFeatureSet;

        fn newFeatureSet() -> UniquePtr<WasmFeatureSet>;

        fn setMVP(self: Pin<&mut Self>);

        fn setAll(self: Pin<&mut Self>);

        fn set(self: Pin<&mut Self>, feature: u32, val: bool);

        fn has(self: &Self, features: &WasmFeatureSet) -> bool;

        fn as_int(self: &Self) -> u32;

        fn getFeatureArray() -> UniquePtr<CxxVector<u32>>;

        fn applyFeatures(
            wasm: Pin<&mut Module>,
            enabled_features: UniquePtr<WasmFeatureSet>,
            disabled_features: UniquePtr<WasmFeatureSet>,
        );
    }

    unsafe extern "C++" {
        type PassRunner<'wasm>;

        fn newPassRunner<'wasm>(wasm: Pin<&'wasm mut Module>) -> UniquePtr<PassRunner<'wasm>>;

        fn newPassRunnerWithOptions<'wasm>(
            wasm: Pin<&'wasm mut Module>,
            options: UniquePtr<PassOptions>,
        ) -> UniquePtr<PassRunner<'wasm>>;

        fn add(self: Pin<&mut Self>, pass_name: Pin<&mut CxxString>);

        fn addDefaultOptimizationPasses(self: Pin<&mut Self>);

        fn run(self: Pin<&mut Self>);

        fn passRemovesDebugInfo(name: Pin<&mut CxxString>) -> bool;
    }

    unsafe extern "C++" {
        fn checkInliningOptionsDefaults(inlining_options: UniquePtr<InliningOptions>) -> bool;

        fn checkPassOptionsDefaults(pass_options: UniquePtr<PassOptions>) -> bool;

        fn checkPassOptionsDefaultsOs(pass_options: UniquePtr<PassOptions>) -> bool;
    }
}
