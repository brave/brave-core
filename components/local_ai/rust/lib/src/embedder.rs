/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use candle_core::quantized::gguf_file;
use candle_core::{DType, Device};
use candle_nn::VarBuilder;
use tokenizers::Tokenizer;

use crate::config::{Batch, Gemma3Config, ModelType, Pool};
use crate::error::EmbedderError;
use crate::model::Gemma3Model;

pub struct Gemma3Embedder {
    model: Gemma3Model,
    tokenizer: Tokenizer,
}

impl Gemma3Embedder {
    pub fn new(
        weights: Vec<u8>,
        weights_dense1: Vec<u8>,
        weights_dense2: Vec<u8>,
        tokenizer: Vec<u8>,
        config: Vec<u8>,
    ) -> Result<Gemma3Embedder, EmbedderError> {
        let tokenizer = Tokenizer::from_bytes(&tokenizer)?;

        let config: Gemma3Config = serde_json::from_slice(&config)
            .map_err(|e| EmbedderError::ModelError(e.to_string()))?;

        let mut cursor = std::io::Cursor::new(weights);

        let content = gguf_file::Content::read(&mut cursor)
            .map_err(|e| EmbedderError::ModelError(format!("GGUF parse error: {e}")))?;

        let vb_dense1 =
            VarBuilder::from_buffered_safetensors(weights_dense1, DType::F32, &Device::Cpu)?;

        let vb_dense2 =
            VarBuilder::from_buffered_safetensors(weights_dense2, DType::F32, &Device::Cpu)?;

        let model = Gemma3Model::load(
            content,
            &mut cursor,
            vb_dense1,
            vb_dense2,
            &config,
            ModelType::Embedding(Pool::Mean),
        )?;

        Ok(Self { model, tokenizer })
    }

    pub fn embed(&self, text: &str) -> Result<Vec<f32>, EmbedderError> {
        let batch = self.encode_batch(&[text.to_string()])?;
        let normed_output = self.model.forward(batch)?;
        let normed_output = normed_output
            .ok_or_else(|| EmbedderError::ModelError("Embedding generation failed!".to_string()))?;
        let normed_output = normed_output.to_vec2::<f32>()?;
        let emb = normed_output[0].clone();

        Ok(emb)
    }

    pub fn embed_batch(&self, texts: Vec<String>) -> Result<Vec<f32>, EmbedderError> {
        let batch = self.encode_batch(&texts)?;
        let normed_output = self.model.forward(batch)?;
        let normed_output = normed_output
            .ok_or_else(|| EmbedderError::ModelError("Embedding generation failed!".to_string()))?;
        let embeddings = normed_output.to_vec2::<f32>()?;

        Ok(embeddings.into_iter().flatten().collect())
    }

    fn encode_batch(&self, texts: &[String]) -> Result<Batch, EmbedderError> {
        let mut all_ids = Vec::new();
        let mut all_positions = Vec::new();
        let mut cumulative = vec![0];
        let mut max_len = 0;

        for text in texts {
            let encoding = self
                .tokenizer
                .encode(text.as_str(), true)
                .map_err(|e| EmbedderError::TokenizerError(format!("Tokenization failed: {e}")))?;

            let ids = encoding.get_ids().to_vec();
            let len = ids.len();
            max_len = max_len.max(len);

            all_ids.extend(ids.iter().cloned());
            all_positions.extend((0..len as u32).collect::<Vec<u32>>());
            cumulative.push(cumulative.last().unwrap() + len as u32);
        }

        let token_len = all_ids.len();
        Ok(Batch {
            input_ids: all_ids,
            token_type_ids: vec![0; token_len],
            position_ids: all_positions,
            cumulative_seq_lengths: cumulative,
            max_length: max_len as u32,
            pooled_indices: (0..texts.len() as u32).collect(),
        })
    }
}
