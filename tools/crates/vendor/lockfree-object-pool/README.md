# Lock Free Object Pool
[![License](https://img.shields.io/badge/License-Boost%201.0-lightblue.svg)](https://github.com/EVaillant/lockfree-object-pool) [![Cargo](https://img.shields.io/crates/v/lockfree-object-pool.svg)](https://crates.io/crates/lockfree-object-pool) [![Documentation](https://docs.rs/lockfree-object-pool/badge.svg)](
https://docs.rs/lockfree-object-pool) ![CI](https://github.com/EVaillant/lockfree-object-pool/workflows/CI/badge.svg)

A thread-safe object pool collection with automatic return.

Some implementations are lockfree :
* LinearObjectPool
* SpinLockObjectPool

Other use std::Mutex :
* MutexObjectPool

And NoneObjectPool basic allocation without pool.

### Usage
```toml
[dependencies]
lockfree-object-pool = "0.1"
```
```rust
extern crate lockfree_object_pool;
```

### Example

The general pool creation looks like this for
```rust
 let pool = LinearObjectPool::<u32>::new(
     ||  Default::default(), 
     |v| {*v = 0; });
```

And use the object pool 
```rust
  let mut item = pool.pull();
  *item = 5;
  ...  
```
At the end of the scope item return in object pool.

### Interface
All implementations support same interface :
```rust
struct ObjectPool<T> {  
}

impl<T> ObjectPool<T> {
  // for LinearObjectPool, SpinLockObjectPool and MutexObjectPool
  // init closure used to create an element
  // reset closure used to reset element a dropped element
  pub fn new<R, I>(init: I, reset: R) -> Self
    where
        R: Fn(&mut T) + 'static + Send + Sync,
        I: Fn() -> T + 'static + Send + Sync + Clone,
    {
      ...
    }

  // for NoneObjectPool
  // init closure used to create an element
  pub fn new<I>(init: I) -> Self
    where
        I: Fn() -> T + 'static
    {
      ...
    }

  pub fn pull(&self) -> Reusable<T> {
    ...
  }

  pub fn pull_owned(self: &Arc<Self>) -> OwnedReusable<T> {
    ...
  }
}

struct Reusable<T> {  
}

impl<'a, T> DerefMut for Reusable<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        ...
    }
}

impl<'a, T> Deref for Reusable<'a, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        ...
    }
}

struct OwnedReusable<T> {  
}

impl<'a, T> DerefMut for OwnedReusable<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        ...
    }
}

impl<'a, T> Deref for OwnedReusable<'a, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        ...
    }
}
```

### Multithreading

All implementation support allocation/desallocation from on or more thread. You only need to wrap the pool in a [`std::sync::Arc`] :

```rust
 let pool = Arc::new(LinearObjectPool::<u32>::new(
     ||  Default::default(), 
     |v| {*v = 0; }));
```

### Performance

Global [report](https://evaillant.github.io/lockfree-object-pool/benches/criterion/report/index.html).

#### Allocation

 ObjectPool | Duration in Monothreading (us) | Duration Multithreading (us)
------------| :----------------------------: | :--------------------------:
NoneObjectPool|1.2848|0.62509
MutexObjectPool|1.3107|1.5178
SpinLockObjectPool|1.3106|1.3684
LinearObjectPool|0.23732|0.38913
[`crate 'sharded-slab'`]|1.6264|0.82607
[`crate 'object-pool'`]|0.77533|0.26224

Report [monothreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/allocation/report/index.html) and [multithreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/multi%20thread%20allocation/report/index.html)

#### Forward Message between Thread

 ObjectPool | 1 Reader - 1 Writter (ns) | 5 Reader - 1 Writter (ns) | 1 Reader - 5 Writter (ns) | 5 Reader - 5 Writter (ns)
 -----------| :-----------------------: | :-----------------------: | :-----------------------: | :-----------------------:
NoneObjectPool|529.75|290.47|926.05|722.35
MutexObjectPool|429.29|207.17|909.88|409.99
SpinLockObjectPool|34.277|182.62|1089.7|483.81
LinearObjectPool|43.876|163.18|365.56|326.92
[`crate 'sharded-slab'`]|525.82|775.79|966.87|1289.2

Not supported by [`crate 'object-pool'`]

Report [1-1](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_1%20nb_readder_1)/report/index.html), [5-1](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_1%20nb_readder_5)/report/index.html), [1-5](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_5%20nb_readder_1)/report/index.html) , [5-5](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_5%20nb_readder_5)/report/index.html)

#### Desallocation

ObjectPool | Duration in Monothreading (ns) | Duration Multithreading (ns)
-----------| :----------------------------: | :--------------------------:
NoneObjectPool|111.81|93.585
MutexObjectPool|26.108|101.86
SpinLockObjectPool|22.441|50.107
LinearObjectPool|7.5379|41.707
[`crate 'sharded-slab'`]|7.0394|10.283
[`crate 'object-pool'`]|20.517|44.798

Report [monothreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/free/report/index.html) and [multithreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/multi%20thread%20free/report/index.html)

### Comparison with Similar Crates

* [`crate 'sharded-slab'`]: I like pull interface but i dislike 
  * Default / Reset trait because not enough flexible
  * Performance
  * create_owned method not use a reference on ```Self ```

* [`crate 'object-pool'`]: use a spinlock to sync and the performance are pretty good but i dislike :
  * need to specify fallback at each pull call :
  ```rust
  use object_pool::Pool;
  let pool = Pool::<Vec<u8>>::new(32, || Vec::with_capacity(4096);
  // ...
  let item1 = pool.pull(|| Vec::with_capacity(4096));
  // ...
  let item2 = pool.pull(|| Vec::with_capacity(4096));
  ```
  * no reset mechanism, need to do manually
  * no possiblity to forward data between thread

### TODO

* why the object-pool with spinlock has so bad performance compared to spinlock mutex use by [`crate 'object-pool'`]
* impl a tree object pool (cf [`toolsbox`])

### Implementation detail

TODO

### Licence

cf [Boost Licence](http://www.boost.org/LICENSE_1_0.txt)

### Related Projects

- [`crate 'object-pool'`] - A thread-safe object pool in rust with mutex 
- [`crate 'sharded-slab'`] - A lock-free concurrent slab
- [`toolsbox`] - Some object pool implementation en c++

[`crate 'sharded-slab'`]: https://crates.io/crates/sharded-slab
[`crate 'object-pool'`]: https://crates.io/crates/object-pool
[`toolsbox`]: https://github.com/EVaillant/toolsbox
