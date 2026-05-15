/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// WASM entry point for Parakeet-TDT-0.6B-v3 (F16 GGUF variant).
// TdtRecognizer600mF16 owns the streaming session: GGUF loading,
// audio buffering, encoder + LSTM predictor + TDT joint decode, and
// tokenizer decode.

mod audio;
mod audio_fft;
mod model;

use candle_core::quantized::gguf_file;
use candle_core::{Device, Tensor};
use serde::Serialize;
use tokenizers::Tokenizer;
use wasm_bindgen::prelude::*;

use model::{TdtConfig, TdtModelBuilder, TdtParakeet};

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_namespace = console, js_name = error)]
    fn log_error(s: &str);
    #[wasm_bindgen(js_namespace = performance)]
    fn now() -> f64;
}

// 16 kHz mono PCM is the only sample rate Parakeet accepts (baked into
// the mel filterbank and the encoder config).
const SAMPLE_RATE: usize = 16_000;

// TDT (RNN-T autoregressive decoder) needs full utterance context for
// multilingual accuracy. MAX=2s tested and broke Spanish; MAX=5s lets
// multi-second utterances finish in one commit while keeping per-cycle
// RTF reasonable. Silence between utterances + mark_done from VAD
// drain the buffer in practice.
const TDT_MIN_SAMPLES: usize = SAMPLE_RATE;
const TDT_MAX_SAMPLES: usize = 5 * SAMPLE_RATE;

// RMS energy below this is treated as silence; the `add_audio` call
// returns an empty string without touching the buffer.
const SILENCE_ENERGY_THRESHOLD: f32 = 0.003;

#[derive(Serialize)]
struct TranscribeResult {
    transcript: String,
    is_final: bool,
}

enum TdtState {
    Loading(TdtModelBuilder),
    Ready(TdtParakeet),
}

#[wasm_bindgen]
pub struct TdtRecognizer600mF16 {
    model: TdtState,
    tokenizer: Tokenizer,
    mel_filters: Vec<f32>,
    config: TdtConfig,
    audio_buffer: Vec<f32>,
    accumulated_transcript: String,
    last_result: String,
    buffer_dirty: bool,
}

#[wasm_bindgen]
impl TdtRecognizer600mF16 {
    /// `weights` must be the GGUF header bytes (first
    /// `config.tensor_data_offset` bytes of `model.gguf`). Tensor data
    /// is streamed via `load_weight_chunk`. `tokenizer` is the raw
    /// bytes of `tokenizer.json`; `mel_filters` is a flat little-endian
    /// f32 matrix of shape `(num_mel_bins, n_fft/2+1)` serialized to
    /// bytes; `config` is the bytes of `config.json`.
    #[wasm_bindgen(constructor)]
    pub fn new(
        weights: Vec<u8>,
        tokenizer: Vec<u8>,
        mel_filters: Vec<u8>,
        config: Vec<u8>,
    ) -> Result<TdtRecognizer600mF16, JsError> {
        console_error_panic_hook::set_once();

        let config: TdtConfig =
            serde_json::from_slice(&config).map_err(|e| JsError::new(&e.to_string()))?;

        let tokenizer =
            Tokenizer::from_bytes(&tokenizer).map_err(|e| JsError::new(&e.to_string()))?;

        let mel_filters = mel_filters
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes(c.try_into().unwrap()))
            .collect::<Vec<f32>>();

        let mut cursor = std::io::Cursor::new(&weights);
        let content = gguf_file::Content::read(&mut cursor)
            .map_err(|e| JsError::new(&format!("GGUF parse: {e}")))?;
        drop(cursor);
        drop(weights);

        let builder = TdtModelBuilder::new(content, &config)
            .map_err(|e| JsError::new(&e.to_string()))?;

        Ok(Self {
            model: TdtState::Loading(builder),
            tokenizer,
            mel_filters,
            config,
            audio_buffer: Vec::new(),
            accumulated_transcript: String::new(),
            last_result: String::new(),
            buffer_dirty: false,
        })
    }

    /// Receive a chunk of tensor data (~8 MB).
    pub fn load_weight_chunk(&mut self, chunk: Vec<u8>) -> Result<(), JsError> {
        match &mut self.model {
            TdtState::Loading(builder) => {
                builder.load_chunk(&chunk).map_err(|e| JsError::new(&e.to_string()))
            }
            _ => Err(JsError::new("not in loading state")),
        }
    }

    /// Assemble the model from accumulated QTensors.
    pub fn finalize(&mut self) -> Result<(), JsError> {
        let model = std::mem::replace(&mut self.model, TdtState::Loading(TdtModelBuilder::empty()));
        match model {
            TdtState::Loading(builder) => {
                let m = builder.build().map_err(|e| JsError::new(&e.to_string()))?;
                self.model = TdtState::Ready(m);
                Ok(())
            }
            other => {
                self.model = other;
                Err(JsError::new("not in loading state"))
            }
        }
    }

    pub fn add_audio(&mut self, audio: Vec<f32>) -> Result<String, JsError> {
        if rms_energy(&audio) < SILENCE_ENERGY_THRESHOLD {
            return Ok(String::new());
        }

        self.audio_buffer.extend_from_slice(&audio);
        self.buffer_dirty = true;

        if self.audio_buffer.len() >= TDT_MAX_SAMPLES {
            self.commit_segment()?;
            self.last_result = self.accumulated_transcript.clone();
            return result_json(&self.last_result, false);
        }

        if self.audio_buffer.len() < TDT_MIN_SAMPLES {
            return result_json(&self.last_result, false);
        }

        self.transcribe(false)
    }

    pub fn mark_done(&mut self) -> Result<String, JsError> {
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
impl TdtRecognizer600mF16 {
    fn commit_segment(&mut self) -> Result<(), JsError> {
        let drain_len = TDT_MAX_SAMPLES.min(self.audio_buffer.len());
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

    fn run_inference(&mut self, audio_f32: &[f32]) -> Result<String, JsError> {
        if audio_f32.is_empty() {
            return Ok(String::new());
        }

        let t0 = now();

        // 1. Mel spectrogram (128 bins, NeMo style)
        let mel = audio::pcm_to_mel_tdt(audio_f32, &self.mel_filters);
        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        // NeMo layout: (B, mel, time) — no transpose; QSubsampling
        // handles the transpose internally.
        let mel_tensor = Tensor::from_vec(mel, (1, n_mels, mel_len), &Device::Cpu)
            .map_err(|e| JsError::new(&e.to_string()))?;

        let t1 = now();

        let model = match &mut self.model {
            TdtState::Ready(m) => m,
            TdtState::Loading(_) => {
                return Err(JsError::new("model not finalized"));
            }
        };
        let token_ids =
            model.transcribe(&mel_tensor).map_err(|e| JsError::new(&e.to_string()))?;

        let t2 = now();

        let text = self
            .tokenizer
            .decode(&token_ids, true)
            .map_err(|e| JsError::new(&e.to_string()))?;

        let t3 = now();

        log_error(&format!(
            "[tdt-600m-f16] timing: mel={:.0}ms enc+dec={:.0}ms tok={:.0}ms \
             total={:.0}ms audio={:.1}s tokens={} text=\"{}\"",
            t1 - t0,
            t2 - t1,
            t3 - t2,
            t3 - t0,
            audio_f32.len() as f64 / SAMPLE_RATE as f64,
            token_ids.len(),
            text,
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

fn result_json(transcript: &str, is_final: bool) -> Result<String, JsError> {
    serde_json::to_string(&TranscribeResult { transcript: transcript.to_string(), is_final })
        .map_err(|e| JsError::new(&e.to_string()))
}
