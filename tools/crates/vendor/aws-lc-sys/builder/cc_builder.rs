// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// NOTE: This module is intended to produce an equivalent "libcrypto" static library to the one
// produced by the CMake. Changes to CMake relating to compiler checks and/or special build flags
// may require modifications to the logic in this module.

mod apple_aarch64;
mod apple_x86_64;
mod linux_aarch64;
mod linux_arm;
mod linux_ppc64le;
mod linux_x86;
mod linux_x86_64;
mod universal;
mod win_aarch64;
mod win_x86;
mod win_x86_64;

use crate::nasm_builder::NasmBuilder;
use crate::{
    cargo_env, compiler_is_cl_like, emit_warning, env_var_to_bool, execute_command,
    export_crate_compilers, find_clang_cl, get_crate_cc, get_crate_cflags, is_cross_compiling,
    is_link_whole_archive, is_no_asm, is_small, out_dir, requested_c_std, set_env_for_target,
    should_build_jitter_entropy, target, target_arch, target_env, target_is_msvc, target_os,
    target_vendor, use_prebuilt_nasm, CStdRequested, EnvGuard, OutputLibType,
};
use std::cell::Cell;
use std::collections::HashMap;
use std::env;
use std::path::{Path, PathBuf};

#[non_exhaustive]
#[derive(PartialEq, Eq)]
pub(crate) enum CompilerFeature {
    NeonSha3,
}

pub(crate) struct CcBuilder {
    manifest_dir: PathBuf,
    out_dir: PathBuf,
    build_prefix: Option<String>,
    output_lib_type: OutputLibType,
    compiler_features: Cell<Vec<CompilerFeature>>,
    /// Never used itself; see [`CcBuilder::new_build`].
    build_template: cc::Build,
}

use std::fs;

fn identify_sources() -> Vec<&'static str> {
    let mut source_files: Vec<&'static str> = vec![];
    source_files.append(&mut Vec::from(universal::CRYPTO_LIBRARY));
    let mut target_specific_source_found = true;
    if target_os() == "windows" {
        if target_arch() == "x86_64" {
            source_files.append(&mut Vec::from(win_x86_64::CRYPTO_LIBRARY));
        } else if target_arch() == "aarch64" {
            source_files.append(&mut Vec::from(win_aarch64::CRYPTO_LIBRARY));
        } else if target_arch() == "x86" {
            source_files.append(&mut Vec::from(win_x86::CRYPTO_LIBRARY));
        } else {
            target_specific_source_found = false;
        }
    } else if target_vendor() == "apple" {
        if target_arch() == "x86_64" {
            source_files.append(&mut Vec::from(apple_x86_64::CRYPTO_LIBRARY));
        } else if target_arch() == "aarch64" {
            source_files.append(&mut Vec::from(apple_aarch64::CRYPTO_LIBRARY));
        } else {
            target_specific_source_found = false;
        }
    } else if target_arch() == "x86_64" {
        source_files.append(&mut Vec::from(linux_x86_64::CRYPTO_LIBRARY));
    } else if target_arch() == "aarch64" {
        source_files.append(&mut Vec::from(linux_aarch64::CRYPTO_LIBRARY));
    } else if target_arch() == "arm" {
        source_files.append(&mut Vec::from(linux_arm::CRYPTO_LIBRARY));
    } else if target_arch() == "x86" {
        source_files.append(&mut Vec::from(linux_x86::CRYPTO_LIBRARY));
    } else if target_arch() == "powerpc64" {
        source_files.append(&mut Vec::from(linux_ppc64le::CRYPTO_LIBRARY));
    } else {
        target_specific_source_found = false;
    }
    if !target_specific_source_found {
        emit_warning(format!(
            "No target-specific source found: {}-{}",
            target_os(),
            target_arch()
        ));
    }

    source_files
}

#[allow(clippy::upper_case_acronyms)]
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum BuildOption {
    STD(String),
    FLAG(String),
    DEFINE(String, String),
    INCLUDE(PathBuf),
}
impl BuildOption {
    fn std<T: ToString + ?Sized>(val: &T) -> Self {
        Self::STD(val.to_string())
    }
    fn flag<T: ToString + ?Sized>(val: &T) -> Self {
        Self::FLAG(val.to_string())
    }
    fn flag_if_supported<T: ToString + ?Sized>(cc_build: &cc::Build, flag: &T) -> Option<Self> {
        if let Ok(true) = cc_build.is_flag_supported(flag.to_string()) {
            Some(Self::FLAG(flag.to_string()))
        } else {
            None
        }
    }

    fn define<K: ToString + ?Sized, V: ToString + ?Sized>(key: &K, val: &V) -> Self {
        Self::DEFINE(key.to_string(), val.to_string())
    }

    fn include<P: Into<PathBuf>>(path: P) -> Self {
        Self::INCLUDE(path.into())
    }

    fn apply_cc<'a>(&self, cc_build: &'a mut cc::Build) -> &'a mut cc::Build {
        match self {
            BuildOption::STD(val) => cc_build.std(val),
            BuildOption::FLAG(val) => cc_build.flag(val),
            BuildOption::DEFINE(key, val) => cc_build.define(key, Some(val.as_str())),
            BuildOption::INCLUDE(path) => cc_build.include(path.as_path()),
        }
    }

    pub(crate) fn apply_cmake<'a>(
        &self,
        cmake_cfg: &'a mut cmake::Config,
        is_cl_like: bool,
    ) -> &'a mut cmake::Config {
        if is_cl_like {
            match self {
                BuildOption::STD(val) => cmake_cfg.define(
                    "CMAKE_C_STANDARD",
                    val.to_ascii_lowercase().strip_prefix('c').unwrap_or("11"),
                ),
                BuildOption::FLAG(val) => cmake_cfg.cflag(val),
                BuildOption::DEFINE(key, val) => cmake_cfg.cflag(format!("/D{key}={val}")),
                BuildOption::INCLUDE(path) => cmake_cfg.cflag(format!("/I{}", path.display())),
            }
        } else {
            match self {
                BuildOption::STD(val) => cmake_cfg.define(
                    "CMAKE_C_STANDARD",
                    val.to_ascii_lowercase().strip_prefix('c').unwrap_or("11"),
                ),
                BuildOption::FLAG(val) => cmake_cfg.cflag(val),
                BuildOption::DEFINE(key, val) => cmake_cfg.cflag(format!("-D{key}={val}")),
                BuildOption::INCLUDE(path) => cmake_cfg.cflag(format!("-I{}", path.display())),
            }
        }
    }

    pub(crate) fn apply_nasm<'a>(&self, nasm_builder: &'a mut NasmBuilder) -> &'a mut NasmBuilder {
        match self {
            BuildOption::FLAG(val) => nasm_builder.flag(val),
            BuildOption::DEFINE(key, val) => nasm_builder.define(key, Some(val.as_str())),
            BuildOption::INCLUDE(path) => nasm_builder.include(path.as_path()),
            BuildOption::STD(_) => nasm_builder, // STD ignored for NASM
        }
    }
}

impl CcBuilder {
    fn small_c_build_options() -> Vec<BuildOption> {
        vec![BuildOption::define("OPENSSL_SMALL", "1")]
    }

    fn small_nasm_build_options() -> Vec<BuildOption> {
        // Raw NASM sources cannot include target.h, where OPENSSL_SMALL implies this gate.
        vec![
            BuildOption::define("OPENSSL_SMALL", "1"),
            BuildOption::define("MY_ASSEMBLER_IS_TOO_OLD_FOR_512AVX", "1"),
        ]
    }

    pub(crate) fn new(
        manifest_dir: PathBuf,
        out_dir: PathBuf,
        build_prefix: Option<String>,
        output_lib_type: OutputLibType,
    ) -> Self {
        Self {
            manifest_dir,
            out_dir,
            build_prefix,
            output_lib_type,
            compiler_features: Cell::new(vec![]),
            build_template: cc::Build::new(),
        }
    }

    /// A new `cc::Build` for compiling the library or probing the compiler.
    ///
    /// Clones share the template's caches, so cc identifies the compiler once for
    /// all of them. With cc < 1.2.57 clones also share env reads: call this only
    /// after the crate's `CC`/`CFLAGS` are exported (by `prepare_builder`, or by
    /// the `CMake` build before `apply_universal_build_options`), and not for builds
    /// that alter `CFLAGS` (jitterentropy, the memcmp probe). Never use the
    /// template directly: on cc 1.6+ that would snapshot the environment for every
    /// later clone.
    fn new_build(&self) -> cc::Build {
        self.build_template.clone()
    }

    pub(crate) fn collect_universal_build_options(
        &self,
        cc_build: &cc::Build,
        do_quote_paths: bool,
    ) -> (bool, Vec<BuildOption>) {
        let mut build_options: Vec<BuildOption> = Vec::new();

        let is_cl_like = compiler_is_cl_like(&cc_build.get_compiler());

        match requested_c_std() {
            CStdRequested::C99 => {
                build_options.push(BuildOption::std("c99"));
            }
            CStdRequested::C11 => {
                build_options.push(BuildOption::std("c11"));
            }
            CStdRequested::None => {
                if !is_cl_like {
                    if self.compiler_check("c11", Vec::<String>::new()) {
                        build_options.push(BuildOption::std("c11"));
                    } else {
                        build_options.push(BuildOption::std("c99"));
                    }
                }
            }
        }

        if target_arch() == "x86" && !is_cl_like {
            if let Some(option) = BuildOption::flag_if_supported(cc_build, "-msse2") {
                build_options.push(option);
            }
        }

        if target_os() == "macos" || target_os() == "darwin" {
            // Certain MacOS system headers are guarded by _POSIX_C_SOURCE and _DARWIN_C_SOURCE
            build_options.push(BuildOption::define("_DARWIN_C_SOURCE", "1"));
        }

        let opt_level = cargo_env("OPT_LEVEL");
        match opt_level.as_str() {
            "0" | "1" | "2" => {
                if is_no_asm() {
                    emit_warning("AWS_LC_SYS_NO_ASM found. Disabling assembly code usage.");
                    build_options.push(BuildOption::define("OPENSSL_NO_ASM", "1"));
                }
            }
            _ => {
                assert!(
                    !is_no_asm(),
                    "AWS_LC_SYS_NO_ASM only allowed for debug builds!"
                );
                if !is_cl_like {
                    let path_str = if do_quote_paths {
                        format!("\"{}\"", self.manifest_dir.display())
                    } else {
                        format!("{}", self.manifest_dir.display())
                    };

                    let flag = format!("-ffile-prefix-map={path_str}=");
                    if let Ok(true) = cc_build.is_flag_supported(&flag) {
                        emit_warning(format!("Using flag: {flag}"));
                        build_options.push(BuildOption::flag(&flag));
                    } else {
                        emit_warning("NOTICE: Build environment source paths might be visible in release binary.");
                        let flag = format!("-fdebug-prefix-map={path_str}=");
                        if let Ok(true) = cc_build.is_flag_supported(&flag) {
                            emit_warning(format!("Using flag: {flag}"));
                            build_options.push(BuildOption::flag(&flag));
                        }
                    }
                }
            }
        }

        if target_os() == "macos" {
            // This compiler error has only been seen on MacOS x86_64:
            // ```
            // clang: error: overriding '-mmacosx-version-min=13.7' option with '--target=x86_64-apple-macosx14.2' [-Werror,-Woverriding-t-option]
            // ```
            if let Some(option) =
                BuildOption::flag_if_supported(cc_build, "-Wno-overriding-t-option")
            {
                build_options.push(option);
            }
            if let Some(option) = BuildOption::flag_if_supported(cc_build, "-Wno-overriding-option")
            {
                build_options.push(option);
            }
        }
        (is_cl_like, build_options)
    }

    pub fn collect_cc_only_build_options(&self) -> Vec<BuildOption> {
        let mut build_options: Vec<BuildOption> = Vec::new();
        let is_cl_like = compiler_is_cl_like(&self.new_build().get_compiler());
        if !is_cl_like {
            build_options.push(BuildOption::flag("-Wno-unused-parameter"));
            // On emscripten, `-pthread` forces a shared-memory wasm module
            // (requires SharedArrayBuffer); build single-threaded instead.
            if target_os() != "emscripten" {
                build_options.push(BuildOption::flag("-pthread"));
            }
            if target_os() == "linux" || target_os() == "emscripten" {
                build_options.push(BuildOption::define("_XOPEN_SOURCE", "700"));
            } else if target_vendor() != "apple" {
                // Needed by illumos
                build_options.push(BuildOption::define("__EXTENSIONS__", "1"));
            }
        }
        if !should_build_jitter_entropy() {
            build_options.push(BuildOption::define("DISABLE_CPU_JITTER_ENTROPY", "1"));
        }
        self.add_includes(&mut build_options);
        self.add_defines(&mut build_options, is_cl_like);

        build_options
    }

    fn add_includes(&self, build_options: &mut Vec<BuildOption>) {
        // The order of includes matters
        if let Some(prefix) = &self.build_prefix {
            build_options.push(BuildOption::define("BORINGSSL_IMPLEMENTATION", "1"));
            build_options.push(BuildOption::define("BORINGSSL_PREFIX", prefix.as_str()));
            build_options.push(BuildOption::include(
                self.manifest_dir.join("generated-include"),
            ));
        }
        build_options.push(BuildOption::include(self.manifest_dir.join("include")));
        build_options.push(BuildOption::include(
            self.manifest_dir.join("aws-lc").join("include"),
        ));
        build_options.push(BuildOption::include(
            self.manifest_dir
                .join("aws-lc")
                .join("third_party")
                .join("s2n-bignum")
                .join("include"),
        ));
        build_options.push(BuildOption::include(
            self.manifest_dir
                .join("aws-lc")
                .join("third_party")
                .join("s2n-bignum")
                .join("s2n-bignum-imported")
                .join("include"),
        ));

        if should_build_jitter_entropy() {
            let jitterentropy_path = self
                .manifest_dir
                .join("aws-lc")
                .join("third_party")
                .join("jitterentropy")
                .join("jitterentropy-library");

            build_options.push(BuildOption::include(&jitterentropy_path));
            build_options.push(BuildOption::include(jitterentropy_path.join("src")));
        }
    }

    pub fn create_builder(&self) -> cc::Build {
        let mut cc_build = self.new_build();
        let build_options = self.collect_cc_only_build_options();
        for option in build_options {
            option.apply_cc(&mut cc_build);
        }
        cc_build
    }

    pub fn prepare_builder(&self) -> cc::Build {
        if let Some(cflags) = get_crate_cflags() {
            set_env_for_target("CFLAGS", cflags);
        }
        export_crate_compilers();

        let mut cc_build = self.create_builder();
        let (_, build_options) = self.collect_universal_build_options(&cc_build, false);
        for option in build_options {
            option.apply_cc(&mut cc_build);
        }
        if is_small() {
            emit_warning(
                "Size-optimized AWS-LC build enabled. Applying OPENSSL_SMALL and disabling AVX-512 assembly.",
            );
            for option in Self::small_c_build_options() {
                option.apply_cc(&mut cc_build);
            }
        }

        // Add --noexecstack flag for assembly files to prevent executable stacks
        // This matches the behavior of AWS-LC's CMake build which uses -Wa,--noexecstack
        // See: https://github.com/aws/aws-lc/blob/main/crypto/CMakeLists.txt#L77
        if target_os() == "linux" || target_os().ends_with("bsd") {
            cc_build.asm_flag("-Wa,--noexecstack");
        }

        // `-ffile-prefix-map` does not reach GNU `as` for `.S` sources, so in
        // release-style builds mirror it with `-Wa,--debug-prefix-map=...`.
        if let Some(asm_flag) = asm_debug_prefix_map_flag(&self.manifest_dir) {
            cc_build.asm_flag(asm_flag);
        }

        cc_build
    }

    #[allow(clippy::zero_sized_map_values)]
    fn build_s2n_bignum_source_feature_map() -> HashMap<String, CompilerFeature> {
        let mut source_feature_map: HashMap<String, CompilerFeature> = HashMap::new();
        source_feature_map.insert("sha3_keccak_f1600_alt.S".into(), CompilerFeature::NeonSha3);
        source_feature_map.insert("sha3_keccak2_f1600.S".into(), CompilerFeature::NeonSha3);
        source_feature_map.insert(
            "sha3_keccak4_f1600_alt2.S".into(),
            CompilerFeature::NeonSha3,
        );
        source_feature_map
    }

    #[allow(clippy::unused_self)]
    fn add_defines(&self, build_options: &mut Vec<BuildOption>, is_cl_like: bool) {
        // WIN32_LEAN_AND_MEAN and NOMINMAX are needed for all Windows targets to avoid
        // header type definition errors, no matter the compiler. This matches the behavior
        // in aws-lc/CMakeLists.txt, which defines these for all WIN32 targets
        if target_os() == "windows" {
            build_options.push(BuildOption::define("WIN32_LEAN_AND_MEAN", ""));
            build_options.push(BuildOption::define("NOMINMAX", ""));
        }

        // Suppress MSVC CRT deprecation warnings (fopen, getenv, strerror, etc.).
        // This applies to any compiler targeting the MSVC environment since the
        // warnings originate from the Windows SDK/CRT headers, not the compiler,
        // so it is keyed on the target ABI rather than the driver mode.
        if target_is_msvc() {
            build_options.push(BuildOption::define("_CRT_SECURE_NO_WARNINGS", "1"));
        }

        // MSVC STL macros: only meaningful when compiling in cl driver mode.
        if is_cl_like {
            build_options.push(BuildOption::define("_HAS_EXCEPTIONS", "0"));
            build_options.push(BuildOption::define(
                "_STL_EXTRA_DISABLED_WARNINGS",
                "4774 4987",
            ));
        }

        // Target Windows 7 (0x0601 == _WIN32_WINNT_WIN7) for any win7 target triple.
        if target().contains("-win7-windows-") {
            build_options.push(BuildOption::define("_WIN32_WINNT", "0x0601"));
            emit_warning(format!(
                "Setting _WIN32_WINNT to _WIN32_WINNT_WIN7 for {} target",
                target()
            ));

            // Additional workaround for MinGW: the upstream C source
            // (crypto/rand_extra/windows.c) gates the Win7 compat path
            // (BCryptGenRandom) with `!defined(__MINGW32__)`, which prevents MinGW
            // from using it even when `_WIN32_WINNT` targets Win7. We define
            // AWSLC_WINDOWS_7_COMPAT directly to bypass that guard until the
            // upstream fix lands: https://github.com/aws/aws-lc/pull/3239
            if !is_cl_like {
                build_options.push(BuildOption::define("AWSLC_WINDOWS_7_COMPAT", ""));
            }
        }
    }

    /// Compiles jitterentropy at `-O0`. The CFLAGS guards span the builder's whole
    /// lifetime because which `cc` call first reads the environment varies by
    /// release (#1064, #1252).
    fn compile_jitter_entropy(
        &self,
        is_cl_like: bool,
        sources: &[PathBuf],
    ) -> Result<Vec<PathBuf>, cc::Error> {
        let je_cflags_guards = Self::jitter_entropy_cflags_guards(is_cl_like);
        let mut je_builder = self.prepare_jitter_entropy_builder(&je_cflags_guards, is_cl_like);
        je_builder.flag(format!(
            "{}{}",
            if is_cl_like { "/FI" } else { "--include=" },
            self.manifest_dir
                .join("generated-include")
                .join("openssl")
                .join("boringssl_prefix_symbols.h")
                .display()
        ));
        je_builder.files(sources);
        je_builder.try_compile_intermediates()
    }

    /// The returned builder must not outlive `_cflags_guards`.
    fn prepare_jitter_entropy_builder(
        &self,
        _cflags_guards: &JitterEntropyCflagsGuards,
        is_cl_like: bool,
    ) -> cc::Build {
        // See: https://github.com/aws/aws-lc/blob/2294510cd0ecb2d5946461e3dbb038363b7b94cb/third_party/jitterentropy/CMakeLists.txt#L19-L35
        let mut build_options: Vec<BuildOption> = Vec::new();
        self.add_includes(&mut build_options);
        self.add_defines(&mut build_options, is_cl_like);

        let mut je_builder = cc::Build::new();
        for option in build_options {
            option.apply_cc(&mut je_builder);
        }

        je_builder.define("AWSLC", "1");
        if target_os() == "macos" || target_os() == "darwin" {
            // Certain MacOS system headers are guarded by _POSIX_C_SOURCE and _DARWIN_C_SOURCE
            je_builder.define("_DARWIN_C_SOURCE", "1");
        }
        // Only enable PIC on non-Windows targets. Windows doesn't support -fPIC.
        if target_os() != "windows" {
            je_builder.pic(true);
        }
        for &flag in Self::jitter_entropy_dialect_flags(is_cl_like) {
            je_builder.flag(flag);
        }

        // Jitter uses a separate `cc::Build`, so it needs its own path-mapping
        // flags even when the main builder already has them. Apply them here
        // unconditionally because jitter is always compiled at `-O0`.
        for option in self.collect_path_reproducibility_options(&je_builder, is_cl_like) {
            option.apply_cc(&mut je_builder);
        }
        je_builder
    }

    /// Returns path-reproducibility flags for the configured compiler.
    /// These rewrite DWARF source paths and `__FILE__`; clang may also need
    /// extra stripping for `UBSan` metadata. Returns an empty `Vec` in cl
    /// driver mode, which has no equivalent flags.
    fn collect_path_reproducibility_options(
        &self,
        cc_build: &cc::Build,
        is_cl_like: bool,
    ) -> Vec<BuildOption> {
        let mut opts: Vec<BuildOption> = Vec::new();
        if is_cl_like {
            return opts;
        }

        let path_str = self.manifest_dir.display().to_string();

        // Prefer the flag that rewrites both `__FILE__` and DWARF; older GCC
        // may only support `-fdebug-prefix-map`.
        let file_flag = format!("-ffile-prefix-map={path_str}=");
        if cc_build.is_flag_supported(&file_flag).unwrap_or(false) {
            opts.push(BuildOption::flag(&file_flag));
        } else {
            let dbg_flag = format!("-fdebug-prefix-map={path_str}=");
            if cc_build.is_flag_supported(&dbg_flag).unwrap_or(false) {
                opts.push(BuildOption::flag(&dbg_flag));
            }
        }

        // Clang UBSan metadata can still carry the full source path at `-O0`.
        if let Some(opt) = BuildOption::flag_if_supported(
            cc_build,
            "-fsanitize-undefined-strip-path-components=-1",
        ) {
            opts.push(opt);
        }

        opts
    }

    /// Dialect-specific compiler flags for the jitterentropy sub-build.
    ///
    /// Keyed on compiler driver mode: `clang-cl` rejects the GNU-only
    /// `--param ssp-buffer-size=4` pair, while plain `clang` rejects the
    /// cl-style `-Od`/`-W4` flags. See: <https://github.com/aws/aws-lc-rs/issues/1146>
    fn jitter_entropy_dialect_flags(is_cl_like: bool) -> &'static [&'static str] {
        if is_cl_like {
            &["-Od", "-W4", "-DYNAMICBASE"]
        } else {
            &[
                "-fwrapv",
                "--param",
                "ssp-buffer-size=4",
                "-fvisibility=hidden",
                "-Wcast-align",
                "-Wmissing-field-initializers",
                "-Wshadow",
                "-Wswitch-enum",
                "-Wextra",
                "-Wall",
                "-pedantic",
                // Compilation will fail if optimizations are enabled.
                "-O0",
                "-fwrapv",
                "-Wconversion",
            ]
        }
    }

    /// The cc crate appends CFLAGS at the end of the compiler command line,
    /// which means CFLAGS optimization flags override build script flags.
    /// Jitterentropy MUST be compiled with -O0, so we temporarily override
    /// CFLAGS to replace any optimization flags with -O0.
    ///
    /// Every variable cc reads must be filtered (see `cflags_env_names`); missing
    /// the raw-triple spelling was <https://github.com/aws/aws-lc-rs/issues/1205>.
    fn jitter_entropy_cflags_guards(is_cl_like: bool) -> JitterEntropyCflagsGuards {
        // Jitterentropy may be cross-compiled, so include TARGET_CFLAGS too.
        let cflags_env_names = cflags_env_names(true);

        let filter_cflags = |value: &str| -> String {
            let filtered: String = value
                .split_whitespace()
                .filter(|flag| !flag.starts_with("-O") && !flag.starts_with("/O"))
                .collect::<Vec<_>>()
                .join(" ");
            if is_cl_like {
                format!("{filtered} -Od").trim().to_string()
            } else {
                format!("{filtered} -O0 -U_FORTIFY_SOURCE")
                    .trim()
                    .to_string()
            }
        };

        JitterEntropyCflagsGuards(
            cflags_env_names
                .into_iter()
                .filter_map(|name| {
                    let value = env::var(&name).ok()?;
                    Some(EnvGuard::new(&name, filter_cflags(&value)))
                })
                .collect(),
        )
    }

    fn add_all_files(&self, sources: &[&'static str], cc_build: &mut cc::Build) {
        let is_cl_like = compiler_is_cl_like(&cc_build.get_compiler());

        let force_include_option = if is_cl_like { "/FI" } else { "--include=" };
        // s2n-bignum is compiled separately due to needing extra flags
        let mut s2n_bignum_builder = cc_build.clone();
        s2n_bignum_builder.flag(format!(
            "{}{}",
            force_include_option,
            self.manifest_dir
                .join("generated-include")
                .join("openssl")
                .join("boringssl_prefix_symbols_asm.h")
                .display()
        ));
        s2n_bignum_builder.define("S2N_BN_HIDE_SYMBOLS", "1");

        // CPU Jitter Entropy is compiled separately due to needing specific flags.
        let build_jitter_entropy = should_build_jitter_entropy();
        let mut jitter_entropy_sources: Vec<PathBuf> = Vec::new();

        let mut build_options = vec![];
        self.add_includes(&mut build_options);
        let mut nasm_builder = NasmBuilder::new(self.manifest_dir.clone(), self.out_dir.clone());

        for option in &build_options {
            option.apply_nasm(&mut nasm_builder);
        }
        if is_small() {
            for option in Self::small_nasm_build_options() {
                option.apply_nasm(&mut nasm_builder);
            }
            if use_prebuilt_nasm() {
                emit_warning(
                    "Prebuilt NASM objects cannot apply size-optimization definitions. \
                     Install NASM to minimize Windows assembly code.",
                );
            }
        }

        let s2n_bignum_source_feature_map = Self::build_s2n_bignum_source_feature_map();
        let compiler_features = self.compiler_features.take();
        for source in sources {
            let source_path = self.manifest_dir.join("aws-lc").join(source);
            let is_s2n_bignum = std::path::Path::new(source).starts_with("third_party/s2n-bignum");
            let is_jitter_entropy =
                std::path::Path::new(source).starts_with("third_party/jitterentropy");

            if !source_path.is_file() {
                emit_warning(format!("Not a file: {:?}", source_path.as_os_str()));
                continue;
            }
            if is_s2n_bignum {
                let filename: String = source_path
                    .file_name()
                    .unwrap()
                    .to_str()
                    .unwrap()
                    .to_string();

                if let Some(compiler_feature) = s2n_bignum_source_feature_map.get(&filename) {
                    if compiler_features.contains(compiler_feature) {
                        s2n_bignum_builder.file(source_path);
                    } else {
                        emit_warning(format!(
                            "Skipping due to missing compiler features: {:?}",
                            source_path.as_os_str()
                        ));
                    }
                } else {
                    s2n_bignum_builder.file(source_path);
                }
            } else if is_jitter_entropy {
                // Only compile if not disabled.
                if build_jitter_entropy {
                    jitter_entropy_sources.push(source_path);
                }
            } else if source_path.extension() == Some("asm".as_ref()) {
                nasm_builder.file(source_path);
            } else {
                cc_build.file(source_path);
            }
        }
        self.compiler_features.set(compiler_features);
        let s2n_bignum_object_files = s2n_bignum_builder.compile_intermediates();
        for object in s2n_bignum_object_files {
            cc_build.object(object);
        }
        if build_jitter_entropy {
            let jitter_entropy_object_files = self
                .compile_jitter_entropy(is_cl_like, &jitter_entropy_sources)
                .unwrap_or_else(|e| panic!("Failed to compile jitterentropy: {e}"));
            for object in jitter_entropy_object_files {
                cc_build.object(object);
            }
        }
        let nasm_object_files = nasm_builder.compile_intermediates();
        for object in nasm_object_files {
            cc_build.object(object);
        }
    }

    fn build_library(&self, sources: &[&'static str]) {
        let mut cc_build = self.prepare_builder();
        self.run_compiler_checks(&mut cc_build);

        self.add_all_files(sources, &mut cc_build);

        let lib_name = if let Some(prefix) = &self.build_prefix {
            format!("{}_crypto", prefix.as_str())
        } else {
            "crypto".to_string()
        };

        // When whole-archive linking is requested, suppress cc-rs's automatic
        // emission of `cargo:rustc-link-{lib,search}=` so we can emit our own
        // directives with the `+whole-archive` modifier (which cc-rs has no
        // way to express). cc still performs the actual compilation and
        // archive creation; only the metadata output is silenced.
        if is_link_whole_archive() {
            cc_build.cargo_metadata(false);
        }
        cc_build.compile(&lib_name);
        if is_link_whole_archive() {
            emit_warning(format!(
                "AWS_LC_SYS_LINK_WHOLE_ARCHIVE set: linking '{lib_name}' with +whole-archive"
            ));
            println!("cargo:rustc-link-search=native={}", self.out_dir.display());
            println!(
                "cargo:rustc-link-lib={}={lib_name}",
                self.output_lib_type.rust_link_lib_kind()
            );
        }
    }

    // This performs basic checks of compiler capabilities and sets an appropriate flag on success.
    // This should be kept in alignment with the checks performed by AWS-LC's CMake build.
    // See: https://github.com/search?q=repo%3Aaws%2Faws-lc%20check_compiler&type=code
    fn compiler_check<T, S>(&self, basename: &str, extra_flags: T) -> bool
    where
        T: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let mut ret_val = false;
        let output_dir = self.out_dir.join(format!("out-{basename}"));
        let source_file = self
            .manifest_dir
            .join("aws-lc")
            .join("tests")
            .join("compiler_features_tests")
            .join(format!("{basename}.c"));
        if !source_file.exists() {
            emit_warning("######");
            emit_warning("###### WARNING: MISSING GIT SUBMODULE ######");
            emit_warning(format!(
                "  -- Did you initialize the repo's git submodules? Unable to find source file: {}.",
                source_file.display()
            ));
            emit_warning("  -- run 'git submodule update --init --recursive' to initialize.");
            emit_warning("######");
            emit_warning("######");
        }
        let mut cc_build = self.new_build();
        cc_build
            .file(source_file)
            .warnings_into_errors(true)
            .out_dir(&output_dir);
        for flag in extra_flags {
            let flag = flag.as_ref();
            cc_build.flag(flag);
        }

        // GNU-style flag, so gated on the actual compiler family.
        let compiler = cc_build.get_compiler();
        if compiler.is_like_gnu() || compiler.is_like_clang() {
            cc_build.flag("-Wno-unused-parameter");
        }
        let result = cc_build.try_compile_intermediates();

        if result.is_ok() {
            ret_val = true;
        }
        if fs::remove_dir_all(&output_dir).is_err() {
            emit_warning(format!("Failed to remove {}", output_dir.display()));
        }
        emit_warning(format!(
            "Compilation of '{basename}.c' {} - {:?}.",
            if ret_val { "succeeded" } else { "failed" },
            result
        ));
        ret_val
    }

    // Detects a GCC bug that causes `memcmp` to erroneously consider two unequal regions of
    // memory to be equal. See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189
    // Mirrors the `check_run` performed by AWS-LC's CMake build:
    // https://github.com/search?q=repo%3Aaws%2Faws-lc%20check_run&type=code
    //
    // The check is layered so that only a high-confidence positive result can fail the build:
    //   1. Skipped when cross-compiling (the probe must run on the build host).
    //   2. Skipped for non-GCC compilers; the bug is GCC-specific, matching CMake's `if(GCC)`.
    //   3. Skipped for GCC versions outside the affected range (9.0-9.3, 10.0-10.1).
    //   4. A probe that fails to build or execute only warns, as in CMake, where `check_run`
    //      compile failures were never fatal. Such failures have historically been
    //      environmental (LTO in CFLAGS, PIE link flags, AIX ABI selection; see #1132,
    //      #1168, #1170), not evidence of the bug.
    //   5. Only a probe that builds, runs, and demonstrates the miscompilation fails the build.
    fn memcmp_check(&self) {
        // (1) The probe binary must run on the build host.
        if is_cross_compiling() {
            return;
        }

        // User CFLAGS are intentionally excluded (#1132): they have no bearing on whether
        // the compiler contains the bug and can break the probe's compile+link (e.g.,
        // -flto=thin without -fuse-ld=lld). The cc crate's computed default flags ARE kept:
        // dropping them broke hardened PIE builds (-fPIC, #1168) and AIX (-maix64, #1170).
        // cc reads the CFLAGS env vars eagerly inside get_compiler(), so guarding them for
        // just this call removes exactly the user-provided flags.
        let memcmp_tool = {
            let _cflags_guards = cflags_ignore_guards();
            let mut probe_build = cc::Build::default();
            // -O3 is required to trigger the bug; the build profile's opt level may be lower.
            probe_build.opt_level(3);
            probe_build.get_compiler()
        };

        // (2) The bug is GCC-specific.
        if !memcmp_tool.is_like_gnu() {
            return;
        }

        // (3) Distributions backport fixes, so the version gate only decides whether to run
        // the probe; the probe decides whether this compiler is actually affected.
        if let Some(version) = gcc_version(&memcmp_tool) {
            if !gcc_version_may_have_memcmp_bug(&version) {
                return;
            }
            emit_warning(format!(
                "GCC {version} may contain a memcmp-related bug \
                (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189); probing the compiler."
            ));
        }

        let basename = "memcmp_invalid_stripped_check";
        let exec_path = out_dir().join(basename);
        let mut memcmp_compile_args: Vec<std::ffi::OsString> = memcmp_tool.args().to_vec();

        // The probe links an executable; honor LDFLAGS for linker configuration (#958).
        if let Ok(ldflags) = std::env::var("LDFLAGS") {
            for flag in ldflags.split_whitespace() {
                memcmp_compile_args.push(flag.into());
            }
        }

        memcmp_compile_args.push(
            self.manifest_dir
                .join("aws-lc")
                .join("tests")
                .join("compiler_features_tests")
                .join(format!("{basename}.c"))
                .into_os_string(),
        );
        memcmp_compile_args.push("-Wno-unused-parameter".into());
        memcmp_compile_args.push("-o".into());
        memcmp_compile_args.push(exec_path.clone().into_os_string());
        let memcmp_args: Vec<_> = memcmp_compile_args
            .iter()
            .map(std::ffi::OsString::as_os_str)
            .collect();
        let memcmp_compile_result =
            execute_command(memcmp_tool.path().as_os_str(), memcmp_args.as_slice());

        // (4) A probe that fails to build only warns, matching CMake's `check_run`.
        if !memcmp_compile_result.status {
            emit_warning(format!(
                "Unable to build the memcmp probe; skipping the GCC memcmp bug check \
                (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189).\n\
                COMPILER: {}\n\
                ARGS: {:?}\n\
                EXECUTED: {}\n\
                ERROR: {}\n\
                OUTPUT: {}\n\
                ",
                memcmp_tool.path().display(),
                memcmp_args.as_slice(),
                memcmp_compile_result.executed,
                memcmp_compile_result.stderr,
                memcmp_compile_result.stdout
            ));
            return;
        }

        let result = execute_command(exec_path.as_os_str(), &[]);
        if result.executed {
            // (5) The probe demonstrated the miscompilation; fail the build.
            assert!(
                result.status,
                "### COMPILER BUG DETECTED ###\nYour compiler ({}) is not supported due to a memcmp related bug reported in \
                https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189. \
                We strongly recommend against using this compiler. \n\
                EXIT CODE: {:?}\n\
                ERROR: {}\n\
                OUTPUT: {}\n\
                ",
                memcmp_tool.path().display(),
                result.exit_code,
                result.stderr,
                result.stdout
            );
        } else {
            // Failure to launch the probe (e.g., a noexec build directory) is
            // environmental, not evidence of the bug.
            emit_warning(format!(
                "Unable to execute the memcmp probe; skipping the GCC memcmp bug check \
                (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189). ERROR: {:?}",
                result.spawn_error
            ));
        }
        let _ = fs::remove_file(exec_path);
    }
    fn run_compiler_checks(&self, cc_build: &mut cc::Build) {
        if self.compiler_check("stdalign_check", Vec::<&'static str>::new()) {
            cc_build.define("AWS_LC_STDALIGN_AVAILABLE", Some("1"));
        }
        // Only run builtin_swap_check for GCC/Clang (matching CMake). On MSVC,
        // try_compile_intermediates succeeds but __builtin_bswap* are unresolved at
        // link time. Without this guard the define is set and crypto/internal.h skips
        // the correct _MSC_VER path that uses _byteswap_* intrinsics.
        if !target_is_msvc()
            && self.compiler_check("builtin_swap_check", Vec::<&'static str>::new())
        {
            cc_build.define("AWS_LC_BUILTIN_SWAP_SUPPORTED", Some("1"));
        }
        if target_arch() == "aarch64"
            && self.compiler_check("neon_sha3_check", vec!["-march=armv8.4-a+sha3"])
        {
            let mut compiler_features = self.compiler_features.take();
            compiler_features.push(CompilerFeature::NeonSha3);
            self.compiler_features.set(compiler_features);
            cc_build.define("MY_ASSEMBLER_SUPPORTS_NEON_SHA3_EXTENSION", Some("1"));
        }
        if target_os() == "linux" || target_os() == "android" {
            if self.compiler_check("linux_random_h", Vec::<&'static str>::new()) {
                cc_build.define("HAVE_LINUX_RANDOM_H", Some("1"));
            } else if self.compiler_check("linux_random_h", vec!["-DDEFINE_U32"]) {
                cc_build.define("HAVE_LINUX_RANDOM_H", Some("1"));
                cc_build.define("AWS_LC_URANDOM_NEEDS_U32", Some("1"));
            }
        }
        self.memcmp_check();
    }
}

/// The `CFLAGS`-family env var names the cc crate consults (its `target_envs`).
/// cc's `envflags` collects flags from EVERY one that is set, so overriding or
/// suppressing user flags must handle all of them -- including the raw-triple
/// spelling (`CFLAGS_aarch64-apple-darwin`), which a shell cannot usually export
/// but a process can set via `setenv` (#1205).
///
/// `include_target_prefixed` adds `TARGET_CFLAGS`: cc reads it when cross-compiling
/// and `HOST_CFLAGS` otherwise, so callers that may cross-compile need both.
fn cflags_env_names(include_target_prefixed: bool) -> Vec<String> {
    let target = target();
    // cc 1.2.26 replaces only `-`, while newer versions replace both `-` and `.`.
    // Include both spellings because either version may satisfy our dependency range.
    let target_u_legacy = target.replace('-', "_");
    let target_u = target.replace(['-', '.'], "_");
    let mut names = vec![
        format!("CFLAGS_{target}"),
        format!("CFLAGS_{target_u_legacy}"),
    ];
    if target_u != target_u_legacy {
        names.push(format!("CFLAGS_{target_u}"));
    }
    names.push("HOST_CFLAGS".to_string());
    if include_target_prefixed {
        names.push("TARGET_CFLAGS".to_string());
    }
    names.push("CFLAGS".to_string());
    names
}

/// Filtered `CFLAGS` overrides for the jitterentropy build, restored on drop. A
/// distinct type so only these guards satisfy `prepare_jitter_entropy_builder`.
struct JitterEntropyCflagsGuards(#[allow(dead_code)] Vec<EnvGuard>);

/// Temporarily removes every `CFLAGS`-family env var the cc crate consults (see its
/// `target_envs`), so that `cc::Build::get_compiler()` computes only the target's
/// default flags. `cc` concatenates all set variants, so all must go.
fn cflags_ignore_guards() -> Vec<EnvGuard> {
    cflags_env_names(true)
        .iter()
        .map(|name| EnvGuard::remove(name))
        .collect()
}

/// Whether the DWARF path-stripping assembler flag applies to this target and
/// profile, independent of the compiler: only GNU `as` targets, only release-style
/// profiles, and `-Wa,...` must stay a single bare token (no spaces).
fn asm_debug_prefix_map_applies(target_os: &str, opt_level: &str, manifest_dir: &Path) -> bool {
    (target_os == "linux" || target_os.ends_with("bsd"))
        && !matches!(opt_level, "0" | "1" | "2")
        && !manifest_dir.to_string_lossy().contains(' ')
}

/// The GNU `as` flag that strips `manifest_dir` from DWARF emitted for `.S`
/// sources, or `None` when it must not be used.
///
/// Gated on the compiler family, not a flag probe: GCC does not forward
/// `-ffile-prefix-map` to `as`, while clang's integrated assembler honors it
/// directly and rejects `--debug-prefix-map` -- but only diagnoses it when a job
/// reaches the assembler, which a C probe under `-flto` never does. The remaining
/// probe, run with user `CFLAGS` suppressed, only guards a GNU `as` predating the
/// option (binutils 2.26).
/// See: <https://github.com/aws/aws-lc-rs/issues/1211>
pub(crate) fn asm_debug_prefix_map_flag(manifest_dir: &Path) -> Option<String> {
    if !asm_debug_prefix_map_applies(&target_os(), &cargo_env("OPT_LEVEL"), manifest_dir) {
        return None;
    }

    let flag = format!("-Wa,--debug-prefix-map={}=", manifest_dir.display());

    let _cflags_guards = cflags_ignore_guards();
    let probe_build = cc::Build::default();
    // `try_get_compiler` so an unresolvable compiler cannot panic the build script.
    let Ok(compiler) = probe_build.try_get_compiler() else {
        return None;
    };
    if !compiler.is_like_gnu() {
        return None;
    }
    if !probe_build.is_flag_supported(&flag).unwrap_or(false) {
        emit_warning(format!("Assembler does not support flag: {flag}"));
        return None;
    }
    emit_warning(format!("Using assembler flag: {flag}"));
    Some(flag)
}

/// Version reported by a GCC-like compiler, if determinable. Prefers `-dumpfullversion`
/// (GCC 7+); `-dumpversion` may report only the major version.
fn gcc_version(compiler: &cc::Tool) -> Option<String> {
    for flag in ["-dumpfullversion", "-dumpversion"] {
        let result = execute_command(compiler.path().as_os_str(), &[flag.as_ref()]);
        if result.status {
            let version = result.stdout.trim();
            if !version.is_empty() {
                return Some(version.to_string());
            }
        }
    }
    None
}

/// GCC bug 95189 (`memcmp` wrongly folded like `strcmp`) was introduced in GCC 9 and fixed
/// in the 9.4 and 10.2 releases; GCC 11+ never contained it.
/// See: <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189>
fn gcc_version_may_have_memcmp_bug(version: &str) -> bool {
    let mut parts = version.trim().split('.');
    let major = parts.next().and_then(|v| v.parse::<u64>().ok());
    let minor = parts.next().and_then(|v| v.parse::<u64>().ok());
    match (major, minor) {
        (Some(9), Some(minor)) => minor < 4,
        (Some(10), Some(minor)) => minor < 2,
        // Major-only version strings and unparseable output are conservatively probed.
        (Some(9 | 10), None) | (None, _) => true,
        (Some(_), _) => false,
    }
}

impl crate::Builder for CcBuilder {
    fn check_dependencies(&self) -> Result<(), String> {
        if OutputLibType::Dynamic == self.output_lib_type {
            // https://github.com/rust-lang/cc-rs/issues/594
            return Err("CcBuilder only supports static builds".to_string());
        }

        if target_env() == "ohos" {
            return Err("OpenHarmony targets must be built with CMake.".to_string());
        }

        if Some(true) == env_var_to_bool("CARGO_FEATURE_SSL") {
            return Err("cc_builder for libssl not supported".to_string());
        }

        Ok(())
    }

    fn build(&self) -> Result<(), String> {
        if target_os() == "windows"
            && target_arch() == "aarch64"
            && target_is_msvc()
            && get_crate_cc().is_none()
        {
            if let Some(clang_cl) = find_clang_cl() {
                set_env_for_target("CC", clang_cl);
            } else {
                emit_warning(
                    "Windows ARM64 (aarch64-pc-windows-msvc) requires clang-cl. \
                     Install the 'C++ Clang Compiler for Windows' component in \
                     Visual Studio Build Tools, or set CC to a working clang-cl. \
                     See User Guide: https://aws.github.io/aws-lc-rs/index.html",
                );
            }
        }

        println!("cargo:root={}", self.out_dir.display());
        let sources = crate::cc_builder::identify_sources();
        self.build_library(sources.as_slice());

        crate::emit_source_library_metadata(
            &self.out_dir,
            self.output_lib_type,
            &self.build_prefix,
        );

        crate::emit_source_build_metadata(&self.manifest_dir, &self.build_prefix);

        Ok(())
    }

    fn name(&self) -> &'static str {
        "CC"
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{EnvGuard, ENV_MUTEX};

    /// Runs `f` with `CARGO_CFG_TARGET_ENV` forced to `env_val`, restoring the
    /// previous value afterward. Holds the shared env lock so it doesn't race
    /// other env-mutating builder tests.
    fn with_target_env<R>(env_val: &str, f: impl FnOnce() -> R) -> R {
        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let _guard = EnvGuard::new("CARGO_CFG_TARGET_ENV", env_val);
        f()
    }

    #[test]
    fn test_target_is_msvc_matches_msvc_abi_only() {
        assert!(with_target_env("msvc", target_is_msvc));
        assert!(!with_target_env("gnu", target_is_msvc));
        assert!(!with_target_env("musl", target_is_msvc));
        assert!(!with_target_env("", target_is_msvc));
    }

    // GCC bug 95189 affected the 9.0-9.3 and 10.0-10.1 releases; it was fixed in
    // 9.4 and 10.2, and GCC 11+ never contained it.
    #[test]
    fn test_gcc_version_may_have_memcmp_bug() {
        // Affected versions must be probed.
        assert!(gcc_version_may_have_memcmp_bug("9"));
        assert!(gcc_version_may_have_memcmp_bug("9.1.0"));
        assert!(gcc_version_may_have_memcmp_bug("9.2.0"));
        assert!(gcc_version_may_have_memcmp_bug("9.3.1"));
        assert!(gcc_version_may_have_memcmp_bug("10"));
        assert!(gcc_version_may_have_memcmp_bug("10.1.0"));
        // Releases containing the fix are skipped.
        assert!(!gcc_version_may_have_memcmp_bug("9.4.0"));
        assert!(!gcc_version_may_have_memcmp_bug("9.5.0"));
        assert!(!gcc_version_may_have_memcmp_bug("10.2.1"));
        // Unaffected major versions are skipped.
        assert!(!gcc_version_may_have_memcmp_bug("4.8.5"));
        assert!(!gcc_version_may_have_memcmp_bug("8.5.0"));
        assert!(!gcc_version_may_have_memcmp_bug("11.4.0"));
        assert!(!gcc_version_may_have_memcmp_bug("12"));
        assert!(!gcc_version_may_have_memcmp_bug("15.1.0"));
        // Whitespace from command output is tolerated.
        assert!(gcc_version_may_have_memcmp_bug("9.3.0\n"));
        // Undeterminable versions are conservatively probed.
        assert!(gcc_version_may_have_memcmp_bug(""));
        assert!(gcc_version_may_have_memcmp_bug("unknown"));
    }

    // The builder probes must exclude user CFLAGS (#1132, #1211): every env var
    // variant `cc` consults must be suppressed, and restored when the guards drop.
    #[test]
    fn test_cflags_ignore_guards_suppress_and_restore() {
        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let _target_guard = EnvGuard::new("TARGET", "x86_64-unknown-linux-gnu");
        let _g1 = EnvGuard::new("CFLAGS", "-flto=thin");
        let _g2 = EnvGuard::new("CFLAGS_x86_64-unknown-linux-gnu", "-fdash-variant");
        let _g3 = EnvGuard::new("CFLAGS_x86_64_unknown_linux_gnu", "-funderscore-variant");
        let _g4 = EnvGuard::new("HOST_CFLAGS", "-fhost");
        let _g5 = EnvGuard::new("TARGET_CFLAGS", "-ftarget");
        {
            let _guards = cflags_ignore_guards();
            assert!(env::var("CFLAGS").is_err());
            assert!(env::var("CFLAGS_x86_64-unknown-linux-gnu").is_err());
            assert!(env::var("CFLAGS_x86_64_unknown_linux_gnu").is_err());
            assert!(env::var("HOST_CFLAGS").is_err());
            // `cc` reads this variant instead of `HOST_CFLAGS` when cross-compiling.
            assert!(env::var("TARGET_CFLAGS").is_err());
        }
        assert_eq!(env::var("CFLAGS").unwrap(), "-flto=thin");
        assert_eq!(
            env::var("CFLAGS_x86_64-unknown-linux-gnu").unwrap(),
            "-fdash-variant"
        );
        assert_eq!(
            env::var("CFLAGS_x86_64_unknown_linux_gnu").unwrap(),
            "-funderscore-variant"
        );
        assert_eq!(env::var("HOST_CFLAGS").unwrap(), "-fhost");
        assert_eq!(env::var("TARGET_CFLAGS").unwrap(), "-ftarget");
    }

    // Compiler-independent gating only; the GNU-family gate (#1211) needs a real
    // compiler and is covered by the test below.
    #[test]
    fn test_asm_debug_prefix_map_applies() {
        use std::path::Path;
        let dir = Path::new("/tmp/aws-lc-sys");

        // Release-style profiles on GNU `as` targets.
        assert!(asm_debug_prefix_map_applies("linux", "3", dir));
        assert!(asm_debug_prefix_map_applies("linux", "s", dir));
        assert!(asm_debug_prefix_map_applies("linux", "z", dir));
        assert!(asm_debug_prefix_map_applies("freebsd", "3", dir));
        assert!(asm_debug_prefix_map_applies("netbsd", "3", dir));

        // Debug-style profiles keep full paths.
        for opt_level in ["0", "1", "2"] {
            assert!(!asm_debug_prefix_map_applies("linux", opt_level, dir));
        }

        // Targets that do not use GNU `as`.
        for target_os in ["macos", "windows", "ios", "android"] {
            assert!(!asm_debug_prefix_map_applies(target_os, "3", dir));
        }

        // `-Wa,...` must stay one bare token, so spaced paths are skipped.
        assert!(!asm_debug_prefix_map_applies(
            "linux",
            "3",
            Path::new("/tmp/aws lc sys")
        ));
    }

    /// Host triple for `TARGET`/`HOST`, which `cc` requires but `cargo test` omits.
    fn host_triple() -> Option<String> {
        let rustc = env::var("RUSTC").unwrap_or_else(|_| "rustc".to_string());
        let output = std::process::Command::new(rustc).arg("-vV").output().ok()?;
        String::from_utf8(output.stdout)
            .ok()?
            .lines()
            .find_map(|line| line.strip_prefix("host: ").map(str::to_string))
    }

    /// Env for natively probing `cc_value` with `-flto=thin` in user CFLAGS, or
    /// `None` where that cannot run: `cc` panics on a `CARGO_CFG_TARGET_OS`
    /// contradicting `TARGET`, so only hosts where the flag applies qualify.
    /// Native TARGET/HOST so the probe reflects the compiler, not a missing sysroot.
    fn native_lto_probe_guards(cc_value: &str) -> Option<(tempfile::TempDir, Vec<EnvGuard>)> {
        let host_os = std::env::consts::OS;
        if !(host_os == "linux" || host_os.ends_with("bsd")) {
            return None;
        }
        let host = host_triple()?;
        let temp_out = tempfile::tempdir().unwrap();
        let guards = vec![
            EnvGuard::new("OUT_DIR", temp_out.path()),
            EnvGuard::new("TARGET", &host),
            EnvGuard::new("HOST", &host),
            EnvGuard::new("CARGO_CFG_TARGET_OS", host_os),
            EnvGuard::new("OPT_LEVEL", "3"),
            EnvGuard::new("CC", cc_value),
            EnvGuard::new("CFLAGS", "-flto=thin"),
        ];
        Some((temp_out, guards))
    }

    // Regression test for https://github.com/aws/aws-lc-rs/issues/1211: clang must
    // not receive the GNU-only assembler flag when user CFLAGS enables LTO. Pins
    // end behavior only -- the family gate and the CFLAGS-suppressed probe each
    // yield `None` here on their own. linux-clang-lto covers the bug end to end.
    #[test]
    fn test_asm_debug_prefix_map_flag_withheld_from_clang_under_lto() {
        use std::path::Path;

        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let Some(_guards) = native_lto_probe_guards("clang") else {
            return; // host cannot run the probe; the builder-tests CI lane covers it
        };

        assert_eq!(
            asm_debug_prefix_map_flag(Path::new("/tmp/aws-lc-sys")),
            None,
            "clang must not receive the GNU-only -Wa,--debug-prefix-map flag"
        );
    }

    // Positive-path companion to the test above: real GCC keeps the flag under the
    // same user LTO CFLAGS, pinning that clang's `None` comes from gating rather
    // than the helper never returning `Some`. GCC rejects `-flto=thin`, so this
    // also fails if the probe stops suppressing CFLAGS.
    #[test]
    fn test_asm_debug_prefix_map_flag_granted_to_gcc_under_lto() {
        use std::path::Path;

        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let Some(_guards) = native_lto_probe_guards("gcc") else {
            return; // host cannot run the probe; the builder-tests CI lane covers it
        };
        // Skip hosts where `gcc` is absent or a clang shim (e.g. BSDs).
        {
            let _cflags_guards = cflags_ignore_guards();
            match cc::Build::default().try_get_compiler() {
                Ok(compiler) if compiler.is_like_gnu() => {}
                _ => return,
            }
        }

        let manifest_dir = Path::new("/tmp/aws-lc-sys");
        assert_eq!(
            asm_debug_prefix_map_flag(manifest_dir),
            Some(format!(
                "-Wa,--debug-prefix-map={}=",
                manifest_dir.display()
            )),
            "GCC must keep the assembler flag when user CFLAGS enables LTO"
        );
    }

    // Guards https://github.com/aws/aws-lc-rs/issues/1205: an optimization flag in ANY
    // CFLAGS variant cc reads (including the raw-triple spelling, settable via setenv)
    // overrides jitterentropy's required -O0, so every variant must be filtered.
    #[test]
    fn test_jitter_entropy_cflags_guards_filter_every_cc_variant() {
        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let _target_guard = EnvGuard::new("TARGET", "thumbv8m.main-none-eabi");
        let _g1 = EnvGuard::new("CFLAGS_thumbv8m.main-none-eabi", "-O3 -fraw");
        let _g2 = EnvGuard::new("CFLAGS_thumbv8m.main_none_eabi", "-O2 -flegacy");
        let _g3 = EnvGuard::new("CFLAGS_thumbv8m_main_none_eabi", "-Oz -fcurrent");
        let _g4 = EnvGuard::new("HOST_CFLAGS", "-Ofast -fhost");
        let _g5 = EnvGuard::new("TARGET_CFLAGS", "-Os -ftarget");
        let _g6 = EnvGuard::new("CFLAGS", "-O1 -fbase");
        {
            let _guards = CcBuilder::jitter_entropy_cflags_guards(false);
            for name in [
                "CFLAGS_thumbv8m.main-none-eabi",
                "CFLAGS_thumbv8m.main_none_eabi",
                "CFLAGS_thumbv8m_main_none_eabi",
                "HOST_CFLAGS",
                "TARGET_CFLAGS",
                "CFLAGS",
            ] {
                let value = env::var(name).unwrap();
                assert!(
                    !value
                        .split_whitespace()
                        .any(|f| f.starts_with("-O") && f != "-O0"),
                    "{name} still carries an optimization flag: {value}"
                );
                assert!(
                    value.split_whitespace().any(|f| f == "-O0"),
                    "{name} must force -O0: {value}"
                );
            }
            // Non-optimization flags are preserved.
            assert!(env::var("CFLAGS_thumbv8m.main-none-eabi")
                .unwrap()
                .contains("-fraw"));
        }
        // Every guarded variable is restored on drop.
        assert_eq!(
            env::var("CFLAGS_thumbv8m.main-none-eabi").unwrap(),
            "-O3 -fraw"
        );
        assert_eq!(
            env::var("CFLAGS_thumbv8m.main_none_eabi").unwrap(),
            "-O2 -flegacy"
        );
        assert_eq!(
            env::var("CFLAGS_thumbv8m_main_none_eabi").unwrap(),
            "-Oz -fcurrent"
        );
        assert_eq!(env::var("HOST_CFLAGS").unwrap(), "-Ofast -fhost");
        assert_eq!(env::var("TARGET_CFLAGS").unwrap(), "-Os -ftarget");
        assert_eq!(env::var("CFLAGS").unwrap(), "-O1 -fbase");
    }

    // Guards #1252 and #1064: user CFLAGS optimization must not reach the jitter
    // compile. GNU-mode compilers only, since `__OPTIMIZE__` is GNU/clang-only.
    #[test]
    fn test_jitter_entropy_compiles_unoptimized_despite_user_cflags() {
        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let Some(host) = host_triple() else {
            return;
        };
        let parts: Vec<&str> = host.split('-').collect();
        let target_env = ["msvc", "gnu", "musl"]
            .into_iter()
            .find(|env| host.ends_with(env))
            .unwrap_or("");
        let temp_out = tempfile::tempdir().unwrap();
        let manifest_dir = tempfile::tempdir().unwrap();
        let target_u = host.replace(['-', '.'], "_");
        let _guards = [
            EnvGuard::new("OUT_DIR", temp_out.path()),
            EnvGuard::new("TARGET", &host),
            EnvGuard::new("HOST", &host),
            EnvGuard::new("CARGO_CFG_TARGET_OS", std::env::consts::OS),
            EnvGuard::new("CARGO_CFG_TARGET_ARCH", std::env::consts::ARCH),
            EnvGuard::new("CARGO_CFG_TARGET_VENDOR", parts[1]),
            EnvGuard::new("CARGO_CFG_TARGET_ENV", target_env),
            EnvGuard::new("OPT_LEVEL", "0"),
            EnvGuard::new("DEBUG", "false"),
            EnvGuard::new("CFLAGS", "-O2"),
            EnvGuard::new(format!("CFLAGS_{target_u}").as_str(), "-O2"),
        ];
        {
            let _cflags_guards = cflags_ignore_guards();
            match cc::Build::new().try_get_compiler() {
                Ok(compiler) if !compiler_is_cl_like(&compiler) => {}
                _ => return,
            }
        }

        let header_dir = manifest_dir
            .path()
            .join("generated-include")
            .join("openssl");
        std::fs::create_dir_all(&header_dir).unwrap();
        std::fs::write(header_dir.join("boringssl_prefix_symbols.h"), "").unwrap();
        let source = manifest_dir.path().join("jitter_opt_check.c");
        std::fs::write(
            &source,
            "#ifdef __OPTIMIZE__\n#error \"jitterentropy compiled with optimizations\"\n#endif\nint jitter_opt_check(void) { return 0; }\n",
        )
        .unwrap();

        let builder = CcBuilder::new(
            manifest_dir.path().to_path_buf(),
            temp_out.path().to_path_buf(),
            None,
            OutputLibType::Static,
        );
        let result = builder.compile_jitter_entropy(false, &[source]);
        assert_eq!(
            env::var("CFLAGS").unwrap(),
            "-O2",
            "CFLAGS must be restored"
        );
        let objects = result.unwrap_or_else(|e| {
            panic!("user CFLAGS optimization reached the jitterentropy compile: {e}")
        });
        assert_eq!(objects.len(), 1);
    }

    // `AWS_LC_SYS_CC` must reach the main library build, not only jitterentropy's
    // separate `cc::Build`. The override is a symlink to the default compiler named
    // `cc` (the macOS xcrun shim dispatches on its name), so it still works but has
    // a distinct path. Only cc releases that cache env reads per `Build` (e.g.
    // 1.2.41, 1.6.0) can detect a late export.
    #[cfg(unix)]
    #[test]
    fn test_crate_cc_reaches_main_builder() {
        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let Some(host) = host_triple() else {
            return;
        };
        let parts: Vec<&str> = host.split('-').collect();
        let target_env = ["gnu", "musl"]
            .into_iter()
            .find(|env| host.ends_with(env))
            .unwrap_or("");
        let temp_out = tempfile::tempdir().unwrap();
        let manifest_dir = tempfile::tempdir().unwrap();
        let target_u = host.to_lowercase().replace('-', "_");
        let mut guards = vec![
            EnvGuard::new("OUT_DIR", temp_out.path()),
            EnvGuard::new("TARGET", &host),
            EnvGuard::new("HOST", &host),
            EnvGuard::new("CARGO_PKG_NAME", "aws-lc-sys"),
            EnvGuard::new("CARGO_CFG_TARGET_OS", std::env::consts::OS),
            EnvGuard::new("CARGO_CFG_TARGET_ARCH", std::env::consts::ARCH),
            EnvGuard::new("CARGO_CFG_TARGET_VENDOR", parts[1]),
            EnvGuard::new("CARGO_CFG_TARGET_ENV", target_env),
            EnvGuard::new("OPT_LEVEL", "0"),
            EnvGuard::new("DEBUG", "false"),
        ];
        // Isolate both cc-rs and crate-specific compiler overrides. Guard CFLAGS
        // too: prepare_builder can export it into the target-specific environment.
        for name in ["CC", "HOST_CC", "CXX", "HOST_CXX", "CFLAGS", "HOST_CFLAGS"] {
            for prefix in ["", "AWS_LC_SYS_"] {
                let name = format!("{prefix}{name}");
                for key in [
                    name.clone(),
                    format!("{name}_{host}"),
                    format!("{name}_{target_u}"),
                ] {
                    guards.push(EnvGuard::remove(&key));
                }
            }
        }
        let Ok(default_cc) = cc::Build::new().try_get_compiler() else {
            return;
        };
        let Some(default_cc_path) = env::var_os("PATH").and_then(|path| {
            env::split_paths(&path)
                .map(|dir| dir.join(default_cc.path()))
                .find(|candidate| candidate.is_file())
        }) else {
            return;
        };
        let crate_cc = manifest_dir.path().join("cc");
        std::os::unix::fs::symlink(default_cc_path, &crate_cc).unwrap();
        // Drop the override before restoring any inherited AWS_LC_SYS_CC.
        let _crate_cc_guard = EnvGuard::new("AWS_LC_SYS_CC", &crate_cc);

        let builder = CcBuilder::new(
            manifest_dir.path().to_path_buf(),
            temp_out.path().to_path_buf(),
            None,
            OutputLibType::Static,
        );
        let cc_build = builder.prepare_builder();
        assert_eq!(
            cc_build.get_compiler().path(),
            crate_cc,
            "AWS_LC_SYS_CC must select the main library's compiler"
        );
    }

    // cc 1.2.26 replaces only `-` in `target_envs`, while newer versions replace
    // both `-` and `.`; a dotted triple pins that we cover both supported forms.
    #[test]
    fn test_cflags_env_names_match_cc_target_envs() {
        let _lock = ENV_MUTEX.lock().unwrap_or_else(|e| e.into_inner());
        let _target_guard = EnvGuard::new("TARGET", "thumbv8m.main-none-eabi");
        let names = cflags_env_names(true);
        assert_eq!(
            names,
            vec![
                "CFLAGS_thumbv8m.main-none-eabi",
                "CFLAGS_thumbv8m.main_none_eabi",
                "CFLAGS_thumbv8m_main_none_eabi",
                "HOST_CFLAGS",
                "TARGET_CFLAGS",
                "CFLAGS",
            ]
        );
        assert!(!cflags_env_names(false).contains(&"TARGET_CFLAGS".to_string()));
    }

    // Guards https://github.com/aws/aws-lc-rs/issues/1146: in cl driver mode
    // the GNU-only `--param ssp-buffer-size=4` pair (rejected by `clang-cl`)
    // must never be selected.
    #[test]
    fn test_jitter_entropy_flags_cl_dialect_omit_gnu_only() {
        let cl = CcBuilder::jitter_entropy_dialect_flags(true);
        assert!(
            !cl.contains(&"--param") && !cl.contains(&"ssp-buffer-size=4"),
            "cl-mode jitter flags must not contain the GNU-only ssp-buffer-size pair: {cl:?}"
        );
        assert!(cl.contains(&"-Od"), "expected cl-style -Od: {cl:?}");
    }

    // Guards the inverse of #1146: in GNU driver mode (e.g. plain `clang`, even
    // on a windows-msvc target) the cl-only `-Od`/`-W4` flags must not be
    // selected and the GNU hardening flags must be preserved.
    #[test]
    fn test_jitter_entropy_gnu_dialect_keeps_hardening_flags() {
        let gnu = CcBuilder::jitter_entropy_dialect_flags(false);
        assert!(gnu.contains(&"--param"));
        assert!(gnu.contains(&"ssp-buffer-size=4"));
        assert!(
            !gnu.contains(&"-Od"),
            "GNU dialect must not use cl-style -Od: {gnu:?}"
        );
        // jitterentropy must always be built unoptimized.
        assert!(gnu.contains(&"-O0"), "expected -O0: {gnu:?}");
    }

    // Driver mode is selected by the compiler program name (argv[0]); this is
    // the robust fallback when `cc`'s family probe misfires. `clang-cl`/`cl`
    // are cl mode; plain `clang`/`gcc` are not -- guarding both #1146 and its
    // inverse (plain clang on a windows-msvc target).
    #[test]
    fn test_program_name_is_cl_driver() {
        use crate::program_name_is_cl_driver;
        use std::path::Path;
        for cl in ["clang-cl", "clang-cl.exe", "CLANG-CL.EXE", "cl", "cl.exe"] {
            assert!(
                program_name_is_cl_driver(Path::new(cl)),
                "{cl} should be detected as cl driver mode"
            );
        }
        for gnu in ["clang", "clang.exe", "clang-18", "gcc", "cc"] {
            assert!(
                !program_name_is_cl_driver(Path::new(gnu)),
                "{gnu} should not be detected as cl driver mode"
            );
        }
    }

    #[test]
    fn test_small_c_build_options() {
        assert_eq!(
            CcBuilder::small_c_build_options(),
            [BuildOption::DEFINE(
                "OPENSSL_SMALL".to_string(),
                "1".to_string()
            )]
        );
    }

    #[test]
    fn test_small_nasm_build_options() {
        assert_eq!(
            CcBuilder::small_nasm_build_options(),
            [
                BuildOption::DEFINE("OPENSSL_SMALL".to_string(), "1".to_string()),
                BuildOption::DEFINE(
                    "MY_ASSEMBLER_IS_TOO_OLD_FOR_512AVX".to_string(),
                    "1".to_string(),
                ),
            ]
        );
    }
}
