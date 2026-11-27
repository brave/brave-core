use deflate64::InflaterManaged;
use std::hint::black_box;
use std::time::Instant;

static ZIP_FILE_DATA: &[u8] = include_bytes!("../test-assets/deflate64.zip");
const BINARY_WAV_DATA_OFFSET: usize = 40;
const BINARY_WAV_COMPRESSED_SIZE: usize = 2669743;
const BINARY_WAV_UNCOMPRESSED_SIZE: usize = 2703788;
const ITERATIONS: usize = 150;

fn main() {
    let compressed = &ZIP_FILE_DATA[BINARY_WAV_DATA_OFFSET..][..BINARY_WAV_COMPRESSED_SIZE];
    let mut output = vec![0u8; BINARY_WAV_UNCOMPRESSED_SIZE + 10];

    let start = Instant::now();
    for _ in 0..ITERATIONS {
        let mut inflater = InflaterManaged::new();
        let result = black_box(inflater.inflate(black_box(compressed), &mut output));
        assert_eq!(result.bytes_written, BINARY_WAV_UNCOMPRESSED_SIZE);
    }
    let elapsed = start.elapsed();

    let ms_per_iter = elapsed.as_secs_f64() * 1000.0 / ITERATIONS as f64;
    let mb_per_sec =
        (BINARY_WAV_UNCOMPRESSED_SIZE * ITERATIONS) as f64 / elapsed.as_secs_f64() / 1_000_000.0;

    println!();
    println!(
        "benchmark complete - {:.2} ms/iter, {:.1} MB/s",
        ms_per_iter, mb_per_sec
    );
    println!();
}
