This crate is a C API for [zlib-rs](https://docs.rs/zlib-rs/latest/zlib_rs/). The API is broadly equivalent to [`libz-sys`](https://docs.rs/libz-sys/latest/libz_sys/) and [`zlib-ng-sys`](https://docs.rs/libz-ng-sys/latest/libz_ng_sys/), but does not currently provide the `gz*` family of functions.

From a rust perspective, this API is not very ergonomic. Use the [`flate2`](https://crates.io/crates/flate2) crate for a more
ergonomic rust interface to zlib.

## Features

**`custom-prefix`**

Add a custom prefix to all exported symbols.

The value of the `LIBZ_RS_SYS_PREFIX` is used as a prefix for all exported symbols. For example:

```text
> LIBZ_RS_SYS_PREFIX="MY_CUSTOM_PREFIX" cargo build -p libz-rs-sys --features=custom-prefix
   Compiling libz-rs-sys v0.2.1 (/home/folkertdev/rust/zlib-rs/libz-rs-sys)
    Finished `dev` profile [optimized + debuginfo] target(s) in 0.21s
> objdump -tT target/debug/liblibz_rs_sys.so | grep "uncompress"
0000000000081028 l     O .got	0000000000000000              _ZN7zlib_rs7inflate10uncompress17he7d985e55c58a189E$got
000000000002c570 l     F .text	00000000000001ef              _ZN7zlib_rs7inflate10uncompress17he7d985e55c58a189E
0000000000024330 g     F .text	000000000000008e              MY_CUSTOM_PREFIXuncompress
0000000000024330 g    DF .text	000000000000008e  Base        MY_CUSTOM_PREFIXuncompress
```

**`c-allocator`, `rust-allocator`**

Pick the default allocator implementation that is used if no `zalloc` and `zfree` are configured in the input `z_stream`.

- `c-allocator`: use `malloc`/`free` for the implementation of `zalloc` and `zfree`
- `rust-allocator`: the rust global allocator for the implementation of `zalloc` and `zfree`

The `rust-allocator` is the default when this crate is used as a rust dependency, and slightly more efficient because alignment is handled by the allocator. When building a dynamic library, it may make sense to use `c-allocator` instead.

**`std`**

Assume that `std` is available. When this feature is turned off, this crate is compatible with `#![no_std]`.

## Example

This example compresses ("deflates") the string `"Hello, World!"` and then decompresses
("inflates") it again.

```rust
let mut strm = libz_rs_sys::z_stream::default();

let version = libz_rs_sys::zlibVersion();
let stream_size = core::mem::size_of_val(&strm) as i32;

let level = 6; // the default compression level
let err = unsafe { libz_rs_sys::deflateInit_(&mut strm, level, version, stream_size) };
assert_eq!(err, libz_rs_sys::Z_OK);

let input = "Hello, World!";
strm.avail_in = input.len() as _;
strm.next_in = input.as_ptr();

let mut output = [0u8; 32];
strm.avail_out = output.len() as _;
strm.next_out = output.as_mut_ptr();

let err = unsafe { libz_rs_sys::deflate(&mut strm, libz_rs_sys::Z_FINISH) };
assert_eq!(err, libz_rs_sys::Z_STREAM_END);

let err = unsafe { libz_rs_sys::deflateEnd(&mut strm) };
assert_eq!(err, libz_rs_sys::Z_OK);

let deflated = &mut output[..strm.total_out as usize];

let mut strm = libz_rs_sys::z_stream::default();
let err = unsafe { libz_rs_sys::inflateInit_(&mut strm, version, stream_size) };
assert_eq!(err, libz_rs_sys::Z_OK);

strm.avail_in = deflated.len() as _;
strm.next_in = deflated.as_ptr();

let mut output = [0u8; 32];
strm.avail_out = output.len() as _;
strm.next_out = output.as_mut_ptr();

let err = unsafe { libz_rs_sys::inflate(&mut strm, libz_rs_sys::Z_FINISH) };
assert_eq!(err, libz_rs_sys::Z_STREAM_END);

let err = unsafe { libz_rs_sys::inflateEnd(&mut strm) };
assert_eq!(err, libz_rs_sys::Z_OK);

let inflated = &output[..strm.total_out as usize];

assert_eq!(inflated, input.as_bytes())
```

## Memory Management

Zlib does not assume a global allocator, but instead accepts `zalloc` and `zfree` functions for allocating and deallocating memory.

```rust,ignore
type alloc_func = unsafe extern "C" fn(*mut c_void, c_uint, c_uint) -> *mut c_void;
type free_func = unsafe extern "C" fn(*mut c_void, *mut c_void);

struct z_stream {
    // ...
    zalloc: Option<alloc_func>,
    zfree: Option<free_func>,
    opaque: *mut c_void,
    // ..
}
```

These functions can run arbitrary logic, so long as they satisfy their contract. This mechanism can be used e.g. on embedded systems to use an array in static memory as the working memory for (de)compression. For more information see the [`z_stream`](https://docs.rs/zlib-rs/latest/zlib_rs/c_api/struct.z_stream.html) documentation.

For convenience we provide two default memory allocators:

- `rust` uses the rust global allocator
- `c` uses `malloc` and `free`

When used as a rust crate, the default is to use the rust global allocator. This behavior can be overridden using [feature flags](#features) to either configure the C allocator as the default, or to configure no default allocator at all. If no default is configured, and no custom allocation and deallocation functions are provided, initialization will return an error.

## Compression Levels

The zlib library supports compression levels 0 up to and including 9. The level indicates a tradeoff between time spent on the compression versus the compression ratio, the factor by which the input is reduced in size:

- level 0: no compression at all
- level 1: fastest compression
- level 6: default (a good tradeoff between speed and compression ratio)
- level 9: best compression

Beyond this intuition, the exact behavior of the compression levels is not specified. The implementation of `zlib-rs` follows the implementation of [`zlib-ng`](https://github.com/zlib-ng/zlib-ng), and deviates from the one in stock zlib.

In particular, our compression level 1 is extremely fast, but also just does not compress that well. On the `silesia-small.tar` input file, we see these output sizes:

| implementation | compression level | output size (mb) |
| --- | --- | --- |
| -     | 0 | `15.74` |
| stock | 1 | ` 7.05` |
| rs    | 1 | ` 8.52` |
| rs    | 2 | ` 6.90` |
| rs    | 4 | ` 6.55` |

But, `zlib-rs` is much faster than stock zlib. In our benchmarks, it is only at level 4 that we spend roughly as much time as stock zlib on level 1:

| implementation | compression level | wall time (ms) |
| --- | --- | --- |
| stock | 1 | 185 |
| rs | 2 | 139 |
| rs | 4 | 181 |

In our example, the main options are:

- level 1: worse compression, but much faster
- level 2: equivalent compression, but significantly faster
- level 4: better compression, at the same speed

In summary, when you upgrade from stock zlib, we recommend that you benchmark on your data and target platform, and pick the right compression level for your use case.
