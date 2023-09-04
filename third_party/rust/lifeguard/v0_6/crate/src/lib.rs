#[allow(dead_code)]
use std::rc::Rc;
use std::cell::RefCell;
use std::fmt;
use std::ops::{Drop, Deref, DerefMut};
use std::convert::{AsRef, AsMut};
use std::cmp::{Ord, PartialOrd, PartialEq, Eq, Ordering};
use std::hash::{Hash, Hasher};
use std::borrow::Borrow;
use std::collections::VecDeque;
use std::mem::ManuallyDrop;
use std::ptr;

/// In order to be managed by a `Pool`, values must be of a type that
/// implements the `Recycleable` trait. This allows the `Pool` to create
/// new instances as well as reset existing instances to a like-new state.
pub trait Recycleable {
  /// Allocates a new instance of the implementing type.
  fn new() -> Self;
  /// Sets the state of the modified instance to be that of a freshly
  /// allocated instance, thereby allowing it to be reused.
  fn reset(&mut self);
}

/// Informs how an already allocated value should be initialized 
/// when provided with a model value or other meaningful input.
pub trait InitializeWith<T> {
  fn initialize_with(&mut self, source: T);
}

impl Recycleable for String {
  #[inline] 
  fn new() -> String {
    String::new()
  }
  #[inline] 
  fn reset(&mut self) {
    self.clear();
  }
}

impl <T> Recycleable for Vec<T> {
  #[inline] 
  fn new() -> Vec<T> {
    Vec::new()
  }
  #[inline] 
  fn reset(&mut self) {
    self.clear();
  }
}

impl <T> Recycleable for VecDeque<T> {
  #[inline] 
  fn new() -> VecDeque<T> {
    VecDeque::new()
  }
  #[inline] 
  fn reset(&mut self) {
    self.clear();
  }
}

impl <A> InitializeWith<A> for String where A : AsRef<str> {
  #[inline] 
  fn initialize_with(&mut self, source: A) {
    let s : &str = source.as_ref();
    self.push_str(s);
  }
}

impl <I, T> InitializeWith<I> for Vec<T> where I: Iterator<Item=T>{
    #[inline]
    fn initialize_with(&mut self, source: I) {
        self.extend(source);
    }
}

/// A smartpointer which uses a shared reference (`&`) to know
/// when to move its wrapped value back to the `Pool` that
/// issued it.
pub struct Recycled<'a, T: 'a> where T: Recycleable {
  value: RecycledInner<&'a RefCell<CappedCollection<T>>, T>
}

/// A smartpointer which uses reference counting (`Rc`) to know
/// when to move its wrapped value back to the `Pool` that
/// issued it.
pub struct RcRecycled<T> where T: Recycleable {
  value: RecycledInner<Rc<RefCell<CappedCollection<T>>>, T>
}

macro_rules! impl_recycled {
  ($name: ident, $typ: ty, $pool: ty) => {
  impl <'a, T> AsRef<T> for $typ where T : Recycleable {
     /// Gets a shared reference to the value wrapped by the smartpointer.
     fn as_ref(&self) -> &T {
      self.value.as_ref()
    }
  }

  impl <'a, T> AsMut<T> for $typ where T : Recycleable {
     /// Gets a mutable reference to the value wrapped by the smartpointer.
     fn as_mut(&mut self) -> &mut T {
      self.value.as_mut()
    }
  }

  impl <'a, T> fmt::Debug for $typ where T : fmt::Debug + Recycleable {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
      self.value.fmt(f)
    }
  }

  impl <'a, T> fmt::Display for $typ where T : fmt::Display + Recycleable {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
      self.value.fmt(f)
    }
  }

  //-------- Passthrough trait implementations -----------

  impl <'a, T> PartialEq for $typ where T : PartialEq + Recycleable {
    fn eq(&self, other: &Self) -> bool {
      self.value.eq(&other.value)
    }
  }

  impl <'a, T> Eq for $typ where T: Eq + Recycleable {}

  impl <'a, T> PartialOrd for $typ where T: PartialOrd + Recycleable {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
      self.value.partial_cmp(&other.value)
    }
  }

  impl <'a, T> Ord for $typ where T: Ord + Recycleable {
    fn cmp(&self, other: &Self) -> Ordering {
      self.value.cmp(&other.value)
    }
  }

  impl <'a, T> Hash for $typ where T: Hash + Recycleable {
    fn hash<H: Hasher>(&self, state: &mut H) {
      self.value.hash(state)
    }
  }

  //------------------------------------------------------

  impl <'a, T> Deref for $typ where T : Recycleable {
    type Target = T;
    #[inline] 
    fn deref(&self) -> &T {
      self.as_ref()
    }
  }

  impl <'a, T> DerefMut for $typ where T : Recycleable {
    #[inline] 
    fn deref_mut(&mut self) -> &mut T {
      self.as_mut()
    }
  }

  impl <'a, T> $typ where T: Recycleable {
    fn new(pool: $pool, value: T) -> $typ {
      $name { value: RecycledInner::new(pool, value) }
    }
    
    #[inline] 
    fn new_from<A>(pool: $pool, value: T, source: A) -> $typ where T : InitializeWith<A> {
      $name { value: RecycledInner::new_from(pool, value, source) }
    }

    #[inline] 
    /// Disassociates the value from the `Pool` that issued it. This
    /// destroys the smartpointer and returns the previously wrapped value.
    pub fn detach(self) -> T {
      self.value.detach()
    }
  }
}
}
impl_recycled!{ RcRecycled, RcRecycled<T>, Rc<RefCell<CappedCollection<T>>> }
impl_recycled!{ Recycled, Recycled<'a, T>, &'a RefCell<CappedCollection<T>> }

impl <T> Clone for RcRecycled<T> where T: Clone + Recycleable {
  fn clone(&self) -> Self {
    RcRecycled {
      value: self.value.clone()
    }
  }
}

impl <'a, T> Clone for Recycled<'a, T> where T: Clone + Recycleable {
  fn clone(&self) -> Self {
    Recycled {
      value: self.value.clone()
    }
  }
}

struct RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
  value: ManuallyDrop<T>,
  pool: P
}

// ---------- Passthrough Trait Implementations ------------

impl <P, T> PartialEq for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>,
                                                    T: PartialEq + Recycleable {
  fn eq(&self, other: &Self) -> bool {
    self.value.eq(&other.value)
  }
}

impl <P, T> Eq for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>,
                                             T: Eq + Recycleable {

}

impl <P, T> PartialOrd for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>,
                                                     T: PartialOrd + Recycleable {
  fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
    self.value.partial_cmp(&other.value)
  }
}

impl <P, T> Ord for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>,
                                              T: Ord + Recycleable {
  fn cmp(&self, other: &Self) -> Ordering {
    self.value.cmp(&other.value)
  }
}

impl <P, T> Hash for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>,
                                               T: Hash + Recycleable {
  fn hash<H: Hasher>(&self, state: &mut H) {
    self.value.hash(state)
  }
}

// Implementing Clone requires duplicating our shared reference to the capped collection, so we have
// to provide separate implementations for RecycledInners used in Recycled and RcRecycled values.
impl <'a, T> Clone for RecycledInner<&'a RefCell<CappedCollection<T>>, T> where T: Clone + Recycleable {
  fn clone(&self) -> Self {
    let pool_ref = &*self.pool;
    let mut cloned_value = pool_ref.borrow_mut().remove_or_create();
    cloned_value.clone_from(&self.value);
    RecycledInner {
      value: ManuallyDrop::new(cloned_value),
      pool: pool_ref
    }
  }
}

impl <T> Clone for RecycledInner<Rc<RefCell<CappedCollection<T>>>, T> where T: Clone + Recycleable {
  fn clone(&self) -> Self {
    let pool_ref = self.pool.clone();
    let mut cloned_value = pool_ref.borrow_mut().remove_or_create();
    cloned_value.clone_from(&self.value);
    RecycledInner {
      value: ManuallyDrop::new(cloned_value),
      pool: pool_ref
    }
  }
}

// -------------------------------------------------------------

impl <P, T> Drop for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
  #[inline] 
  fn drop(&mut self) {
    // We need to rescue `self.value` from `self` and then allow `self` to drop normally.
    let value = unsafe {
      // Make a byte-for-byte copy of `self.value`
      ptr::read(&self.value as *const ManuallyDrop<T>)
      // Because its type is ManuallyDrop<T>, the original value inside of `self` will not be
      // dropped when `self` drops.
    };

    // Convert our newly-rescued `ManuallyDrop<T>` into a `T` so it will eventually drop normally.
    let mut value = ManuallyDrop::into_inner(value);

    // If there's no room left in the pool, drop the value here.
    let pool_ref = self.pool.borrow();
    if pool_ref.borrow().is_full() {
      drop(value);
      return;
    }

    // Otherwise, reset value and return it to the pool.
    value.reset();
    pool_ref.borrow_mut().insert_prepared_value(value);
  }
}

impl <P, T> AsRef<T> for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
   fn as_ref(&self) -> &T {
     &self.value
  }
}

impl <P, T> AsMut<T> for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
   fn as_mut(&mut self) -> &mut T {
     &mut self.value
  }
}

impl <P, T> fmt::Debug for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : fmt::Debug + Recycleable {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    self.value.fmt(f)
  }
}

impl <P, T> fmt::Display for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : fmt::Display + Recycleable {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    self.value.fmt(f)
  }
}

impl <P, T> Deref for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
  type Target = T;
  #[inline] 
  fn deref(& self) -> &T {
    self.as_ref()
  }
}

impl <P, T> DerefMut for RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
  #[inline] 
  fn deref_mut(&mut self) -> & mut T {
    self.as_mut()
  }
}

impl <P, T> RecycledInner<P, T> where P: Borrow<RefCell<CappedCollection<T>>>, T : Recycleable {
  #[inline] 
  fn new(pool: P, value: T) -> RecycledInner<P, T> {
    RecycledInner {
      value: ManuallyDrop::new(value),
      pool
    }
  }
  
  #[inline] 
  fn new_from<A>(pool: P, mut value: T, source: A) -> RecycledInner<P, T> where T : InitializeWith<A> {
    value.initialize_with(source);
    RecycledInner {
      value: ManuallyDrop::new(value),
      pool
    }
  }

  #[inline]
  fn detach(self) -> T {
    // We need to split `self` into its component `value` and `pool` fields, drop the pool,
    // return the value, and forget `self` since its fields are now unwanted clones.
    let value = unsafe {
      // Make a byte-for-byte copy of `self.value`
      ptr::read(&self.value as *const ManuallyDrop<T>)
    };
    let pool = unsafe {
      // Make a byte-for-byte copy of `self.pool`
      ptr::read(&self.pool as *const P)
    };
    // Forget `self` so it doesn't go through our custom `Drop` implementation
    mem::forget(self);
    // Allow `pool` to drop normally
    drop(pool);
    // Return the only surviving copy of `value`
    ManuallyDrop::into_inner(value)
  }
}

struct CappedCollection <T> where T: Recycleable {
  values: Vec<T>,
  cap: usize,
  supplier: Box<dyn Supply<Output=T>>
}

impl <T> CappedCollection <T> where T: Recycleable {
  #[inline]
  pub fn new(mut supplier: Box<dyn Supply<Output=T>>, starting_size: usize, max_size: usize) -> CappedCollection<T> {
    use std::cmp;
    let starting_size = cmp::min(starting_size, max_size);
    let values: Vec<T> = 
      (0..starting_size)
      .map(|_| supplier.get() )
      .collect();
    CappedCollection {
      values: values,
      cap: max_size,
      supplier: supplier
    }
  }

  /// Note: This method does not perform a length check.
  /// The provided value must be reset() and there must be room in the pool before this is called.
  #[inline]
  pub fn insert_prepared_value(&mut self, value: T) {
    self.values.push(value)
  }

  #[inline]
  pub fn remove(&mut self) -> Option<T> {
    self.values.pop()
  }

  #[inline]
  pub fn remove_or_create(&mut self) -> T {
    match self.remove() {
      Some(value) => value,
      None => self.supplier.get()
    }
  }

  #[inline]
  pub fn is_full(&self) -> bool {
    self.values.len() >= self.cap
  }
  
  #[inline]
  pub fn len(&self) -> usize {
    self.values.len()
  }
  
  #[inline]
  pub fn cap(&self) -> usize {
    self.cap
  }
}

/// Provides a method which will produce new instances of a type
pub trait Supply {
  type Output: Recycleable;

  fn get(&mut self) -> Self::Output;
}

impl <F, T> Supply for F where F: FnMut() -> T, T: Recycleable {
  type Output = T;
  fn get(&mut self) -> T {
    self()
  }
}

/// A collection of values that can be reused without requiring new allocations.
/// 
/// `Pool` issues each value wrapped in a smartpointer. When the smartpointer goes out of
/// scope, the wrapped value is automatically returned to the pool.
pub struct Pool <T> where T : Recycleable {
  values: Rc<RefCell<CappedCollection<T>>>,
}

impl <T> Pool <T> where T: Recycleable {

  /// Creates a pool with `size` elements of type `T` allocated.
  #[inline]
  pub fn with_size(size: usize) -> Pool <T> {
    Pool::with_size_and_max(size, usize::MAX)
  }

  /// Creates a pool with `size` elements of type `T` allocated
  /// and sets a maximum pool size of `max_size`. Values being
  /// added to the pool via `Pool::attach` or being returned to
  /// the pool upon dropping will instead be discarded if the pool
  /// is full.
  #[inline]
  pub fn with_size_and_max(starting_size: usize, max_size: usize) -> Pool <T> {
    let supplier = Box::new(|| T::new());
    let values: CappedCollection<T> = CappedCollection::new(supplier, starting_size, max_size);
    Pool {
      values: Rc::new(RefCell::new(values))
    }
  }

  /// Returns the number of values remaining in the pool.
  #[inline] 
  pub fn size(&self) -> usize {
    (*self.values).borrow().len()
  }
  
  /// Returns the maximum number of values the pool can hold.
  #[inline] 
  pub fn max_size(&self) -> usize {
    (*self.values).borrow().cap()
  }

  /// Removes a value from the pool and returns it wrapped in
  /// a `Recycled smartpointer. If the pool is empty when the
  /// method is called, a new value will be allocated.
  #[inline] 
  pub fn new(&self) -> Recycled<T> {
    let t = self.detached();
    Recycled { value: RecycledInner::new(&*self.values, t) }
  }

  /// Removes a value from the pool, initializes it using the provided
  /// source value, and returns it wrapped in a `Recycled` smartpointer.
  /// If the pool is empty when the method is called, a new value will be
  /// allocated.
  #[inline(always)] 
  pub fn new_from<A>(&self, source: A) -> Recycled<T> where T: InitializeWith<A> {
    let t = self.detached();
    Recycled { value: RecycledInner::new_from(&*self.values, t, source) }
  }

  /// Associates the provided value with the pool by wrapping it in a
  /// `Recycled` smartpointer.
  #[inline] 
  pub fn attach(&self, value: T) -> Recycled<T> {
    Recycled { value: RecycledInner::new(&*self.values, value) }
  }

  /// Removes a value from the pool and returns it without wrapping it in
  /// a smartpointer. When the value goes out of scope it will not be
  /// returned to the pool.
  #[inline] 
  pub fn detached(&self) -> T {
    let mut collection = self.values.borrow_mut();
    collection.remove_or_create()
  }

  /// Removes a value from the pool and returns it wrapped in
  /// an `RcRecycled` smartpointer. If the pool is empty when the
  /// method is called, a new value will be allocated.
  #[inline] 
  pub fn new_rc(&self) -> RcRecycled<T> {
    let t = self.detached();
    let pool_reference = self.values.clone();
    RcRecycled { value: RecycledInner::new(pool_reference, t) }
  }
 
  /// Removes a value from the pool, initializes it using the provided
  /// source value, and returns it wrapped in an `RcRecycled` smartpointer.
  /// If the pool is empty when the method is called, a new value will be
  /// allocated.
  #[inline(always)] 
  pub fn new_rc_from<A>(&self, source: A) -> RcRecycled<T> where T: InitializeWith<A> {
    let t = self.detached();
    let pool_reference = self.values.clone();
    RcRecycled { value: RecycledInner::new_from(pool_reference, t, source) }
  }

  /// Associates the provided value with the pool by wrapping it in an
  /// `RcRecycled` smartpointer.
  #[inline] 
  pub fn attach_rc(&self, value: T) -> RcRecycled<T> {
    let pool_reference = self.values.clone();
    RcRecycled { value: RecycledInner::new(pool_reference, value) }
  }
}

/// Produces a `PoolBuilder` instance
/// 
/// # Example
/// 
/// ```
/// extern crate lifeguard;
/// use lifeguard::*;
///
/// fn main() {
///   let mut pool: Pool<String> = pool()
///     .with(StartingSize(128))
///     .with(MaxSize(4096))
///     .with(Supplier(|| String::with_capacity(1024)))
///     .build();
/// }
/// ```
pub fn pool<T>() -> PoolBuilder<T> where T: Recycleable {
  PoolBuilder {
    starting_size: 16,
    max_size: usize::MAX,
    supplier: None
  }
}

/// Used to define settings for and ultimately create a `Pool`.
pub struct PoolBuilder<T> where T: Recycleable {
  pub starting_size: usize,
  pub max_size: usize,
  pub supplier: Option<Box<dyn Supply<Output=T>>>,
}

impl <T> PoolBuilder<T> where T: Recycleable {
  pub fn with<U>(self, option_setter: U) -> PoolBuilder<T> where 
      U: OptionSetter<PoolBuilder<T>> {
    option_setter.set_option(self)
  }

  pub fn build(self) -> Pool<T> where T: Recycleable {
    let supplier = self.supplier.unwrap_or(Box::new(|| T::new()));
    let values: CappedCollection<T> = CappedCollection::new(supplier, self.starting_size, self.max_size);
    Pool {
      values: Rc::new(RefCell::new(values))
    }
  }
}

pub mod settings {
  use ::{PoolBuilder, Recycleable, Supply};
    /// Implementing this trait allows a struct to act as a configuration
    /// parameter in the builder API.
  pub trait OptionSetter<T> {
    fn set_option(self, T) -> T;
  }
  
    /// Specifies how many values should be requested from the Supplier at
    /// initialization time. These values will be available for immediate use.
  pub struct StartingSize(pub usize);
    /// Specifies the largest number of values the `Pool` will hold before it
    /// will begin to drop values being returned to it.
  pub struct MaxSize(pub usize);
    /// Specifies a value implementing `Supply<Output=T>` that will be used to allocate
    /// new values. If unspecified, `T::new()` will be invoked.
  pub struct Supplier<S>(pub S) where S: Supply;
  
  impl <T> OptionSetter<PoolBuilder<T>> for StartingSize where T: Recycleable {
    fn set_option(self, mut builder: PoolBuilder<T>) -> PoolBuilder<T> {
      let StartingSize(size) = self;
      builder.starting_size = size;
      builder
    }
  }
   
  impl <T> OptionSetter<PoolBuilder<T>> for MaxSize where T: Recycleable {
    fn set_option(self, mut builder: PoolBuilder<T>) -> PoolBuilder<T> {
      let MaxSize(size) = self;
      builder.max_size = size;
      builder
    }
  }
  
  impl <T, S> OptionSetter<PoolBuilder<T>> for Supplier<S> where
      S: Supply<Output=T> + 'static,
      T: Recycleable {
    fn set_option(self, mut builder: PoolBuilder<T>) -> PoolBuilder<T> {
      let Supplier(supplier) = self;
      builder.supplier = Some(Box::new(supplier) as Box<dyn Supply<Output=T>>);
      builder
    }
  }
}

pub use settings::{OptionSetter, StartingSize, MaxSize, Supplier};
use std::mem;
