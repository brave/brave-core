fn main() {
    println!("cargo:rerun-if-env-changed=DEFMT_LOG");
}
