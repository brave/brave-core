all: build
	
.PHONY: build
build: 
	cargo build --lib --release --target wasm32-unknown-unknown
	wasm-bindgen --target web --out-dir pkg target/wasm32-unknown-unknown/release/skus_sdk.wasm
