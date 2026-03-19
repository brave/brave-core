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

// Whisper speech recognition WASM crate, using model
// code adapted from candle-transformers.

mod audio;
mod model;

use candle_core::{DType, Device, IndexOp, Tensor};
use candle_nn::VarBuilder;
use serde::{Deserialize, Serialize};
use tokenizers::Tokenizer;
use wasm_bindgen::prelude::*;

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

        Ok(Self { model, tokenizer, mel_filters, config })
    }

    /// Transcribe audio samples (int16 PCM at 16kHz).
    /// Returns JSON: {"transcript":"...","is_final":true}
    pub fn transcribe(&mut self, audio_pcm16: Vec<i16>) -> Result<String, JsError> {
        // Convert int16 PCM to f32 in [-1.0, 1.0].
        let audio_f32: Vec<f32> = audio_pcm16.iter().map(|&s| s as f32 / 32768.0).collect();

        // 1. Compute mel spectrogram
        let mel = audio::pcm_to_mel(&self.config, &audio_f32, &self.mel_filters);

        let n_mels = self.config.num_mel_bins;
        let mel_len = mel.len() / n_mels;
        let mel_tensor = Tensor::from_vec(mel, (1, n_mels, mel_len), &Device::Cpu)
            .map_err(|e| JsError::new(&e.to_string()))?;

        // 2. Encode
        let encoder_output = self
            .model
            .encoder
            .forward(&mel_tensor, true)
            .map_err(|e| JsError::new(&e.to_string()))?;

        // 3. Greedy decode
        let transcript =
            self.greedy_decode(&encoder_output).map_err(|e| JsError::new(&e.to_string()))?;

        let result = TranscribeResult { transcript, is_final: true };

        serde_json::to_string(&result).map_err(|e| JsError::new(&e.to_string()))
    }

    pub fn reset(&mut self) {
        self.model.reset_kv_cache();
    }
}

impl WhisperRecognizer {
    fn greedy_decode(&mut self, encoder_output: &Tensor) -> candle_core::Result<String> {
        let sot_token = self.tokenizer.token_to_id(SOT_TOKEN).unwrap_or(50258);
        let lang_token = self.tokenizer.token_to_id(LANG_TOKEN).unwrap_or(50259);
        let transcribe_token = self.tokenizer.token_to_id(TRANSCRIBE_TOKEN).unwrap_or(50359);
        let no_timestamps_token = self.tokenizer.token_to_id(NO_TIMESTAMPS_TOKEN).unwrap_or(50363);
        let eot_token = self.tokenizer.token_to_id(EOT_TOKEN).unwrap_or(50257);

        // Initial prompt tokens:
        // <|startoftranscript|><|en|><|transcribe|>
        // <|notimestamps|>
        let mut tokens = vec![sot_token, lang_token, transcribe_token, no_timestamps_token];
        let max_tokens = self.config.max_target_positions / 2;

        self.model.decoder.reset_kv_cache();

        for i in 0..max_tokens {
            let tokens_tensor = Tensor::new(tokens.as_slice(), &Device::Cpu)?.unsqueeze(0)?;

            let flush = i == 0;
            let logits = self.model.decoder.forward(&tokens_tensor, encoder_output, flush)?;
            let logits = self.model.decoder.final_linear(&logits)?;

            // Get last token logits
            let seq_len = logits.dim(1)?;
            let last_logits = logits.i((0, seq_len - 1))?;

            // Suppress tokens
            let last_logits = self.apply_suppress_tokens(&last_logits)?;

            // Greedy: argmax
            let next_token = last_logits.argmax(0)?.to_scalar::<u32>()?;

            if next_token == eot_token {
                break;
            }

            tokens.push(next_token);
        }

        // Decode tokens (skip 4 prompt tokens)
        let output_tokens = &tokens[4..];
        let text = self
            .tokenizer
            .decode(output_tokens, true)
            .map_err(|e| candle_core::Error::Msg(e.to_string()))?;

        Ok(text)
    }

    fn apply_suppress_tokens(&self, logits: &Tensor) -> candle_core::Result<Tensor> {
        if self.config.suppress_tokens.is_empty() {
            return Ok(logits.clone());
        }
        let mut logits_vec = logits.to_vec1::<f32>()?;
        for &token_id in &self.config.suppress_tokens {
            let idx = token_id as usize;
            if idx < logits_vec.len() {
                logits_vec[idx] = f32::NEG_INFINITY;
            }
        }
        Tensor::from_vec(logits_vec, logits.shape(), logits.device())
    }
}
