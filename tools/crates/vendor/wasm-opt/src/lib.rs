//! Rust bindings to the `wasm-opt` WebAssembly optimizer.
//!
//! `wasm-opt` is a component of the [Binaryen] toolkit
//! that optimizes [WebAssembly] modules. It is written
//! in C++.
//!
//! [Binaryen]: https://github.com/WebAssembly/binaryen
//! [WebAssembly]: https://webassembly.org/
//!
//! This project provides a Rust crate that builds `wasm-opt` and:
//!
//! 1) makes its command-line interface installable via `cargo install`,
//! 2) provides an API to access it programmatically.
//!
//!
//! ## Installing the binary
//!
//! ```text
//! cargo install wasm-opt --locked
//! ```
//!
//! It should behave exactly the same as `wasm-opt` installed from other sources.
//!
//!
//! ## Using the library
//!
//! The crate provides an [`OptimizationOptions`] type that
//! follows the builder pattern, with options that closely
//! mirror the command line options of `wasm-opt`. Once built,
//! call [`OptimizationOptions::run`] to load, optimize, and write
//! the optimized module.
//!
//! ```no_run
//! use wasm_opt::OptimizationOptions;
//!
//! let infile = "hello_world.wasm";
//! let outfile = "hello_world_optimized.wasm";
//!
//! OptimizationOptions::new_optimize_for_size()
//!     .run(infile, outfile)?;
//!
//! # Ok::<(), anyhow::Error>(())
//! ```
//!
//! There are constructors for all the typical optimization profiles:
//!
//! - [`OptimizationOptions::new_optimize_for_size`] &middot; `-Os` or `-O`
//! - [`OptimizationOptions::new_optimize_for_size_aggressively`] &middot; `-Oz`
//! - [`OptimizationOptions::new_opt_level_0`] &middot; `-O0`, or no `-O*` argument.
//! - [`OptimizationOptions::new_opt_level_1`] &middot; `-O1`
//! - [`OptimizationOptions::new_opt_level_2`] &middot; `-O2`
//! - [`OptimizationOptions::new_opt_level_3`] &middot; `-O3`
//! - [`OptimizationOptions::new_opt_level_4`] &middot; `-O4`
//!
//! By default, the `run` method will read either binary `wasm` or text `wat` files,
//! inspecting the first few bytes for the binary header and choosing as appropriate,
//! and it will write a binary `wasm` file.
//! This behavior can be changed with [`OptimizationOptions::reader_file_type`]
//! and [`OptimizationOptions::writer_file_type`].
//!
//!
//! ## Enabling and disabling WASM features
//!
//! The WebAssembly specification has [optional features](https://webassembly.org/roadmap/),
//! represeted by the [`Feature`] enum.
//! The `Feature` variants link to the relevant specifications of each feature when known.
//! `wasm-opt` can be configured with support for them individually using the
//! [`OptimizationOptions::enable_feature`] and [`OptimizationOptions::disable_feature`]
//! methods.
//!
//! By default Binaryen (and this crate) enables these common features by default:
//!
//! - [`Feature::SignExt`]
//! - [`Feature::MutableGlobals`].
//!
//! The original WebAssembly specification with no additional features is known
//! as the _MVP_ specification. __To enable only the MVP features call
//! [`OptimizationOptions::mvp_features_only`]__.
//!
//! After resetting to MVP features, additional calls to `enable_feature` will
//! add features to the MVP feature set.
//!
//!
//! ## Customizing passes
//!
//! All Binaryen optimization passes are represented in the [`Pass`]
//! enum, and can be added to `OptimizationOptions` via [`OptimizationOptions::add_pass`].
//! These are added after the default set of passes, which are
//! enabled by most `OptimizationOptions` constructors. The default passes
//! can be disabled either with the [`OptimizationOptions::new_opt_level_0`] constructor,
//! or by calling [`OptimizationOptions::add_default_passes`]
//! with a `false` argument.
//!
//! ```no_run
//! use wasm_opt::{OptimizationOptions, Pass};
//!
//! let infile = "hello_world.wasm";
//! let outfile = "hello_world_optimized.wasm";
//!
//! // Just run the inliner.
//! OptimizationOptions::new_opt_level_0()
//!     .add_pass(Pass::InliningOptimizing)
//!     .run(infile, outfile)?;
//!
//! # Ok::<(), anyhow::Error>(())
//! ```
//!
//! Note that while this crate exposes all Binaryen passes
//! some may not make sense to actually use &mdash; Binaryen
//! is a command-line oriented tool, and some passes are
//! for debug purposes or print directly to the console.
//!
//!
//! ## Integrating with existing tooling
//!
//! For ease of integration with tools that already use `wasm-opt` via CLI, this
//! crate provides the [`integration`] module, which presents an API that is
//! compatible with `std`s `Command`. This allows client code to use mostly the
//! same code path for executing the `wasm-opt` CLI, and the crate-based API.
//!
//!
//! ## Cargo features
//!
//! Enabled by default, the `dwarf` feature enables passes related to DWARF
//! debug info. When enabled, this crate includes C++ code from the LLVM project.
//! This can cause duplicate symbol linkage errors when _also_ linking to LLVM.
//! When disabled, this code is not built, so can link successfully to LLVM,
//! but the Binaryen DWARF passes will do nothing.

// Most of the API surface is exported here.
//
// Many public methods are defined in other non-pub modules.
pub use api::*;

// Returned by the `run` method.
pub use run::OptimizationError;

// Easy integration with tools that already use `wasm-opt` via CLI.
pub mod integration;

// The "base" API.
//
// This API hides the `cxx` types,
// but otherwise sticks closely to the Binaryen API.
//
// This is hidden because we don't need to commit to these low-level APIs,
// but want to keep testing them from the `tests` folder.
#[doc(hidden)]
pub mod base;

// Types and constructors used in the API.
mod api;

// A builder interface for `OptimizationOptions`.
mod builder;

// The list of optimization passes.
mod passes;

// Definitions of -O1, -O2, etc.
mod profiles;

// The list of wasm features.
mod features;

// The `run` method that re-implements the logic from `wasm-opt.cpp`
// on top of `OptimizationOptions`.
mod run;

// A thin wrapper around `std::process::Command` that provides the unstable
// `get_args` method.
mod fake_command;
