/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/* This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright 2022 Hugging Face
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Embedding Gemma inference implementation,
// modified from 'https://github.com/huggingface/text-embeddings-inference/blob/main/backends/candle/src/models/gemma3.rs'

use candle_core::{DType, Device, IndexOp, Tensor, D};
use candle_nn::{Embedding, Module, VarBuilder};
use serde::Deserialize;
use tokenizers::Tokenizer;
use wasm_bindgen::prelude::*;

use candle_core::quantized::gguf_file;
use candle_core::quantized::{QTensor, QMatMul};

// each 'dim' dimensional vector is split into even and odd indexed vectors.
// 1 / base ^ (i / dim) is calculated for each position.
pub fn get_inv_freqs(dim: usize, base: f32) -> candle_core::Result<Tensor> {
    let inv_freq: Vec<_> =
        (0..dim).step_by(2).map(|i| 1f32 / base.powf(i as f32 / dim as f32)).collect();
    let inv_freq_len = inv_freq.len();
    Tensor::from_vec(inv_freq, (1, inv_freq_len), &Device::Cpu)
}

// sine and cosine of each freq is calculated and multipled with Q/K values.
pub fn get_cos_sin(
    length: usize,
    inv_freqs: &Tensor,
    dtype: DType,
    repeat_freqs: bool,
) -> candle_core::Result<(Tensor, Tensor)> {
    let t = Tensor::arange(0u32, length as u32, &Device::Cpu)?
        .to_dtype(DType::F32)?
        .reshape((length, 1))?;
    let mut freqs = t.matmul(inv_freqs)?;
    if repeat_freqs {
        freqs = Tensor::cat(&[&freqs, &freqs], 1)?;
    }
    let cos = freqs.cos()?.to_dtype(dtype)?;
    let sin = freqs.sin()?.to_dtype(dtype)?;
    Ok((cos, sin))
}

// Position info is injected fresh at every attention layer by rotating Q and K
// vectors before computing attention scores.
pub fn apply_rotary(
    x: &Tensor,
    cos: &Tensor,
    sin: &Tensor,
    attention_head_size: usize,
) -> candle_core::Result<Tensor> {
    let dim = attention_head_size / 2;
    let x1 = x.narrow(D::Minus1, 0, dim)?;
    let x2 = x.narrow(D::Minus1, dim, dim)?;
    let rotate_x = Tensor::cat(&[&x2.neg()?, &x1], D::Minus1)?;
    let rope = (x.broadcast_mul(cos)? + rotate_x.broadcast_mul(sin)?)?;
    Ok(rope)
}

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

// Linear layer modified to support quantization
#[derive(Debug, Clone)]
pub struct QLinear {
    inner: QMatMul,
    bias: Option<Tensor>,
}

impl QLinear {
    pub fn new(qweight: QTensor, bias: Option<Tensor>) -> candle_core::Result<Self> {
        Ok(Self {
            inner: QMatMul::from_qtensor(qweight)?,
            bias,
        })
    }

    pub fn forward(&self, x: &Tensor) -> candle_core::Result<Tensor> {
        // Quantized matmul (weights remain quantized internally)
        let mut y = self.inner.forward(x)?;

        // Bias always stays FP32
        if let Some(bias) = &self.bias {
            y = y.broadcast_add(bias)?;
        }

        Ok(y)
    }
}

#[derive(Debug)]
pub struct Linear {
    weight: Tensor,
    bias: Option<Tensor>,
    act: Option<HiddenAct>,
}

/// FP linear layer for final dense layers
impl Linear {
    pub fn new(weight: Tensor, bias: Option<Tensor>, act: Option<HiddenAct>) -> Self {
        Self { weight, bias, act }
    }

    pub fn forward(&self, x: &Tensor) -> candle_core::Result<Tensor> {
        let (x, w) = match x.dims() {
            &[bsize, _, _] => (x, self.weight.broadcast_left(bsize)?.t()?),
            _ => (x, self.weight.t()?),
        };

        let x = x.matmul(&w)?;
        let x = match &self.bias {
            None => x,
            Some(bias) => x.broadcast_add(bias)?,
        };

        if let Some(act) = &self.act {
            match act {
                HiddenAct::Gelu => x.gelu(),
                HiddenAct::Relu => x.relu(),
                HiddenAct::Silu => x.silu(),
                HiddenAct::Swiglu => candle_nn::ops::swiglu(&x),
            }
        } else {
            Ok(x)
        }
    }
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

/// Model type, only embeddings are supported in Gemma.
#[derive(Debug, PartialEq, Clone)]
pub enum ModelType {
    Classifier,
    Embedding(Pool),
}

/// Supported pooling strategies.
#[derive(Debug, PartialEq, Clone, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Pool {
    /// Take the CLS token embedding
    Cls,
    /// Mean pool across tokens
    Mean,
    /// SPLADE (sparse lexical embeddings, not supported in Gemma)
    Splade,
    /// Take the last token embedding
    LastToken,
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

#[derive(Debug)]
pub struct Gemma3RMSNorm {
    weight: Tensor,
    epsilon: f32,
}

impl Gemma3RMSNorm {
    pub fn load_from_gguf<R: std::io::Seek + std::io::Read>(
        ct: &gguf_file::Content,
        reader: &mut R,
        weight_name: &str,
        epsilon: f32,
    ) -> candle_core::Result<Self> {
        let weight = 
            ct.tensor(reader, weight_name, &Device::Cpu)?.dequantize(&Device::Cpu)?;

        Ok(Self {weight, epsilon})
    }

    pub fn forward(
        &self,
        hidden_states: &Tensor,
        residual: Option<&Tensor>,
    ) -> candle_core::Result<(Tensor, Tensor)> {

        let residual_add = if let Some(residual) = residual {
            hidden_states.add(residual)?
        } else {
            hidden_states.clone()
        };
        let hidden_states = residual_add.clone();

        let hidden_states_dtype = hidden_states.dtype();
        let internal_dtype = match hidden_states_dtype {
            DType::F16 | DType::BF16 => DType::F32,
            d => d,
        };

        let hidden_size = hidden_states.dim(D::Minus1)?;
        let hidden_states = hidden_states.to_dtype(internal_dtype)?;
        let norm_hidden_states =
            (hidden_states.sqr()?.sum_keepdim(D::Minus1)? / hidden_size as f64)?;
        let hidden_states_normed =
            hidden_states.broadcast_div(&(norm_hidden_states + self.epsilon as f64)?.sqrt()?)?;

        // NOTE: Gemma3 multiplies by (1.0 + weight) for scaling after normalization 
        // but quant model weights are already scaled, so DO NOT add 1 if using quant model
        let output = 
            hidden_states_normed.to_dtype(hidden_states_dtype)?.broadcast_mul(&self.weight)?;
            
        Ok((output, residual_add))
    }
}

enum Gemma3AttentionType {
    FullAttention,
    SlidingAttention,
}

struct Gemma3Attention {
    q_proj: QLinear,
    k_proj: QLinear,
    v_proj: QLinear,
    o_proj: QLinear,

    q_norm: Gemma3RMSNorm,
    k_norm: Gemma3RMSNorm,

    attention_head_size: usize,
    num_attention_heads: usize,
    num_key_value_heads: usize,
    scaling: f64,

    sliding_window: Option<usize>,
}

impl Gemma3Attention {
    pub fn load_from_gguf<R: std::io::Seek + std::io::Read>(
        ct: &gguf_file::Content,
        reader: &mut R,
        config: &Gemma3Config,
        attention_type: Gemma3AttentionType,
        prefix: &str,
    ) -> candle_core::Result<Self> {
        let num_attention_heads = config.num_attention_heads;
        let attention_head_size = 
            config.head_dim.unwrap_or(config.hidden_size / num_attention_heads);
        let num_key_value_heads = config.num_key_value_heads;

        // ---- Quantized Q/K/V weights ----
        let q_w = ct.tensor(reader, &format!("{prefix}.attn_q.weight"), &Device::Cpu)?;
        let k_w = ct.tensor(reader, &format!("{prefix}.attn_k.weight"), &Device::Cpu)?;
        let v_w = ct.tensor(reader, &format!("{prefix}.attn_v.weight"), &Device::Cpu)?;
        let o_w = ct.tensor(reader, &format!("{prefix}.attn_output.weight"), &Device::Cpu)?;    

        // q-k-v weight concatenation cannot be used for quantized models
        
        let q_proj = QLinear::new(q_w, None)?;
        let k_proj = QLinear::new(k_w, None)?;
        let v_proj = QLinear::new(v_w, None)?;
        let o_proj = QLinear::new(o_w, None)?;

        // not using q-k-v bias as attention_bias is false in this config

        let q_norm = 
            Gemma3RMSNorm::load_from_gguf(ct, reader, &format!("{prefix}.attn_q_norm.weight"), config.rms_norm_eps)?;
        
        let k_norm = 
            Gemma3RMSNorm::load_from_gguf(ct, reader, &format!("{prefix}.attn_k_norm.weight"), config.rms_norm_eps)?;

        let scaling = 1.0 / (config.query_pre_attn_scalar as f64).sqrt();

        let sliding_window = match attention_type {
            Gemma3AttentionType::FullAttention => None,
            Gemma3AttentionType::SlidingAttention => config.sliding_window,
        };
    
        Ok(Self {
            q_proj,
            k_proj,
            v_proj,
            o_proj,
            q_norm,
            k_norm,
            attention_head_size,
            num_attention_heads,
            num_key_value_heads,
            scaling,
            sliding_window,
        })

    }

    fn create_causal_mask(
        &self,
        batch_size: usize,
        dim: usize,
        seq_len: usize,
        dtype: DType,
        sliding_window: Option<usize>,
    ) -> candle_core::Result<Tensor> {
        const F16_MIN: f32 = -65504.0;

        let min_value = match dtype {
            DType::F32 => f32::MIN,
            DType::F16 | DType::BF16 => F16_MIN,
            unsupported => {
                return Err(candle_core::Error::Msg(
                    format!("unsupported dtype in causal mask: {:?}", unsupported)
                ));
            }
        };

        let mask: Vec<u8> = if let Some(window_size) = sliding_window {
            // Bi-directional sliding window mask, meaning a token can attend to any
            // other token if their absolute distance is within half the sliding window size
            let half_window = window_size / 2;
            (0..seq_len)
                .flat_map(|i| {
                    (0..seq_len).map(move |j| {
                        let distance = i.abs_diff(j);
                        (distance <= half_window) as u8
                    })
                })
                .collect()
        } else {
            // Full attention mask, meaning a token can attend to all tokens
            vec![1u8; seq_len * seq_len]
        };

        let mask_tensor = Tensor::from_slice(&mask, (seq_len, seq_len), &Device::Cpu)?;
        let expanded_mask = mask_tensor.expand(&[batch_size, dim, seq_len, seq_len])?;

        let zeros = Tensor::zeros((batch_size, dim, seq_len, seq_len), dtype, &Device::Cpu)?;
        let negatives = Tensor::full(min_value, (batch_size, dim, seq_len, seq_len), &Device::Cpu)?
            .to_dtype(dtype)?;

        expanded_mask.where_cond(&zeros, &negatives)

    }

    fn repeat_kv(&self, x: &Tensor) -> candle_core::Result<Tensor> {
        if self.num_key_value_heads == self.num_attention_heads {
            return Ok(x.clone());
        }

        let repeat_factor = self.num_attention_heads / self.num_key_value_heads;
        let (b, h, s, d) = x.shape().dims4()?;
        let expanded = x.unsqueeze(2)?.expand((b, h, repeat_factor, s, d))?.reshape((
            b,
            h * repeat_factor,
            s,
            d,
        ))?;

        Ok(expanded)
    }

    pub fn forward(
        &self,
        hidden_states: &Tensor,
        attention_bias: Option<&Tensor>,
        cos: &Tensor,
        sin: &Tensor,
    ) -> candle_core::Result<Tensor> {
        let input_dims = hidden_states.dims();
        let input_shape = &input_dims[..input_dims.len() - 1];

        let q = self.q_proj.forward(hidden_states)?;
        let k = self.k_proj.forward(hidden_states)?;
        let v = self.v_proj.forward(hidden_states)?;

        let q = q.reshape(
            [
                input_shape,
                &[self.num_attention_heads, self.attention_head_size],
            ]
            .concat(),
        )?;
        let k = k.reshape(
            [
                input_shape,
                &[self.num_key_value_heads, self.attention_head_size],
            ]
            .concat(),
        )?;
        let v = v.reshape(
            [
                input_shape,
                &[self.num_key_value_heads, self.attention_head_size],
            ]
            .concat(),
        )?;

        let (q, _) = self.q_norm.forward(&q, None)?;
        let (k, _) = self.k_norm.forward(&k, None)?;

        let q = q.transpose(1, 2)?;
        let k = k.transpose(1, 2)?;
        let v = v.transpose(1, 2)?;

        let q = apply_rotary(&q, cos, sin, self.attention_head_size)?;
        let k = apply_rotary(&k, cos, sin, self.attention_head_size)?;

        // For simplicity, expand k and v to match number of q heads if needed (GQA)
        let k = self.repeat_kv(&k)?;
        let v = self.repeat_kv(&v)?;

        let attention_bias = match attention_bias {
            Some(attention_bias) => {
                let (batch_size, dim, seq_length, _) = attention_bias.shape().dims4()?;
                let causal_mask = self.create_causal_mask(
                    batch_size,
                    dim,
                    seq_length,
                    attention_bias.dtype(),
                    self.sliding_window,
                )?;
                Some(attention_bias.broadcast_add(&causal_mask)?)
            }
            None => None,
        };

        let context_layer = {
            let attn_weights = q.matmul(&k.t()?)?;
            let mut attn_weights = (attn_weights * self.scaling)?;

            if let Some(attention_bias) = attention_bias {
                attn_weights = attn_weights.broadcast_add(&attention_bias)?;
            }

            let attn_weights = candle_nn::ops::softmax_last_dim(&attn_weights)?;
            attn_weights.matmul(&v.contiguous()?)
        }?;

        let context_layer = context_layer.transpose(1, 2)?.flatten_from(D::Minus2)?;
        self.o_proj.forward(&context_layer)
    }
}

struct Gemma3MLP {
    gate_proj: QLinear,
    up_proj: QLinear,
    down_proj: QLinear,
    
    hidden_activation: HiddenAct,
}

impl Gemma3MLP {
    pub fn load_from_gguf<R: std::io::Seek + std::io::Read>(
        ct: &gguf_file::Content,
        reader: &mut R,
        prefix: &str,
        config: &Gemma3Config,
        ) -> candle_core::Result<Self> {

        let gate_w = ct.tensor(reader, &format!("{prefix}.ffn_gate.weight"), &Device::Cpu)?;
        let up_w   = ct.tensor(reader, &format!("{prefix}.ffn_up.weight"), &Device::Cpu)?;
        let down_w = ct.tensor(reader, &format!("{prefix}.ffn_down.weight"), &Device::Cpu)?;
    
        let gate_proj = QLinear::new(gate_w, None)?;
        let up_proj   = QLinear::new(up_w, None)?;
        let down_proj = QLinear::new(down_w, None)?;

        Ok(Self {
            gate_proj,
            up_proj,
            down_proj,
            hidden_activation: config.hidden_activation.clone(),
        })
    }

    pub fn forward(&self, hidden_states: &Tensor) -> candle_core::Result<Tensor> {

        let gate_states = self.gate_proj.forward(hidden_states)?;
        let up_states   = self.up_proj.forward(hidden_states)?;

        let gate_activated = self.hidden_activation.forward(&gate_states)?;
        let fused = (gate_activated * up_states)?;

        self.down_proj.forward(&fused)
    }
}

struct Gemma3Layer {
    input_layernorm: Gemma3RMSNorm,
    self_attn: Gemma3Attention,
    post_attention_layernorm: Gemma3RMSNorm,

    pre_feedforward_layernorm: Gemma3RMSNorm,
    mlp: Gemma3MLP,
    post_feedforward_layernorm: Gemma3RMSNorm,

}

impl Gemma3Layer {
    pub fn load_from_gguf<R: std::io::Seek + std::io::Read>(
        ct: &gguf_file::Content,
        reader: &mut R,
        prefix: &str,
        config: &Gemma3Config,
        attention_type: Gemma3AttentionType,
        ) -> candle_core::Result<Self> {

        let input_layernorm = 
            Gemma3RMSNorm::load_from_gguf(ct, reader, &format!("{prefix}.attn_norm.weight"), config.rms_norm_eps)?;

        let self_attn = Gemma3Attention::load_from_gguf(
            ct,
            reader,
            config,
            attention_type,
            prefix,
        )?;

        let post_attention_layernorm = 
            Gemma3RMSNorm::load_from_gguf(ct, reader, &format!("{prefix}.post_attention_norm.weight"), config.rms_norm_eps)?;

        let pre_feedforward_layernorm = 
            Gemma3RMSNorm::load_from_gguf(ct, reader, &format!("{prefix}.ffn_norm.weight"), config.rms_norm_eps)?;

        let mlp = Gemma3MLP::load_from_gguf(
            ct,
            reader,
            prefix,
            config,
        )?;

        let post_feedforward_layernorm = 
            Gemma3RMSNorm::load_from_gguf(ct, reader, &format!("{prefix}.post_ffw_norm.weight"), config.rms_norm_eps)?;

        Ok(Self {
            input_layernorm,
            self_attn,
            post_attention_layernorm,
            pre_feedforward_layernorm,
            mlp,
            post_feedforward_layernorm,
        })
    }

    pub fn forward(
        &self,
        hidden_states: &Tensor,
        attention_bias: Option<&Tensor>,
        cos: &Tensor,
        sin: &Tensor,
    ) -> candle_core::Result<Tensor> {
        
        let residual = hidden_states.clone();

        let (hidden_states, _) = self.input_layernorm.forward(hidden_states, None)?;

        let hidden_states = self.self_attn.forward(&hidden_states, attention_bias, cos, sin)?;

        let (hidden_states, _) = self.post_attention_layernorm.forward(&hidden_states, None)?;
        let hidden_states = residual.broadcast_add(&hidden_states)?;

        let residual = hidden_states.clone();
        let (hidden_states, _) = self.pre_feedforward_layernorm.forward(&hidden_states, None)?;
        let hidden_states = self.mlp.forward(&hidden_states)?;

        let (hidden_states, _) = self.post_feedforward_layernorm.forward(&hidden_states, None)?;
        let output = residual.broadcast_add(&hidden_states)?;

        Ok(output)
    }
}

pub struct Gemma3Embedding {
    embedding: Embedding,
    scale: f64,
}

impl Gemma3Embedding {
    pub fn load_from_gguf<R: std::io::Seek + std::io::Read>(
        ct: &gguf_file::Content,
        reader: &mut R,
        config: &Gemma3Config,
    ) -> candle_core::Result<Self> {

        // Load quantized embedding weights from GGUF
        let qweight = ct.tensor(reader, "token_embd.weight", &Device::Cpu)?;

        // Dequantize to FP32
        let weight = qweight.dequantize(&Device::Cpu)?;

        // Build embedding with FP32 weights
        let embedding = Embedding::new(weight, config.hidden_size);

        // Gemma3 embedding scale
        let scale = (config.hidden_size as f64).sqrt();

        Ok(Self {
            embedding,
            scale,
        })
    }

    pub fn forward(&self, input_ids: &Tensor) -> candle_core::Result<Tensor> {
        let hidden = self.embedding.forward(input_ids)?;
        let result = (hidden * self.scale)?;

        Ok(result)
    }
}

pub struct Gemma3Model {
    embed_tokens: Gemma3Embedding,
    layers: Vec<Gemma3Layer>,
    norm: Gemma3RMSNorm,

    rotary_cache: (Tensor, Tensor),
    rotary_cache_local_attention: (Tensor, Tensor),
    rotary_dim: usize,

    num_attention_heads: usize,
    pad_token_id: u32,
    pool: Pool,

    dense1: Linear,
    dense2: Linear,

    dtype: DType,
}

impl Gemma3Model {
    pub fn load<R: std::io::Seek + std::io::Read>(
        ct: gguf_file::Content,
        reader: &mut R,
        vb_dense1: VarBuilder,
        vb_dense2: VarBuilder,
        config: &Gemma3Config,
        model_type: ModelType,
    ) -> candle_core::Result<Self> {
        let pool = match model_type {
            ModelType::Classifier => {
                candle_core::bail!("`classifier` model type is not supported for Gemma3")
            }
            ModelType::Embedding(pool) => pool,
        };

        let embed_tokens = Gemma3Embedding::load_from_gguf(&ct, reader, config)?;

        let layers = (0..config.num_hidden_layers)
            .map(|layer_idx| {
                let attention_type = if (layer_idx + 1) % config.sliding_window_pattern > 0 {
                    Gemma3AttentionType::SlidingAttention
                } else {
                    Gemma3AttentionType::FullAttention
                };
                Gemma3Layer::load_from_gguf(&ct, reader, &format!("blk.{layer_idx}"), config, attention_type)
            })
            .collect::<candle_core::Result<Vec<Gemma3Layer>>>()?;

        let norm = Gemma3RMSNorm::load_from_gguf(&ct, reader, "output_norm.weight", config.rms_norm_eps)?;

        let rotary_dim = config
            .head_dim
            .unwrap_or(config.hidden_size / config.num_attention_heads);

        let inv_freqs = get_inv_freqs(rotary_dim, config.rope_theta)?;
        let rotary_cache =
            get_cos_sin(config.max_position_embeddings, &inv_freqs, DType::F32, true)?;

        let inv_freqs_local =
            get_inv_freqs(rotary_dim, config.rope_local_base_freq)?;

        let rotary_cache_local_attention = get_cos_sin(
            config.max_position_embeddings,
            &inv_freqs_local,
            DType::F32,
            true,
        )?;

        let dense1_weight =
            vb_dense1.pp("linear").get((4 * config.hidden_size, config.hidden_size), "weight")?;
        let dense1 = Linear::new(dense1_weight, None, None);

        let dense2_weight =
            vb_dense2.pp("linear").get((config.hidden_size, config.hidden_size * 4), "weight")?;
        let dense2 = Linear::new(dense2_weight, None, None);

        Ok(Self {
            embed_tokens,
            layers,
            norm,
            rotary_cache,
            rotary_cache_local_attention,
            rotary_dim,
            pool,
            dense1,
            dense2,
            pad_token_id: config.pad_token_id,
            num_attention_heads: config.num_attention_heads,
            dtype: DType::F32,
        })
    }

    pub fn forward(&self, batch: Batch) -> candle_core::Result<Option<Tensor>> {
        let batch_size = batch.len();
        let max_length = batch.max_length as usize;

        let shape = (batch_size, max_length);

        let (input_ids, position_ids, input_lengths, attention_bias) = if batch_size > 1 {
            let elems = batch_size * max_length;

            let mut input_ids = Vec::with_capacity(elems);
            let mut position_ids = Vec::with_capacity(elems);
            let mut attention_bias = Vec::with_capacity(elems);
            let mut input_lengths = Vec::with_capacity(batch_size);
            let mut masking = false;

            for i in 0..batch_size {
                let start = batch.cumulative_seq_lengths[i] as usize;
                let end = batch.cumulative_seq_lengths[i + 1] as usize;
                let seq_length = end - start;
                input_lengths.push(seq_length);

                for j in start..end {
                    input_ids.push(batch.input_ids[j]);
                    position_ids.push(batch.position_ids[j]);
                    attention_bias.push(0.0);
                }

                let padding = max_length - seq_length;
                if padding > 0 {
                    masking = true;
                    for _ in 0..padding {
                        input_ids.push(self.pad_token_id);
                        position_ids.push(0);
                        attention_bias.push(f32::NEG_INFINITY);
                    }
                }
            }

            let input_ids = Tensor::from_vec(input_ids, shape, &Device::Cpu)?;
            let position_ids = Tensor::from_vec(position_ids, shape, &Device::Cpu)?;

            let attention_bias = if masking {
                let attention_bias =
                    Tensor::from_vec(attention_bias, (batch_size, 1, 1, max_length), &Device::Cpu)?
                        .to_dtype(self.dtype)?;

                let attention_bias = attention_bias
                    .broadcast_as((batch_size, self.num_attention_heads, max_length, max_length))?
                    .contiguous()?;
                Some(attention_bias)
            } else {
                None
            };

            (input_ids, position_ids, input_lengths, attention_bias)
        } else {
            let input_ids = Tensor::from_vec(
                batch.input_ids.clone(),
                (1, batch.input_ids.len()),
                &Device::Cpu,
            )?;
            let position_ids = Tensor::from_vec(
                batch.position_ids.clone(),
                (1, batch.position_ids.len()),
                &Device::Cpu,
            )?;
            let input_lengths = vec![batch.input_ids.len()];

            let seq_len = batch.input_ids.len();
            let attention_bias = Tensor::zeros(
                (1, self.num_attention_heads, seq_len, seq_len),
                self.dtype,
                &Device::Cpu,
            )?;

            (input_ids, position_ids, input_lengths, Some(attention_bias))
        };
        
        let mut hidden_states = self.embed_tokens.forward(&input_ids)?;

        let cos = self.rotary_cache.0.index_select(&position_ids.flatten_all()?, 0)?;
        let cos = cos.reshape((batch_size, 1, max_length, self.rotary_dim))?;
        let sin = self.rotary_cache.1.index_select(&position_ids.flatten_all()?, 0)?;
        let sin = sin.reshape((batch_size, 1, max_length, self.rotary_dim))?;

        let cos_local = 
            self.rotary_cache_local_attention.0.index_select(&position_ids.flatten_all()?, 0)?;
        let cos_local = cos_local.reshape((batch_size, 1, max_length, self.rotary_dim))?;
        let sin_local = 
            self.rotary_cache_local_attention.1.index_select(&position_ids.flatten_all()?, 0)?;
        let sin_local = sin_local.reshape((batch_size, 1, max_length, self.rotary_dim))?;

        for layer in &self.layers {
            hidden_states = if layer.self_attn.sliding_window.is_some() {
                layer.forward(&hidden_states, attention_bias.as_ref(), &cos_local, &sin_local)?
            } else {
                layer.forward(&hidden_states, attention_bias.as_ref(), &cos, &sin)?
            };
        }

        let (outputs, _) = self.norm.forward(&hidden_states, None)?;

        let has_pooling_requests = !batch.pooled_indices.is_empty();

        let pooled_embeddings = if has_pooling_requests {
            match self.pool {
                Pool::Cls | Pool::LastToken | Pool::Splade => {
                    return Err(candle_core::Error::Msg(
                        "Only mean pooling is supported for Gemma3".to_string(),
                    ));
                }
                Pool::Mean => {
                    if batch_size > 1 {
                        let results: candle_core::Result<Vec<Tensor>> = batch
                            .pooled_indices
                            .iter()
                            .map(|&i| {
                                let i = i as usize;
                                let length = input_lengths[i];
                                let embeddings = outputs.i((i, 0..length))?;
                                embeddings.sum_keepdim(0)? / (length as f64)
                            })
                            .collect();

                        Some(Tensor::cat(&results?, 0)?)
                    } else {
                        let length = input_lengths[0];
                        let embeddings = outputs.i((0, 0..length))?;
                        Some((embeddings.sum_keepdim(0)? / (length as f64))?)
                    }
                }
            }     
        } else {
            None
        };

        let dense1_hidden = self.dense1.forward(
            pooled_embeddings.as_ref()
                .ok_or_else(|| candle_core::Error::Msg(
                    "pooled_embeddings is None".to_string()
                ))?
        )?;

        let output_before_norm = self.dense2.forward(&dense1_hidden)?;

        // L2 normalize the tensor (before converting to Vec)
        let norm = output_before_norm.sqr()?.sum_keepdim(1)?.sqrt()?;
        let normalized = output_before_norm.broadcast_div(&norm)?;

        // Return normalized tensor
        Ok(Some(normalized))
    }
}

#[wasm_bindgen]
pub struct Gemma3Embedder {
    model: Gemma3Model,
    tokenizer: Tokenizer,
}

#[wasm_bindgen]
impl Gemma3Embedder {
    /// Load model + tokenizer from HF hub or local path
    #[wasm_bindgen(constructor)]
    pub fn new(
        weights: Vec<u8>,
        weights_dense1: Vec<u8>,
        weights_dense2: Vec<u8>,
        tokenizer: Vec<u8>, 
        config: Vec<u8>
    ) -> Result<Gemma3Embedder, JsError> {
        console_error_panic_hook::set_once();
        
        let tokenizer = 
            Tokenizer::from_bytes(&tokenizer).map_err(|e| JsError::new(&e.to_string()))?;
        
        let config: Gemma3Config = 
            serde_json::from_slice(&config).map_err(|e| JsError::new(&e.to_string()))?;

        let mut cursor = std::io::Cursor::new(weights);

        let content = gguf_file::Content::read(&mut cursor)
            .map_err(|e| JsError::new(&format!("GGUF parse error: {e}")))?;

        let vb_dense1 =
            VarBuilder::from_buffered_safetensors(weights_dense1, DType::F32, &Device::Cpu)
                .map_err(|e| JsError::new(&e.to_string()))?;

        let vb_dense2 =
            VarBuilder::from_buffered_safetensors(weights_dense2, DType::F32, &Device::Cpu)
                .map_err(|e| JsError::new(&e.to_string()))?;

        let model = 
            Gemma3Model::load(content, &mut cursor, vb_dense1, vb_dense2, &config, ModelType::Embedding(Pool::Mean))
            .map_err(|e| JsError::new(&e.to_string()))?;

        Ok(Self { model, tokenizer })
    }

    /// Embed a single sentence
    pub fn embed(&self, text: &str) -> Result<Vec<f32>, JsError> {
        let batch = self.encode_batch(&[text.to_string()])?;
        let normed_output = self.model.forward(batch).map_err(|e| JsError::new(&e.to_string()))?;
        let normed_output =
            normed_output.ok_or_else(|| JsError::new("Embedding generation failed!"))?;
        let normed_output =
            normed_output.to_vec2::<f32>().map_err(|e| JsError::new(&e.to_string()))?;
        let emb = normed_output[0].clone();

        Ok(emb)
    }

    /// Embed multiple sentences - returns flattened array
    pub fn embed_batch(&self, texts: Vec<String>) -> Result<Vec<f32>, JsError> {
        let batch = self.encode_batch(&texts)?;
        let normed_output = self.model.forward(batch).map_err(|e| JsError::new(&e.to_string()))?;
        let normed_output =
            normed_output.ok_or_else(|| JsError::new("Embedding generation failed!"))?;
        let embeddings =
            normed_output.to_vec2::<f32>().map_err(|e| JsError::new(&e.to_string()))?;

        // Flatten: [batch_size, 768] -> [batch_size * 768]
        Ok(embeddings.into_iter().flatten().collect())
    }

    /// Internal helper: convert texts -> Batch
    fn encode_batch(&self, texts: &[String]) -> Result<Batch, JsError> {
        let mut all_ids = Vec::new();
        let mut all_positions = Vec::new();
        let mut cumulative = vec![0];
        let mut max_len = 0;

        for text in texts {
            let encoding = self
                .tokenizer
                .encode(text.as_str(), true)
                .map_err(|e| JsError::new(&format!("Tokenization failed: {e}")))?;

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