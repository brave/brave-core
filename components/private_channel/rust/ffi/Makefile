CFLAGS :=

ifdef NDEBUG
CFLAGS += -DNDEBUG=${NDEBUG}
endif

ifdef NO_CXXEXCEPTIONS
CFLAGS += -DNO_CXXEXCEPTIONS=${NO_CXXEXCEPTIONS}
endif

all: tests/cpp.out

src/lib.h: src/lib.rs
	cbindgen -o src/lib.h

test: tests/cpp.out
	./tests/cpp.out

tests/cpp.out: target/debug/libprivate_channel.a tests/private_channel.o tests/main.cpp 
	g++ $(CFLAGS) -std=gnu++0x tests/main.cpp tests/private_channel.o ./target/debug/libprivate_channel.a -I ./src -lpthread -ldl -o tests/cpp.out

tests/private_channel.o: src/lib.h src/private_channel.cpp src/private_channel.hpp
	g++ $(CFLAGS) -std=gnu++0x src/private_channel.cpp -I src/ -c  -o tests/private_channel.o

target/debug/libprivate_channel.a: src/lib.rs Cargo.toml
	cargo build

valgrind-supp: tests
	valgrind --gen-suppressions=all --suppressions=.valgrind.supp  --leak-check=yes --error-exitcode=1 ./tests/cpp.out --gen-suppressions=all

valgrind: tests
	valgrind -v --suppressions=.valgrind.supp  --leak-check=yes --error-exitcode=1 ./tests/cpp.out --gen-suppressions=all

clean:
	rm -rf target
	rm ./src/lib.h
	rm ./tests/private_channel.o

lint:
	cargo fmt -- --check
	cargo clippy
