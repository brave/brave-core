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

// Whisper model implementation, adapted from
// https://github.com/huggingface/candle/blob/main/
//   candle-transformers/src/models/whisper/model.rs

use crate::Config;
use candle_core::{DType, Device, IndexOp, Result, Tensor, D};
use candle_nn::{
    embedding, linear, linear_no_bias, Conv1d, Conv1dConfig, Embedding, LayerNorm, Linear, Module,
    VarBuilder,
};

fn conv1d(
    in_channels: usize,
    out_channels: usize,
    kernel_size: usize,
    config: Conv1dConfig,
    vb: VarBuilder,
) -> Result<Conv1d> {
    let weight = vb.get((out_channels, in_channels, kernel_size), "weight")?;
    let bias = vb.get(out_channels, "bias")?;
    Ok(Conv1d::new(weight, Some(bias), config))
}

fn layer_norm(size: usize, vb: VarBuilder) -> Result<LayerNorm> {
    let weight = vb.get(size, "weight")?;
    let bias = vb.get(size, "bias")?;
    Ok(LayerNorm::new(weight, bias, 1e-5))
}

#[derive(Debug, Clone)]
struct MultiHeadAttention {
    query: Linear,
    key: Linear,
    value: Linear,
    out: Linear,
    n_head: usize,
    kv_cache: Option<(Tensor, Tensor)>,
}

impl MultiHeadAttention {
    fn load(n_state: usize, n_head: usize, vb: VarBuilder) -> Result<Self> {
        let query = linear(n_state, n_state, vb.pp("q_proj"))?;
        let value = linear(n_state, n_state, vb.pp("v_proj"))?;
        let key = linear_no_bias(n_state, n_state, vb.pp("k_proj"))?;
        let out = linear(n_state, n_state, vb.pp("out_proj"))?;
        Ok(Self { query, key, value, out, n_head, kv_cache: None })
    }

    fn forward(
        &mut self,
        x: &Tensor,
        xa: Option<&Tensor>,
        mask: Option<&Tensor>,
        flush_cache: bool,
    ) -> Result<Tensor> {
        let q = self.query.forward(x)?;
        let (k, v) = match xa {
            None => {
                let k = self.key.forward(x)?;
                let v = self.value.forward(x)?;
                (k, v)
            }
            Some(x) => {
                if flush_cache {
                    self.kv_cache = None;
                }
                if let Some((k, v)) = &self.kv_cache {
                    (k.clone(), v.clone())
                } else {
                    let k = self.key.forward(x)?;
                    let v = self.value.forward(x)?;
                    self.kv_cache = Some((k.clone(), v.clone()));
                    (k, v)
                }
            }
        };
        let wv = self.qkv_attention(&q, &k, &v, mask)?;
        self.out.forward(&wv)
    }

    fn reshape_head(&self, x: &Tensor) -> Result<Tensor> {
        let (n_batch, n_ctx, n_state) = x.dims3()?;
        let target_dims = &[n_batch, n_ctx, self.n_head, n_state / self.n_head];
        x.reshape(target_dims)?.transpose(1, 2)
    }

    fn qkv_attention(
        &self,
        q: &Tensor,
        k: &Tensor,
        v: &Tensor,
        mask: Option<&Tensor>,
    ) -> Result<Tensor> {
        let (_, n_ctx, n_state) = q.dims3()?;
        let scale = ((n_state / self.n_head) as f64).powf(-0.25);
        let q = (self.reshape_head(q)? * scale)?;
        let k = (self.reshape_head(k)?.transpose(2, 3)? * scale)?;
        let v = self.reshape_head(v)?.contiguous()?;
        let mut qk = q.matmul(&k)?;
        if let Some(mask) = mask {
            let mask = mask.i((0..n_ctx, 0..n_ctx))?;
            qk = qk.broadcast_add(&mask)?;
        }
        let w = candle_nn::ops::softmax_last_dim(&qk)?;
        w.matmul(&v)?.transpose(1, 2)?.flatten_from(2)
    }

    fn reset_kv_cache(&mut self) {
        self.kv_cache = None;
    }
}

#[derive(Debug, Clone)]
struct ResidualAttentionBlock {
    attn: MultiHeadAttention,
    attn_ln: LayerNorm,
    cross_attn: Option<(MultiHeadAttention, LayerNorm)>,
    mlp_linear1: Linear,
    mlp_linear2: Linear,
    mlp_ln: LayerNorm,
}

impl ResidualAttentionBlock {
    fn load(n_state: usize, n_head: usize, ca: bool, vb: VarBuilder) -> Result<Self> {
        let attn = MultiHeadAttention::load(n_state, n_head, vb.pp("self_attn"))?;
        let attn_ln = layer_norm(n_state, vb.pp("self_attn_layer_norm"))?;
        let cross_attn = if ca {
            let cross_attn = MultiHeadAttention::load(n_state, n_head, vb.pp("encoder_attn"))?;
            let cross_attn_ln = layer_norm(n_state, vb.pp("encoder_attn_layer_norm"))?;
            Some((cross_attn, cross_attn_ln))
        } else {
            None
        };
        let n_mlp = n_state * 4;
        let mlp_linear1 = linear(n_state, n_mlp, vb.pp("fc1"))?;
        let mlp_linear2 = linear(n_mlp, n_state, vb.pp("fc2"))?;
        let mlp_ln = layer_norm(n_state, vb.pp("final_layer_norm"))?;
        Ok(Self { attn, attn_ln, cross_attn, mlp_linear1, mlp_linear2, mlp_ln })
    }

    fn forward(
        &mut self,
        x: &Tensor,
        xa: Option<&Tensor>,
        mask: Option<&Tensor>,
        flush_kv_cache: bool,
    ) -> Result<Tensor> {
        let attn = self.attn.forward(&self.attn_ln.forward(x)?, None, mask, flush_kv_cache)?;
        let mut x = (x + attn)?;
        if let Some((attn, ln)) = &mut self.cross_attn {
            x = (&x + attn.forward(&ln.forward(&x)?, xa, None, flush_kv_cache)?)?;
        }
        let mlp = self
            .mlp_linear2
            .forward(&self.mlp_linear1.forward(&self.mlp_ln.forward(&x)?)?.gelu()?)?;
        x + mlp
    }

    fn reset_kv_cache(&mut self) {
        self.attn.reset_kv_cache();
        if let Some((attn, _)) = &mut self.cross_attn {
            attn.reset_kv_cache();
        }
    }
}

fn sinusoids(length: usize, channels: usize, device: &Device) -> Result<Tensor> {
    let max_timescale = 10000f32;
    let log_timescale_increment = max_timescale.ln() / (channels / 2 - 1) as f32;
    let inv_timescales: Vec<_> =
        (0..channels / 2).map(|i| (i as f32 * (-log_timescale_increment)).exp()).collect();
    let inv_timescales = Tensor::new(inv_timescales.as_slice(), device)?.unsqueeze(0)?;
    let arange = Tensor::arange(0, length as u32, device)?.to_dtype(DType::F32)?.unsqueeze(1)?;
    let sh = (length, channels / 2);
    let scaled_time = (arange.broadcast_as(sh)? * inv_timescales.broadcast_as(sh)?)?;
    let sincos = Tensor::cat(&[scaled_time.sin()?, scaled_time.cos()?], 1)?;
    Ok(sincos)
}

#[derive(Debug, Clone)]
pub struct AudioEncoder {
    conv1: Conv1d,
    conv2: Conv1d,
    positional_embedding: Tensor,
    blocks: Vec<ResidualAttentionBlock>,
    ln_post: LayerNorm,
}

impl AudioEncoder {
    fn load(vb: VarBuilder, cfg: &Config) -> Result<Self> {
        let n_state = cfg.d_model;
        let n_head = cfg.encoder_attention_heads;
        let n_ctx = cfg.max_source_positions;
        let cfg1 = Conv1dConfig { padding: 1, ..Default::default() };
        let cfg2 = Conv1dConfig { padding: 1, stride: 2, ..Default::default() };
        let conv1 = conv1d(cfg.num_mel_bins, n_state, 3, cfg1, vb.pp("conv1"))?;
        let conv2 = conv1d(n_state, n_state, 3, cfg2, vb.pp("conv2"))?;
        let positional_embedding = sinusoids(n_ctx, n_state, vb.device())?;
        let blocks = (0..cfg.encoder_layers)
            .map(|i| {
                ResidualAttentionBlock::load(n_state, n_head, false, vb.pp(format!("layers.{i}")))
            })
            .collect::<Result<Vec<_>>>()?;
        let ln_post = layer_norm(n_state, vb.pp("layer_norm"))?;
        Ok(Self { conv1, conv2, positional_embedding, blocks, ln_post })
    }

    pub fn forward(&mut self, x: &Tensor, flush_kv_cache: bool) -> Result<Tensor> {
        let x = self.conv1.forward(x)?.gelu()?;
        let x = self.conv2.forward(&x)?.gelu()?;
        let x = x.transpose(1, 2)?;
        let (_bsize, seq_len, _hidden) = x.dims3()?;
        let positional_embedding = self.positional_embedding.narrow(0, 0, seq_len)?;
        let mut x = x.broadcast_add(&positional_embedding)?;
        for block in self.blocks.iter_mut() {
            x = block.forward(&x, None, None, flush_kv_cache)?;
        }
        self.ln_post.forward(&x)
    }
}

#[derive(Debug, Clone)]
pub struct TextDecoder {
    token_embedding: Embedding,
    positional_embedding: Tensor,
    blocks: Vec<ResidualAttentionBlock>,
    ln: LayerNorm,
    mask: Tensor,
}

impl TextDecoder {
    fn load(vb: VarBuilder, cfg: &Config) -> Result<Self> {
        let n_state = cfg.d_model;
        let n_head = cfg.decoder_attention_heads;
        let n_ctx = cfg.max_target_positions;
        let token_embedding = embedding(cfg.vocab_size, n_state, vb.pp("embed_tokens"))?;
        let positional_embedding = vb.get((n_ctx, n_state), "embed_positions.weight")?;
        let blocks = (0..cfg.decoder_layers)
            .map(|i| {
                ResidualAttentionBlock::load(n_state, n_head, true, vb.pp(format!("layers.{i}")))
            })
            .collect::<Result<Vec<_>>>()?;
        let ln = layer_norm(n_state, vb.pp("layer_norm"))?;
        let mask: Vec<_> = (0..n_ctx)
            .flat_map(|i| (0..n_ctx).map(move |j| if j > i { f32::NEG_INFINITY } else { 0f32 }))
            .collect();
        let mask = Tensor::from_vec(mask, (n_ctx, n_ctx), vb.device())?;
        Ok(Self { token_embedding, positional_embedding, blocks, ln, mask })
    }

    pub fn forward(&mut self, x: &Tensor, xa: &Tensor, flush_kv_cache: bool) -> Result<Tensor> {
        let last = x.dim(D::Minus1)?;
        let token_embedding = self.token_embedding.forward(x)?;
        let positional_embedding = self.positional_embedding.narrow(0, 0, last)?;
        let mut x = token_embedding.broadcast_add(&positional_embedding)?;
        for block in self.blocks.iter_mut() {
            x = block.forward(&x, Some(xa), Some(&self.mask), flush_kv_cache)?;
        }
        self.ln.forward(&x)
    }

    pub fn final_linear(&self, x: &Tensor) -> Result<Tensor> {
        let b_size = x.dim(0)?;
        let w = self.token_embedding.embeddings().broadcast_left(b_size)?.t()?;
        x.matmul(&w)
    }

    pub fn reset_kv_cache(&mut self) {
        for block in self.blocks.iter_mut() {
            block.reset_kv_cache();
        }
    }
}

#[derive(Debug, Clone)]
pub struct Whisper {
    pub encoder: AudioEncoder,
    pub decoder: TextDecoder,
}

impl Whisper {
    pub fn load(vb: &VarBuilder, config: Config) -> Result<Self> {
        let encoder = AudioEncoder::load(vb.pp("model.encoder"), &config)?;
        let decoder = TextDecoder::load(vb.pp("model.decoder"), &config)?;
        Ok(Self { encoder, decoder })
    }

    pub fn reset_kv_cache(&mut self) {
        self.encoder.blocks.iter_mut().for_each(|b| b.reset_kv_cache());
        self.decoder.reset_kv_cache();
    }
}
