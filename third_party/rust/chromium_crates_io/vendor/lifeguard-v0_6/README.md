# lifeguard [![](https://api.travis-ci.org/zslayton/lifeguard.png?branch=master)](https://travis-ci.org/zslayton/lifeguard) [![](http://meritbadge.herokuapp.com/lifeguard)](https://crates.io/crates/lifeguard)
## Object Pool Manager
[API Documentation](http://zslayton.github.io/lifeguard/lifeguard/)

### Examples

`Pool` issues owned values wrapped in smartpointers.

```rust
extern crate lifeguard;
use lifeguard::*;

fn main() {
    let pool : Pool<String> = pool().with(StartingSize(10)).build();
    {
        let string = pool.new_from("Hello, World!"); // Remove a value from the pool
        assert_eq!(9, pool.size());
    } // Values that have gone out of scope are automatically moved back into the pool.
    assert_eq!(10, pool.size());
}
```

Values taken from the pool can be dereferenced to access/mutate their contents.

```rust
extern crate lifeguard;
use lifeguard::*;

fn main() {
    let pool : Pool<String> = pool().with(StartingSize(10)).build();
    let mut string = pool.new_from("cat");
    string.push_str("s love eating mice"); //string.as_mut() also works
    assert_eq!("cats love eating mice", *string);
}
```

Values can be unwrapped, detaching them from the pool.

```rust
extern crate lifeguard;
use lifeguard::*;

fn main() {
    let pool : Pool<String> = pool().with(StartingSize(10)).build();
    {
        let string : String = pool.new().detach();
    } // The String goes out of scope and is dropped; it is not returned to the pool
    assert_eq!(9, pool.size());
}
```

Values can be manually entered into / returned to the pool.

```rust
extern crate lifeguard;
use lifeguard::*;

fn main() {
    let pool : Pool<String> = pool().with(StartingSize(10)).build();
    {
        let string : String = pool.detached(); // An unwrapped String, detached from the Pool
        assert_eq!(9, pool.size());
        let rstring : Recycled<String> = pool.attach(string); // The String is attached to the pool again
        assert_eq!(9, pool.size()); // but it is still checked out from the pool
    } // rstring goes out of scope and is added back to the pool
    assert_eq!(10, pool.size());
}
```

`Pool`'s builder API can be used to customize the behavior of the pool.

```rust
extern crate lifeguard;
use lifeguard::*;

fn main() {
 let pool : Pool<String> = pool()
   // The pool will allocate 128 values for immediate use. More will be allocated on demand.
   .with(StartingSize(128))
   // The pool will only grow up to 4096 values. Further values will be dropped.
   .with(MaxSize(4096))
   // The pool will use this closure (or other object implementing Supply<T>) to allocate
   .with(Supplier(|| String::with_capacity(1024)))
   .build();
  // ...
}
```

### Highly Unscientific Benchmarks

Benchmark source can be found [here](https://github.com/zslayton/lifeguard/blob/master/benches/lib.rs). Tests were run on an early 2015 MacBook Pro.

Each benchmark comes in three flavors:

1. `tests::*_standard`: Uses the system allocator to create new values.
2. `tests::*_pooled_rc`: Uses a `Pool` to create new values which hold `Rc` references to the `Pool`. These values can be freely passed to other scopes.
3. `tests::*_pooled`: Uses a `Pool` to create new values which hold `&` locally-scoped references to the `Pool`. These values are the cheapest to create but are bound to the lifetime of the `Pool`.

#### Uninitialized Allocation

Compares the cost of allocating a new `String` (using `String::with_capacity`, as `String::new` does not allocate immediately) with the cost of retrieving a `String` from the pool.

```ignore
tests::allocation_standard                          ... bench:   5,322,513 ns/iter (+/- 985,898)
tests::allocation_pooled_rc                         ... bench:     784,885 ns/iter (+/- 95,245)
tests::allocation_pooled                            ... bench:     565,864 ns/iter (+/- 66,036)
```

#### Initialized Allocation

Compares the cost of allocating a new `String` and initializing it to a given value (via `&str::to_owned`) with the cost of retrieving a `String` from the pool and initializing it to the same value.

```ignore
tests::initialized_allocation_standard              ... bench:   5,329,948 ns/iter (+/- 547,725)
tests::initialized_allocation_pooled_rc             ... bench:   1,151,493 ns/iter (+/- 119,293)
tests::initialized_allocation_pooled                ... bench:     927,214 ns/iter (+/- 147,935)
```

#### Vec&lt;Vec&lt;String>> Allocation

Creates a two-dimensional vector of initialized Strings. All `Vec`s and `String`s created are from a `Pool` where applicable. Adapted from [this benchmark](https://github.com/frankmcsherry/recycler/blob/master/benches/benches.rs#L10).

```ignore
tests::vec_vec_str_standard                         ... bench:   1,353,906 ns/iter (+/- 142,094)
tests::vec_vec_str_pooled_rc                        ... bench:     298,087 ns/iter (+/- 168,703)
tests::vec_vec_str_pooled                           ... bench:     251,082 ns/iter (+/- 24,408)
```

Ideas and PRs welcome!

Inspired by frankmcsherry's [recycler](https://github.com/frankmcsherry/recycler).
