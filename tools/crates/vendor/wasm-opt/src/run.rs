use crate::api::*;
use crate::base::{
    validate_wasm, Feature as BaseFeature, FeatureSet as BaseFeatureSet,
    InliningOptions as BaseInliningOptions, Module, ModuleReader, ModuleWriter,
    PassOptions as BasePassOptions, PassRunner,
};
use std::fs;
use std::path::Path;
use thiserror::Error;

/// An error resulting from the [`OptimizationOptions::run`] method.
#[derive(Error, Debug)]
pub enum OptimizationError {
    /// The input module did not validate.
    #[error("Failed to validate wasm: error validating input")]
    ValidateWasmInput,
    /// The output module did not validate.
    #[error("Failed to validate wasm: error after opts")]
    ValidateWasmOutput,
    /// An error occurred while reading the input module.
    #[error("Failed to read module")]
    Read {
        #[source]
        source: Box<dyn std::error::Error + Send + Sync + 'static>,
    },
    /// An error occurred while writing the output module.
    #[error("Failed to write module")]
    Write {
        #[source]
        source: Box<dyn std::error::Error + Send + Sync + 'static>,
    },
    /// The input file path represents stdin to Binaryen,
    /// but the API does not support reading stdin.
    #[error("Refusing to read from stdin")]
    InvalidStdinPath,
}

/// Execution.
impl OptimizationOptions {
    /// Run the Binaryen wasm optimizer.
    ///
    /// This loads a module from a file,
    /// runs optimization passes,
    /// and writes the module back to a file.
    ///
    /// To supply sourcemaps for the input module,
    /// and preserve them for the output module,
    /// use [`OptimizationOptions::run_with_sourcemaps`].
    ///
    /// # Errors
    ///
    /// Returns error on I/O failure, or if the input fails to parse.
    /// If [`PassOptions::validate`] is true, it returns an error
    /// if the input module fails to validate, or if the optimized
    /// module fails to validate.
    ///
    /// The Rust API does not support reading a module on stdin, as the CLI
    /// does. If `infile` is empty or "-",
    /// [`OptimizationError::InvalidStdinPath`] is returned.
    pub fn run(
        &self,
        infile: impl AsRef<Path>,
        outfile: impl AsRef<Path>,
    ) -> Result<(), OptimizationError> {
        self.run_with_sourcemaps(infile, None::<&str>, outfile, None::<&str>, None::<&str>)
    }

    /// Run the Binaryen wasm optimizer.
    ///
    /// This loads a module from a file,
    /// runs optimization passes,
    /// and writes the module back to a file.
    ///
    /// The sourcemap arguments are optional, and only have effect
    /// when reading or writing binary `wasm` files. When using
    /// text `wat` files the respective sourcemap argument is ignored.
    ///
    /// # Errors
    ///
    /// Returns error on I/O failure, or if the input fails to parse.
    /// If [`PassOptions::validate`] is true, it returns an error
    /// if the input module fails to validate, or if the optimized
    /// module fails to validate.
    ///
    /// The Rust API does not support reading a module on stdin, as the CLI
    /// does. If `infile` is empty or "-",
    /// [`OptimizationError::InvalidStdinPath`] is returned.
    pub fn run_with_sourcemaps(
        &self,
        infile: impl AsRef<Path>,
        infile_sourcemap: Option<impl AsRef<Path>>,
        outfile: impl AsRef<Path>,
        outfile_sourcemap: Option<impl AsRef<Path>>,
        sourcemap_url: Option<impl AsRef<str>>,
    ) -> Result<(), OptimizationError> {
        let infile: &Path = infile.as_ref();
        let infile_sourcemap: Option<&Path> = infile_sourcemap.as_ref().map(AsRef::as_ref);
        let outfile: &Path = outfile.as_ref();
        let outfile_sourcemap: Option<&Path> = outfile_sourcemap.as_ref().map(AsRef::as_ref);
        let sourcemap_url: Option<&str> = sourcemap_url.as_ref().map(AsRef::as_ref);

        if infile.as_os_str().is_empty() || infile == Path::new("-") {
            return Err(OptimizationError::InvalidStdinPath);
        }

        let mut m = Module::new();
        self.apply_features(&mut m);

        {
            let mut reader = ModuleReader::new();

            let set_dwarf =
                self.passopts.debug_info && !will_remove_debug_info(&self.passes.more_passes);
            reader.set_dwarf(set_dwarf);

            match self.reader.file_type {
                FileType::Wasm => reader.read_binary(infile, &mut m, infile_sourcemap),
                FileType::Wat => reader.read_text(infile, &mut m),
                FileType::Any => reader.read(infile, &mut m, infile_sourcemap),
            }
            .map_err(|e| OptimizationError::Read {
                source: Box::from(e),
            })?;
        }

        {
            if self.passopts.validate && !validate_wasm(&mut m) {
                return Err(OptimizationError::ValidateWasmInput);
            }

            self.create_and_run_pass_runner(&mut m);

            if self.converge {
                self.run_until_convergence(&mut m)
                    .map_err(|e| OptimizationError::Write {
                        source: Box::from(e),
                    })?;
            }

            if self.passopts.validate && !validate_wasm(&mut m) {
                return Err(OptimizationError::ValidateWasmOutput);
            }
        }

        {
            let mut writer = ModuleWriter::new();
            writer.set_debug_info(self.passopts.debug_info);

            if let Some(filename) = outfile_sourcemap {
                writer
                    .set_source_map_filename(filename)
                    .map_err(|e| OptimizationError::Write {
                        source: Box::from(e),
                    })?;
            }

            if let Some(url) = sourcemap_url {
                writer.set_source_map_url(url);
            }

            match self.writer.file_type {
                FileType::Wasm => writer.write_binary(&mut m, outfile),
                FileType::Wat => writer.write_text(&mut m, outfile),
                FileType::Any => match self.reader.file_type {
                    FileType::Any | FileType::Wasm => writer.write_binary(&mut m, outfile),
                    FileType::Wat => writer.write_text(&mut m, outfile),
                },
            }
            .map_err(|e| OptimizationError::Write {
                source: Box::from(e),
            })?;
        }

        Ok(())
    }

    fn create_and_run_pass_runner(&self, m: &mut Module) {
        let passopts = self.translate_pass_options();

        let mut pass_runner = PassRunner::new_with_options(m, passopts);

        if self.passes.add_default_passes {
            pass_runner.add_default_optimization_passes();
        }

        self.passes
            .more_passes
            .iter()
            .for_each(|pass| pass_runner.add(pass.name()));

        pass_runner.run();
    }

    fn run_until_convergence(&self, m: &mut Module) -> anyhow::Result<()> {
        let mut last_size = Self::get_module_size(m)?;
        let mut current_size;

        loop {
            self.create_and_run_pass_runner(m);

            current_size = Self::get_module_size(m)?;

            if current_size >= last_size {
                break;
            }

            last_size = current_size;
        }

        Ok(())
    }

    fn get_module_size(m: &mut Module) -> anyhow::Result<usize> {
        let tempdir = tempfile::tempdir()?;
        let temp_outfile = tempdir.path().join("wasm_opt_temp_outfile.wasm");

        let mut writer = ModuleWriter::new();
        writer.write_binary(m, &temp_outfile)?;

        let file_size = fs::read(&temp_outfile)?.len();

        Ok(file_size)
    }

    fn apply_features(&self, m: &mut Module) {
        let (enabled_features, disabled_features) = convert_feature_sets(&self.features);

        m.apply_features(enabled_features, disabled_features);
    }

    fn translate_pass_options(&self) -> BasePassOptions {
        let mut opts = BasePassOptions::new();

        opts.set_validate(self.passopts.validate);
        opts.set_validate_globally(self.passopts.validate_globally);
        opts.set_optimize_level(self.passopts.optimize_level as i32);
        opts.set_shrink_level(self.passopts.shrink_level as i32);
        opts.set_traps_never_happen(self.passopts.traps_never_happen);
        opts.set_low_memory_unused(self.passopts.low_memory_unused);
        opts.set_fast_math(self.passopts.fast_math);
        opts.set_zero_filled_memory(self.passopts.zero_filled_memory);
        opts.set_debug_info(self.passopts.debug_info);

        self.passopts
            .arguments
            .iter()
            .for_each(|(key, value)| opts.set_arguments(key, value));

        let mut inlining = BaseInliningOptions::new();
        inlining.set_always_inline_max_size(self.inlining.always_inline_max_size);
        inlining.set_one_caller_inline_max_size(self.inlining.one_caller_inline_max_size);
        inlining.set_flexible_inline_max_size(self.inlining.flexible_inline_max_size);
        inlining.set_allow_functions_with_loops(self.inlining.allow_functions_with_loops);
        inlining.set_partial_inlining_ifs(self.inlining.partial_inlining_ifs);

        opts.set_inlining_options(inlining);

        opts
    }
}

fn will_remove_debug_info(passes: &[Pass]) -> bool {
    passes
        .iter()
        .any(|pass| PassRunner::pass_removes_debug_info(pass.name()) == true)
}

fn convert_feature_sets(features: &Features) -> (BaseFeatureSet, BaseFeatureSet) {
    let mut feature_set_enabled = BaseFeatureSet::new();
    let mut feature_set_disabled = BaseFeatureSet::new();

    match features.baseline {
        FeatureBaseline::Default => {
            feature_set_enabled.set(BaseFeature::Default, true);
        }
        FeatureBaseline::MvpOnly => {
            feature_set_enabled.set_mvp();
            feature_set_disabled.set_all();
        }
        FeatureBaseline::All => {
            feature_set_enabled.set_all();
            feature_set_disabled.set_mvp();
        }
    }

    features.enabled.iter().for_each(|f| {
        let feature = convert_feature(f);
        feature_set_enabled.set(feature, true);
        feature_set_disabled.set(feature, false);
    });

    features.disabled.iter().for_each(|f| {
        let feature = convert_feature(f);
        feature_set_enabled.set(feature, false);
        feature_set_disabled.set(feature, true);
    });

    (feature_set_enabled, feature_set_disabled)
}

fn convert_feature(feature: &Feature) -> BaseFeature {
    match feature {
        Feature::None => BaseFeature::None,
        Feature::Atomics => BaseFeature::Atomics,
        Feature::MutableGlobals => BaseFeature::MutableGlobals,
        Feature::TruncSat => BaseFeature::TruncSat,
        Feature::Simd => BaseFeature::Simd,
        Feature::BulkMemory => BaseFeature::BulkMemory,
        Feature::SignExt => BaseFeature::SignExt,
        Feature::ExceptionHandling => BaseFeature::ExceptionHandling,
        Feature::TailCall => BaseFeature::TailCall,
        Feature::ReferenceTypes => BaseFeature::ReferenceTypes,
        Feature::Multivalue => BaseFeature::Multivalue,
        Feature::Gc => BaseFeature::Gc,
        Feature::Memory64 => BaseFeature::Memory64,
        Feature::RelaxedSimd => BaseFeature::RelaxedSimd,
        Feature::ExtendedConst => BaseFeature::ExtendedConst,
        Feature::Strings => BaseFeature::Strings,
        Feature::MultiMemory => BaseFeature::MultiMemory,
        Feature::Mvp => BaseFeature::None,
        Feature::Default => BaseFeature::Default,
        Feature::All => BaseFeature::All,
    }
}

#[cfg(test)]
mod test {
    use super::*;

    fn has(set: &BaseFeatureSet, feature: BaseFeature) -> bool {
        let mut new_set = BaseFeatureSet::new();
        new_set.set(feature, true);
        set.has(&new_set)
    }

    #[test]
    fn test_features_default() {
        let features = Features::default();
        let (enabled, disabled) = convert_feature_sets(&features);

        assert_eq!(enabled.as_int(), BaseFeature::Default as u32);
        assert_eq!(disabled.as_int(), BaseFeature::None as u32);

        assert!(has(&enabled, BaseFeature::SignExt));
        assert!(!has(&disabled, BaseFeature::SignExt));
        assert!(has(&enabled, BaseFeature::MutableGlobals));
        assert!(!has(&disabled, BaseFeature::MutableGlobals));
    }

    #[test]
    fn test_features_remove_defaults() {
        let mut opts = OptimizationOptions::new_optimize_for_size();
        opts.disable_feature(Feature::SignExt)
            .disable_feature(Feature::MutableGlobals);
        let (enabled, disabled) = convert_feature_sets(&opts.features);

        assert!(!has(&enabled, BaseFeature::SignExt));
        assert!(has(&disabled, BaseFeature::SignExt));
        assert!(!has(&enabled, BaseFeature::MutableGlobals));
        assert!(has(&disabled, BaseFeature::MutableGlobals));
    }

    #[test]
    fn test_features_mvp_and_enable() {
        let mut opts = OptimizationOptions::new_optimize_for_size();
        opts.mvp_features_only();

        let (enabled, disabled) = convert_feature_sets(&opts.features);

        assert!(has(&enabled, BaseFeature::None));
        assert!(has(&disabled, BaseFeature::All));

        assert!(!has(&enabled, BaseFeature::Gc));
        assert!(has(&disabled, BaseFeature::Gc));

        opts.enable_feature(Feature::Gc);

        let (enabled, disabled) = convert_feature_sets(&opts.features);

        assert!(has(&enabled, BaseFeature::Gc));
        assert!(!has(&disabled, BaseFeature::Gc));

        // Other features still disabled

        assert!(!has(&enabled, BaseFeature::Atomics));
        assert!(has(&disabled, BaseFeature::Atomics));
    }

    #[test]
    fn test_features_all_and_disable() {
        let mut opts = OptimizationOptions::new_optimize_for_size();
        opts.all_features();

        let (enabled, disabled) = convert_feature_sets(&opts.features);

        assert!(has(&enabled, BaseFeature::All));
        assert!(has(&disabled, BaseFeature::None));

        assert!(has(&enabled, BaseFeature::Gc));
        assert!(!has(&disabled, BaseFeature::Gc));

        opts.disable_feature(Feature::Gc);

        let (enabled, disabled) = convert_feature_sets(&opts.features);

        assert!(!has(&enabled, BaseFeature::Gc));
        assert!(has(&disabled, BaseFeature::Gc));

        // Other features still enabled

        assert!(has(&enabled, BaseFeature::Atomics));
        assert!(!has(&disabled, BaseFeature::Atomics));
    }
}
