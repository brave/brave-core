all: example

example: example/main
	./example/main

example/main: target/debug/libconstellation_cxx.a example/main.cc
	g++ example/main.cc target/cxxbridge/constellation-cxx/src/lib.rs.cc ./target/debug/libconstellation_cxx.a -I ./target/cxxbridge/constellation-cxx/src -L ./target/debug -lssl -lcrypto -o example/main

target/debug/libconstellation_cxx.a: src/lib.rs Cargo.toml
	cargo build

clean:
	rm -rf target example/main
