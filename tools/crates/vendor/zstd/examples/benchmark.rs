use clap::Parser;
use humansize::{format_size, DECIMAL};
use std::io::Read;
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about=None)]
struct Args {
    /// Directory containing the data to compress.
    /// To use the silesia corpus, run the following commands:
    ///
    /// ```
    /// wget http://sun.aei.polsl.pl/~sdeor/corpus/silesia.zip
    /// unzip silesia.zip -d silesia/
    /// cargo run --example benchmark -- silesia/",
    /// ```
    dir: PathBuf,

    /// First compression level to test.
    #[arg(short, long)]
    begin: i32,

    /// Last compression level to test.
    #[arg(short, long)]
    end: i32,
}

fn main() {
    let args = Args::parse();

    // Step 1: load data in memory
    let files: Vec<Vec<u8>> = std::fs::read_dir(args.dir)
        .unwrap()
        .map(|file| {
            let file = file.unwrap();

            let mut content = Vec::new();
            std::fs::File::open(file.path())
                .unwrap()
                .read_to_end(&mut content)
                .unwrap();
            content
        })
        .collect();

    let total_size: usize = files.iter().map(|data| data.len()).sum();

    // Step 3: compress data

    // Print tsv headers
    println!(
        "{}\t{}\t{}\t{}",
        "Compression level",
        "Compression ratio",
        "Compression speed",
        "Decompression speed"
    );

    for level in args.begin..args.end {
        // Compress each sample sequentially.
        let start = std::time::Instant::now();

        let compressed: Vec<Vec<u8>> = files
            .iter()
            .map(|data| zstd::encode_all(&data[..], level).unwrap())
            .collect();
        let mid = std::time::Instant::now();

        let uncompressed: Vec<Vec<u8>> = compressed
            .iter()
            .map(|data| zstd::decode_all(&data[..]).unwrap())
            .collect();
        let end = std::time::Instant::now();

        for (original, processed) in files.iter().zip(uncompressed.iter()) {
            assert_eq!(&original[..], &processed[..]);
        }

        let compress_time = mid - start;
        let decompress_time = end - mid;

        let compress_seconds = compress_time.as_secs() as f64
            + compress_time.subsec_nanos() as f64 * 1e-9;

        let decompress_seconds = decompress_time.as_secs() as f64
            + decompress_time.subsec_nanos() as f64 * 1e-9;

        let compressed_size: usize = compressed.iter().map(Vec::len).sum();

        let speed = (total_size as f64 / compress_seconds) as usize;
        let speed = format_size(speed, DECIMAL);

        let d_speed = (total_size as f64 / decompress_seconds) as usize;
        let d_speed = format_size(d_speed, DECIMAL);

        let ratio = compressed_size as f64 / total_size as f64;
        println!("{}\t{:.3}\t{}/s\t{}/s", level, 1.0 / ratio, speed, d_speed);
    }
}
