use core::alloc;

use super::*;

impl Format for alloc::Layout {
    fn format(&self, fmt: Formatter) {
        crate::write!(
            fmt,
            "Layout {{ size: {}, align: {} }}",
            self.size(),
            self.align()
        );
    }
}
