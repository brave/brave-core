all: examples/out

src/wrapper.hpp: src/lib.rs
	cxxbridge src/lib.rs --header > src/wrapper.hpp

src/wrapper.cpp: src/lib.rs
	cxxbridge src/lib.rs > src/wrapper.cpp

examples/wrapper.o: src/wrapper.cpp src/wrapper.hpp
	g++ $(CFLAGS) -std=c++11 src/wrapper.cpp -I src/ -c  -o examples/wrapper.o

../target/debug/libbrave_rewards_cxx.a: src/lib.rs src/httpclient.rs src/storage.rs
	cargo build

examples/out: examples/wrapper.o ../target/debug/libbrave_rewards_cxx.a  examples/main.cpp
	g++ $(CFLAGS) -std=c++11 examples/main.cpp examples/wrapper.o ../target/debug/libbrave_rewards_cxx.a -I ./src -lpthread -ldl -o examples/out

clean:
	rm examples/out
