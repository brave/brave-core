/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// WASM entry point for nemotron-speech-streaming-en-0.6b. Cache-
// aware streaming: each `add_audio` chunk advances the encoder's
// per-layer K/V + depthwise-conv caches and the RNN-T predictor
// state, instead of re-running the full audio buffer through a
// stateless model each time.

// Modules exposed publicly so native diagnostic binaries (under
// `examples/`) can drive the model directly. The WASM build only
// uses them through the `NemotronTranscriber` wrapper below.
pub mod audio;
pub mod decode;
pub mod model;

#[cfg(target_arch = "wasm32")]
use candle_core::quantized::gguf_file;
#[cfg(target_arch = "wasm32")]
use candle_core::{Device, Tensor};
#[cfg(target_arch = "wasm32")]
use serde::Serialize;
#[cfg(target_arch = "wasm32")]
use tokenizers::Tokenizer;
#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

#[cfg(target_arch = "wasm32")]
use decode::{rnnt_greedy_step, DecodeState};
#[cfg(target_arch = "wasm32")]
use model::{EncoderCache, ModelBuilder, Nemotron, NemotronConfig};

// Everything below is the WASM-side wrapper. Native builds (used by
// the `examples/transcribe.rs` diagnostic binary) only need the
// inner modules above.
#[cfg(target_arch = "wasm32")]
mod wasm_api {
use super::*;

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_namespace = console, js_name = error)]
    fn log_error(s: &str);
    #[wasm_bindgen(js_namespace = performance)]
    fn now() -> f64;
}

// nemotron-speech-streaming uses 16 kHz mono PCM. Baked into both
// the mel filterbank and the encoder.
const SAMPLE_RATE: usize = 16_000;

// Streaming chunk size. Each chunk is processed once: encoder
// advances its cache, RNN-T decode produces 0..N new tokens. The
// upstream model card lists 80/160/560/1120 ms chunk options
// (matching att_context_size right_context = 0/1/6/13). 1120 ms
// is the largest training context and gives the best WER -- on
// our native test, dropping to 560 ms more than doubled WER. The
// latency cost is acceptable for the dictation use case Brave's
// WebSpeech path covers.
const CHUNK_AUDIO_MS: usize = 1120;
const CHUNK_SAMPLES: usize = SAMPLE_RATE * CHUNK_AUDIO_MS / 1000;

// RMS below this counts as silence -- skip the encoder + decode
// pass on near-zero buffers to save CPU. Cache state stays.
const SILENCE_ENERGY_THRESHOLD: f32 = 0.003;

// Cap on the per-session audio retained for the final non-streaming
// re-pass. ~30 s covers the typical Web Speech dictation window;
// older audio rolls off the front. 30 s of f32 mono @ 16 kHz =
// 480 000 samples = ~1.83 MB, cheap.
const FULL_AUDIO_MAX_SAMPLES: usize = SAMPLE_RATE * 30;

#[derive(Serialize)]
struct TranscribeResult {
    transcript: String,
    is_final: bool,
}

enum NemotronState {
    Loading(Option<ModelBuilder>),
    Ready(Nemotron),
}

#[wasm_bindgen]
pub struct NemotronTranscriber {
    state: NemotronState,
    tokenizer: Tokenizer,
    mel_filters: Vec<f32>,
    config: NemotronConfig,
    encoder_cache: Option<EncoderCache>,
    decode_state: Option<DecodeState>,
    audio_buffer: Vec<f32>,
    accumulated_transcript: String,
    last_result: String,
    /// True once any incoming chunk has RMS above the silence
    /// threshold. We KEEP leading silence (it warms the encoder
    /// cache before the first speech chunk, lifting the RNN-T
    /// emission-lag floor) and DROP trailing silence once speech
    /// has been seen.
    has_seen_speech: bool,
    /// Number of run_chunk calls so far in this session. Used for
    /// per-chunk debug logging only.
    chunk_index: usize,
    /// Full session audio retained for the final non-streaming
    /// re-pass at `mark_done`. RNN-T streaming has emission lag
    /// that swallows the first chunk's words on short utterances;
    /// re-running with fresh state + the entire audio at once
    /// recovers them. Capped at FULL_AUDIO_MAX_SAMPLES; older
    /// samples fall off the front when the cap is hit.
    full_audio: Vec<f32>,
}

#[wasm_bindgen]
impl NemotronTranscriber {
    /// `weights` must be the first `config.tensor_data_offset` bytes
    /// of `model.gguf` (the header). `tokenizer`, `mel_filters`,
    /// `config` are the bytes of the matching JSON / filterbank /
    /// JSON files.
    #[wasm_bindgen(constructor)]
    pub fn new(
        weights: Vec<u8>,
        tokenizer: Vec<u8>,
        mel_filters: Vec<u8>,
        config: Vec<u8>,
    ) -> Result<NemotronTranscriber, JsError> {
        console_error_panic_hook::set_once();

        let config: NemotronConfig = serde_json::from_slice(&config)
            .map_err(|e| JsError::new(&e.to_string()))?;

        let tokenizer = Tokenizer::from_bytes(&tokenizer)
            .map_err(|e| JsError::new(&e.to_string()))?;

        let mel_filters = mel_filters
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes(c.try_into().unwrap()))
            .collect::<Vec<f32>>();

        let mut cursor = std::io::Cursor::new(&weights);
        let content = gguf_file::Content::read(&mut cursor)
            .map_err(|e| JsError::new(&format!("GGUF parse: {e}")))?;
        drop(cursor);
        let cfg_offset = config.tensor_data_offset;
        let gguf_offset = content.tensor_data_offset as usize;
        if weights.len() != cfg_offset || gguf_offset != cfg_offset {
            return Err(JsError::new(&format!(
                "tensor_data_offset mismatch: weights={} config={} gguf={}",
                weights.len(),
                cfg_offset,
                gguf_offset,
            )));
        }
        drop(weights);

        Ok(Self {
            state: NemotronState::Loading(Some(ModelBuilder::new(
                content,
            ))),
            tokenizer,
            mel_filters,
            config,
            encoder_cache: None,
            decode_state: None,
            audio_buffer: Vec::new(),
            accumulated_transcript: String::new(),
            last_result: String::new(),
            has_seen_speech: false,
            chunk_index: 0,
            full_audio: Vec::new(),
        })
    }

    pub fn load_weight_chunk(
        &mut self,
        chunk: Vec<u8>,
    ) -> Result<(), JsError> {
        match &mut self.state {
            NemotronState::Loading(Some(b)) => b
                .load_chunk(&chunk)
                .map_err(|e| JsError::new(&e.to_string())),
            _ => Err(JsError::new("not in loading state")),
        }
    }

    pub fn finalize(&mut self) -> Result<(), JsError> {
        let builder = match &mut self.state {
            NemotronState::Loading(slot) => slot
                .take()
                .ok_or_else(|| JsError::new("not in loading state"))?,
            _ => return Err(JsError::new("not in loading state")),
        };
        let model = builder
            .build(&self.config)
            .map_err(|e| JsError::new(&e.to_string()))?;
        self.state = NemotronState::Ready(model);
        // Initialize the streaming state once the model is loaded.
        self.encoder_cache = Some(
            EncoderCache::new(&self.config, 1)
                .map_err(|e| JsError::new(&e.to_string()))?,
        );
        self.decode_state = Some(
            DecodeState::new(&self.config)
                .map_err(|e| JsError::new(&e.to_string()))?,
        );
        Ok(())
    }

    pub fn add_audio(
        &mut self,
        audio: Vec<f32>,
    ) -> Result<String, JsError> {
        if !matches!(self.state, NemotronState::Ready(_)) {
            return Err(JsError::new("model not finalized"));
        }

        // Silence handling. RNN-T has heavy emission lag on its
        // first chunk (empty cache + empty predictor state), so we
        // WANT the pre-speech silence to reach the model -- those
        // silent chunks warm up the encoder cache and let the first
        // real-speech chunk see non-zero left context, just like in
        // training. After speech has been observed, trailing silence
        // is dropped so we don't waste encoder passes once the user
        // finishes speaking.
        let speaking = rms_energy(&audio) >= SILENCE_ENERGY_THRESHOLD;
        if !speaking && self.has_seen_speech {
            return Ok(String::new());
        }
        if speaking {
            self.has_seen_speech = true;
        }

        self.audio_buffer.extend_from_slice(&audio);

        // Also retain every sample for the non-streaming re-pass at
        // mark_done. Roll the front off when we exceed the cap so
        // peak memory stays bounded.
        self.full_audio.extend_from_slice(&audio);
        if self.full_audio.len() > FULL_AUDIO_MAX_SAMPLES {
            let drop_n = self.full_audio.len() - FULL_AUDIO_MAX_SAMPLES;
            self.full_audio.drain(..drop_n);
        }

        // Drain whole chunks until the buffer is short of a full
        // chunk -- the residue waits for the next add_audio call.
        while self.audio_buffer.len() >= CHUNK_SAMPLES {
            let chunk: Vec<f32> =
                self.audio_buffer.drain(..CHUNK_SAMPLES).collect();
            let text = self.run_chunk(&chunk)?;
            if !text.is_empty() {
                if !self.accumulated_transcript.is_empty() {
                    self.accumulated_transcript.push(' ');
                }
                self.accumulated_transcript.push_str(text.trim());
            }
        }

        self.last_result = self.accumulated_transcript.clone();
        result_json(&self.last_result, false)
    }

    pub fn mark_done(&mut self) -> Result<String, JsError> {
        if !matches!(self.state, NemotronState::Ready(_)) {
            return Err(JsError::new("model not finalized"));
        }
        // Flush any residue through the streaming path so the interim
        // result reflects everything we ever buffered.
        if !self.audio_buffer.is_empty() {
            let mut chunk = std::mem::take(&mut self.audio_buffer);
            if chunk.len() < CHUNK_SAMPLES {
                chunk.resize(CHUNK_SAMPLES, 0.0);
            }
            let text = self.run_chunk(&chunk)?;
            if !text.is_empty() {
                if !self.accumulated_transcript.is_empty() {
                    self.accumulated_transcript.push(' ');
                }
                self.accumulated_transcript.push_str(text.trim());
            }
        }

        // We previously ran a non-streaming re-pass over the full
        // utterance at this point to try to recover the early words
        // that streaming RNN-T's emission lag drops. It backfired:
        // the model was trained in chunked / cache-aware mode, so a
        // single big self-attention pass is out-of-distribution and
        // the quality went DOWN (e.g. emitted just `.` on a 2.5 s
        // clip where streaming had gotten `like today.`).
        // `final_pass` and `full_audio` are kept in case a future
        // experiment wants them, but the FINAL transcript is now
        // just the streaming accumulation.
        self.last_result = self.accumulated_transcript.clone();
        result_json(&self.last_result, true)
    }

    pub fn reset(&mut self) {
        self.audio_buffer.clear();
        self.full_audio.clear();
        self.accumulated_transcript.clear();
        self.last_result.clear();
        self.has_seen_speech = false;
        self.chunk_index = 0;
        // Zero the per-layer encoder cache and reset the predictor
        // to its initial (blank-token-seeded) state.
        if let Some(cache) = &mut self.encoder_cache {
            let _ = cache.clear(&self.config);
        }
        if let Some(decode_state) = &mut self.decode_state {
            if let Ok(ps) = model::PredictorState::new(&self.config) {
                decode_state.predictor = ps;
            }
            decode_state.prev_token = self.config.blank_id as u32;
        }
    }
}

impl NemotronTranscriber {
    /// Non-streaming pass over `full_audio` with fresh encoder cache
    /// + predictor state. Called from `mark_done` to produce the
    /// FINAL transcript free of the streaming-RNN-T emission lag
    /// that swallows the first chunk's words.
    fn final_pass(&self) -> Result<String, JsError> {
        if self.full_audio.is_empty() {
            return Ok(String::new());
        }
        let model = match &self.state {
            NemotronState::Ready(m) => m,
            _ => return Err(JsError::new("model not finalized")),
        };

        let t0 = now();

        let mel = audio::pcm_to_mel(
            &self.full_audio,
            &self.mel_filters,
            &self.config,
        );
        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor = Tensor::from_vec(
            mel,
            (1, n_mels, mel_len),
            &Device::Cpu,
        )
        .and_then(|t| t.transpose(1, 2))
        .and_then(|t| t.contiguous())
        .map_err(|e| JsError::new(&e.to_string()))?;

        let t1 = now();

        let mut cache = EncoderCache::new(&self.config, 1)
            .map_err(|e| JsError::new(&e.to_string()))?;
        let enc_out = model
            .encoder
            .forward(&mel_tensor, &mut cache)
            .map_err(|e| JsError::new(&e.to_string()))?;

        let t2 = now();

        let mut decode_state = DecodeState::new(&self.config)
            .map_err(|e| JsError::new(&e.to_string()))?;
        let token_ids = rnnt_greedy_step(
            model,
            &enc_out,
            &mut decode_state,
            &self.config,
        )
        .map_err(|e| JsError::new(&e.to_string()))?;

        let t3 = now();

        let text = if token_ids.is_empty() {
            String::new()
        } else {
            self.tokenizer
                .decode(&token_ids, true)
                .map_err(|e| JsError::new(&e.to_string()))?
        };

        let t4 = now();

        log_error(&format!(
            "[nemotron] final_pass audio={:.2}s mel={:.0}ms \
             encoder={:.0}ms rnnt={:.0}ms tokenize={:.0}ms \
             total={:.0}ms tokens={} text=\"{}\"",
            self.full_audio.len() as f32 / SAMPLE_RATE as f32,
            t1 - t0,
            t2 - t1,
            t3 - t2,
            t4 - t3,
            t4 - t0,
            token_ids.len(),
            text,
        ));

        Ok(text)
    }

    /// Process exactly one chunk (CHUNK_SAMPLES samples) through the
    /// streaming encoder + RNN-T decode. Returns the chunk's emitted
    /// text (may be empty when the chunk is silent or produces only
    /// blanks).
    fn run_chunk(&mut self, pcm: &[f32]) -> Result<String, JsError> {
        let model = match &self.state {
            NemotronState::Ready(m) => m,
            _ => return Err(JsError::new("model not finalized")),
        };
        let cache = self
            .encoder_cache
            .as_mut()
            .ok_or_else(|| JsError::new("no encoder cache"))?;
        let decode_state = self
            .decode_state
            .as_mut()
            .ok_or_else(|| JsError::new("no decode state"))?;

        let chunk_idx = self.chunk_index;
        self.chunk_index += 1;
        let chunk_rms = rms_energy(pcm);
        // Cache extent BEFORE the encoder forward updates it. Layer 0
        // is representative; all layers share the same time dim.
        let pre_cache_len = cache
            .last_channel
            .first()
            .and_then(|t| t.dim(1).ok())
            .unwrap_or(0);
        let prev_token = decode_state.prev_token;

        let t0 = now();

        // Mel preprocessing: (n_mels, T) row-major. Encoder wants
        // (B, T, n_mels) -- reshape + transpose.
        let mel = audio::pcm_to_mel(pcm, &self.mel_filters, &self.config);
        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor =
            Tensor::from_vec(mel, (1, n_mels, mel_len), &Device::Cpu)
                .and_then(|t| t.transpose(1, 2))
                .and_then(|t| t.contiguous())
                .map_err(|e| JsError::new(&e.to_string()))?;

        let t1 = now();

        let enc_out = model
            .encoder
            .forward(&mel_tensor, cache)
            .map_err(|e| JsError::new(&e.to_string()))?;
        // Read post-encoder cache length through the same borrow so
        // we don't have to re-borrow self.encoder_cache later.
        let post_cache_len = cache
            .last_channel
            .first()
            .and_then(|t| t.dim(1).ok())
            .unwrap_or(0);

        let t2 = now();

        // Skip the RNN-T decoder on the first chunk of a session:
        // it's pure warmup for the encoder cache, and the first
        // emission tends to be a spurious "." that then poisons the
        // predictor's autoregressive history for every chunk after
        // it (each later chunk sees prev_token = "." and the joint
        // collapses to blank). Leaving prev_token at blank_id lets
        // chunk 1 start emitting words against a non-empty cache,
        // mirroring how the native test's chunk 2 recovers cleanly
        // even after chunk 1's spurious ".".
        let token_ids = if chunk_idx == 0 {
            Vec::new()
        } else {
            rnnt_greedy_step(model, &enc_out, decode_state, &self.config)
                .map_err(|e| JsError::new(&e.to_string()))?
        };
        let post_prev_token = decode_state.prev_token;

        let t3 = now();

        let text = if token_ids.is_empty() {
            String::new()
        } else {
            self.tokenizer
                .decode(&token_ids, true)
                .map_err(|e| JsError::new(&e.to_string()))?
        };

        let t4 = now();

        log_error(&format!(
            "[nemotron] chunk={chunk_idx} rms={chunk_rms:.4} \
             cache={pre_cache_len}->{post_cache_len} \
             prev_token={prev_token}->{post_prev_token} \
             tokens={n_toks} \
             mel={:.0}ms encoder={:.0}ms rnnt={:.0}ms \
             tokenize={:.0}ms total={:.0}ms audio={:.2}s \
             text=\"{}\"",
            t1 - t0,
            t2 - t1,
            t3 - t2,
            t4 - t3,
            t4 - t0,
            pcm.len() as f32 / SAMPLE_RATE as f32,
            text,
            n_toks = token_ids.len(),
        ));

        Ok(text)
    }
}

fn rms_energy(samples: &[f32]) -> f32 {
    if samples.is_empty() {
        return 0.0;
    }
    let sum_sq: f32 = samples.iter().map(|&s| s * s).sum();
    (sum_sq / samples.len() as f32).sqrt()
}

fn result_json(
    transcript: &str,
    is_final: bool,
) -> Result<String, JsError> {
    serde_json::to_string(&TranscribeResult {
        transcript: transcript.to_string(),
        is_final,
    })
    .map_err(|e| JsError::new(&e.to_string()))
}

} // mod wasm_api
