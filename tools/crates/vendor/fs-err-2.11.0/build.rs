extern crate autocfg;

fn main() {
    let ac = autocfg::new();
    // Allows `#[cfg(rustc_1_63)]` to be used in code
    ac.emit_rustc_version(1, 63);
}
