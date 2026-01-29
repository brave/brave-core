# dynstack
Stack that allows users to allocate dynamically sized arrays.

The stack wraps a buffer of bytes that it uses as a workspace.
Allocating an array takes a chunk of memory from the stack, which can be reused once the array
is dropped.

# Features
 - `std`: enables a `std::error::Error` implementation for error types.
 - `nightly`: enables a drop check eye patch for `DynArray` and enables the
   allocator backend for the memory buffers.

# Examples
```rust
use core::mem::MaybeUninit;
use dynstack::{DynStack, StackReq};
use reborrow::ReborrowMut;

// We allocate enough storage for 3 `i32` and 4 `u8`.
let mut buf = [MaybeUninit::uninit();
    StackReq::new::<i32>(3)
        .and(StackReq::new::<u8>(4))
        .unaligned_bytes_required()];
let mut stack = DynStack::new(&mut buf);

{
    // We can have nested allocations.
    // 3×`i32`
    let (array_i32, substack) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32);
    // and 4×`u8`
    let (mut array_u8, _) = substack.make_with::<u8, _>(4, |_| 0);

    // We can read from the arrays,
    assert_eq!(array_i32[0], 0);
    assert_eq!(array_i32[1], 1);
    assert_eq!(array_i32[2], 2);

    // and write to them.
    array_u8[0] = 1;

    assert_eq!(array_u8[0], 1);
    assert_eq!(array_u8[1], 0);
    assert_eq!(array_u8[2], 0);
    assert_eq!(array_u8[3], 0);
}

{
    // We can also have disjoint allocations.
    // 3×`i32`
    let (mut array_i32, _) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32);
    assert_eq!(array_i32[0], 0);
    assert_eq!(array_i32[1], 1);
    assert_eq!(array_i32[2], 2);
}

{
    // or 4×`u8`
    let (mut array_u8, _) = stack.rb_mut().make_with::<i32, _>(4, |i| i as i32 + 3);
    assert_eq!(array_u8[0], 3);
    assert_eq!(array_u8[1], 4);
    assert_eq!(array_u8[2], 5);
    assert_eq!(array_u8[3], 6);
}
```
