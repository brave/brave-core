# num-conv

`num_conv` is a crate to convert between integer types without using `as` casts. This provides
better certainty when refactoring, makes the exact behavior of code more explicit, and allows using
turbofish syntax. The crate is currently in the process of being uplifted into the standard library;
see [rust-lang/rust#154330](https://github.com/rust-lang/rust/issues/154330) for details.
