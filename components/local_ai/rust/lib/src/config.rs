/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use candle_core::Tensor;
use serde::Deserialize;

#[derive(Debug, Deserialize, PartialEq, Clone)]
#[serde(rename_all = "lowercase")]
pub enum HiddenAct {
    #[serde(alias = "gelu_pytorch_tanh")]
    Gelu,
    Relu,
    Silu,
    Swiglu,
}

impl HiddenAct {
    pub fn forward(&self, x: &Tensor) -> candle_core::Result<Tensor> {
        match self {
            Self::Gelu => x.gelu(),
            Self::Relu => x.relu(),
            Self::Silu => x.silu(),
            Self::Swiglu => candle_nn::ops::swiglu(x),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize)]
pub struct Gemma3Config {
    pub attention_bias: bool,
    pub pad_token_id: u32,
    pub head_dim: Option<usize>,
    pub hidden_activation: HiddenAct,
    pub hidden_size: usize,
    pub max_position_embeddings: usize,
    pub num_attention_heads: usize,
    pub num_hidden_layers: usize,
    pub num_key_value_heads: usize,
    pub query_pre_attn_scalar: usize,
    pub rms_norm_eps: f32,
    pub rope_local_base_freq: f32,
    pub rope_theta: f32,
    pub sliding_window: Option<usize>,
    #[serde(rename(deserialize = "_sliding_window_pattern"))]
    pub sliding_window_pattern: usize,
    pub vocab_size: usize,
}

#[derive(Debug, PartialEq, Clone)]
pub enum ModelType {
    Classifier,
    Embedding(Pool),
}

#[derive(Debug, PartialEq, Clone, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Pool {
    Cls,
    Mean,
    Splade,
    LastToken,
}

#[derive(Debug)]
pub struct Batch {
    pub input_ids: Vec<u32>,
    pub token_type_ids: Vec<u32>,
    pub position_ids: Vec<u32>,
    pub cumulative_seq_lengths: Vec<u32>,
    pub max_length: u32,
    pub pooled_indices: Vec<u32>,
}

impl Batch {
    pub fn len(&self) -> usize {
        self.cumulative_seq_lengths.len() - 1
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}
