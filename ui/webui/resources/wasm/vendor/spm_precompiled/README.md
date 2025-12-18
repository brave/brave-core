![](https://github.com/Narsil/spm_precompiled/workflows/build/badge.svg)
[![Crate](https://img.shields.io/crates/v/spm_precompiled.svg)](https://crates.io/crates/spm_precompiled)
[![API](https://docs.rs/spm_precompiled/badge.svg)](https://docs.rs/spm_precompiled)

# spm_precompiled

This crate aims to emulate https://github.com/google/sentencepiece Dart::DoubleArray
struct and it's Normalizer. It's main intent is to be used with tokenizers
that is a Rust library that aims to provide facilities to tokenize string
for use with HuggingFace's transformers library

This crate is highly specialized and not intended for general use.

The core of the algorithm is to read spm's binary `precompiled_charsmap`.
