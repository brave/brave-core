/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* This file incorporates work covered by the following
 * copyright and permission notice:
 *
 * Copyright 2022 Hugging Face
 *
 * Licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the License is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See
 * the License for the specific language governing
 * permissions and limitations under the License.
 */

// Speech recognition WASM crate: Whisper and Parakeet
// models, using candle for inference.

mod audio;
mod model;
mod parakeet_audio;
mod parakeet_model;

use candle_core::{DType, Device, IndexOp, Tensor};
use candle_nn::VarBuilder;
use serde::{Deserialize, Serialize};
use tokenizers::Tokenizer;
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_namespace = console)]
    fn log(s: &str);
    #[wasm_bindgen(js_namespace = console, js_name = error)]
    fn log_error(s: &str);
    #[wasm_bindgen(js_namespace = performance)]
    fn now() -> f64;
}

// Whisper model configuration, matching the HuggingFace
// config.json format.
#[derive(Debug, Clone, PartialEq, Deserialize)]
pub struct Config {
    pub num_mel_bins: usize,
    pub max_source_positions: usize,
    pub d_model: usize,
    pub encoder_attention_heads: usize,
    pub encoder_layers: usize,
    pub vocab_size: usize,
    pub max_target_positions: usize,
    pub decoder_attention_heads: usize,
    pub decoder_layers: usize,
    #[serde(default)]
    pub suppress_tokens: Vec<u32>,
}

// Audio parameters
pub const SAMPLE_RATE: usize = 16000;
pub const N_FFT: usize = 400;
pub const HOP_LENGTH: usize = 160;
pub const CHUNK_LENGTH: usize = 30;
pub const N_SAMPLES: usize = CHUNK_LENGTH * SAMPLE_RATE;

// Special token strings
const SOT_TOKEN: &str = "<|startoftranscript|>";
const LANG_TOKEN: &str = "<|en|>";
const TRANSCRIBE_TOKEN: &str = "<|transcribe|>";
const NO_TIMESTAMPS_TOKEN: &str = "<|notimestamps|>";
const EOT_TOKEN: &str = "<|endoftext|>";

#[derive(Serialize)]
struct TranscribeResult {
    transcript: String,
    is_final: bool,
}


#[wasm_bindgen]
pub struct WhisperRecognizer {
    model: model::Whisper,
    tokenizer: Tokenizer,
    mel_filters: Vec<f32>,
    config: Config,
    audio_buffer: Vec<f32>,
    accumulated_transcript: String,
    /// Cached result from last transcribe() call.
    /// Used by mark_done to skip redundant inference
    /// when buffer hasn't changed.
    last_result: String,
    buffer_dirty: bool,
}

#[wasm_bindgen]
impl WhisperRecognizer {
    #[wasm_bindgen(constructor)]
    pub fn new(
        weights: Vec<u8>,
        tokenizer: Vec<u8>,
        mel_filters: Vec<u8>,
        config: Vec<u8>,
    ) -> Result<WhisperRecognizer, JsError> {
        console_error_panic_hook::set_once();

        let config: Config =
            serde_json::from_slice(&config).map_err(|e| JsError::new(&e.to_string()))?;

        let tokenizer =
            Tokenizer::from_bytes(&tokenizer).map_err(|e| JsError::new(&e.to_string()))?;

        // Parse mel filters from binary
        // (f32 little-endian)
        let mel_filters = mel_filters
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes(c.try_into().unwrap()))
            .collect::<Vec<f32>>();

        let vb = VarBuilder::from_buffered_safetensors(weights, DType::F32, &Device::Cpu)
            .map_err(|e| JsError::new(&e.to_string()))?;

        let model =
            model::Whisper::load(&vb, config.clone()).map_err(|e| JsError::new(&e.to_string()))?;

        Ok(Self {
            model,
            tokenizer,
            mel_filters,
            config,
            audio_buffer: Vec::new(),
            accumulated_transcript: String::new(),
            last_result: String::new(),
            buffer_dirty: false,
        })
    }

    /// Push audio samples. Accumulates audio and
    /// re-transcribes the entire buffer each time
    /// so Whisper sees full context. When buffer
    /// exceeds 30s, commits the segment transcript
    /// and keeps the overflow.
    pub fn add_audio(
        &mut self,
        audio: Vec<f32>,
    ) -> Result<String, JsError> {
        // Skip silent chunks. Don't add them (real
        // silence hurts Whisper, which is trained on
        // zero-padded segments) and don't re-run
        // inference (buffer unchanged, avoids ~2s
        // of wasted compute per silent chunk).
        let energy = Self::rms_energy(&audio);
        log_error(&format!(
            "[whisper] chunk energy={:.6} samples={}",
            energy,
            audio.len(),
        ));
        if energy < SILENCE_ENERGY_THRESHOLD {
            return Ok(String::new());
        }

        self.audio_buffer.extend_from_slice(&audio);
        self.buffer_dirty = true;

        // When buffer exceeds 30s, commit current
        // segment transcript and keep overflow.
        if self.audio_buffer.len() > N_SAMPLES {
            self.commit_segment()?;
        }

        self.transcribe(false)
    }

    /// Signal end of audio. Returns cached result
    /// if buffer hasn't changed, otherwise runs
    /// final inference.
    pub fn mark_done(
        &mut self,
    ) -> Result<String, JsError> {
        if self.audio_buffer.is_empty()
            && self.accumulated_transcript.is_empty()
        {
            return Ok(String::new());
        }
        if !self.buffer_dirty
            && !self.last_result.is_empty()
        {
            // Buffer unchanged since last inference.
            // Return cached result as final.
            let result = TranscribeResult {
                transcript:
                    self.last_result.clone(),
                is_final: true,
            };
            return serde_json::to_string(&result)
                .map_err(|e| {
                    JsError::new(&e.to_string())
                });
        }
        self.transcribe(true)
    }

    pub fn reset(&mut self) {
        self.model.reset_kv_cache();
        self.audio_buffer.clear();
        self.accumulated_transcript.clear();
        self.last_result.clear();
        self.buffer_dirty = false;
    }
}

// Minimum audio length for meaningful inference.
// 0.5s at 16kHz = 8000 samples.
const MIN_AUDIO_SAMPLES: usize = SAMPLE_RATE / 2;

// RMS energy below this is silence. Skip inference
// to save compute. Set conservatively low so quiet
// speech passes through.
// Observed: silence ~0.002-0.004,
//           quiet speech ~0.005-0.007,
//           normal speech ~0.02+.
const SILENCE_ENERGY_THRESHOLD: f32 = 0.003;

impl WhisperRecognizer {
    fn rms_energy(samples: &[f32]) -> f32 {
        if samples.is_empty() {
            return 0.0;
        }
        let sum_sq: f32 =
            samples.iter().map(|&s| s * s).sum();
        (sum_sq / samples.len() as f32).sqrt()
    }

    /// Commit the first 30s of audio as a finalized
    /// segment. Saves transcript and keeps overflow.
    fn commit_segment(
        &mut self,
    ) -> Result<(), JsError> {
        let segment: Vec<f32> = self
            .audio_buffer
            .drain(..N_SAMPLES)
            .collect();
        let text = self.run_inference(&segment)?;
        if !text.is_empty() {
            if !self.accumulated_transcript.is_empty()
            {
                self.accumulated_transcript.push(' ');
            }
            self.accumulated_transcript
                .push_str(text.trim());
        }
        Ok(())
    }

    /// Re-transcribe the entire audio buffer and
    /// return combined result (committed segments +
    /// current buffer).
    fn transcribe(
        &mut self,
        is_final: bool,
    ) -> Result<String, JsError> {
        let audio = self.audio_buffer.clone();
        let text = self.run_inference(&audio)?;

        // Combine committed segments with current.
        let mut full =
            self.accumulated_transcript.clone();
        if !text.is_empty() {
            if !full.is_empty() {
                full.push(' ');
            }
            full.push_str(text.trim());
        }

        self.last_result = full.clone();
        self.buffer_dirty = false;

        let result = TranscribeResult {
            transcript: full,
            is_final,
        };
        serde_json::to_string(&result)
            .map_err(|e| JsError::new(&e.to_string()))
    }

    /// Core inference: audio -> mel -> encode ->
    /// decode. Returns decoded text, or empty string
    /// if audio is too short or silent.
    fn run_inference(
        &mut self,
        audio_f32: &[f32],
    ) -> Result<String, JsError> {
        log_error(&format!(
            "[whisper] samples={}",
            audio_f32.len(),
        ));

        // Buffer only contains non-silent chunks
        // (filtered in add_audio), so no energy
        // check here. Just skip if too short.
        if audio_f32.len() < MIN_AUDIO_SAMPLES {
            return Ok(String::new());
        }

        let t0 = now();

        // 1. Compute mel spectrogram
        let mel = audio::pcm_to_mel(
            &self.config,
            audio_f32,
            &self.mel_filters,
        );
        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor = Tensor::from_vec(
            mel,
            (1, n_mels, mel_len),
            &Device::Cpu,
        )
        .map_err(|e| {
            JsError::new(&e.to_string())
        })?;

        let t1 = now();

        // 2. Encode
        let encoder_output = self
            .model
            .encoder
            .forward(&mel_tensor, true)
            .map_err(|e| {
                JsError::new(&e.to_string())
            })?;

        let t2 = now();

        // 3. Greedy decode
        let text = self
            .greedy_decode(&encoder_output)
            .map_err(|e| {
                JsError::new(&e.to_string())
            })?;

        let t3 = now();
        log_error(&format!(
            "[whisper] timing: mel={:.0}ms \
             encode={:.0}ms decode={:.0}ms \
             total={:.0}ms audio={:.1}s \
             text=\"{}\"",
            t1 - t0,
            t2 - t1,
            t3 - t2,
            t3 - t0,
            audio_f32.len() as f64
                / SAMPLE_RATE as f64,
            text,
        ));

        Ok(text)
    }

    fn greedy_decode(
        &mut self,
        encoder_output: &Tensor,
    ) -> candle_core::Result<String> {
        let sot_token = self
            .tokenizer
            .token_to_id(SOT_TOKEN)
            .unwrap_or(50258);
        let lang_token = self
            .tokenizer
            .token_to_id(LANG_TOKEN)
            .unwrap_or(50259);
        let transcribe_token = self
            .tokenizer
            .token_to_id(TRANSCRIBE_TOKEN)
            .unwrap_or(50359);
        let no_timestamps_token = self
            .tokenizer
            .token_to_id(NO_TIMESTAMPS_TOKEN)
            .unwrap_or(50363);
        let eot_token = self
            .tokenizer
            .token_to_id(EOT_TOKEN)
            .unwrap_or(50257);

        let mut tokens = vec![
            sot_token,
            lang_token,
            transcribe_token,
            no_timestamps_token,
        ];
        let max_tokens =
            self.config.max_target_positions / 2;

        self.model.decoder.reset_kv_cache();

        for i in 0..max_tokens {
            let tokens_tensor = Tensor::new(
                tokens.as_slice(),
                &Device::Cpu,
            )?
            .unsqueeze(0)?;

            let flush = i == 0;
            let logits = self.model.decoder.forward(
                &tokens_tensor,
                encoder_output,
                flush,
            )?;
            let logits =
                self.model.decoder.final_linear(&logits)?;

            let seq_len = logits.dim(1)?;
            let last_logits =
                logits.i((0, seq_len - 1))?;

            let last_logits =
                self.apply_suppress_tokens(&last_logits)?;

            let next_token = last_logits
                .argmax(0)?
                .to_scalar::<u32>()?;

            if next_token == eot_token {
                break;
            }

            tokens.push(next_token);
        }

        let output_tokens = &tokens[4..];
        self.tokenizer
            .decode(output_tokens, true)
            .map_err(|e| {
                candle_core::Error::Msg(e.to_string())
            })
    }

    fn apply_suppress_tokens(
        &self,
        logits: &Tensor,
    ) -> candle_core::Result<Tensor> {
        if self.config.suppress_tokens.is_empty() {
            return Ok(logits.clone());
        }
        let mut logits_vec =
            logits.to_vec1::<f32>()?;
        for &token_id in &self.config.suppress_tokens {
            let idx = token_id as usize;
            if idx < logits_vec.len() {
                logits_vec[idx] = f32::NEG_INFINITY;
            }
        }
        Tensor::from_vec(
            logits_vec,
            logits.shape(),
            logits.device(),
        )
    }
}

// ======================================================
// Parakeet (FastConformer + CTC) Recognizer
// ======================================================

#[wasm_bindgen]
pub struct ParakeetRecognizer {
    model: parakeet_model::Parakeet,
    tokenizer: Tokenizer,
    mel_filters: Vec<f32>,
    config: parakeet_model::ParakeetConfig,
    audio_buffer: Vec<f32>,
    accumulated_transcript: String,
    last_result: String,
    buffer_dirty: bool,
}

// Parakeet uses the same sample rate as Whisper.
const PARAKEET_MAX_SAMPLES: usize =
    30 * SAMPLE_RATE; // 30s segments

#[wasm_bindgen]
impl ParakeetRecognizer {
    #[wasm_bindgen(constructor)]
    pub fn new(
        weights: Vec<u8>,
        tokenizer: Vec<u8>,
        mel_filters: Vec<u8>,
        config: Vec<u8>,
    ) -> Result<ParakeetRecognizer, JsError> {
        console_error_panic_hook::set_once();

        let config: parakeet_model::ParakeetConfig =
            serde_json::from_slice(&config)
                .map_err(|e| {
                    JsError::new(&e.to_string())
                })?;

        let tokenizer = Tokenizer::from_bytes(
            &tokenizer,
        )
        .map_err(|e| JsError::new(&e.to_string()))?;

        let mel_filters = mel_filters
            .chunks_exact(4)
            .map(|c| {
                f32::from_le_bytes(
                    c.try_into().unwrap(),
                )
            })
            .collect::<Vec<f32>>();

        let vb =
            VarBuilder::from_buffered_safetensors(
                weights,
                DType::F32,
                &Device::Cpu,
            )
            .map_err(|e| {
                JsError::new(&e.to_string())
            })?;

        let model =
            parakeet_model::Parakeet::load(&vb, &config)
                .map_err(|e| {
                    JsError::new(&e.to_string())
                })?;

        Ok(Self {
            model,
            tokenizer,
            mel_filters,
            config,
            audio_buffer: Vec::new(),
            accumulated_transcript: String::new(),
            last_result: String::new(),
            buffer_dirty: false,
        })
    }

    pub fn add_audio(
        &mut self,
        audio: Vec<f32>,
    ) -> Result<String, JsError> {
        let energy =
            WhisperRecognizer::rms_energy(&audio);
        log_error(&format!(
            "[parakeet] chunk energy={:.6} \
             samples={}",
            energy,
            audio.len(),
        ));
        if energy < SILENCE_ENERGY_THRESHOLD {
            return Ok(String::new());
        }

        self.audio_buffer.extend_from_slice(&audio);
        self.buffer_dirty = true;

        if self.audio_buffer.len()
            > PARAKEET_MAX_SAMPLES
        {
            self.commit_segment()?;
        }

        self.transcribe(false)
    }

    pub fn mark_done(
        &mut self,
    ) -> Result<String, JsError> {
        if self.audio_buffer.is_empty()
            && self.accumulated_transcript.is_empty()
        {
            return Ok(String::new());
        }
        if !self.buffer_dirty
            && !self.last_result.is_empty()
        {
            let result = TranscribeResult {
                transcript: self
                    .last_result
                    .clone(),
                is_final: true,
            };
            return serde_json::to_string(&result)
                .map_err(|e| {
                    JsError::new(&e.to_string())
                });
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

impl ParakeetRecognizer {
    fn commit_segment(
        &mut self,
    ) -> Result<(), JsError> {
        let segment: Vec<f32> = self
            .audio_buffer
            .drain(..PARAKEET_MAX_SAMPLES)
            .collect();
        let text = self.run_inference(&segment)?;
        if !text.is_empty() {
            if !self
                .accumulated_transcript
                .is_empty()
            {
                self.accumulated_transcript.push(' ');
            }
            self.accumulated_transcript
                .push_str(text.trim());
        }
        Ok(())
    }

    fn transcribe(
        &mut self,
        is_final: bool,
    ) -> Result<String, JsError> {
        let audio = self.audio_buffer.clone();
        let text = self.run_inference(&audio)?;

        let mut full =
            self.accumulated_transcript.clone();
        if !text.is_empty() {
            if !full.is_empty() {
                full.push(' ');
            }
            full.push_str(text.trim());
        }

        self.last_result = full.clone();
        self.buffer_dirty = false;

        let result = TranscribeResult {
            transcript: full,
            is_final,
        };
        serde_json::to_string(&result)
            .map_err(|e| JsError::new(&e.to_string()))
    }

    fn run_inference(
        &self,
        audio_f32: &[f32],
    ) -> Result<String, JsError> {
        log_error(&format!(
            "[parakeet] samples={}",
            audio_f32.len(),
        ));

        if audio_f32.len() < MIN_AUDIO_SAMPLES {
            return Ok(String::new());
        }

        let t0 = now();

        // 1. Mel spectrogram (Parakeet-style)
        let mel =
            parakeet_audio::pcm_to_mel_parakeet(
                audio_f32,
                &self.mel_filters,
            );
        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor = Tensor::from_vec(
            mel,
            (1, n_mels, mel_len),
            &Device::Cpu,
        )
        .map_err(|e| {
            JsError::new(&e.to_string())
        })?;

        let t1 = now();

        // 2. Encoder forward (single pass)
        let logits = self
            .model
            .forward(&mel_tensor)
            .map_err(|e| {
                JsError::new(&e.to_string())
            })?;

        let t2 = now();

        // Debug: log mel and logits stats
        {
            let mel_flat =
                mel_tensor.flatten_all().map_err(
                    |e| JsError::new(&e.to_string()),
                )?;
            let mel_vec: Vec<f32> = mel_flat
                .to_vec1()
                .map_err(|e| {
                    JsError::new(&e.to_string())
                })?;
            let mel_min = mel_vec
                .iter()
                .cloned()
                .fold(f32::INFINITY, f32::min);
            let mel_max = mel_vec
                .iter()
                .cloned()
                .fold(f32::NEG_INFINITY, f32::max);
            log_error(&format!(
                "[parakeet] mel: shape=({},{}) \
                 min={:.3} max={:.3}",
                n_mels, mel_len, mel_min, mel_max,
            ));

            let logits_shape = logits.dims3().map_err(
                |e| JsError::new(&e.to_string()),
            )?;
            // Sample logits for first few frames
            let logits_flat: Vec<f32> = logits
                .squeeze(0)
                .map_err(|e| {
                    JsError::new(&e.to_string())
                })?
                .flatten_all()
                .map_err(|e| {
                    JsError::new(&e.to_string())
                })?
                .to_vec1()
                .map_err(|e| {
                    JsError::new(&e.to_string())
                })?;
            let vocab = logits_shape.2;
            let n_t = logits_shape.1;
            // Show argmax for first 10 frames
            let mut frame_info = String::new();
            for t in 0..n_t.min(10) {
                let frame_start = t * vocab;
                let frame_logits =
                    &logits_flat[frame_start
                        ..frame_start + vocab];
                let (argmax, max_val) = frame_logits
                    .iter()
                    .enumerate()
                    .max_by(|a, b| {
                        a.1.partial_cmp(b.1).unwrap()
                    })
                    .unwrap();
                frame_info.push_str(&format!(
                    "t{}:{}({:.1}) ",
                    t, argmax, max_val,
                ));
            }
            log_error(&format!(
                "[parakeet] logits: shape={:?} \
                 frames: {}",
                logits_shape, frame_info,
            ));
        }

        // 3. CTC greedy decode
        let blank_id =
            (self.config.vocab_size - 1) as u32;
        let token_ids =
            parakeet_model::ctc_greedy_decode(
                &logits, blank_id,
            )
            .map_err(|e| {
                JsError::new(&e.to_string())
            })?;

        log_error(&format!(
            "[parakeet] ctc: {} tokens, ids={:?}",
            token_ids.len(),
            &token_ids[..token_ids.len().min(30)],
        ));

        let text = self
            .tokenizer
            .decode(&token_ids, true)
            .map_err(|e| {
                JsError::new(&e.to_string())
            })?;

        let t3 = now();
        log_error(&format!(
            "[parakeet] timing: mel={:.0}ms \
             encode={:.0}ms ctc={:.0}ms \
             total={:.0}ms audio={:.1}s \
             text=\"{}\"",
            t1 - t0,
            t2 - t1,
            t3 - t2,
            t3 - t0,
            audio_f32.len() as f64
                / SAMPLE_RATE as f64,
            text,
        ));

        Ok(text)
    }
}
