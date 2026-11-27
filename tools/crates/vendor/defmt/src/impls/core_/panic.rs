use core::panic;

use super::*;

impl Format for panic::PanicInfo<'_> {
    fn format(&self, f: Formatter) {
        if let Some(location) = self.location() {
            crate::write!(f, "panicked at {}", location);
        } else {
            crate::write!(f, "panicked");
        }
        #[cfg(feature = "panic-message")]
        if let Some(message) = self.message().as_str() {
            crate::write!(f, ": {=str}", message);
        }
    }
}

impl Format for panic::Location<'_> {
    fn format(&self, f: Formatter) {
        crate::write!(
            f,
            "{=str}:{=u32}:{=u32}",
            self.file(),
            self.line(),
            self.column()
        );
    }
}
