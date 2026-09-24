use crate::{messages::MessageLevel, progress::Id, Count, NestedProgress, Progress, Unit};
use std::sync::atomic::AtomicUsize;
use std::sync::Arc;

/// An implementation of [`NestedProgress`] which discards all calls.
pub struct Discard;

impl Count for Discard {
    fn set(&self, _step: usize) {}

    fn step(&self) -> usize {
        0
    }

    fn inc_by(&self, _step: usize) {}

    fn counter(&self) -> StepShared {
        Arc::new(AtomicUsize::default())
    }
}

impl Progress for Discard {
    fn init(&mut self, _max: Option<usize>, _unit: Option<Unit>) {}

    fn set_max(&mut self, _max: Option<Step>) -> Option<Step> {
        None
    }
    fn set_name(&mut self, _name: String) {}

    fn name(&self) -> Option<String> {
        None
    }

    fn id(&self) -> Id {
        crate::progress::UNKNOWN
    }

    fn message(&self, _level: MessageLevel, _message: String) {}
}

impl NestedProgress for Discard {
    type SubProgress = Self;

    fn add_child(&mut self, _name: impl Into<String>) -> Self {
        Discard
    }

    fn add_child_with_id(&mut self, _name: impl Into<String>, _id: Id) -> Self {
        Discard
    }
}

/// An implementation of [`NestedProgress`] showing either one or the other implementation.
///
/// Useful in conjunction with [`Discard`] and a working implementation, making it as a form of `Option<Progress>` which
/// can be passed to methods requiring `impl Progress`.
/// See [`DoOrDiscard`] for an incarnation of this.
#[allow(missing_docs)]
pub enum Either<L, R> {
    Left(L),
    Right(R),
}

impl<L, R> Count for Either<L, R>
where
    L: Count,
    R: Count,
{
    fn set(&self, step: usize) {
        match self {
            Either::Left(l) => l.set(step),
            Either::Right(r) => r.set(step),
        }
    }
    fn step(&self) -> usize {
        match self {
            Either::Left(l) => l.step(),
            Either::Right(r) => r.step(),
        }
    }
    fn inc_by(&self, step: usize) {
        match self {
            Either::Left(l) => l.inc_by(step),
            Either::Right(r) => r.inc_by(step),
        }
    }
    fn counter(&self) -> StepShared {
        match self {
            Either::Left(l) => l.counter(),
            Either::Right(r) => r.counter(),
        }
    }
}

impl<L, R> Progress for Either<L, R>
where
    L: Progress,
    R: Progress,
{
    fn init(&mut self, max: Option<usize>, unit: Option<Unit>) {
        match self {
            Either::Left(l) => l.init(max, unit),
            Either::Right(r) => r.init(max, unit),
        }
    }

    fn unit(&self) -> Option<Unit> {
        match self {
            Either::Left(l) => l.unit(),
            Either::Right(r) => r.unit(),
        }
    }

    fn max(&self) -> Option<usize> {
        match self {
            Either::Left(l) => l.max(),
            Either::Right(r) => r.max(),
        }
    }

    fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
        match self {
            Either::Left(l) => l.set_max(max),
            Either::Right(r) => r.set_max(max),
        }
    }

    fn set_name(&mut self, name: String) {
        match self {
            Either::Left(l) => l.set_name(name),
            Either::Right(r) => r.set_name(name),
        }
    }

    fn name(&self) -> Option<String> {
        match self {
            Either::Left(l) => l.name(),
            Either::Right(r) => r.name(),
        }
    }

    fn id(&self) -> Id {
        match self {
            Either::Left(l) => l.id(),
            Either::Right(r) => r.id(),
        }
    }

    fn message(&self, level: MessageLevel, message: String) {
        match self {
            Either::Left(l) => l.message(level, message),
            Either::Right(r) => r.message(level, message),
        }
    }
}

impl<L, R> NestedProgress for Either<L, R>
where
    L: NestedProgress,
    R: NestedProgress,
{
    type SubProgress = Either<L::SubProgress, R::SubProgress>;

    fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
        match self {
            Either::Left(l) => Either::Left(l.add_child(name)),
            Either::Right(r) => Either::Right(r.add_child(name)),
        }
    }

    fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
        match self {
            Either::Left(l) => Either::Left(l.add_child_with_id(name, id)),
            Either::Right(r) => Either::Right(r.add_child_with_id(name, id)),
        }
    }
}

/// An implementation of `Progress` which can be created easily from `Option<impl Progress>`.
pub struct DoOrDiscard<T>(Either<T, Discard>);

impl<T> From<Option<T>> for DoOrDiscard<T>
where
    T: NestedProgress,
{
    fn from(p: Option<T>) -> Self {
        match p {
            Some(p) => DoOrDiscard(Either::Left(p)),
            None => DoOrDiscard(Either::Right(Discard)),
        }
    }
}

impl<T: NestedProgress> DoOrDiscard<T> {
    /// Obtain either the original [`NestedProgress`] implementation or `None`.
    pub fn into_inner(self) -> Option<T> {
        match self {
            DoOrDiscard(Either::Left(p)) => Some(p),
            DoOrDiscard(Either::Right(_)) => None,
        }
    }

    /// Take out the implementation of [`NestedProgress`] and replace it with [`Discard`].
    pub fn take(&mut self) -> Option<T> {
        let this = std::mem::replace(self, DoOrDiscard::from(None));
        match this {
            DoOrDiscard(Either::Left(p)) => Some(p),
            DoOrDiscard(Either::Right(_)) => None,
        }
    }
}

impl<T> Count for DoOrDiscard<T>
where
    T: Count,
{
    fn set(&self, step: usize) {
        self.0.set(step)
    }
    fn step(&self) -> usize {
        self.0.step()
    }

    fn inc_by(&self, step: usize) {
        self.0.inc_by(step)
    }

    fn counter(&self) -> StepShared {
        self.0.counter()
    }
}

impl<T> Progress for DoOrDiscard<T>
where
    T: Progress,
{
    fn init(&mut self, max: Option<usize>, unit: Option<Unit>) {
        self.0.init(max, unit)
    }

    fn unit(&self) -> Option<Unit> {
        self.0.unit()
    }

    fn max(&self) -> Option<usize> {
        self.0.max()
    }

    fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
        self.0.set_max(max)
    }

    fn set_name(&mut self, name: String) {
        self.0.set_name(name);
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
}

impl<T> NestedProgress for DoOrDiscard<T>
where
    T: NestedProgress,
{
    type SubProgress = DoOrDiscard<T::SubProgress>;

    fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
        DoOrDiscard(self.0.add_child(name))
    }

    fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
        DoOrDiscard(self.0.add_child_with_id(name, id))
    }
}

use std::time::Instant;

use crate::progress::{Step, StepShared};

/// Emit a message with throughput information when the instance is dropped.
pub struct ThroughputOnDrop<T: NestedProgress>(T, Instant);

impl<T: NestedProgress> ThroughputOnDrop<T> {
    /// Create a new instance by providing the `inner` [`NestedProgress`] implementation.
    pub fn new(inner: T) -> Self {
        ThroughputOnDrop(inner, Instant::now())
    }
}

impl<T: NestedProgress> Count for ThroughputOnDrop<T> {
    fn set(&self, step: usize) {
        self.0.set(step)
    }

    fn step(&self) -> usize {
        self.0.step()
    }

    fn inc_by(&self, step: usize) {
        self.0.inc_by(step)
    }

    fn counter(&self) -> StepShared {
        self.0.counter()
    }
}

impl<T: NestedProgress> Progress for ThroughputOnDrop<T> {
    fn init(&mut self, max: Option<usize>, unit: Option<Unit>) {
        self.0.init(max, unit)
    }

    fn unit(&self) -> Option<Unit> {
        self.0.unit()
    }

    fn max(&self) -> Option<usize> {
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
}

impl<T: NestedProgress> NestedProgress for ThroughputOnDrop<T> {
    type SubProgress = ThroughputOnDrop<T::SubProgress>;

    fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
        ThroughputOnDrop::new(self.0.add_child(name))
    }

    fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
        ThroughputOnDrop::new(self.0.add_child_with_id(name, id))
    }
}

impl<T: NestedProgress> Drop for ThroughputOnDrop<T> {
    fn drop(&mut self) {
        self.0.show_throughput(self.1)
    }
}
