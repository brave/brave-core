use std::{env, fs::File, io::Write, path::Path};

fn main() {
    let out_dir = env::var_os("OUT_DIR").unwrap();

    let const_path = Path::new(&out_dir).join("const.rs");
    let mut const_file = File::create(&const_path).expect("Create const.rs file failed");
    libsecp256k1_gen_ecmult::generate_to(&mut const_file).expect("Write const.rs file failed");
    const_file.flush().expect("Flush const.rs file failed");

    let gen_path = Path::new(&out_dir).join("const_gen.rs");
    let mut gen_file = File::create(&gen_path).expect("Create const_gen.rs file failed");
    libsecp256k1_gen_genmult::generate_to(&mut gen_file).expect("Write const_gen.rs file failed");
    gen_file.flush().expect("Flush const_gen.rs file failed");
}
