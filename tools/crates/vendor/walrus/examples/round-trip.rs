//! A small example which is primarily used to help benchmark walrus right now.

fn main() -> anyhow::Result<()> {
    env_logger::init();
    let a = std::env::args()
        .nth(1)
        .ok_or_else(|| anyhow::anyhow!("must provide the input wasm file as the first argument"))?;
    let mut m = walrus::Module::from_file(&a)?;
    let wasm = m.emit_wasm();
    if let Some(destination) = std::env::args().nth(2) {
        std::fs::write(destination, wasm)?;
    }
    Ok(())
}
