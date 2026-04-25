extern crate autocfg;

fn main() {
    let ac = autocfg::new();
    ac.emit_rustc_version(1, 63); // #[cfg(rustc_1_63)]
    ac.emit_rustc_version(1, 75); // #[cfg(rustc_1_75)]
    ac.emit_rustc_version(1, 81); // #[cfg(rustc_1_81)]
    ac.emit_rustc_version(1, 89); // #[cfg(rustc_1_89)]

    // Re-run if this file changes
    autocfg::rerun_path("build.rs");
}
