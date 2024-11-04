//! Main entry point for `cargo audit`

#![deny(warnings, missing_docs, trivial_casts, unused_qualifications)]
#![forbid(unsafe_code)]

use cargo_audit::application::APP;

fn main() {
    abscissa_core::boot(&APP);
}
