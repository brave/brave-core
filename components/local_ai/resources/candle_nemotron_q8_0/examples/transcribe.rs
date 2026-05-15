// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Native diagnostic transcriber for nemotron-speech-streaming-en-0.6b.
// Drives the same model the WASM crate uses, but on a wav file.
// Cache-aware streaming: each chunk is processed once and the
// encoder's per-layer K/V + depthwise-conv caches plus the RNN-T
// predictor state persist to the next chunk.
//
// Build + run:
//   .../rust-toolchain/bin/cargo run --release \
//     --manifest-path components/local_ai/resources/candle_nemotron_q8_0/Cargo.toml \
//     --example transcribe -- MODEL_DIR WAV_PATH
//
// Optional: NEMOTRON_CHUNK_MS sets the chunk size (default 1120 ms,
// matching the model's largest training context [70 left, 13 right]).

use std::fs;
use std::path::Path;
use std::time::Instant;

use candle_core::quantized::gguf_file;
use candle_core::{Device, Tensor};
use candle_nemotron_q8_0::audio;
use candle_nemotron_q8_0::decode::{rnnt_greedy_step, DecodeState};
use candle_nemotron_q8_0::model::{
    EncoderCache, ModelBuilder, NemotronConfig,
};
use tokenizers::Tokenizer;

const SAMPLE_RATE: u32 = 16_000;
// 1120 ms = 14 encoder frames after 8x subsampling. Matches the
// largest att_context_size = [70, 13] the model was trained on.
const CHUNK_MS_DEFAULT: usize = 1120;

fn main() -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
    let mut args = std::env::args().skip(1);
    let model_dir = args
        .next()
        .ok_or("usage: transcribe MODEL_DIR WAV_PATH")?;
    let wav_path = args
        .next()
        .ok_or("usage: transcribe MODEL_DIR WAV_PATH")?;
    let model_dir = Path::new(&model_dir);

    let chunk_ms = std::env::var("NEMOTRON_CHUNK_MS")
        .ok()
        .and_then(|s| s.parse::<usize>().ok())
        .unwrap_or(CHUNK_MS_DEFAULT);
    let chunk_samples = SAMPLE_RATE as usize * chunk_ms / 1000;

    println!("Loading config + tokenizer + mel filters ...");
    let config_bytes = fs::read(model_dir.join("config.json"))?;
    let config: NemotronConfig =
        serde_json::from_slice(&config_bytes)?;
    let tokenizer =
        Tokenizer::from_file(model_dir.join("tokenizer.json"))?;
    let mel_filter_bytes =
        fs::read(model_dir.join("mel_filters.bytes"))?;
    let mel_filters: Vec<f32> = mel_filter_bytes
        .chunks_exact(4)
        .map(|c| f32::from_le_bytes(c.try_into().unwrap()))
        .collect();

    println!(
        "Loading model.gguf (chunked, ~2.4 GB of F32) ..."
    );
    // Mirror the WASM lifecycle: read the GGUF header (first
    // `tensor_data_offset` bytes) to set up the `ModelBuilder`, then
    // stream the tensor-data section in 8 MB chunks via
    // `load_chunk`. This caps peak memory at roughly the loaded
    // model size plus one chunk, instead of (file bytes + loaded
    // model = ~5 GB) which causes hard OOM on Macs under pressure.
    use std::io::{BufReader, Read};
    let gguf_path = model_dir.join("model.gguf");
    let f = fs::File::open(&gguf_path)?;
    let mut reader = BufReader::new(f);

    let mut header = vec![0u8; config.tensor_data_offset];
    reader.read_exact(&mut header)?;
    let mut cursor = std::io::Cursor::new(&header[..]);
    let content = gguf_file::Content::read(&mut cursor)?;
    drop(cursor);
    drop(header);

    let mut builder = ModelBuilder::new(content);
    const LOAD_CHUNK_BYTES: usize = 8 * 1024 * 1024;
    let mut chunk_buf = vec![0u8; LOAD_CHUNK_BYTES];
    let mut total_loaded: usize = 0;
    loop {
        let n = reader.read(&mut chunk_buf)?;
        if n == 0 {
            break;
        }
        builder.load_chunk(&chunk_buf[..n])?;
        total_loaded += n;
        eprint!(
            "\r  streaming weights: {:.1} MB",
            total_loaded as f64 / 1e6
        );
    }
    eprintln!();
    let model = builder.build(&config)?;

    println!("Reading WAV from {} ...", wav_path);
    let pcm = read_wav_pcm_f32(&wav_path)?;
    println!(
        "  {} samples = {:.2} s @ {} Hz",
        pcm.len(),
        pcm.len() as f32 / SAMPLE_RATE as f32,
        SAMPLE_RATE
    );
    println!(
        "Streaming chunks of {} samples ({} ms) ...",
        chunk_samples, chunk_ms
    );
    println!();

    let mut encoder_cache = EncoderCache::new(&config, 1)?;
    let mut decode_state = DecodeState::new(&config)?;
    let mut transcript = String::new();
    let n_chunks = pcm.len().div_ceil(chunk_samples);

    for (i, chunk) in pcm.chunks(chunk_samples).enumerate() {
        let t0 = Instant::now();

        let mut padded = chunk.to_vec();
        if padded.len() < chunk_samples {
            padded.resize(chunk_samples, 0.0);
        }

        let mel = audio::pcm_to_mel(
            &padded,
            &mel_filters,
            &config,
        );
        let n_mels = config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor = Tensor::from_vec(
            mel,
            (1, n_mels, mel_len),
            &Device::Cpu,
        )?
        .transpose(1, 2)?
        .contiguous()?;

        let enc_out = model
            .encoder
            .forward(&mel_tensor, &mut encoder_cache)?;
        let tokens = rnnt_greedy_step(
            &model,
            &enc_out,
            &mut decode_state,
            &config,
        )?;

        let dt = t0.elapsed().as_millis();
        if tokens.is_empty() {
            println!(
                "[chunk {:3}/{:3}] {:>5} ms  (blank)",
                i + 1,
                n_chunks,
                dt
            );
        } else {
            let text = tokenizer.decode(&tokens, true)?;
            println!(
                "[chunk {:3}/{:3}] {:>5} ms  {text}",
                i + 1,
                n_chunks,
                dt
            );
            if !transcript.is_empty() {
                transcript.push(' ');
            }
            transcript.push_str(text.trim());
        }
    }

    println!();
    println!("=== FINAL TRANSCRIPT ===");
    println!("{transcript}");
    Ok(())
}

/// Minimal WAV reader. Accepts 16 kHz mono PCM in either s16le or
/// f32le -- what `ffmpeg -ac 1 -ar 16000` produces.
fn read_wav_pcm_f32(
    path: &str,
) -> Result<Vec<f32>, Box<dyn std::error::Error + Send + Sync>> {
    let bytes = fs::read(path)?;
    if &bytes[0..4] != b"RIFF" || &bytes[8..12] != b"WAVE" {
        return Err(format!("not a RIFF WAVE file: {path}").into());
    }

    let mut i = 12usize;
    let mut audio_format: u16 = 0;
    let mut channels: u16 = 0;
    let mut sample_rate: u32 = 0;
    let mut bits_per_sample: u16 = 0;
    let mut data_offset: usize = 0;
    let mut data_size: usize = 0;

    while i + 8 <= bytes.len() {
        let id = &bytes[i..i + 4];
        let sz = u32::from_le_bytes(
            bytes[i + 4..i + 8].try_into()?,
        ) as usize;
        let body_start = i + 8;
        let body_end = body_start + sz;
        match id {
            b"fmt " => {
                audio_format = u16::from_le_bytes(
                    bytes[body_start..body_start + 2].try_into()?,
                );
                channels = u16::from_le_bytes(
                    bytes[body_start + 2..body_start + 4]
                        .try_into()?,
                );
                sample_rate = u32::from_le_bytes(
                    bytes[body_start + 4..body_start + 8]
                        .try_into()?,
                );
                bits_per_sample = u16::from_le_bytes(
                    bytes[body_start + 14..body_start + 16]
                        .try_into()?,
                );
            }
            b"data" => {
                data_offset = body_start;
                data_size = sz;
                break;
            }
            _ => {}
        }
        i = body_end;
        if sz % 2 == 1 {
            i += 1;
        }
    }

    if data_offset == 0 {
        return Err("no data chunk in WAV".into());
    }
    if channels != 1 {
        return Err(format!(
            "expected mono WAV, got {channels} channels"
        )
        .into());
    }
    if sample_rate != SAMPLE_RATE {
        return Err(format!(
            "expected {} Hz, got {} Hz",
            SAMPLE_RATE, sample_rate
        )
        .into());
    }

    let data = &bytes[data_offset..data_offset + data_size];
    let pcm = match (audio_format, bits_per_sample) {
        (1, 16) => data
            .chunks_exact(2)
            .map(|c| {
                let v = i16::from_le_bytes(c.try_into().unwrap());
                v as f32 / 32768.0
            })
            .collect(),
        (3, 32) => data
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes(c.try_into().unwrap()))
            .collect(),
        (af, bps) => {
            return Err(format!(
                "unsupported WAV: format={af} bps={bps}"
            )
            .into());
        }
    };
    Ok(pcm)
}
