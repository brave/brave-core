//! Main entry point for `cargo audit`

#![forbid(unsafe_code)]

fn main() {
    cargo_audit::application::boot();
}
