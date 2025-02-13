use std::ffi::OsString;
use std::fs::{self, File};
use std::io::BufWriter;
use std::io::Write;
use std::path::PathBuf;
use tempfile::Builder;
use wasm_opt::integration::Command;
use wasm_opt::*;

static WASM_FILE: &[u8] = include_bytes!("hello_world.wasm");
static MULTISIG_WASM: &[u8] = include_bytes!("ink_example_multisig.wasm");

#[test]
fn pass_arg_works() -> anyhow::Result<()> {
    let temp_dir = Builder::new().prefix("wasm_opt_tests").tempdir()?;
    let inpath = temp_dir.path().join("infile.wasm");

    let infile = File::create(&inpath)?;

    let mut buf_writer = BufWriter::new(&infile);
    buf_writer.write_all(MULTISIG_WASM)?;

    let outfile = temp_dir.path().join("outfile.wasm");

    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR")?;
    let manifest_dir = PathBuf::from(manifest_dir);
    let workspace = manifest_dir.join("../..");
    let rust_wasm_opt_dir = workspace.join("target/release/wasm-opt");

    let mut cmd = Command::new(rust_wasm_opt_dir);
    cmd.arg(&inpath);
    cmd.arg("--output");
    cmd.arg(outfile.to_str().expect("PathBuf"));

    cmd.arg("--extract-function");
    cmd.arg("--pass-arg");
    cmd.arg("extract-function@rust_begin_unwind");

    integration::run_from_command_args(cmd)?;

    let infile_reader = fs::read(inpath)?;
    let outfile_reader = fs::read(outfile)?;

    assert!(infile_reader.len() >= outfile_reader.len());

    Ok(())
}

#[test]
fn pass_arg_unsupported_works() -> anyhow::Result<()> {
    let temp_dir = Builder::new().prefix("wasm_opt_tests").tempdir()?;
    let inpath = temp_dir.path().join("infile.wasm");

    let infile = File::create(&inpath)?;

    let mut buf_writer = BufWriter::new(&infile);
    buf_writer.write_all(WASM_FILE)?;

    let outfile = temp_dir.path().join("outfile.wasm");

    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR")?;
    let manifest_dir = PathBuf::from(manifest_dir);
    let workspace = manifest_dir.join("../..");
    let rust_wasm_opt_dir = workspace.join("target/release/wasm-opt");

    let mut cmd = Command::new(rust_wasm_opt_dir);
    cmd.arg(&inpath);
    cmd.arg("--output");
    cmd.arg(outfile.to_str().expect("PathBuf"));

    cmd.arg("-p");
    cmd.arg("--whatever");
    cmd.arg("--no-validation");

    let res = integration::run_from_command_args(cmd);

    assert!(res.is_err());

    if let Some(err) = res.err() {
        match err {
            integration::Error::Unsupported { args } => {
                if args.contains(&OsString::from("--no-validation")) {
                    panic!();
                }

                if !(args.contains(&OsString::from("-p"))
                    && args.contains(&OsString::from("--whatever")))
                {
                    panic!();
                }
            }
            _ => panic!(),
        }
    }

    Ok(())
}
