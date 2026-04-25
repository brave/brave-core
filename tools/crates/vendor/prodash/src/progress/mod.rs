use std::{
    sync::{
        atomic::{AtomicUsize, Ordering},
        Arc,
    },
    time::SystemTime,
};

use crate::unit::Unit;

///
pub mod key;
#[doc(inline)]
pub use key::Key;

mod utils;

#[cfg(feature = "progress-log")]
mod log;
pub use utils::{Discard, DoOrDiscard, Either, ThroughputOnDrop};

#[cfg(feature = "progress-log")]
pub use self::log::Log;

/// Four bytes of function-local unique and stable identifier for each item added as progress,
/// like b"TREE" or b"FILE".
///
/// Note that uniqueness only relates to one particular method call where those interested in its progress
/// may assume certain stable ids to look for when selecting specific bits of progress to process.
pub type Id = [u8; 4];

/// The default Id to use if there is no need for an id.
///
/// This is the default unless applications wish to make themselves more introspectable.
pub const UNKNOWN: Id = *b"\0\0\0\0";

/// The amount of steps a progress can make
pub type Step = usize;

/// The amount of steps a progress can make, for threadsafe counting.
pub type AtomicStep = AtomicUsize;

/// As step, but shareable.
pub type StepShared = Arc<AtomicStep>;

/// Indicate whether a progress can or cannot be made.
#[derive(Default, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Debug, Hash)]
pub enum State {
    /// Indicates a task is blocked and cannot indicate progress, optionally until the
    /// given time. The task cannot easily be interrupted.
    Blocked(&'static str, Option<SystemTime>),
    /// Indicates a task cannot indicate progress, optionally until the
    /// given time. The task can be interrupted.
    Halted(&'static str, Option<SystemTime>),
    /// The task is running
    #[default]
    Running,
}

/// Progress associated with some item in the progress tree.
#[derive(Clone, Default, Debug)]
pub struct Value {
    /// The amount of progress currently made
    pub step: StepShared,
    /// The step at which no further progress has to be made.
    ///
    /// If unset, the progress is unbounded.
    pub done_at: Option<Step>,
    /// The unit associated with the progress.
    pub unit: Option<Unit>,
    /// Whether progress can be made or not
    pub state: State,
}

impl std::hash::Hash for Value {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        let Self {
            step,
            done_at,
            unit,
            state: our_state,
        } = self;
        done_at.hash(state);
        unit.hash(state);
        our_state.hash(state);
        step.load(Ordering::Relaxed).hash(state);
    }
}

impl Value {
    /// Returns a number between `Some(0.0)` and `Some(1.0)`, or `None` if the progress is unbounded.
    ///
    /// A task half done would return `Some(0.5)`.
    pub fn fraction(&self) -> Option<f32> {
        self.done_at
            .map(|done_at| self.step.load(Ordering::SeqCst) as f32 / done_at as f32)
    }
}

/// The value associated with a spot in the hierarchy.
#[derive(Clone, Default, Debug, Hash)]
pub struct Task {
    /// The name of the `Item` or task.
    pub name: String,
    /// The stable identifier of this task.
    /// Useful for selecting specific tasks out of a set of them.
    pub id: Id,
    /// The progress itself, unless this value belongs to an `Item` serving as organizational unit.
    pub progress: Option<Value>,
}
