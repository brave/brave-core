CFLAGS :=

ifdef NDEBUG
CFLAGS += -DNDEBUG=${NDEBUG}
endif

ifdef NO_CXXEXCEPTIONS
CFLAGS += -DNO_CXXEXCEPTIONS=${NO_CXXEXCEPTIONS}
endif

all: tests/cc.out

src/lib.h: src/lib.rs
	cbindgen -o src/lib.h

test: tests/cc.out
	./tests/cc.out

tests/cc.out: target/debug/libprivate_channel.a tests/private_channel.o tests/main.cc 
	g++ $(CFLAGS) -std=gnu++0x tests/main.cc tests/private_channel.o ./target/debug/libprivate_channel.a -I ./src -lpthread -ldl -o tests/cc.out

tests/private_channel.o: src/lib.h src/private_channel.cc src/private_channel.h
	g++ $(CFLAGS) -std=gnu++0x src/private_channel.cc -I src/ -c  -o tests/private_channel.o

target/debug/libprivate_channel.a: src/lib.rs Cargo.toml
	cargo build

valgrind-supp: tests
	valgrind --gen-suppressions=all --suppressions=.valgrind.supp  --leak-check=yes --error-exitcode=1 ./tests/cc.out --gen-suppressions=all

valgrind: tests
	valgrind -v --suppressions=.valgrind.supp  --leak-check=yes --error-exitcode=1 ./tests/cc.out --gen-suppressions=all

clean:
	rm -rf target
	rm ./src/lib.h
	rm ./tests/private_channel.o

lint:
	cargo fmt -- --check
	cargo clippy
