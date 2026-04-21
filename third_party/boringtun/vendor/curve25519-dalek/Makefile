FEATURES := serde rand_core digest legacy_compatibility

export RUSTDOCFLAGS := \
	--cfg docsrs \
	--html-in-header docs/assets/rustdoc-include-katex-header.html

doc:
	cargo +nightly rustdoc --features "$(FEATURES)"

doc-internal:
	cargo +nightly rustdoc --features "$(FEATURES)" -- --document-private-items
