/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stddef.h>
#include <stdlib.h>

#include "build/rust/std/immediate_crash.h"

// When linking a final binary, rustc has to pick between either:
// * The default Rust allocator
// * Any #[global_allocator] defined in *any rlib in its dependency tree*
//   (https://doc.rust-lang.org/edition-guide/rust-2018/platform-and-target-support/global-allocators.html)
//
// In this latter case, this fact will be recorded in some of the metadata
// within the .rlib file. (An .rlib file is just a .a file, but does have
// additional metadata for use by rustc. This is, as far as I know, the only
// such metadata we would ideally care about.)
//
// In all the linked rlibs,
// * If 0 crates define a #[global_allocator], rustc uses its default allocator
// * If 1 crate defines a #[global_allocator], rustc uses that
// * If >1 crates define a #[global_allocator], rustc bombs out.
//
// Because rustc does these checks, it doesn't just have the __rust_alloc
// symbols defined anywhere (neither in the stdlib nor in any of these
// crates which have a #[global_allocator] defined.)
//
// Instead:
// Rust's final linking stage invokes dynamic LLVM codegen to create symbols
// for the basic heap allocation operations. It literally creates a
// __rust_alloc symbol at link time. Unless any crate has specified a
// #[global_allocator], it simply calls from __rust_alloc into
// __rdl_alloc, which is the default Rust allocator. The same applies to a
// few other symbols.
//
// We're not (always) using rustc for final linking. For cases where we're not
// Rustc as the final linker, we'll define those symbols here instead.
//
// In future, we may wish to do something different from using the Rust
// default allocator (e.g. explicitly redirect to PartitionAlloc). We could
// do that here, or we could build a crate with a #[global_allocator] and
// redirect these symbols to that crate instead. The advantage of the latter
// is that it would work equally well for those cases where rustc is doing
// the final linking. At present, this is not necessary because
// PartitionAlloc-Everywhere successfully handles the calls to malloc which
// result from passing through this code. We might want to call into
// PA directly if we wished for Rust allocations to be in a different
// partition, or similar, in future.
//
// They're weak symbols, because this file will sometimes end up in targets
// which are linked by rustc, and thus we would otherwise get duplicate
// definitions. The following definitions will therefore only end up being
// used in targets which are linked by our C++ toolchain.

extern "C" {

void* __rdl_alloc(size_t, size_t);
void __rdl_dealloc(void*);
void* __rdl_realloc(void*, size_t, size_t, size_t);
void* __rdl_alloc_zeroed(size_t, size_t);

void* __attribute__((weak)) __rust_alloc(size_t a, size_t b) {
  return __rdl_alloc(a, b);
}

void __attribute__((weak)) __rust_dealloc(void* a) {
  __rdl_dealloc(a);
}

void* __attribute__((weak))
__rust_realloc(void* a, size_t b, size_t c, size_t d) {
  return __rdl_realloc(a, b, c, d);
}

void* __attribute__((weak)) __rust_alloc_zeroed(size_t a, size_t b) {
  return __rdl_alloc_zeroed(a, b);
}

void __attribute__((weak)) __rust_alloc_error_handler(size_t a, size_t b) {
  IMMEDIATE_CRASH();
}

extern const unsigned char __attribute__((weak))
__rust_alloc_error_handler_should_panic = 0;

}  // extern "C"
