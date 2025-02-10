use std::time::Instant;

use crate::{messages::MessageLevel, progress, progress::Id, Unit};

/// A trait for describing hierarchical progress.
pub trait NestedProgress: Progress {
    /// The type of progress returned by [`add_child()`][Progress::add_child()].
    type SubProgress: NestedProgress;

    /// Adds a new child, whose parent is this instance, with the given `name`.
    ///
    /// This will make the child progress to appear contained in the parent progress.
    /// Note that such progress does not have a stable identifier, which can be added
    /// with [`add_child_with_id()`][Progress::add_child_with_id()] if desired.
    fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress;

    /// Adds a new child, whose parent is this instance, with the given `name` and `id`.
    ///
    /// This will make the child progress to appear contained in the parent progress, and it can be identified
    /// using `id`.
    fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress;
}

/// A thread-safe read-only counter, with unknown limits.
pub trait Count {
    /// Set the current progress to the given `step`. The cost of this call is negligible,
    /// making manual throttling *not* necessary.
    ///
    /// **Note**: that this call has no effect unless `init(…)` was called before.
    fn set(&self, step: progress::Step);

    /// Returns the current step, as controlled by `inc*(…)` calls
    fn step(&self) -> progress::Step;

    /// Increment the current progress to the given `step`.
    /// The cost of this call is negligible, making manual throttling *not* necessary.
    fn inc_by(&self, step: progress::Step);

    /// Increment the current progress to the given 1. The cost of this call is negligible,
    /// making manual throttling *not* necessary.
    fn inc(&self) {
        self.inc_by(1)
    }

    /// Return an atomic counter for direct access to the underlying state.
    ///
    /// This is useful if multiple threads want to access the same progress, without the need
    /// for provide each their own progress and aggregating the result.
    fn counter(&self) -> StepShared;
}

/// An object-safe trait for describing hierarchical progress.
///
/// This will be automatically implemented for any type that implements
/// [`NestedProgress`].
pub trait DynNestedProgress: Progress + impls::Sealed {
    /// See [`NestedProgress::add_child`]
    fn add_child(&mut self, name: String) -> BoxedDynNestedProgress;

    /// See [`NestedProgress::add_child_with_id`]
    fn add_child_with_id(&mut self, name: String, id: Id) -> BoxedDynNestedProgress;
}

/// An opaque type for storing [`DynNestedProgress`].
pub struct BoxedDynNestedProgress(Box<dyn DynNestedProgress>);

/// An owned version of [`Progress`] which can itself implement said trait.
pub type BoxedProgress = Box<dyn Progress>;

/// A bridge type that implements [`NestedProgress`] for any type that implements [`DynNestedProgress`].
pub struct DynNestedProgressToNestedProgress<T: ?Sized>(pub T);

/// A trait for describing non-hierarchical progress.
///
/// It differs by not being able to add child progress dynamically, but in turn is object safe. It's recommended to
/// use this trait whenever there is no need to add child progress, at the leaf of a computation.
// NOTE: keep this in-sync with `Progress`.
pub trait Progress: Count + Send + Sync {
    /// Initialize the Item for receiving progress information.
    ///
    /// If `max` is `Some(…)`, it will be treated as upper bound. When progress is [set(…)](./struct.Item.html#method.set)
    /// it should not exceed the given maximum.
    /// If `max` is `None`, the progress is unbounded. Use this if the amount of work cannot accurately
    /// be determined in advance.
    ///
    /// If `unit` is `Some(…)`, it is used for display purposes only. See `prodash::Unit` for more information.
    ///
    /// If both `unit` and `max` are `None`, the item will be reset to be equivalent to 'uninitialized'.
    ///
    /// If this method is never called, this `Progress` instance will serve as organizational unit, useful to add more structure
    /// to the progress tree (e.g. a headline).
    ///
    /// **Note** that this method can be called multiple times, changing the bounded-ness and unit at will.
    fn init(&mut self, max: Option<progress::Step>, unit: Option<Unit>);

    /// Returns the (cloned) unit associated with this Progress
    fn unit(&self) -> Option<Unit> {
        None
    }

    /// Returns the maximum about of items we expect, as provided with the `init(…)` call
    fn max(&self) -> Option<progress::Step> {
        None
    }

    /// Set the maximum value to `max` and return the old maximum value.
    fn set_max(&mut self, _max: Option<progress::Step>) -> Option<progress::Step> {
        None
    }

    /// Set the name of the instance, altering the value given when crating it with `add_child(…)`
    /// The progress is allowed to discard it.
    fn set_name(&mut self, name: String);

    /// Get the name of the instance as given when creating it with `add_child(…)`
    /// The progress is allowed to not be named, thus there is no guarantee that a previously set names 'sticks'.
    fn name(&self) -> Option<String>;

    /// Get a stable identifier for the progress instance.
    /// Note that it could be [unknown][crate::progress::UNKNOWN].
    fn id(&self) -> Id;

    /// Create a `message` of the given `level` and store it with the progress tree.
    ///
    /// Use this to provide additional,human-readable information about the progress
    /// made, including indicating success or failure.
    fn message(&self, level: MessageLevel, message: String);

    /// Create a message providing additional information about the progress thus far.
    fn info(&self, message: String) {
        self.message(MessageLevel::Info, message)
    }
    /// Create a message indicating the task is done successfully
    fn done(&self, message: String) {
        self.message(MessageLevel::Success, message)
    }
    /// Create a message indicating the task failed
    fn fail(&self, message: String) {
        self.message(MessageLevel::Failure, message)
    }
    /// A shorthand to print throughput information
    fn show_throughput(&self, start: Instant) {
        let step = self.step();
        match self.unit() {
            Some(unit) => self.show_throughput_with(start, step, unit, MessageLevel::Info),
            None => {
                let elapsed = start.elapsed().as_secs_f32();
                let steps_per_second = (step as f32 / elapsed) as progress::Step;
                self.info(format!(
                    "done {} items in {:.02}s ({} items/s)",
                    step, elapsed, steps_per_second
                ))
            }
        };
    }

    /// A shorthand to print throughput information, with the given step and unit, and message level.
    fn show_throughput_with(&self, start: Instant, step: progress::Step, unit: Unit, level: MessageLevel) {
        use std::fmt::Write;
        let elapsed = start.elapsed().as_secs_f32();
        let steps_per_second = (step as f32 / elapsed) as progress::Step;
        let mut buf = String::with_capacity(128);
        let unit = unit.as_display_value();
        let push_unit = |buf: &mut String| {
            buf.push(' ');
            let len_before_unit = buf.len();
            unit.display_unit(buf, step).ok();
            if buf.len() == len_before_unit {
                buf.pop();
            }
        };

        buf.push_str("done ");
        unit.display_current_value(&mut buf, step, None).ok();
        push_unit(&mut buf);

        buf.write_fmt(format_args!(" in {:.02}s (", elapsed)).ok();
        unit.display_current_value(&mut buf, steps_per_second, None).ok();
        push_unit(&mut buf);
        buf.push_str("/s)");

        self.message(level, buf);
    }
}

use crate::{
    messages::{Message, MessageCopyState},
    progress::StepShared,
};

/// The top-level root as weak handle, which needs an upgrade to become a usable root.
///
/// If the underlying reference isn't present anymore, such upgrade will fail permanently.
pub trait WeakRoot {
    /// The type implementing the `Root` trait
    type Root: Root;

    /// Equivalent to `std::sync::Weak::upgrade()`.
    fn upgrade(&self) -> Option<Self::Root>;
}

/// The top level of a progress task hierarchy, with `progress::Task`s identified with `progress::Key`s
pub trait Root {
    /// The type implementing the `WeakRoot` trait
    type WeakRoot: WeakRoot;

    /// Returns the maximum amount of messages we can keep before overwriting older ones.
    fn messages_capacity(&self) -> usize;

    /// Returns the current amount of tasks underneath the root, transitively.
    /// **Note** that this is at most a guess as tasks can be added and removed in parallel.
    fn num_tasks(&self) -> usize;

    /// Copy the entire progress tree into the given `out` vector, so that
    /// it can be traversed from beginning to end in order of hierarchy.
    /// The `out` vec will be cleared automatically.
    fn sorted_snapshot(&self, out: &mut Vec<(progress::Key, progress::Task)>);

    /// Copy all messages from the internal ring buffer into the given `out`
    /// vector. Messages are ordered from oldest to newest.
    fn copy_messages(&self, out: &mut Vec<Message>);

    /// Copy only new messages from the internal ring buffer into the given `out`
    /// vector. Messages are ordered from oldest to newest.
    fn copy_new_messages(&self, out: &mut Vec<Message>, prev: Option<MessageCopyState>) -> MessageCopyState;

    /// Similar to `Arc::downgrade()`
    fn downgrade(&self) -> Self::WeakRoot;
}

mod impls {
    use std::{
        ops::{Deref, DerefMut},
        time::Instant,
    };

    use crate::traits::{BoxedProgress, Progress};
    use crate::{
        messages::MessageLevel,
        progress::{Id, Step, StepShared},
        BoxedDynNestedProgress, Count, DynNestedProgress, DynNestedProgressToNestedProgress, NestedProgress, Unit,
    };

    pub trait Sealed {}

    impl<'a, T> Count for &'a T
    where
        T: Count + ?Sized,
    {
        fn set(&self, step: Step) {
            (*self).set(step)
        }

        fn step(&self) -> Step {
            (*self).step()
        }

        fn inc_by(&self, step: Step) {
            (*self).inc_by(step)
        }

        fn inc(&self) {
            (*self).inc()
        }

        fn counter(&self) -> StepShared {
            (*self).counter()
        }
    }

    impl<'a, T> Count for &'a mut T
    where
        T: Count + ?Sized,
    {
        fn set(&self, step: Step) {
            self.deref().set(step)
        }

        fn step(&self) -> Step {
            self.deref().step()
        }

        fn inc_by(&self, step: Step) {
            self.deref().inc_by(step)
        }

        fn inc(&self) {
            self.deref().inc()
        }

        fn counter(&self) -> StepShared {
            self.deref().counter()
        }
    }

    impl<'a, T> Progress for &'a mut T
    where
        T: Progress + ?Sized,
    {
        fn init(&mut self, max: Option<Step>, unit: Option<Unit>) {
            self.deref_mut().init(max, unit)
        }

        fn unit(&self) -> Option<Unit> {
            self.deref().unit()
        }

        fn max(&self) -> Option<Step> {
            self.deref().max()
        }

        fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
            self.deref_mut().set_max(max)
        }

        fn set_name(&mut self, name: String) {
            self.deref_mut().set_name(name)
        }

        fn name(&self) -> Option<String> {
            self.deref().name()
        }

        fn id(&self) -> Id {
            self.deref().id()
        }

        fn message(&self, level: MessageLevel, message: String) {
            self.deref().message(level, message)
        }

        fn info(&self, message: String) {
            self.deref().info(message)
        }

        fn done(&self, message: String) {
            self.deref().done(message)
        }

        fn fail(&self, message: String) {
            self.deref().fail(message)
        }

        fn show_throughput(&self, start: Instant) {
            self.deref().show_throughput(start)
        }

        fn show_throughput_with(&self, start: Instant, step: Step, unit: Unit, level: MessageLevel) {
            self.deref().show_throughput_with(start, step, unit, level)
        }
    }

    impl<'a, T> NestedProgress for &'a mut T
    where
        T: NestedProgress + ?Sized,
    {
        type SubProgress = T::SubProgress;

        fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
            self.deref_mut().add_child(name)
        }

        fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
            self.deref_mut().add_child_with_id(name, id)
        }
    }

    impl<T> Sealed for T where T: NestedProgress + ?Sized {}

    impl<T, SubP> DynNestedProgress for T
    where
        T: NestedProgress<SubProgress = SubP> + ?Sized,
        SubP: NestedProgress + 'static,
    {
        fn add_child(&mut self, name: String) -> BoxedDynNestedProgress {
            BoxedDynNestedProgress::new(self.add_child(name))
        }

        fn add_child_with_id(&mut self, name: String, id: Id) -> BoxedDynNestedProgress {
            BoxedDynNestedProgress::new(self.add_child_with_id(name, id))
        }
    }

    impl BoxedDynNestedProgress {
        /// Create new instance from a `DynProgress` implementation.
        pub fn new(progress: impl DynNestedProgress + 'static) -> Self {
            Self(Box::new(progress))
        }
    }

    impl Progress for BoxedDynNestedProgress {
        fn init(&mut self, max: Option<Step>, unit: Option<Unit>) {
            self.0.init(max, unit)
        }

        fn unit(&self) -> Option<Unit> {
            self.0.unit()
        }

        fn max(&self) -> Option<Step> {
            self.0.max()
        }

        fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
            self.0.set_max(max)
        }

        fn set_name(&mut self, name: String) {
            self.0.set_name(name)
        }

        fn name(&self) -> Option<String> {
            self.0.name()
        }

        fn id(&self) -> Id {
            self.0.id()
        }

        fn message(&self, level: MessageLevel, message: String) {
            self.0.message(level, message)
        }

        fn show_throughput(&self, start: Instant) {
            self.0.show_throughput(start)
        }

        fn show_throughput_with(&self, start: Instant, step: Step, unit: Unit, level: MessageLevel) {
            self.0.show_throughput_with(start, step, unit, level)
        }
    }

    impl Progress for BoxedProgress {
        fn init(&mut self, max: Option<Step>, unit: Option<Unit>) {
            self.deref_mut().init(max, unit)
        }

        fn unit(&self) -> Option<Unit> {
            self.deref().unit()
        }

        fn max(&self) -> Option<Step> {
            self.deref().max()
        }

        fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
            self.deref_mut().set_max(max)
        }

        fn set_name(&mut self, name: String) {
            self.deref_mut().set_name(name)
        }

        fn name(&self) -> Option<String> {
            self.deref().name()
        }

        fn id(&self) -> Id {
            self.deref().id()
        }

        fn message(&self, level: MessageLevel, message: String) {
            self.deref().message(level, message)
        }

        fn show_throughput(&self, start: Instant) {
            self.deref().show_throughput(start)
        }

        fn show_throughput_with(&self, start: Instant, step: Step, unit: Unit, level: MessageLevel) {
            self.deref().show_throughput_with(start, step, unit, level)
        }
    }

    impl Count for BoxedDynNestedProgress {
        fn set(&self, step: Step) {
            self.0.set(step)
        }

        fn step(&self) -> Step {
            self.0.step()
        }

        fn inc_by(&self, step: Step) {
            self.0.inc_by(step)
        }

        fn inc(&self) {
            self.0.inc()
        }

        fn counter(&self) -> StepShared {
            self.0.counter()
        }
    }

    impl Count for BoxedProgress {
        fn set(&self, step: Step) {
            self.deref().set(step)
        }

        fn step(&self) -> Step {
            self.deref().step()
        }

        fn inc_by(&self, step: Step) {
            self.deref().inc_by(step)
        }

        fn inc(&self) {
            self.deref().inc()
        }

        fn counter(&self) -> StepShared {
            self.deref().counter()
        }
    }

    impl NestedProgress for BoxedDynNestedProgress {
        type SubProgress = Self;

        fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
            self.0.add_child(name.into())
        }

        fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
            self.0.add_child_with_id(name.into(), id)
        }
    }

    impl<T> Progress for DynNestedProgressToNestedProgress<T>
    where
        T: ?Sized + Progress,
    {
        fn init(&mut self, max: Option<Step>, unit: Option<Unit>) {
            self.0.init(max, unit)
        }

        fn unit(&self) -> Option<Unit> {
            self.0.unit()
        }

        fn max(&self) -> Option<Step> {
            self.0.max()
        }

        fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
            self.0.set_max(max)
        }

        fn set_name(&mut self, name: String) {
            self.0.set_name(name)
        }

        fn name(&self) -> Option<String> {
            self.0.name()
        }

        fn id(&self) -> Id {
            self.0.id()
        }

        fn message(&self, level: MessageLevel, message: String) {
            self.0.message(level, message)
        }

        fn show_throughput(&self, start: Instant) {
            self.0.show_throughput(start)
        }

        fn show_throughput_with(&self, start: Instant, step: Step, unit: Unit, level: MessageLevel) {
            self.0.show_throughput_with(start, step, unit, level)
        }
    }

    impl<T> Count for DynNestedProgressToNestedProgress<T>
    where
        T: ?Sized + Count,
    {
        fn set(&self, step: Step) {
            self.0.set(step)
        }

        fn step(&self) -> Step {
            self.0.step()
        }

        fn inc_by(&self, step: Step) {
            self.0.inc_by(step)
        }

        fn inc(&self) {
            self.0.inc()
        }

        fn counter(&self) -> StepShared {
            self.0.counter()
        }
    }

    impl<T> NestedProgress for DynNestedProgressToNestedProgress<T>
    where
        T: DynNestedProgress + ?Sized,
    {
        type SubProgress = BoxedDynNestedProgress;

        fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
            self.0.add_child(name.into())
        }

        fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
            self.0.add_child_with_id(name.into(), id)
        }
    }
}
