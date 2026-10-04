all: test

check:
	@cargo check --all-features

build:
	@cargo build --all-features

doc:
	@cargo doc --all-features

test:
	@echo "CARGO TESTS"
	@cargo test
	@cargo test --all-features
	@cargo test --no-default-features

format:
	@rustup component add rustfmt 2> /dev/null
	@cargo fmt --all

format-check:
	@rustup component add rustfmt 2> /dev/null
	@cargo fmt --all -- --check

lint:
	@rustup component add clippy 2> /dev/null
	@cargo clippy --examples --tests

.PHONY: all doc build check test format format-check lint
