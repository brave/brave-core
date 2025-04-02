#![feature(test)]
extern crate test;

cipher::stream_cipher_bench!(
    chacha20::ChaCha8;
    chacha8_bench1_16b 16;
    chacha8_bench2_256b 256;
    chacha8_bench3_1kib 1024;
    chacha8_bench4_16kib 16384;
);

cipher::stream_cipher_bench!(
    chacha20::ChaCha12;
    chacha12_bench1_16b 16;
    chacha12_bench2_256b 256;
    chacha12_bench3_1kib 1024;
    chacha12_bench4_16kib 16384;
);

cipher::stream_cipher_bench!(
    chacha20::ChaCha20;
    chacha20_bench1_16b 16;
    chacha20_bench2_256b 256;
    chacha20_bench3_1kib 1024;
    chacha20_bench4_16kib 16384;
);
