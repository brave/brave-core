fn main() -> Result<(), Box<dyn std::error::Error>> {
    let path = std::env::args_os().nth(1).unwrap();
    let file = std::fs::read(path)?;
    let versions = quitters::versions(&file);
    for (krate, version) in versions.iter() {
        println!("{} v{}", krate, version)
    }
    Ok(())
}
