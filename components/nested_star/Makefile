all: example

example: example/main
	./example/main

example/main: target/debug/libnested_star_cxx.a example/main.cc
	g++ example/main.cc target/cxxbridge/nested-star-cxx/src/lib.rs.cc ./target/debug/libnested_star_cxx.a -I ./target/cxxbridge/nested-star-cxx/src -L ./target/debug -lssl -lcrypto -o example/main

target/debug/libnested_star_cxx.a: src/lib.rs Cargo.toml
	cargo build

clean:
	rm -rf target example/main
