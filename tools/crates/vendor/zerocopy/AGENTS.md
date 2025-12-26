<!-- Copyright 2025 The Fuchsia Authors

Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
<LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
This file may not be copied, modified, or distributed except according to
those terms. -->

# Development Instructions

This repository uses a wrapper script around Cargo to ensure consistent toolchain usage and configuration.

## Build and Test

**IMPORTANT:** You must **NEVER** run `cargo` directly. Instead, you must **ALWAYS** use `yes | ./cargo.sh` for all `cargo` sub-commands (e.g., `check`, `test`, `build`). Using `yes |` is required to bypass interactive prompts for toolchain installation.

### Syntax
`yes | ./cargo.sh +<toolchain> <command> [args]`

### Toolchains
The `<toolchain>` argument is mandatory and can be one of the following:

- `msrv`: Runs with the Minimum Supported Rust Version.
- `stable`: Runs with the stable toolchain.
- `nightly`: Runs with the nightly toolchain.
- `all`: Runs the command on `msrv`, `stable`, and `nightly` sequentially.
- Version-gated toolchains: You can also pass specific version-gated toolchains defined in `Cargo.toml`, such as `zerocopy-core-error-1-81-0`.

### Linting

Clippy should **always** be run on the `nightly` toolchain.

```bash
yes | ./cargo.sh +nightly clippy
yes | ./cargo.sh +nightly clippy --tests
```

### Examples

```bash
# Check the code using the nightly toolchain
# DO NOT RUN: cargo check
yes | ./cargo.sh +nightly check

# Run tests on all supported toolchains
# DO NOT RUN: cargo test
yes | ./cargo.sh +all test

# Run a specific test on stable
yes | ./cargo.sh +stable test -- test_name
```

## Workflow

### Pre-submission Checks

Before submitting code, run `./githooks/pre-push` to confirm that all pre-push hooks succeed.

### UI Tests

When updating UI test files (in `tests/ui*` or `zerocopy-derive/tests/ui*`), run `./tools/update-ui-test-files.sh` to update the corresponding stderr files.

### Pull Requests and Commit Messages

When a PR resolves an issue, the PR description and commit message should include a line like `Closes #123`.
When a PR makes progress on, but does not close, an issue, the PR description and commit message should include a line like `Makes progress on #123`.

## Safety

### Pointer Casts

- **Avoid `&slice[0] as *const T` or `&slice[0] as *mut T`.**
  Instead, use `slice.as_ptr()` or `slice.as_mut_ptr()`. Casting a reference to
  a single element creates a raw pointer that is only valid for that element.
  Accessing subsequent elements via pointer arithmetic is Undefined Behavior.
  See [unsafe-code-guidelines#134](https://github.com/rust-lang/unsafe-code-guidelines/issues/134).

- **Avoid converting `&mut T` to `*const T` (or `*const U`)**.
  This advice applies if you intend to later cast the pointer to `*mut T` and
  mutate the data. This conversion reborrows `&mut T` as a shared reference
  `&T`, which may restrict permissions under Stacked Borrows. Instead, cast
  `&mut T` directly to `*mut T` first, then to `*const T` if necessary. See
  [rust#56604](https://github.com/rust-lang/rust/issues/56604).

## Code Style

### File Headers

Each file should contain a copyright header (excluding auto-generated files such as `.stderr` files). The header should follow the format found in existing files (e.g. `src/lib.rs`), using the appropriate comment syntax for the file type.

### Formatting

To determine how to format code, read the formatting checker script in `ci/check_fmt.sh`.

### Comments

All comments (including `//`, `///`, and `//!`) should be wrapped at 80 columns.

**Exceptions:**
- Markdown tables
- Inline ASCII diagrams
- Long URLs
- Comments inside of code blocks
- Comments which trail non-comment code
- Other cases where wrapping would significantly degrade readability (use your judgment).
