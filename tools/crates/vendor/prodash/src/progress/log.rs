use std::{
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
    time::Duration,
};

use crate::{
    messages::MessageLevel,
    progress::{Id, Step, StepShared},
    Count, NestedProgress, Progress, Unit,
};

/// A [`NestedProgress`] implementation which displays progress as it happens without the use of a renderer.
///
/// Note that this incurs considerable performance cost as each progress calls ends up getting the system time
/// to see if progress information should actually be emitted.
pub struct Log {
    name: String,
    id: Id,
    max: Option<usize>,
    unit: Option<Unit>,
    step: StepShared,
    current_level: usize,
    max_level: usize,
    trigger: Arc<AtomicBool>,
}

const EMIT_LOG_EVERY_S: f32 = 0.5;
const SEP: &str = "::";

impl Log {
    /// Create a new instance from `name` while displaying progress information only up to `max_level`.
    pub fn new(name: impl Into<String>, max_level: Option<usize>) -> Self {
        let trigger = Arc::new(AtomicBool::new(true));
        std::thread::spawn({
            let duration = Duration::from_secs_f32(EMIT_LOG_EVERY_S);
            let trigger = Arc::downgrade(&trigger);
            move || {
                while let Some(t) = trigger.upgrade() {
                    t.store(true, Ordering::Relaxed);
                    std::thread::sleep(duration);
                }
            }
        });
        Log {
            name: name.into(),
            id: crate::progress::UNKNOWN,
            current_level: 0,
            max_level: max_level.unwrap_or(usize::MAX),
            max: None,
            step: Default::default(),
            unit: None,
            trigger,
        }
    }
}

impl Log {
    fn maybe_log(&self) {
        if self.current_level > self.max_level {
            return;
        }
        let step = self.step();
        if self.trigger.swap(false, Ordering::Relaxed) {
            match (self.max, &self.unit) {
                (max, Some(unit)) => log::info!("{} → {}", self.name, unit.display(step, max, None)),
                (Some(max), None) => log::info!("{} → {} / {}", self.name, step, max),
                (None, None) => log::info!("{} → {}", self.name, step),
            }
        }
    }
}

impl Count for Log {
    fn set(&self, step: Step) {
        self.step.store(step, Ordering::SeqCst);
        self.maybe_log()
    }

    fn step(&self) -> usize {
        self.step.load(Ordering::Relaxed)
    }

    fn inc_by(&self, step: Step) {
        self.step.fetch_add(step, Ordering::Relaxed);
        self.maybe_log()
    }

    fn counter(&self) -> StepShared {
        self.step.clone()
    }
}

impl Progress for Log {
    fn init(&mut self, max: Option<Step>, unit: Option<Unit>) {
        self.max = max;
        self.unit = unit;
    }
    fn unit(&self) -> Option<Unit> {
        self.unit.clone()
    }

    fn max(&self) -> Option<Step> {
        self.max
    }

    fn set_max(&mut self, max: Option<Step>) -> Option<Step> {
        let prev = self.max;
        self.max = max;
        prev
    }

    fn set_name(&mut self, name: String) {
        self.name = self
            .name
            .split("::")
            .next()
            .map(|parent| format!("{}{}{}", parent.to_owned(), SEP, name))
            .unwrap_or(name);
    }

    fn name(&self) -> Option<String> {
        self.name.split(SEP).nth(1).map(ToOwned::to_owned)
    }

    fn id(&self) -> Id {
        self.id
    }

    fn message(&self, level: MessageLevel, message: String) {
        match level {
            MessageLevel::Info => log::info!("ℹ{} → {}", self.name, message),
            MessageLevel::Failure => log::error!("𐄂{} → {}", self.name, message),
            MessageLevel::Success => log::info!("✓{} → {}", self.name, message),
        }
    }
}

impl NestedProgress for Log {
    type SubProgress = Log;

    fn add_child(&mut self, name: impl Into<String>) -> Self::SubProgress {
        self.add_child_with_id(name, crate::progress::UNKNOWN)
    }

    fn add_child_with_id(&mut self, name: impl Into<String>, id: Id) -> Self::SubProgress {
        Log {
            name: format!("{}{}{}", self.name, SEP, Into::<String>::into(name)),
            id,
            current_level: self.current_level + 1,
            max_level: self.max_level,
            step: Default::default(),
            max: None,
            unit: None,
            trigger: Arc::clone(&self.trigger),
        }
    }
}
