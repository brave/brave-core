# getset

[![Download](https://img.shields.io/crates/d/getset)](https://crates.io/crates/getset)
[![License](https://img.shields.io/crates/l/getset)](https://github.com/Hoverbear/getset/blob/master/LICENSE)
[![Docs](https://docs.rs/getset/badge.svg)](https://docs.rs/getset/)
[![Coverage Status](https://coveralls.io/repos/github/Hoverbear/getset/badge.svg)](https://coveralls.io/github/Hoverbear/getset)

Getset, we're ready to go!

A procedural macro for generating the most basic getters and setters on fields.

Getters are generated as `fn field(&self) -> &type`, while setters are generated as `fn field(&mut self, val: type)`.

These macros are not intended to be used on fields which require custom logic inside of their setters and getters. Just write your own in that case!

```rust
use std::sync::Arc;

use getset::{CloneGetters, CopyGetters, Getters, MutGetters, Setters, WithSetters};

#[derive(Getters, Setters, WithSetters, MutGetters, CopyGetters, CloneGetters, Default)]
pub struct Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get, set, get_mut, set_with)]
    private: T,

    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get_copy = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    public: T,

    /// Arc supported through CloneGetters
    #[getset(get_clone = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    arc: Arc<u16>,
}

fn main() {
    let mut foo = Foo::default();
    foo.set_private(1);
    (*foo.private_mut()) += 1;
    assert_eq!(*foo.private(), 2);
    foo = foo.with_private(3);
    assert_eq!(*foo.private(), 3);
    assert_eq!(*foo.arc(), 0);
}
```

You can use `cargo-expand` to generate the output. Here are the functions that the above generates (Replicate with `cargo expand --example simple`):

```rust
use std::sync::Arc;                                        
use getset::{CloneGetters, CopyGetters, Getters, MutGetters, Setters, WithSetters};                                    
pub struct Foo<T>                                          
where                                                      
    T: Copy + Clone + Default,                             
{                                                          
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[getset(get, set, get_mut, set_with)]                 
    private: T,                                            
    /// Doc comments are supported!                                                                                    
    /// Multiline, even.                                   
    #[getset(get_copy = "pub", set = "pub", get_mut = "pub", set_with = "pub")]                                        
    public: T,                                             
    /// Arc supported through CloneGetters                 
    #[getset(get_clone = "pub", set = "pub", get_mut = "pub", set_with = "pub")]                                       
    arc: Arc<u16>,                                         
}                                                          
impl<T> Foo<T>                                             
where                                                      
    T: Copy + Clone + Default,                             
{                                                          
    /// Doc comments are supported!                        
    /// Multiline, even.                                                                                               
    #[inline(always)]                                      
    fn private(&self) -> &T {                                                                                          
        &self.private                                      
    }                                                      
}                                                          
impl<T> Foo<T>                                             
where                                                      
    T: Copy + Clone + Default,                             
{                                                          
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[inline(always)]                                      
    fn set_private(&mut self, val: T) -> &mut Self {       
        self.private = val;
        self                                                                                                           
    }                                                      
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[inline(always)]                                                                                                  
    pub fn set_public(&mut self, val: T) -> &mut Self {    
        self.public = val;                                 
        self                                                                                                           
    }                                                      
    /// Arc supported through CloneGetters                 
    #[inline(always)]                                      
    pub fn set_arc(&mut self, val: Arc<u16>) -> &mut Self {
        self.arc = val;                                                                                                
        self                                               
    }                                                      
}                                                          
impl<T> Foo<T>                                             
where                                                      
    T: Copy + Clone + Default,                             
{                                                          
    /// Doc comments are supported!                        
    /// Multiline, even.                                                                                               
    #[inline(always)]                                      
    fn with_private(mut self, val: T) -> Self {                                                                        
        self.private = val;                                
        self                                               
    }                                                                                                                  
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[inline(always)]                                      
    pub fn with_public(mut self, val: T) -> Self {         
        self.public = val;                                 
        self                                               
    }                                                      
    /// Arc supported through CloneGetters                                                                             
    #[inline(always)]                                      
    pub fn with_arc(mut self, val: Arc<u16>) -> Self {                                                                 
        self.arc = val;                                    
        self                                               
    }                                                      
}                                                          
impl<T> Foo<T>                                             
where                                                      
    T: Copy + Clone + Default,                             
{                                                          
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[inline(always)]                                      
    fn private_mut(&mut self) -> &mut T {
        &mut self.private                                                                                              
    }                                                      
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[inline(always)]                                                                                                  
    pub fn public_mut(&mut self) -> &mut T {               
        &mut self.public                                   
    }                                                                                                                  
    /// Arc supported through CloneGetters                 
    #[inline(always)]                                      
    pub fn arc_mut(&mut self) -> &mut Arc<u16> {           
        &mut self.arc                                      
    }                                                                                                                  
}                                                          
impl<T> Foo<T>                                             
where                                                      
    T: Copy + Clone + Default,                             
{                                                          
    /// Doc comments are supported!                        
    /// Multiline, even.                                   
    #[inline(always)]                                      
    pub fn public(&self) -> T {                                                                                        
        self.public                                        
    }                                                                                                                  
}                                                          
impl<T> Foo<T>                                             
where                                                                                                                  
    T: Copy + Clone + Default,                             
{                                                          
    /// Arc supported through CloneGetters                 
    #[inline(always)]                                      
    pub fn arc(&self) -> Arc<u16> {                        
        self.arc.clone()                                   
    }                                                      
}
```

Attributes can be set on struct level for all fields in struct as well. Field level attributes take
precedence.

```rust
#[macro_use]
extern crate getset;

mod submodule {
    #[derive(Getters, CopyGetters, Default)]
    #[get_copy = "pub"] // By default add a pub getting for all fields.
    pub struct Foo {
        public: i32,
        #[get_copy] // Override as private
        private: i32,
    }
    fn demo() {
        let mut foo = Foo::default();
        foo.private();
    }
}
fn main() {
    let mut foo = submodule::Foo::default();
    foo.public();
}
```

For some purposes, it's useful to have the `get_` prefix on the getters for
either legacy of compatability reasons. It is done with `with_prefix`.

```rust
#[macro_use]
extern crate getset;

#[derive(Getters, Default)]
pub struct Foo {
    #[get = "pub with_prefix"]
    field: bool,
}

fn main() {
    let mut foo = Foo::default();
    let val = foo.get_field();
}
```

Skipping setters and getters generation for a field when struct level attribute is used
is possible with `#[getset(skip)]`.

```rust
use getset::{CopyGetters, Setters};

#[derive(CopyGetters, Setters)]
#[getset(get_copy, set, set_with)]
pub struct Foo {
    // If the field was not skipped, the compiler would complain about moving
    // a non-copyable type in copy getter.
    #[getset(skip)]
    skipped: String,

    field1: usize,
    field2: usize,
}

impl Foo {
    // It is possible to write getters and setters manually,
    // possibly with a custom logic.
    fn skipped(&self) -> &str {
        &self.skipped
    }

    fn set_skipped(&mut self, val: &str) -> &mut Self {
        self.skipped = val.to_string();
        self
    }

    fn with_skipped(mut self, val: &str) -> Self {
        self.skipped = val.to_string();
        self
    }
}
```
