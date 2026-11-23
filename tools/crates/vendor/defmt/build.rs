use std::{env, error::Error, fs, path::PathBuf};

fn main() -> Result<(), Box<dyn Error>> {
    // Read the linker script
    let mut linker_script = fs::read_to_string("defmt.x.in")?;

    // Optionally exclude default panic handler
    if cfg!(feature = "avoid-default-panic") {
        linker_script = avoid_default_panic(linker_script);
    }

    // Put the linker script somewhere the linker can find it
    let out = &PathBuf::from(env::var("OUT_DIR")?);
    fs::write(out.join("defmt.x"), linker_script)?;
    println!("cargo:rustc-link-search={}", out.display());
    let target = env::var("TARGET")?;

    // `"atomic-cas": false` in `--print target-spec-json`
    // last updated: rust 1.48.0
    match &target[..] {
        "avr-gnu-base"
        | "msp430-none-elf"
        | "riscv32i-unknown-none-elf"
        | "riscv32imc-unknown-none-elf"
        | "thumbv4t-none-eabi"
        | "thumbv6m-none-eabi"
        | "xtensa-esp32s2-none-elf" => {
            println!("cargo:rustc-cfg=no_cas");
        }
        _ => {}
    }

    // allow #[cfg(cfg_name)]
    for cfg_name in ["c_variadic", "no_cas"] {
        println!("cargo:rustc-check-cfg=cfg({cfg_name})");
    }

    Ok(())
}

fn avoid_default_panic(linker_script: String) -> String {
    linker_script.replacen("PROVIDE(_defmt_panic = __defmt_default_panic);", "", 1)
}
