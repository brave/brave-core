//! Easy integration with tools that already use `wasm-opt` via CLI.
//!
//! The [`run_from_command_args`] function interprets the arguments to
//! a [`Command`], typically used for executing a subprocess, to construct
//! an [`OptimizationOptions`], then runs the optimizer.
//!
//! Note that integrators must used the provided `Command` type, _not_
//! `std::process::Command`. The provided type is a thin wrapper around the
//! standard type that is needed for backwards compatibility with older versions
//! of Rust.

use crate::api::{Feature, FileType, OptimizationOptions, OptimizeLevel, Pass, ShrinkLevel};
use crate::profiles::Profile;
use crate::run::OptimizationError;
use std::ffi::{OsStr, OsString};
use std::iter::Iterator;
use std::num::ParseIntError;
use std::path::PathBuf;
use std::result::Result;
use std::str::FromStr;
use strum::IntoEnumIterator;
use thiserror::Error;

pub use crate::fake_command::Command;

/// Interpret a pre-built [`Command`] as an [`OptimizationOptions`],
/// then call [`OptimizationOptions::run`] on it.
///
/// This function is meant for easy integration with tools that already
/// call `wasm-opt` via the command line, allowing them to use either
/// the command-line tool or the integrated API from a single `Command` builder.
/// New programs that just need to optimize wasm should use `OptimizationOptions` directly.
///
/// This function is provided on a best-effort basis to support programs
/// trying to integrate with the crate.
/// In general, it should support any command line options that are also supported
/// by the `OptimizationOptions` API,
/// but it may not parse &mdash; and in some cases may not interpret &mdash;
/// those commands in exactly the same way.
/// It is meant to make it _possible_ to produce a single command-line that works
/// with both the CLI and the API,
/// not to reproduce the behavior of the CLI perfectly.
///
/// The `-o` argument is required, followed by a path &mdash;
/// the `wasm-opt` tool writes the optimized module to stdout by default,
/// but this library is not currently capable of that.
/// `-o` specifies a file in which to write the module.
/// If `-o` is not provided, [`Error::OutputFileRequired`] is returned.
///
/// Only the arguments to `command` are interpreted;
/// environment variables and other settings are ignored.
///
/// # Errors
///
/// - Returns [`Error::Unsupported`] if any argument is not understood.
/// - Returns [`Error::OutputFileRequired`] if the `-o` argument and subsequent path
///   are not provided.
pub fn run_from_command_args(command: Command) -> Result<(), Error> {
    let parsed = parse_command_args(command)?;

    parsed.opts.run_with_sourcemaps(
        parsed.input_file,
        parsed.input_sourcemap,
        parsed.output_file,
        parsed.output_sourcemap,
        parsed.sourcemap_url,
    )?;

    Ok(())
}

/// An error resulting from [`run_from_command_args`].
#[derive(Error, Debug)]
pub enum Error {
    /// No input file specified.
    #[error("An input file is required")]
    InputFileRequired,
    /// No output file specified.
    #[error("The `-o` option to `wasm-opt` is required")]
    OutputFileRequired,
    /// Expected another argument.
    #[error("The `wasm-opt` argument list ended while expecting another argument")]
    UnexpectedEndOfArgs,
    /// Expected to parse unicode.
    #[error("Argument must be unicode: {arg:?}")]
    NeedUnicode { arg: OsString },
    /// Expected to parse a number.
    #[error("Argument must be a number: {arg:?}")]
    NeedNumber {
        arg: OsString,
        #[source]
        source: ParseIntError,
    },
    /// Unsupported or unrecognized command-line option.
    #[error("Unsupported `wasm-opt` command-line arguments: {args:?}")]
    Unsupported { args: Vec<OsString> },
    /// An error occurred while executing [`OptimizationOptions::run`].
    #[error("Error while optimization wasm modules")]
    ExecutionError(
        #[from]
        #[source]
        OptimizationError,
    ),
}

struct ParsedCliArgs {
    opts: OptimizationOptions,
    input_file: PathBuf,
    input_sourcemap: Option<PathBuf>,
    output_file: PathBuf,
    output_sourcemap: Option<PathBuf>,
    sourcemap_url: Option<String>,
}

#[rustfmt::skip]
fn parse_command_args(command: Command) -> Result<ParsedCliArgs, Error> {
    let mut opts = OptimizationOptions::new_opt_level_0();

    let mut args = command.get_args();

    let mut input_file: Option<PathBuf> = None;
    let mut input_sourcemap: Option<PathBuf> = None;
    let mut output_file: Option<PathBuf> = None;
    let mut output_sourcemap: Option<PathBuf> = None;
    let mut sourcemap_url: Option<String> = None;

    let mut unsupported: Vec<OsString> = vec![];

    while let Some(arg) = args.next() {
        let arg = if let Some(arg) = arg.to_str() {
            arg
        } else {
            // Not unicode. Might still be the infile.
            parse_infile_path(arg, &mut input_file, &mut unsupported);
            continue;
        };

        // Keep these cases in order they are listed in the original cpp files.
        match arg {
            /* from wasm-opt.cpp */

            "--output" | "-o" => {
                parse_path_into(&mut args, &mut output_file, &mut unsupported)?;
            }
            "--emit-text" | "-S" => {
                opts.writer_file_type(FileType::Wat);
            }
            "--converge" | "-c" => {
                opts.set_converge();
            }
            "--input-source-map" | "-ism" => {
                parse_path_into(&mut args, &mut input_sourcemap, &mut unsupported)?;
            }
            "--output-source-map" | "-osm" => {
                parse_path_into(&mut args, &mut output_sourcemap, &mut unsupported)?;
            }
            "--output-source-map-url" | "-osu" => {
                sourcemap_url = Some(parse_unicode(&mut args)?);
            }

            /* from optimization-options.h */

            "-O" => {
                Profile::optimize_for_size().apply_to_opts(&mut opts);
            }
            "-O0" => {
                Profile::opt_level_0().apply_to_opts(&mut opts);
            }
            "-O1" => {
                Profile::opt_level_1().apply_to_opts(&mut opts);
            }
            "-O2" => {
                Profile::opt_level_2().apply_to_opts(&mut opts);
            }
            "-O3" => {
                Profile::opt_level_3().apply_to_opts(&mut opts);
            }
            "-O4" => {
                Profile::opt_level_4().apply_to_opts(&mut opts);
            }
            "-Os" => {
                Profile::optimize_for_size().apply_to_opts(&mut opts);
            }
            "-Oz" => {
                Profile::optimize_for_size_aggressively().apply_to_opts(&mut opts);
            }
            "--optimize-level" | "-ol" => {
                match parse_unicode(&mut args)?.as_str() {
                    "0" => { opts.optimize_level(OptimizeLevel::Level0); },
                    "1" => { opts.optimize_level(OptimizeLevel::Level1); },
                    "2" => { opts.optimize_level(OptimizeLevel::Level2); },
                    "3" => { opts.optimize_level(OptimizeLevel::Level3); },
                    "4" => { opts.optimize_level(OptimizeLevel::Level4); },
                    arg => {
                        unsupported.push(OsString::from(arg));
                    }
                }
            }
            "--shrink-level" | "-s" => {
                match parse_unicode(&mut args)?.as_str() {
                    "0" => { opts.shrink_level(ShrinkLevel::Level0); },
                    "1" => { opts.shrink_level(ShrinkLevel::Level1); },
                    "2" => { opts.shrink_level(ShrinkLevel::Level2); },
                    arg => {
                        unsupported.push(OsString::from(arg));
                    }
                }
            }
            "--debuginfo" | "-g" => {
                opts.debug_info(true);
            }
            "--always-inline-max-function-size" | "-aimfs" => {
                opts.always_inline_max_size(parse_u32(&mut args)?);
            }
            "--flexible-inline-max-function-size" | "-fimfs" => {
                opts.flexible_inline_max_size(parse_u32(&mut args)?);
            }
            "--one-caller-inline-max-function-size" | "-ocifms" => {
                opts.one_caller_inline_max_size(parse_u32(&mut args)?);
            }
            "--inline-functions-with-loops" | "-ifwl" => {
                opts.allow_functions_with_loops(true);
            }
            "--partial-inlining-ifs" | "-pii" => {
                opts.partial_inlining_ifs(parse_u32(&mut args)?);
            }
            "--traps-never-happen" | "-tnh" => {
                opts.traps_never_happen(true);
            }
            "--low-memory-unused" | "-lmu" => {
                opts.low_memory_unused(true);
            }
            "--fast-math" | "-ffm" => {
                opts.fast_math(true);
            }
            "--zero-filled-memory" | "-uim" => {
                opts.zero_filled_memory(true);
            }

            /* from tool-options.h */

            "--mvp-features" | "-mvp" => {
                opts.mvp_features_only();
            }
            "--all-features" | "-all" => {
                opts.all_features();
            }
            "--quiet" | "-q" => {
                /* pass */
            }
            "--no-validation" | "-n" => {
                opts.validate(false);
            }
            "--pass-arg" | "-pa" => {
                let args = parse_unicode(&mut args)?;
                if args.contains("@") {
                    let args: Vec<&str> = args.split("@").collect();
                    opts.set_pass_arg(args[0], args[1]);
                } else {
                    opts.set_pass_arg(&args, "1");
                }
            }

            /* fallthrough */

            _ => {
                // todo parse pass names w/ pass args (--pass-name=value).

                if arg.starts_with("--enable-") {
                    let feature = &arg[9..];
                    if let Ok(feature) = Feature::from_str(feature) {
                        opts.enable_feature(feature);
                    } else {
                        unsupported.push(OsString::from(arg));
                    }
                } else if arg.starts_with("--disable-") {
                    let feature = &arg[10..];
                    if let Ok(feature) = Feature::from_str(feature) {
                        opts.disable_feature(feature);
                    } else {
                        unsupported.push(OsString::from(arg));
                    }
                } else {
                    let mut is_pass = false;
                    for pass in Pass::iter() {
                        if is_pass_argument(arg, &pass) {
                            opts.add_pass(pass);
                            is_pass = true;
                        }
                    }

                    if !is_pass {
                        if arg.starts_with("-") && arg.len() > 1 {
                            // Reject args that look like flags that we don't support.
                            unsupported.push(OsString::from(arg));
                        } else {
                            parse_infile_path(OsStr::new(arg), &mut input_file, &mut unsupported);
                        }
                    }
                }
            }
        }
    }

    let input_file = if let Some(input_file) = input_file {
        input_file
    } else {
        return Err(Error::InputFileRequired);
    };
    let output_file = if let Some(output_file) = output_file {
        output_file
    } else {
        return Err(Error::OutputFileRequired);
    };

    if unsupported.len() > 0 {
        return Err(Error::Unsupported {
            args: unsupported,
        });
    }

    Ok(ParsedCliArgs {
        opts,
        input_file,
        input_sourcemap,
        output_file,
        output_sourcemap,
        sourcemap_url,
    })
}

fn is_pass_argument(arg: &str, pass: &Pass) -> bool {
    let pass_name = pass.name();
    arg.starts_with("--") && arg.contains(pass_name) && arg.len() == 2 + pass_name.len()
}

fn parse_infile_path(
    arg: &OsStr,
    maybe_input_file: &mut Option<PathBuf>,
    unsupported: &mut Vec<OsString>,
) {
    parse_path_into(&mut Some(arg).into_iter(), maybe_input_file, unsupported).expect("impossible")
}

fn parse_path_into<'item>(
    args: &mut impl Iterator<Item = &'item OsStr>,
    maybe_path: &mut Option<PathBuf>,
    unsupported: &mut Vec<OsString>,
) -> Result<(), Error> {
    if let Some(arg) = args.next() {
        if maybe_path.is_none() {
            *maybe_path = Some(PathBuf::from(arg));
        } else {
            unsupported.push(OsString::from(arg));
        }

        Ok(())
    } else {
        Err(Error::UnexpectedEndOfArgs)
    }
}

fn parse_unicode<'item>(args: &mut impl Iterator<Item = &'item OsStr>) -> Result<String, Error> {
    if let Some(arg) = args.next() {
        if let Some(arg) = arg.to_str() {
            Ok(arg.to_owned())
        } else {
            Err(Error::NeedUnicode {
                arg: arg.to_owned(),
            })
        }
    } else {
        Err(Error::UnexpectedEndOfArgs)
    }
}

fn parse_u32<'item>(args: &mut impl Iterator<Item = &'item OsStr>) -> Result<u32, Error> {
    let arg = parse_unicode(args)?;
    let number: u32 = arg.parse().map_err(|e| Error::NeedNumber {
        arg: OsString::from(arg),
        source: e,
    })?;
    Ok(number)
}
