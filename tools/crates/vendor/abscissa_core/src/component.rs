//! Application components: extensions/plugins for Abscissa applications.
//!
//! See docs on the `Component` trait for more information.

#![allow(unused_variables)]

mod handle;
mod id;
pub mod registry;

pub use self::{handle::Handle, id::Id, registry::Registry};
pub use abscissa_derive::Component;

use crate::{application::Application, shutdown::Shutdown, FrameworkError, Version};
use std::{any::Any, cmp::Ordering, fmt::Debug, slice::Iter};

/// Application components.
///
/// Components are Abscissa's primary extension mechanism, and are aware of
/// the application lifecycle. They are owned by the application as boxed trait
/// objects in a runtime type registry which is aware of a dependency ordering
/// and can (potentially in the future) support runtime reinitialization.
///
/// During application initialization, callbacks are sent to all components
/// upon events like application configuration being loaded. The
/// `register_dependency` callback is called for each dependency returned
/// by the `dependencies` method.
///
/// Additionally, they receive a callback prior to application shutdown.
///
/// ## Custom Derive
///
/// The main intended way to impl this trait is by using the built-in custom
/// derive functionality.
///
/// ```rust
/// use abscissa_core::Component;
///
/// #[derive(Component, Debug)]
/// pub struct MyComponent {}
/// ```
///
/// This will automatically implement the entire trait for you.
pub trait Component<A>: AsAny + Debug + Send + Sync
where
    A: Application,
{
    /// Identifier for this component.
    ///
    /// These are the Rust path (e.g. `crate_name:foo::Foo`) by convention.
    fn id(&self) -> Id;

    /// Version of this component
    fn version(&self) -> Version;

    /// Lifecycle event called when application configuration should be loaded
    /// if it were possible.
    fn after_config(&mut self, config: &A::Cfg) -> Result<(), FrameworkError> {
        Ok(())
    }

    /// Names of the components this component depends on.
    ///
    /// After this app's `after_config` callback is fired, the
    /// `register_dependency` callback below will be fired for each of these.
    fn dependencies(&self) -> Iter<'_, Id> {
        [].iter()
    }

    /// Register a dependency of this component (a.k.a. "dependency injection")
    fn register_dependency(
        &mut self,
        handle: Handle,
        dependency: &mut dyn Component<A>,
    ) -> Result<(), FrameworkError> {
        unimplemented!();
    }

    /// Perform any tasks which should occur before the app exits
    fn before_shutdown(&self, kind: Shutdown) -> Result<(), FrameworkError> {
        Ok(())
    }
}

impl<A> PartialEq for Box<dyn Component<A>>
where
    A: Application,
{
    fn eq(&self, other: &Self) -> bool {
        self.id() == other.id()
    }
}

impl<A> PartialOrd for Box<dyn Component<A>>
where
    A: Application,
{
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        if other.dependencies().any(|dep| *dep == self.id()) {
            if self.dependencies().any(|dep| *dep == other.id()) {
                None
            } else {
                Some(Ordering::Greater)
            }
        } else if self.dependencies().any(|dep| *dep == other.id()) {
            Some(Ordering::Less)
        } else {
            Some(Ordering::Equal)
        }
    }
}

/// Dynamic type helper trait
// TODO(tarcieri): eliminate this trait or hide it from the public API
pub trait AsAny: Any {
    /// Borrow this concrete type as a `&dyn Any`
    fn as_any(&self) -> &dyn Any;

    /// Borrow this concrete type as a `&mut dyn Any`
    fn as_mut_any(&mut self) -> &mut dyn Any;
}

impl<T> AsAny for T
where
    T: Any,
{
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_mut_any(&mut self) -> &mut dyn Any {
        self
    }
}
