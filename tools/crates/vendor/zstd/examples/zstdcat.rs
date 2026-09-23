use clap::Parser;
use std::fs;
use std::io;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about=None)]
struct Args {
    /// Files to decompress. With no file, or when given -, read standard input.
    file: Vec<String>,
}

fn main() {
    // This will be a simple application:
    // takes a single (repeatable and optional) argument.
    let args = Args::parse();

    // If nothing was given, act as if `-` was there.
    if args.file.is_empty() {
        decompress_file("-").unwrap();
    } else {
        for file in &args.file {
            decompress_file(file).unwrap();
        }
    }
}

// Dispatch the source reader depending on the filename
fn decompress_file(file: &str) -> io::Result<()> {
    match file {
        "-" => decompress_from(io::stdin()),
        other => decompress_from(io::BufReader::new(fs::File::open(other)?)),
    }
}

// Decompress from a `Reader` into stdout
fn decompress_from<R: io::Read>(r: R) -> io::Result<()> {
    let mut decoder = zstd::Decoder::new(r)?;
    io::copy(&mut decoder, &mut io::stdout())?;
    Ok(())
}
