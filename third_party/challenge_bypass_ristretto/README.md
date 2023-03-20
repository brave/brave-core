# challenge-bypass-ristretto-ffi [![Build Status](https://travis-ci.org/brave-intl/challenge-bypass-ristretto-ffi.svg?branch=master)](https://travis-ci.org/brave-intl/challenge-bypass-ristretto-ffi)

**A FFI crate, C++ and Golang wrappers to expose functionality from [challenge-bypass-ristretto](https://github.com/brave-intl/challenge-bypass-ristretto)**

The `challenge-bypass-ristretto` crate implements a form of blinded tokens using a VOPRF protocol in rust. This
crate exposes C FFI functions and is configured to produce a static library so that the functionality
can be used in other languages.

Currently there are bindings for C++ and Golang.

# Notes

The FFI crate handles all allocation of blinded token structures, returning
a pointer to the underlying memory using `Box::into_raw(Box::new(...))`.
Correspondingly, there are destructor functions to free the memory for each
structure. The higher level bindings take care to ensure that these destructors
are automatically called, any new bindings added should take care to do the
same.

No direct introspection / modification of the Rust allocated objects from external
bindings is allowed. The C header exposes only an opaque pointer to the Rust allocated
objects.

Other than the destructor no FFI functions exposed mutate or take ownership of the passed structures.

Error handling at the C FFI layer is done via a thread local variable
`LAST_ERROR` which is set when an error
occurs. For most functions, a NULL return is indicative of an error. For
verification functions that would normally return a bool, an int is returned
instead. In these cases -1 indicates an error.

Convenience functions are included to handle encoding / decoding structures to / from base64.

UTF-8 strings are expected in all places strings are passed. These bindings do
not attempt to address cross-platform string differences such as UTF-16 on
windows.

[Cbindgen](https://github.com/eqrion/cbindgen) was used to generate the C header
file.

This crate instantiates challenge-bypass-ristretto with Sha512 as the hash and
`rand::OsRng` as the cryptographically secure random number generator.

The C++ bindings can optionally be built with exceptions turned off. This is
intended for interoperability with Chromium which by default does not use
exceptions. If exceptions are turned off, instead of being thrown exceptions
are assigned to a thread local variable. The `exception_occured()` function
must be called after every function call and if an error occurred the
`get_last_exception()` function can be called to retrieve and clear the exception.

# Development

Working on this repository requires having Rust, Go 1.11, g++, musl and valgrind to be installed.
Alternatively - Rust, g++ / valgrind and Docker can be used as there is a
Dockerfile for building and testing the Golang bindings.

## Testing

There are end to end test binaries for the C++ and Golang bindings, when run under
valgrind we can ensure memory is being properly freed.

## C++

### Running e2e test

```
make examples/cpp.out
valgrind --leak-check=yes --error-exitcode=1 ./examples/cpp.out
```

## Golang

### Running e2e test

```
make examples/golang.dyn.out
valgrind --suppressions=.valgrind.supp --run-libc-freeres=no --leak-check=yes --undef-value-errors=no --error-exitcode=1 examples/golang.dyn.out
```

### Running e2e test with Docker

```
make go-docker-test
```

## Regenerating the C header

```
cbindgen -o src/lib.h
```

