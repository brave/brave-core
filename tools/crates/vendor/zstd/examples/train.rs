use clap::Parser;
use std::io;
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about=None)]
/// This program trains a dictionary from one or more files,
/// to make future compression of similar small files more efficient.
///
/// The dictionary will need to be present during decompression,
/// but if you need to compress many small files individually,
/// it may be worth the trouble.
struct Args {
    /// Maximum dictionary size in bytes.
    #[arg(short, long)]
    max_size: usize,

    /// Files to use as input.
    files: Vec<PathBuf>,
}

fn main() {
    let args = Args::parse();

    let dict = zstd::dict::from_files(&args.files, args.max_size).unwrap();

    let mut dict_reader: &[u8] = &dict;
    io::copy(&mut dict_reader, &mut io::stdout()).unwrap();
}
