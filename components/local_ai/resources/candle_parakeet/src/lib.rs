/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// WASM entry point for Parakeet-CTC-110M speech recognition.
// ParakeetTranscriber owns the streaming session: GGUF loading,
// audio buffering, model forward, CTC decode, and tokenizer decode.

mod audio;
mod model;

use candle_core::quantized::gguf_file;
use candle_core::{Device, Tensor, D};
use serde::Serialize;
use tokenizers::Tokenizer;
use wasm_bindgen::prelude::*;

use model::{ModelBuilder, Parakeet, ParakeetConfig};

// 16 kHz mono PCM is the only sample rate Parakeet-CTC-110M
// accepts (baked into the mel filterbank and the encoder config).
const SAMPLE_RATE: usize = 16_000;

// Growing-window cap: re-transcribe the whole accumulating buffer
// each call so Parakeet sees full context, and only drain when the
// buffer would otherwise exceed this length. 30 s mirrors Whisper's
// hard segment length and matches the previous working baseline.
const PARAKEET_COMMIT_SAMPLES: usize = SAMPLE_RATE * 30; // 30 s

// Minimum buffer length before running inference. Below this,
// run_inference returns an empty string; the streaming caller still
// emits the accumulated_transcript as the interim text.
const PARAKEET_MIN_SAMPLES: usize = SAMPLE_RATE / 2; // 500 ms

// RMS energy below this is treated as silence; the `add_audio`
// call returns an empty string without touching the buffer.
const SILENCE_ENERGY_THRESHOLD: f32 = 0.003;

#[derive(Serialize)]
struct TranscribeResult {
    transcript: String,
    is_final: bool,
}

// Streaming lifecycle state. `Loading(Some(_))` is the normal
// loading phase; `Loading(None)` appears only if `finalize` took
// the builder but failed to build the model (subsequent calls
// error out naturally). `Ready` is the post-finalize state.
enum ParakeetState {
    Loading(Option<ModelBuilder>),
    Ready(Parakeet),
}

#[wasm_bindgen]
pub struct ParakeetTranscriber {
    state: ParakeetState,
    tokenizer: Tokenizer,
    mel_filters: Vec<f32>,
    config: ParakeetConfig,
    audio_buffer: Vec<f32>,
    accumulated_transcript: String,
    last_result: String,
    buffer_dirty: bool,
}

#[wasm_bindgen]
impl ParakeetTranscriber {
    /// `weights` must be the first `config.tensor_data_offset` bytes
    /// of `model.gguf` (the header). The tensor-data blob that
    /// follows is streamed via `load_weight_chunk`. `tokenizer` is
    /// the raw bytes of `tokenizer.json`; `mel_filters` is a flat
    /// little-endian f32 matrix of shape (num_mel_bins, n_fft/2+1)
    /// serialized to bytes; `config` is the bytes of `config.json`.
    #[wasm_bindgen(constructor)]
    pub fn new(
        weights: Vec<u8>,
        tokenizer: Vec<u8>,
        mel_filters: Vec<u8>,
        config: Vec<u8>,
    ) -> Result<ParakeetTranscriber, JsError> {
        console_error_panic_hook::set_once();

        let config: ParakeetConfig =
            serde_json::from_slice(&config).map_err(|e| JsError::new(&e.to_string()))?;

        let tokenizer =
            Tokenizer::from_bytes(&tokenizer).map_err(|e| JsError::new(&e.to_string()))?;

        let mel_filters = mel_filters
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes(c.try_into().unwrap()))
            .collect::<Vec<f32>>();

        // Parse the GGUF header so the builder knows each tensor's
        // offset, shape, and dtype before chunks start arriving.
        let mut cursor = std::io::Cursor::new(&weights);
        let content = gguf_file::Content::read(&mut cursor)
            .map_err(|e| JsError::new(&format!("GGUF parse: {e}")))?;
        drop(cursor);
        // Validate that JS sliced exactly the header out of the GGUF
        // file and that config.json is in sync with the shipped GGUF.
        // Without this, a mismatch would silently misalign every
        // tensor offset in `load_weight_chunk` and produce garbled
        // weights with no obvious error.
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
            state: ParakeetState::Loading(Some(ModelBuilder::new(content))),
            tokenizer,
            mel_filters,
            config,
            audio_buffer: Vec::new(),
            accumulated_transcript: String::new(),
            last_result: String::new(),
            buffer_dirty: false,
        })
    }

    pub fn load_weight_chunk(&mut self, chunk: Vec<u8>) -> Result<(), JsError> {
        match &mut self.state {
            ParakeetState::Loading(Some(b)) => {
                b.load_chunk(&chunk).map_err(|e| JsError::new(&e.to_string()))
            }
            _ => Err(JsError::new("not in loading state")),
        }
    }

    pub fn finalize(&mut self) -> Result<(), JsError> {
        let builder = match &mut self.state {
            ParakeetState::Loading(slot) => {
                slot.take().ok_or_else(|| JsError::new("not in loading state"))?
            }
            _ => return Err(JsError::new("not in loading state")),
        };
        // `self.state` is now `Loading(None)`; no borrow remains.
        let model = builder.build(&self.config).map_err(|e| JsError::new(&e.to_string()))?;
        self.state = ParakeetState::Ready(model);
        Ok(())
    }

    pub fn add_audio(&mut self, audio: Vec<f32>) -> Result<String, JsError> {
        if !matches!(self.state, ParakeetState::Ready(_)) {
            return Err(JsError::new("model not finalized"));
        }
        // Skip silent chunks entirely. Don't extend the buffer
        // (avoids encoding bloat with trailing silence) and don't
        // re-run inference (buffer unchanged → result unchanged).
        if rms_energy(&audio) < SILENCE_ENERGY_THRESHOLD {
            return Ok(String::new());
        }

        self.audio_buffer.extend_from_slice(&audio);
        self.buffer_dirty = true;

        // When the buffer exceeds the growing-window cap, drain the
        // first PARAKEET_COMMIT_SAMPLES into accumulated_transcript
        // and keep the overflow as the new in-flight buffer.
        if self.audio_buffer.len() > PARAKEET_COMMIT_SAMPLES {
            self.commit_segment()?;
        }

        // Always re-transcribe so the caller sees a fresh interim
        // covering the leftover after a commit. run_inference's
        // MIN_SAMPLES guard handles too-short buffers internally.
        self.transcribe(false)
    }

    pub fn mark_done(&mut self) -> Result<String, JsError> {
        if !matches!(self.state, ParakeetState::Ready(_)) {
            return Err(JsError::new("model not finalized"));
        }
        if self.audio_buffer.is_empty() && self.accumulated_transcript.is_empty() {
            return Ok(String::new());
        }
        if !self.buffer_dirty && !self.last_result.is_empty() {
            return result_json(&self.last_result, true);
        }
        self.transcribe(true)
    }

    pub fn reset(&mut self) {
        self.audio_buffer.clear();
        self.accumulated_transcript.clear();
        self.last_result.clear();
        self.buffer_dirty = false;
    }
}

// Non-wasm-bindgen helpers.
impl ParakeetTranscriber {
    fn commit_segment(&mut self) -> Result<(), JsError> {
        let drain_len = PARAKEET_COMMIT_SAMPLES.min(self.audio_buffer.len());
        let segment: Vec<f32> = self.audio_buffer.drain(..drain_len).collect();
        let text = self.run_inference(&segment)?;
        if !text.is_empty() {
            if !self.accumulated_transcript.is_empty() {
                self.accumulated_transcript.push(' ');
            }
            self.accumulated_transcript.push_str(text.trim());
        }
        Ok(())
    }

    fn transcribe(&mut self, is_final: bool) -> Result<String, JsError> {
        let audio = self.audio_buffer.clone();
        let text = self.run_inference(&audio)?;

        let mut full = self.accumulated_transcript.clone();
        if !text.is_empty() {
            if !full.is_empty() {
                full.push(' ');
            }
            full.push_str(text.trim());
        }

        self.last_result = full.clone();
        self.buffer_dirty = false;
        result_json(&full, is_final)
    }

    fn run_inference(&self, pcm: &[f32]) -> Result<String, JsError> {
        // Buffer too short to be useful — encoder needs at least a
        // few mel frames of context before CTC produces anything
        // meaningful. The streaming caller still emits the
        // accumulated_transcript as the interim.
        if pcm.len() < PARAKEET_MIN_SAMPLES {
            return Ok(String::new());
        }

        let model = match &self.state {
            ParakeetState::Ready(m) => m,
            _ => return Err(JsError::new("model not finalized")),
        };

        // Mel preprocessing returns (num_mel_bins, T) row-major.
        // Encoder wants (B, T, num_mel_bins), so reshape + transpose.
        let mel = audio::pcm_to_mel(pcm, &self.mel_filters);
        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor = Tensor::from_vec(mel, (1, n_mels, mel_len), &Device::Cpu)
            .and_then(|t| t.transpose(1, 2))
            .and_then(|t| t.contiguous())
            .map_err(|e| JsError::new(&e.to_string()))?;

        let logits = model.forward(&mel_tensor).map_err(|e| JsError::new(&e.to_string()))?;

        let blank_id = (self.config.vocab_size - 1) as u32;
        let token_ids =
            ctc_greedy_decode(&logits, blank_id).map_err(|e| JsError::new(&e.to_string()))?;

        self.tokenizer.decode(&token_ids, true).map_err(|e| JsError::new(&e.to_string()))
    }
}

fn rms_energy(samples: &[f32]) -> f32 {
    if samples.is_empty() {
        return 0.0;
    }
    let sum_sq: f32 = samples.iter().map(|&s| s * s).sum();
    (sum_sq / samples.len() as f32).sqrt()
}

fn result_json(transcript: &str, is_final: bool) -> Result<String, JsError> {
    serde_json::to_string(&TranscribeResult { transcript: transcript.to_string(), is_final })
        .map_err(|e| JsError::new(&e.to_string()))
}

/// Greedy CTC decode: per-frame argmax, collapse consecutive
/// duplicates, drop the blank token. The last vocab id is reserved
/// for blank by convention in Parakeet's tokenizer.
fn ctc_greedy_decode(logits: &Tensor, blank_id: u32) -> candle_core::Result<Vec<u32>> {
    // logits: (B, T, vocab). We run B = 1, so drop the batch axis.
    let predictions = logits.squeeze(0)?.argmax(D::Minus1)?;
    let preds: Vec<u32> = predictions.to_vec1::<u32>()?;

    let mut result = Vec::new();
    let mut prev = blank_id;
    for &tok in &preds {
        if tok != prev && tok != blank_id {
            result.push(tok);
        }
        prev = tok;
    }
    Ok(result)
}
