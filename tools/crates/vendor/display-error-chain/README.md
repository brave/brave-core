# display-error-chain

<!-- cargo-rdme start -->

A lightweight library for displaying errors and their sources.

A sample output:

```rust
macro_rules! impl_error {
    // ...
}

// `TopLevel` is caused by a `MidLevel`.
#[derive(Debug)]
struct TopLevel;
impl_error!(TopLevel, "top level", Some(&MidLevel));

// `MidLevel` is caused by a `LowLevel`.
#[derive(Debug)]
struct MidLevel;
impl_error!(MidLevel, "mid level", Some(&LowLevel));

// `LowLevel` is the cause itself.
#[derive(Debug)]
struct LowLevel;
impl_error!(LowLevel, "low level", None);

// Now let's see how it works:
let formatted = display_error_chain::DisplayErrorChain::new(&TopLevel).to_string();
assert_eq!(
    formatted,
    "\
top level
Caused by:
  -> mid level
  -> low level"
);

// Or with `.chain()` helper:
use display_error_chain::ErrorChainExt as _;
let formatted = TopLevel.chain().to_string();
assert_eq!(
    formatted,
    "\
top level
Caused by:
  -> mid level
  -> low level"
);

// Or even with `.into_chain()` helper to consume the error.
use display_error_chain::ErrorChainExt as _;
let formatted = TopLevel.into_chain().to_string();
assert_eq!(
    formatted,
    "\
top level
Caused by:
  -> mid level
  -> low level"
);
```

<!-- cargo-rdme end -->

License: Apache-2.0/MIT
