[private]
default:
  @just --list


# run `cargo build` on everything
build:
  cargo build --workspace --all-targets


# run `cargo check` on everything
check:
  cargo check --workspace --all-targets


# run code formatter
format:
  cargo +nightly fmt


# run tests
test: build
  cargo test --all-features


# run `cargo clippy` on everything
clippy:
  cargo clippy --locked --offline --workspace --all-targets -- --deny warnings

# run `cargo clippy --fix` on everything
clippy-fix:
  cargo clippy --locked --offline --workspace --all-targets --fix

# Check for API changes.
check-api:
 contrib/check-for-api-changes.sh
