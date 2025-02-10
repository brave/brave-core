use cxx_build::CFG;
use std::fmt::Write as FmtWrite;
use std::fs::{self, File};
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::path::{Path, PathBuf};

fn main() -> anyhow::Result<()> {
    check_cxx17_support()?;

    let output_dir = std::env::var("OUT_DIR")?;
    let output_dir = Path::new(&output_dir);

    let binaryen_dir = get_binaryen_dir()?;

    let src_dir = binaryen_dir.join("src");
    let src_files = get_src_files(&src_dir)?;

    #[cfg(feature = "dwarf")]
    let llvm_dir = binaryen_dir.join("third_party/llvm-project");
    #[cfg(feature = "dwarf")]
    let llvm_files = get_llvm_files(&llvm_dir)?;

    let tools_dir = src_dir.join("tools");
    let wasm_opt_src = tools_dir.join("wasm-opt.cpp");
    let wasm_opt_src = get_converted_wasm_opt_cpp(&wasm_opt_src)?;

    let wasm_intrinsics_src = get_converted_wasm_intrinsics_cpp(&src_dir)?;

    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR")?;
    let manifest_dir = Path::new(&manifest_dir);
    let wasm_opt_main_shim = manifest_dir.join("src/wasm-opt-main-shim.cpp");

    create_config_header()?;

    // Set up cxx's include path so that wasm-opt-cxx-sys's C++ header can
    // include from these same dirs.
    CFG.exported_header_dirs.push(&src_dir);
    CFG.exported_header_dirs.push(&tools_dir);
    CFG.exported_header_dirs.push(&output_dir);

    #[cfg(feature = "dwarf")]
    {
        let llvm_include = llvm_dir.join("include");
        CFG.exported_header_dirs.push(&llvm_include);
    }

    let mut builder = cxx_build::bridge("src/lib.rs");

    {
        let target_env = std::env::var("CARGO_CFG_TARGET_ENV")?;

        let flags: &[_] = if target_env != "msvc" {
            &[
                "-std=c++17",
                "-w",
                "-Wno-unused-parameter",
                "-DTHROW_ON_FATAL",
                #[cfg(feature = "dwarf")]
                "-DBUILD_LLVM_DWARF",
                "-DNDEBUG",
            ]
        } else {
            &[
                "/std:c++17",
                "/w",
                "/DTHROW_ON_FATAL",
                #[cfg(feature = "dwarf")]
                "/DBUILD_LLVM_DWARF",
                "/DNDEBUG",
            ]
        };

        for flag in flags {
            builder.flag(flag);
        }
    }

    builder
        .file(wasm_opt_main_shim)
        .files(src_files)
        .file(wasm_opt_src)
        .file(wasm_intrinsics_src);

    #[cfg(feature = "dwarf")]
    builder.files(&llvm_files);

    builder.compile("wasm-opt-cc");

    Ok(())
}

/// Finds the binaryen source directory.
///
/// During development this will be at the workspace level submodule,
/// but as packaged, will be a subdirectory of the manifest directory.
///
/// The packaged subdirectories are put in place by `publish.sh`.
///
/// The packaged source is pre-processed to remove Binaryen's large test suite.
fn get_binaryen_dir() -> anyhow::Result<PathBuf> {
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR")?;
    let manifest_dir = Path::new(&manifest_dir);
    let binaryen_packaged_dir = manifest_dir.join("binaryen");
    let binaryen_submodule_dir = manifest_dir.join("../../binaryen");

    match (
        binaryen_packaged_dir.is_dir(),
        binaryen_submodule_dir.is_dir(),
    ) {
        (true, _) => Ok(binaryen_packaged_dir),
        (_, true) => Ok(binaryen_submodule_dir),
        (false, false) => anyhow::bail!(
            "binaryen source directory doesn't exist (maybe `git submodule update --init`?)"
        ),
    }
}

/// Replaces the `main` declaration with a C ABI and a different name.
///
/// It can be called from Rust and doesn't clash with Rust's `main`.
fn get_converted_wasm_opt_cpp(src_dir: &Path) -> anyhow::Result<PathBuf> {
    let wasm_opt_file = File::open(src_dir)?;
    let reader = BufReader::new(wasm_opt_file);

    let output_dir = std::env::var("OUT_DIR")?;
    let output_dir = Path::new(&output_dir);

    let temp_file_dir = output_dir.join("wasm_opt.cpp.temp");
    let temp_file = File::create(&temp_file_dir)?;

    let mut writer = BufWriter::new(temp_file);
    for line in reader.lines() {
        let mut line = line?;

        if line.contains("int main") {
            line = line.replace("int main", "extern \"C\" int wasm_opt_main_actual");
        }

        writer.write_all(line.as_bytes())?;
        writer.write_all(b"\n")?;
    }

    let output_wasm_opt_file = output_dir.join("wasm-opt.cpp");
    fs::rename(&temp_file_dir, &output_wasm_opt_file)?;

    Ok(output_wasm_opt_file)
}

fn get_src_files(src_dir: &Path) -> anyhow::Result<Vec<PathBuf>> {
    let analysis_dir = src_dir.join("analysis");
    let analysis_files = ["cfg.cpp"];
    let analysis_files = analysis_files.iter().map(|f| analysis_dir.join(f));

    let wasm_dir = src_dir.join("wasm");
    let wasm_files = [
        "literal.cpp",
        "parsing.cpp",
        "wasm-binary.cpp",
        "wasm-debug.cpp",
        "wasm-emscripten.cpp",
        "wasm-interpreter.cpp",
        "wasm-ir-builder.cpp",
        "wasm-io.cpp",
        "wasm-stack.cpp",
        "wasm-s-parser.cpp",
        "wasm-type.cpp",
        "wasm-validator.cpp",
        "wasm.cpp",
        "wat-lexer.cpp",
        "wat-parser.cpp",
    ];
    let wasm_files = wasm_files.iter().map(|f| wasm_dir.join(f));

    let support_dir = src_dir.join("support");
    let support_files = [
        "bits.cpp",
        "colors.cpp",
        "command-line.cpp",
        "debug.cpp",
        "dfa_minimization.cpp",
        "file.cpp",
        "safe_integer.cpp",
        "threads.cpp",
        "utilities.cpp",
        "istring.cpp",
    ];
    let support_files = support_files.iter().map(|f| support_dir.join(f));

    let ir_dir = src_dir.join("ir");
    let ir_files = [
        "drop.cpp",
        "eh-utils.cpp",
        "ExpressionManipulator.cpp",
        "ExpressionAnalyzer.cpp",
        "export-utils.cpp",
        "LocalGraph.cpp",
        "LocalStructuralDominance.cpp",
        "lubs.cpp",
        "memory-utils.cpp",
        "module-utils.cpp",
        "names.cpp",
        "possible-contents.cpp",
        "properties.cpp",
        "ReFinalize.cpp",
        "stack-utils.cpp",
        "table-utils.cpp",
        "type-updating.cpp",
    ];
    let ir_files = ir_files.iter().map(|f| ir_dir.join(f));

    let passes_dir = src_dir.join("passes");
    let passes_files = get_files_from_dir(&passes_dir)?;

    let fuzzing_dir = src_dir.join("tools/fuzzing");
    let fuzzing_files = ["fuzzing.cpp", "random.cpp", "heap-types.cpp"];
    let fuzzing_files = fuzzing_files.iter().map(|f| fuzzing_dir.join(f));

    let asmjs_dir = src_dir.join("asmjs");
    let asmjs_files = ["asm_v_wasm.cpp", "shared-constants.cpp"];
    let asmjs_files = asmjs_files.iter().map(|f| asmjs_dir.join(f));

    let cfg_dir = src_dir.join("cfg");
    let cfg_files = ["Relooper.cpp"];
    let cfg_files = cfg_files.iter().map(|f| cfg_dir.join(f));

    let file_intrinsics = disambiguate_file(&ir_dir.join("intrinsics.cpp"), "intrinsics-ir.cpp")?;

    let src_files: Vec<_> = None
        .into_iter()
        .chain(analysis_files)
        .chain(wasm_files)
        .chain(support_files)
        .chain(ir_files)
        .chain(passes_files)
        .chain(fuzzing_files)
        .chain(asmjs_files)
        .chain(cfg_files)
        .chain(Some(file_intrinsics).into_iter())
        .collect();

    Ok(src_files)
}

fn get_files_from_dir(src_dir: &Path) -> anyhow::Result<impl Iterator<Item = PathBuf> + '_> {
    let files = fs::read_dir(src_dir)?
        .map(|f| f.expect("error reading dir"))
        .filter(|f| f.file_name().into_string().expect("UTF8").ends_with(".cpp"))
        .map(move |f| src_dir.join(f.path()));

    Ok(files)
}

fn disambiguate_file(input_file: &Path, new_file_name: &str) -> anyhow::Result<PathBuf> {
    let output_dir = std::env::var("OUT_DIR")?;
    let output_dir = Path::new(&output_dir);
    let output_file = output_dir.join(new_file_name);

    fs::copy(input_file, &output_file)?;

    Ok(output_file)
}

/// Pre-process the WasmIntrinsics.cpp.in file and return a path to the processed file.
///
/// This file needs to be injected with the contents of wasm-intrinsics.wat,
/// replacing `@WASM_INTRINSICS_SIZE@` with the size of the wat + 1,
/// and `@WASM_INTRINSICS_EMBED@` with the hex-encoded contents of the wat,
/// appended with `0x00`.
///
/// The extra byte is presumably a null terminator.
fn get_converted_wasm_intrinsics_cpp(src_dir: &Path) -> anyhow::Result<PathBuf> {
    let src_passes_dir = src_dir.join("passes");

    let output_dir = std::env::var("OUT_DIR")?;
    let output_dir = Path::new(&output_dir);

    let wasm_intrinsics_cpp_in_file = src_passes_dir.join("WasmIntrinsics.cpp.in");
    let wasm_intrinsics_cpp_out_file = output_dir.join("WasmIntrinsics.cpp");

    let (wasm_intrinsics_wat_hex, wasm_intrinsics_wat_bytes) =
        load_wasm_intrinsics_wat(&src_passes_dir)?;

    configure_file(
        &wasm_intrinsics_cpp_in_file,
        &wasm_intrinsics_cpp_out_file,
        &[
            (
                "WASM_INTRINSICS_SIZE",
                format!("{}", wasm_intrinsics_wat_bytes),
            ),
            ("WASM_INTRINSICS_EMBED", wasm_intrinsics_wat_hex),
        ],
    )?;

    Ok(wasm_intrinsics_cpp_out_file)
}

fn load_wasm_intrinsics_wat(passes_dir: &Path) -> anyhow::Result<(String, usize)> {
    let wasm_intrinsics_wat = passes_dir.join("wasm-intrinsics.wat");
    let wat_contents = std::fs::read_to_string(&wasm_intrinsics_wat)?;

    let mut buffer = String::with_capacity(wat_contents.len() * 5 /* 0xNN, */ + 4 /* null */);

    for byte in wat_contents.bytes() {
        write!(buffer, "0x{:02x},", byte)?;
    }
    write!(buffer, "0x00")?;

    Ok((buffer, wat_contents.len() + 1))
}

/// A rough implementation of CMake's `configure_file` directive.
///
/// Consume `src_file` and output `dst_file`.
///
/// `replacements` is a list of key-value pairs from variable name
/// to a textual substitute for that variable.
///
/// Any variables in the source file, surrounded by `@`, e.g.
/// `@WASM_INTRINSICS_SIZE@`, will be replaced with the specified value. The
/// variable as specified in the `replacements` list does not include the `@`
/// symbols.
///
/// re: <https://cmake.org/cmake/help/latest/command/configure_file.html>
fn configure_file(
    src_file: &Path,
    dst_file: &Path,
    replacements: &[(&str, String)],
) -> anyhow::Result<()> {
    let mut src = std::fs::read_to_string(src_file)?;

    for (var, txt) in replacements {
        let var = format!("@{}@", var);
        src = src.replace(&var, txt);
    }

    std::fs::write(dst_file, src)?;

    Ok(())
}

fn create_config_header() -> anyhow::Result<()> {
    let output_dir = std::env::var("OUT_DIR")?;
    let output_dir = Path::new(&output_dir);
    let config_file = output_dir.join("config.h");

    let config_text = "#define PROJECT_VERSION \"116 (version_116)\"";

    fs::write(&config_file, config_text)?;

    Ok(())
}

fn check_cxx17_support() -> anyhow::Result<()> {
    let mut builder = cc::Build::new();
    builder.cpp(true);

    let target_env = std::env::var("CARGO_CFG_TARGET_ENV")?;
    let cxx17_flag = if target_env != "msvc" {
        "-std=c++17"
    } else {
        "/std:c++17"
    };

    if !builder.is_flag_supported(cxx17_flag)? {
        return Err(anyhow::anyhow!(
            "C++ compiler does not support `{}` flag",
            cxx17_flag
        ));
    }

    Ok(())
}

#[cfg(feature = "dwarf")]
fn get_llvm_files(llvm_dir: &Path) -> anyhow::Result<[PathBuf; 63]> {
    let llvm_dwarf = disambiguate_file(&llvm_dir.join("Dwarf.cpp"), "LLVMDwarf.cpp")?;
    let llvm_debug = disambiguate_file(&llvm_dir.join("Debug.cpp"), "LLVMDebug.cpp")?;
    // Array taken from
    // https://github.com/WebAssembly/binaryen/blob/616c08bc1ec604a822d354cbb0353b7994cec72d/third_party/llvm-project/CMakeLists.txt
    Ok([
        llvm_dir.join("Binary.cpp"),
        llvm_dir.join("ConvertUTF.cpp"),
        llvm_dir.join("DataExtractor.cpp"),
        llvm_dir.join("DJB.cpp"),
        llvm_debug,
        llvm_dir.join("dwarf2yaml.cpp"),
        llvm_dir.join("DWARFAbbreviationDeclaration.cpp"),
        llvm_dir.join("DWARFAcceleratorTable.cpp"),
        llvm_dir.join("DWARFAddressRange.cpp"),
        llvm_dir.join("DWARFCompileUnit.cpp"),
        llvm_dir.join("DWARFContext.cpp"),
        llvm_dir.join("DWARFDataExtractor.cpp"),
        llvm_dir.join("DWARFDebugAbbrev.cpp"),
        llvm_dir.join("DWARFDebugAddr.cpp"),
        llvm_dir.join("DWARFDebugAranges.cpp"),
        llvm_dir.join("DWARFDebugArangeSet.cpp"),
        llvm_dir.join("DWARFDebugFrame.cpp"),
        llvm_dir.join("DWARFDebugInfoEntry.cpp"),
        llvm_dir.join("DWARFDebugLine.cpp"),
        llvm_dir.join("DWARFDebugLoc.cpp"),
        llvm_dir.join("DWARFDebugMacro.cpp"),
        llvm_dir.join("DWARFDebugPubTable.cpp"),
        llvm_dir.join("DWARFDebugRangeList.cpp"),
        llvm_dir.join("DWARFDebugRnglists.cpp"),
        llvm_dir.join("DWARFDie.cpp"),
        llvm_dir.join("DWARFEmitter.cpp"),
        llvm_dir.join("DWARFExpression.cpp"),
        llvm_dir.join("DWARFFormValue.cpp"),
        llvm_dir.join("DWARFGdbIndex.cpp"),
        llvm_dir.join("DWARFListTable.cpp"),
        llvm_dir.join("DWARFTypeUnit.cpp"),
        llvm_dir.join("DWARFUnit.cpp"),
        llvm_dir.join("DWARFUnitIndex.cpp"),
        llvm_dir.join("DWARFVerifier.cpp"),
        llvm_dir.join("DWARFVisitor.cpp"),
        llvm_dir.join("DWARFYAML.cpp"),
        llvm_dir.join("Error.cpp"),
        llvm_dir.join("ErrorHandling.cpp"),
        llvm_dir.join("FormatVariadic.cpp"),
        llvm_dir.join("Hashing.cpp"),
        llvm_dir.join("LEB128.cpp"),
        llvm_dir.join("LineIterator.cpp"),
        llvm_dir.join("MCRegisterInfo.cpp"),
        llvm_dir.join("MD5.cpp"),
        llvm_dir.join("MemoryBuffer.cpp"),
        llvm_dir.join("NativeFormatting.cpp"),
        llvm_dir.join("ObjectFile.cpp"),
        llvm_dir.join("obj2yaml_Error.cpp"),
        llvm_dir.join("Optional.cpp"),
        llvm_dir.join("Path.cpp"),
        llvm_dir.join("raw_ostream.cpp"),
        llvm_dir.join("ScopedPrinter.cpp"),
        llvm_dir.join("SmallVector.cpp"),
        llvm_dir.join("SourceMgr.cpp"),
        llvm_dir.join("StringMap.cpp"),
        llvm_dir.join("StringRef.cpp"),
        llvm_dir.join("SymbolicFile.cpp"),
        llvm_dir.join("Twine.cpp"),
        llvm_dir.join("UnicodeCaseFold.cpp"),
        llvm_dir.join("WithColor.cpp"),
        llvm_dir.join("YAMLParser.cpp"),
        llvm_dir.join("YAMLTraits.cpp"),
        llvm_dwarf,
    ])
}
