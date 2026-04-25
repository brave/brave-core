use std::{
    fmt::Debug,
    ops::Deref,
    sync::{
        atomic::{AtomicUsize, Ordering},
        Arc,
    },
    time::SystemTime,
};

use parking_lot::Mutex;

use crate::{
    messages::MessageLevel,
    progress::{Id, State, Step, StepShared, Task, Value},
    tree::Item,
    unit::Unit,
};

impl Drop for Item {
    fn drop(&mut self) {
        self.tree.remove(&self.key);
    }
}

impl Debug for Item {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Item")
            .field("key", &self.key)
            .field("value", &self.value)
            .finish_non_exhaustive()
    }
}

impl Item {
    /// Initialize the Item for receiving progress information.
    ///
    /// If `max` is `Some(…)`, it will be treated as upper bound. When progress is [set(…)](./struct.Item.html#method.set)
    /// it should not exceed the given maximum.
    /// If `max` is `None`, the progress is unbounded. Use this if the amount of work cannot accurately
    /// be determined.
    ///
    /// If `unit` is `Some(…)`, it is used for display purposes only. It should be using the plural.
    ///
    /// If this method is never called, this `Item` will serve as organizational unit, useful to add more structure
    /// to the progress tree.
    ///
    /// **Note** that this method can be called multiple times, changing the bounded-ness and unit at will.
    pub fn init(&self, max: Option<usize>, unit: Option<Unit>) {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            if let Some(mut r) = self.tree.get_mut(&self.key) {
                self.value.store(0, Ordering::SeqCst);
                r.value_mut().progress = (max.is_some() || unit.is_some()).then(|| Value {
                    done_at: max,
                    unit,
                    step: Arc::clone(&self.value),
                    ..Default::default()
                })
            };
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree.get_mut(&self.key, |v| {
                self.value.store(0, Ordering::SeqCst);
                v.progress = (max.is_some() || unit.is_some()).then(|| Value {
                    done_at: max,
                    unit,
                    step: Arc::clone(&self.value),
                    ..Default::default()
                });
            });
        }
    }

    fn alter_progress(&self, f: impl FnMut(&mut Value)) {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            if let Some(mut r) = self.tree.get_mut(&self.key) {
                // NOTE: since we wrap around, if there are more tasks than we can have IDs for,
                // and if all these tasks are still alive, two progress trees may see the same ID
                // when these go out of scope, they delete the key and the other tree will not find
                // its value anymore. Besides, it's probably weird to see tasks changing their progress
                // all the time…
                r.value_mut().progress.as_mut().map(f);
            };
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree.get_mut(&self.key, |v| {
                v.progress.as_mut().map(f);
            });
        }
    }

    /// Set the name of this task's progress to the given `name`.
    pub fn set_name(&self, name: impl Into<String>) {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            if let Some(mut r) = self.tree.get_mut(&self.key) {
                r.value_mut().name = name.into();
            };
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree.get_mut(&self.key, |v| {
                v.name = name.into();
            });
        }
    }

    /// Get the name of this task's progress
    pub fn name(&self) -> Option<String> {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            self.tree.get(&self.key).map(|r| r.value().name.to_owned())
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree.get(&self.key, |v| v.name.to_owned())
        }
    }

    /// Get the stable identifier of this instance.
    pub fn id(&self) -> Id {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            self.tree
                .get(&self.key)
                .map(|r| r.value().id)
                .unwrap_or(crate::progress::UNKNOWN)
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree.get(&self.key, |v| v.id).unwrap_or(crate::progress::UNKNOWN)
        }
    }

    /// Returns the current step, as controlled by `inc*(…)` calls
    pub fn step(&self) -> Option<Step> {
        self.value.load(Ordering::Relaxed).into()
    }

    /// Returns the maximum about of items we expect, as provided with the `init(…)` call
    pub fn max(&self) -> Option<Step> {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            self.tree
                .get(&self.key)
                .and_then(|r| r.value().progress.as_ref().and_then(|p| p.done_at))
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree
                .get(&self.key, |v| v.progress.as_ref().and_then(|p| p.done_at))
                .flatten()
        }
    }

    /// Set the maximum value to `max` and return the old maximum value.
    pub fn set_max(&self, max: Option<Step>) -> Option<Step> {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            self.tree
                .get_mut(&self.key)?
                .value_mut()
                .progress
                .as_mut()
                .and_then(|p| {
                    let prev = p.done_at;
                    p.done_at = max;
                    prev
                })
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree
                .get_mut(&self.key, |v| {
                    v.progress.as_mut().and_then(|p| {
                        let prev = p.done_at;
                        p.done_at = max;
                        prev
                    })
                })
                .flatten()
        }
    }

    /// Returns the (cloned) unit associated with this Progress
    pub fn unit(&self) -> Option<Unit> {
        #[cfg(feature = "progress-tree-hp-hashmap")]
        {
            self.tree
                .get(&self.key)
                .and_then(|r| r.value().progress.as_ref().and_then(|p| p.unit.clone()))
        }
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        {
            self.tree
                .get(&self.key, |v| v.progress.as_ref().and_then(|p| p.unit.clone()))
                .flatten()
        }
    }

    /// Set the current progress to the given `step`.
    ///
    /// **Note**: that this call has no effect unless `init(…)` was called before.
    pub fn set(&self, step: Step) {
        self.value.store(step, Ordering::SeqCst);
    }

    /// Increment the current progress by the given `step`.
    ///
    /// **Note**: that this call has no effect unless `init(…)` was called before.
    pub fn inc_by(&self, step: Step) {
        self.value.fetch_add(step, Ordering::Relaxed);
    }

    /// Increment the current progress by one.
    ///
    /// **Note**: that this call has no effect unless `init(…)` was called before.
    pub fn inc(&self) {
        self.value.fetch_add(1, Ordering::Relaxed);
    }

    /// Call to indicate that progress cannot be indicated, and that the task cannot be interrupted.
    /// Use this, as opposed to `halted(…)`, if a non-interruptable call is about to be made without support
    /// for any progress indication.
    ///
    /// If `eta` is `Some(…)`, it specifies the time at which this task is expected to
    /// make progress again.
    ///
    /// The halted-state is undone next time [`tree::Item::running(…)`][Item::running()] is called.
    pub fn blocked(&self, reason: &'static str, eta: Option<SystemTime>) {
        self.alter_progress(|p| p.state = State::Blocked(reason, eta));
    }

    /// Call to indicate that progress cannot be indicated, even though the task can be interrupted.
    /// Use this, as opposed to `blocked(…)`, if an interruptable call is about to be made without support
    /// for any progress indication.
    ///
    /// If `eta` is `Some(…)`, it specifies the time at which this task is expected to
    /// make progress again.
    ///
    /// The halted-state is undone next time [`tree::Item::running(…)`][Item::running()] is called.
    pub fn halted(&self, reason: &'static str, eta: Option<SystemTime>) {
        self.alter_progress(|p| p.state = State::Halted(reason, eta));
    }

    /// Call to indicate that progress is back in running state, which should be called after the reason for
    /// calling `blocked()` or `halted()` has passed.
    pub fn running(&self) {
        self.alter_progress(|p| p.state = State::Running);
    }

    /// Adds a new child `Tree`, whose parent is this instance, with the given `name`.
    ///
    /// **Important**: The depth of the hierarchy is limited to [`tree::Key::max_level`](./struct.Key.html#method.max_level).
    /// Exceeding the level will be ignored, and new tasks will be added to this instance's
    /// level instead.
    pub fn add_child(&mut self, name: impl Into<String>) -> Item {
        self.add_child_with_id(name, crate::progress::UNKNOWN)
    }

    /// Adds a new child `Tree`, whose parent is this instance, with the given `name` and `id`.
    ///
    /// **Important**: The depth of the hierarchy is limited to [`tree::Key::max_level`](./struct.Key.html#method.max_level).
    /// Exceeding the level will be ignored, and new tasks will be added to this instance's
    /// level instead.
    pub fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Item {
        let child_key = self.key.add_child(self.highest_child_id);
        let task = Task {
            name: name.into(),
            id,
            progress: None,
        };
        #[cfg(feature = "progress-tree-hp-hashmap")]
        self.tree.insert(child_key, task);
        #[cfg(not(feature = "progress-tree-hp-hashmap"))]
        self.tree.insert(child_key, task);
        self.highest_child_id = self.highest_child_id.wrapping_add(1);
        Item {
            highest_child_id: 0,
            value: Default::default(),
            key: child_key,
            tree: Arc::clone(&self.tree),
            messages: Arc::clone(&self.messages),
        }
    }

    /// Create a `message` of the given `level` and store it with the progress tree.
    ///
    /// Use this to provide additional,human-readable information about the progress
    /// made, including indicating success or failure.
    pub fn message(&self, level: MessageLevel, message: impl Into<String>) {
        let message: String = message.into();
        self.messages.lock().push_overwrite(
            level,
            {
                let name;
                #[cfg(feature = "progress-tree-hp-hashmap")]
                {
                    name = self.tree.get(&self.key).map(|v| v.name.to_owned()).unwrap_or_default();
                }
                #[cfg(not(feature = "progress-tree-hp-hashmap"))]
                {
                    name = self.tree.get(&self.key, |v| v.name.to_owned()).unwrap_or_default()
                }

                #[cfg(feature = "progress-tree-log")]
                match level {
                    MessageLevel::Failure => crate::warn!("{} → {}", name, message),
                    MessageLevel::Info | MessageLevel::Success => crate::info!("{} → {}", name, message),
                };

                name
            },
            message,
        )
    }

    /// Create a message indicating the task is done
    pub fn done(&mut self, message: impl Into<String>) {
        self.message(MessageLevel::Success, message)
    }

    /// Create a message indicating the task failed
    pub fn fail(&mut self, message: impl Into<String>) {
        self.message(MessageLevel::Failure, message)
    }

    /// Create a message providing additional information about the progress thus far.
    pub fn info(&mut self, message: impl Into<String>) {
        self.message(MessageLevel::Info, message)
    }

    pub(crate) fn deep_clone(&self) -> Item {
        Item {
            key: self.key,
            value: Arc::new(AtomicUsize::new(self.value.load(Ordering::SeqCst))),
            highest_child_id: self.highest_child_id,
            tree: Arc::new(self.tree.deref().clone()),
            messages: Arc::new(Mutex::new(self.messages.lock().clone())),
        }
    }
}

impl crate::Count for Item {
    fn set(&self, step: usize) {
        Item::set(self, step)
    }

    fn step(&self) -> usize {
        Item::step(self).unwrap_or(0)
    }

    fn inc_by(&self, step: usize) {
        self.inc_by(step)
    }

    fn counter(&self) -> StepShared {
        Arc::clone(&self.value)
    }
}

impl crate::Progress for Item {
    fn init(&mut self, max: Option<Step>, unit: Option<Unit>) {
        Item::init(self, max, unit)
    }

    fn unit(&self) -> Option<Unit> {
        Item::unit(self)
    }

    fn max(&self) -> Option<usize> {
        Item::max(self)
    }

    fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
        Item::set_max(self, max)
    }

    fn set_name(&mut self, name: String) {
        Item::set_name(self, name)
    }

    fn name(&self) -> Option<String> {
        Item::name(self)
    }

    fn id(&self) -> Id {
        Item::id(self)
    }

    fn message(&self, level: MessageLevel, message: String) {
        Item::message(self, level, message)
    }
}

impl crate::NestedProgress for Item {
    type SubProgress = Item;

    fn add_child(&mut self, name: impl Into<String>) -> Self {
        Item::add_child(self, name)
    }

    fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self {
        Item::add_child_with_id(self, name, id)
    }
}
