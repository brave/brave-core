//! A thread-safe object pool collection with automatic return.
//!
//! Some implementations are lockfree :
//! * [`LinearObjectPool`]
//! * [`SpinLockObjectPool`]
//!
//! Other use std::Mutex :
//! * [`MutexObjectPool`]
//!
//! And [`NoneObjectPool`] basic allocation without pool.
//!
//! ## Example
//!
//! The general pool creation looks like this for
//! ```rust
//!   use lockfree_object_pool::LinearObjectPool;
//!   
//!   let pool = LinearObjectPool::<u32>::new(
//!     ||  Default::default(),
//!     |v| {*v = 0; });
//!
//!   // And use the object pool
//!   let mut item = pool.pull();
//!   *item = 5;
//! ```
//! At the end of the scope item return in object pool.
//! ## Multithreading
//!
//! All implementation support allocation/desallocation from on or more thread. You only need to wrap the pool in a [`std::sync::Arc`] :
//!
//! ```rust
//!   use lockfree_object_pool::LinearObjectPool;
//!   use std::sync::Arc;
//!
//!   let pool = Arc::new(LinearObjectPool::<u32>::new(
//!        ||  Default::default(),
//!        |v| {*v = 0; }));
//! ```
//! ## Performance
//!
//! Global [report](https://evaillant.github.io/lockfree-object-pool/benches/criterion/report/index.html).
//!
//!
//! #### Allocation
//!
//! ObjectPool | Duration in Monothreading (us) | Duration Multithreading (us)
//! ------------| :----------------------------: | :--------------------------:
//! [`NoneObjectPool`]|1.2848|0.62509
//! [`MutexObjectPool`]|1.3107|1.5178
//! [`SpinLockObjectPool`]|1.3106|1.3684
//! [`LinearObjectPool`]|0.23732|0.38913
//! [`crate 'sharded-slab'`]|1.6264|0.82607
//! [`crate 'object-pool'`]|0.77533|0.26224
//!  
//!  Report [monothreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/allocation/report/index.html) and [multithreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/multi%20thread%20allocation/report/index.html)
//!  
//!  #### Forward Message between Thread
//!  
//!   ObjectPool | 1 Reader - 1 Writter (ns) | 5 Reader - 1 Writter (ns) | 1 Reader - 5 Writter (ns) | 5 Reader - 5 Writter (ns)
//!   -----------| :-----------------------: | :-----------------------: | :-----------------------: | :-----------------------:
//!  [`NoneObjectPool`]|529.75|290.47|926.05|722.35
//!  [`MutexObjectPool`]|429.29|207.17|909.88|409.99
//!  [`SpinLockObjectPool`]|34.277|182.62|1089.7|483.81
//!  [`LinearObjectPool`]|43.876|163.18|365.56|326.92
//!  [`crate 'sharded-slab'`]|525.82|775.79|966.87|1289.2
//!  
//!  Not supported by [`crate 'object-pool'`]
//!  
//!  Report [1-1](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_1%20nb_readder_1)/report/index.html), [5-1](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_1%20nb_readder_5)/report/index.html), [1-5](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_5%20nb_readder_1)/report/index.html) , [5-5](https://evaillant.github.io/lockfree-object-pool/benches/criterion//forward%20msg%20from%20pull%20(nb_writter_5%20nb_readder_5)/report/index.html)
//!  
//!  #### Desallocation
//!  
//!  ObjectPool | Duration in Monothreading (ns) | Duration Multithreading (ns)
//!  -----------| :----------------------------: | :--------------------------:
//!  [`NoneObjectPool`]|111.81|93.585
//!  [`MutexObjectPool`]|26.108|101.86
//!  [`SpinLockObjectPool`]|22.441|50.107
//!  [`LinearObjectPool`]|7.5379|41.707
//!  [`crate 'sharded-slab'`]|7.0394|10.283
//!  [`crate 'object-pool'`]|20.517|44.798
//!  
//!  Report [monothreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/free/report/index.html) and [multithreading](https://evaillant.github.io/lockfree-object-pool/benches/criterion/multi%20thread%20free/report/index.html)
mod linear_object_pool;
mod linear_owned_reusable;
mod linear_page;
mod linear_reusable;
mod mutex_object_pool;
mod mutex_owned_reusable;
mod mutex_reusable;
mod none_object_pool;
mod none_reusable;
mod page;
mod spin_lock;
mod spin_lock_object_pool;
mod spin_lock_owned_reusable;
mod spin_lock_reusable;

pub use linear_object_pool::LinearObjectPool;
pub use linear_owned_reusable::LinearOwnedReusable;
pub use linear_reusable::LinearReusable;
pub use mutex_object_pool::MutexObjectPool;
pub use mutex_owned_reusable::MutexOwnedReusable;
pub use mutex_reusable::MutexReusable;
pub use none_object_pool::NoneObjectPool;
pub use none_reusable::NoneReusable;
pub use spin_lock_object_pool::SpinLockObjectPool;
pub use spin_lock_owned_reusable::SpinLockOwnedReusable;
pub use spin_lock_reusable::SpinLockReusable;
