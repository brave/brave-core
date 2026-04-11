/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Parakeet-CTC (FastConformer + CTC) model for speech
// recognition. Implements the NVIDIA Parakeet architecture
// using candle primitives for WASM inference.
//
// Reference: HuggingFace Transformers modeling_parakeet.py
// and parakeet.cpp (github.com/Frikallo/parakeet.cpp).

use candle_core::{Device, Result, Tensor, D};
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(
        js_namespace = console,
        js_name = error
    )]
    fn log_err(s: &str);
}
use candle_nn::{
    batch_norm, conv2d, linear, linear_no_bias,
    BatchNorm, BatchNormConfig, Conv1d, Conv1dConfig,
    Conv2d, Conv2dConfig, LayerNorm, Linear, Module,
    ModuleT, VarBuilder,
};
use serde::Deserialize;

// --- Config ---

#[derive(Debug, Clone, Deserialize)]
pub struct ParakeetConfig {
    pub hidden_size: usize,
    pub num_heads: usize,
    pub num_layers: usize,
    pub intermediate_size: usize,
    pub conv_kernel_size: usize,
    pub num_mel_bins: usize,
    pub vocab_size: usize,
    pub subsampling_channels: usize,
}

impl Default for ParakeetConfig {
    fn default() -> Self {
        // Parakeet-TDT_CTC-110M defaults
        Self {
            hidden_size: 512,
            num_heads: 8,
            num_layers: 17,
            intermediate_size: 2048,
            conv_kernel_size: 9,
            num_mel_bins: 80,
            vocab_size: 1025,
            subsampling_channels: 256,
        }
    }
}

// --- Helpers ---

fn layer_norm(
    size: usize,
    vb: VarBuilder,
) -> Result<LayerNorm> {
    let weight = vb.get(size, "weight")?;
    let bias = vb.get(size, "bias")?;
    Ok(LayerNorm::new(weight, bias, 1e-5))
}

fn conv1d_with_bias(
    in_c: usize,
    out_c: usize,
    kernel: usize,
    cfg: Conv1dConfig,
    vb: VarBuilder,
) -> Result<Conv1d> {
    let w = vb.get(
        (out_c, in_c / cfg.groups, kernel),
        "weight",
    )?;
    let b = vb.get(out_c, "bias")?;
    Ok(Conv1d::new(w, Some(b), cfg))
}

// --- Subsampling (8x Conv2d downsample) ---

#[derive(Debug, Clone)]
struct Subsampling {
    layers: Vec<Conv2d>,
    linear: Linear,
}

impl Subsampling {
    fn load(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let ch = cfg.subsampling_channels;
        let vb_layers = vb.pp("layers");

        // Layer 0: Conv2d(1, 256, k=3, s=2, p=1)
        let c0 = Conv2dConfig {
            padding: 1,
            stride: 2,
            ..Default::default()
        };
        let conv0 = conv2d(1, ch, 3, c0, vb_layers.pp("0"))?;

        // Layers 2,5: Depthwise Conv2d(256, 256, k=3, s=2,
        // p=1, groups=256)
        // Layers 3,6: Pointwise Conv2d(256, 256, k=1)
        let c_dw = Conv2dConfig {
            padding: 1,
            stride: 2,
            groups: ch,
            ..Default::default()
        };
        let c_pw = Conv2dConfig::default();

        let dw1 = conv2d(ch, ch, 3, c_dw, vb_layers.pp("2"))?;
        let pw1 = conv2d(ch, ch, 1, c_pw, vb_layers.pp("3"))?;
        let dw2 = conv2d(ch, ch, 3, c_dw, vb_layers.pp("5"))?;
        let pw2 = conv2d(ch, ch, 1, c_pw, vb_layers.pp("6"))?;

        let freq_after = cfg.num_mel_bins / 8;
        let linear_in = ch * freq_after;
        let proj = linear(
            linear_in,
            cfg.hidden_size,
            vb.pp("linear"),
        )?;

        Ok(Self {
            layers: vec![conv0, dw1, pw1, dw2, pw2],
            linear: proj,
        })
    }

    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // x: (B, n_mel, time)
        // Reshape for Conv2d: (B, 1, n_mel, time)
        let mut x = x.unsqueeze(1)?;

        // Conv0 + ReLU
        x = self.layers[0].forward(&x)?.relu()?;
        // DW1 + PW1 + ReLU
        x = self.layers[1].forward(&x)?;
        x = self.layers[2].forward(&x)?.relu()?;
        // DW2 + PW2 + ReLU
        x = self.layers[3].forward(&x)?;
        x = self.layers[4].forward(&x)?.relu()?;

        // x: (B, C=256, T/8, F/8=10) after
        // transposed mel input.
        // HF: transpose(1,2).reshape(B, T/8, C*F/8)
        let (b, c, time, freq) = x.dims4()?;
        x = x
            .transpose(1, 2)?
            .contiguous()?
            .reshape((b, time, c * freq))?;
        self.linear.forward(&x)
    }
}

// --- FeedForward ---

#[derive(Debug, Clone)]
struct FeedForward {
    linear1: Linear,
    linear2: Linear,
}

impl FeedForward {
    fn load(
        hidden: usize,
        ff_dim: usize,
        vb: VarBuilder,
    ) -> Result<Self> {
        let linear1 = linear(hidden, ff_dim, vb.pp("linear1"))?;
        let linear2 = linear(ff_dim, hidden, vb.pp("linear2"))?;
        Ok(Self { linear1, linear2 })
    }

    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = self.linear1.forward(x)?.silu()?;
        self.linear2.forward(&x)
    }
}

// --- Relative Positional Encoding ---

fn compute_pos_emb(
    inv_freq: &Tensor,
    seq_len: usize,
    device: &Device,
) -> Result<Tensor> {
    // Positions from (seq_len-1) down to -(seq_len-1)
    let n_pos = 2 * seq_len - 1;
    let positions: Vec<f32> = (0..n_pos)
        .map(|i| (seq_len as f32 - 1.0) - i as f32)
        .collect();
    let positions = Tensor::new(positions, device)?
        .unsqueeze(1)?; // (n_pos, 1)
    let inv = inv_freq.unsqueeze(0)?; // (1, hidden/2)

    // freqs: (n_pos, hidden/2)
    let freqs = positions.broadcast_mul(&inv)?;
    let sin = freqs.sin()?;
    let cos = freqs.cos()?;
    // Interleave sin and cos: [s0,c0,s1,c1,...]
    // (NOT concatenate [s0,s1,...,c0,c1,...])
    // HF: stack([sin,cos], dim=-1).reshape(...)
    let sin = sin.unsqueeze(D::Minus1)?;
    let cos = cos.unsqueeze(D::Minus1)?;
    let interleaved =
        Tensor::cat(&[sin, cos], D::Minus1)?;
    let (n, half) = freqs.dims2()?;
    // pos_emb: (1, n_pos, hidden)
    interleaved
        .reshape((n, half * 2))?
        .unsqueeze(0)
}

// --- Relative Multi-Head Attention ---

#[derive(Debug, Clone)]
struct RelativeMHA {
    q_proj: Linear,
    k_proj: Linear,
    v_proj: Linear,
    o_proj: Linear,
    relative_k_proj: Linear,
    bias_u: Tensor,
    bias_v: Tensor,
    n_head: usize,
    head_dim: usize,
}

impl RelativeMHA {
    fn load(
        hidden: usize,
        n_head: usize,
        vb: VarBuilder,
    ) -> Result<Self> {
        let head_dim = hidden / n_head;
        let q_proj = linear(hidden, hidden, vb.pp("q_proj"))?;
        let k_proj = linear(hidden, hidden, vb.pp("k_proj"))?;
        let v_proj = linear(hidden, hidden, vb.pp("v_proj"))?;
        let o_proj = linear(hidden, hidden, vb.pp("o_proj"))?;
        let relative_k_proj = linear_no_bias(
            hidden,
            hidden,
            vb.pp("relative_k_proj"),
        )?;
        let bias_u = vb.get((n_head, head_dim), "bias_u")?;
        let bias_v = vb.get((n_head, head_dim), "bias_v")?;
        Ok(Self {
            q_proj,
            k_proj,
            v_proj,
            o_proj,
            relative_k_proj,
            bias_u,
            bias_v,
            n_head,
            head_dim,
        })
    }

    fn forward(
        &self,
        x: &Tensor,
        pos_emb: &Tensor,
    ) -> Result<Tensor> {
        let (b, s, _) = x.dims3()?;
        let h = self.n_head;
        let d = self.head_dim;

        // Q, K, V projections -> (B, H, S, D)
        let q = self
            .q_proj
            .forward(x)?
            .reshape((b, s, h, d))?
            .transpose(1, 2)?;
        let k = self
            .k_proj
            .forward(x)?
            .reshape((b, s, h, d))?
            .transpose(1, 2)?;
        let v = self
            .v_proj
            .forward(x)?
            .reshape((b, s, h, d))?
            .transpose(1, 2)?;

        // R = relative_k_proj(pos_emb) -> (1, H, 2S-1, D)
        let r = self
            .relative_k_proj
            .forward(pos_emb)?
            .reshape((1, 2 * s - 1, h, d))?
            .transpose(1, 2)?;

        // bias_u, bias_v: (H, D) -> (1, H, 1, D)
        let bu = self
            .bias_u
            .unsqueeze(0)?
            .unsqueeze(2)?;
        let bv = self
            .bias_v
            .unsqueeze(0)?
            .unsqueeze(2)?;

        // matrix_ac = (Q + bias_u) @ K^T
        let q_u = q.broadcast_add(&bu)?;
        let matrix_ac =
            q_u.matmul(&k.transpose(2, 3)?)?;

        // matrix_bd = (Q + bias_v) @ R^T
        let q_v = q.broadcast_add(&bv)?;
        let matrix_bd =
            q_v.matmul(&r.transpose(2, 3)?)?;
        let matrix_bd = Self::rel_shift(&matrix_bd)?;

        // scores = (ac + bd) / sqrt(head_dim)
        let scale = (d as f64).sqrt();
        let scores =
            ((matrix_ac + matrix_bd)? / scale)?;

        // softmax -> weighted sum
        let attn =
            candle_nn::ops::softmax_last_dim(&scores)?;
        let out = attn.matmul(&v)?;

        // Reshape back -> (B, S, H*D)
        let out = out
            .transpose(1, 2)?
            .contiguous()?
            .reshape((b, s, h * d))?;
        self.o_proj.forward(&out)
    }

    /// Shaw-style relative shift.
    /// Input: (B, H, S, 2S-1) -> Output: (B, H, S, S)
    fn rel_shift(x: &Tensor) -> Result<Tensor> {
        let (b, h, s, _) = x.dims4()?;
        // Pad left by 1 on last dim
        let zero_pad = Tensor::zeros(
            (b, h, s, 1),
            x.dtype(),
            x.device(),
        )?;
        let x_padded =
            Tensor::cat(&[&zero_pad, x], 3)?;
        // (B, H, S, 2S) -> (B, H, 2S, S)
        let x_padded = x_padded
            .contiguous()?
            .reshape((b, h, 2 * s, s))?;
        // Skip first row -> (B, H, 2S-1, S)
        let x_padded = x_padded.narrow(2, 1, 2 * s - 1)?;
        // -> (B, H, S, 2S-1) -> take first S cols
        x_padded
            .contiguous()?
            .reshape((b, h, s, 2 * s - 1))?
            .narrow(3, 0, s)
    }
}

// --- Convolution Module ---

#[derive(Debug, Clone)]
struct ConvModule {
    pointwise_conv1: Conv1d,
    depthwise_conv: Conv1d,
    norm: BatchNorm,
    pointwise_conv2: Conv1d,
}

impl ConvModule {
    fn load(
        hidden: usize,
        kernel_size: usize,
        vb: VarBuilder,
    ) -> Result<Self> {
        let pw1_cfg = Conv1dConfig::default();
        let pointwise_conv1 = conv1d_with_bias(
            hidden,
            2 * hidden,
            1,
            pw1_cfg,
            vb.pp("pointwise_conv1"),
        )?;

        let pad = (kernel_size - 1) / 2;
        let dw_cfg = Conv1dConfig {
            padding: pad,
            groups: hidden,
            ..Default::default()
        };
        let depthwise_conv = conv1d_with_bias(
            hidden,
            hidden,
            kernel_size,
            dw_cfg,
            vb.pp("depthwise_conv"),
        )?;

        let norm = batch_norm(
            hidden,
            BatchNormConfig::default(),
            vb.pp("norm"),
        )?;

        let pw2_cfg = Conv1dConfig::default();
        let pointwise_conv2 = conv1d_with_bias(
            hidden,
            hidden,
            1,
            pw2_cfg,
            vb.pp("pointwise_conv2"),
        )?;

        Ok(Self {
            pointwise_conv1,
            depthwise_conv,
            norm,
            pointwise_conv2,
        })
    }

    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // x: (B, T, H) -> transpose to (B, H, T)
        let x = x.transpose(1, 2)?;

        // Pointwise conv1 -> GLU
        let x = self.pointwise_conv1.forward(&x)?;
        let chunks = x.chunk(2, 1)?;
        let gate = candle_nn::ops::sigmoid(&chunks[1])?;
        let x = (&chunks[0] * &gate)?;

        // Depthwise conv
        let x = self.depthwise_conv.forward(&x)?;

        // BatchNorm (eval mode)
        let x = self.norm.forward_t(&x, false)?;

        // SiLU + pointwise conv2
        let x = x.silu()?;
        let x = self.pointwise_conv2.forward(&x)?;

        // Back to (B, T, H)
        x.transpose(1, 2)
    }
}

// --- Conformer Block ---

#[derive(Debug, Clone)]
struct ConformerBlock {
    norm_ff1: LayerNorm,
    ff1: FeedForward,
    norm_self_att: LayerNorm,
    self_attn: RelativeMHA,
    norm_conv: LayerNorm,
    conv: ConvModule,
    norm_ff2: LayerNorm,
    ff2: FeedForward,
    norm_out: LayerNorm,
}

impl ConformerBlock {
    fn load(
        cfg: &ParakeetConfig,
        vb: VarBuilder,
    ) -> Result<Self> {
        let h = cfg.hidden_size;
        let ff = cfg.intermediate_size;
        Ok(Self {
            norm_ff1: layer_norm(
                h,
                vb.pp("norm_feed_forward1"),
            )?,
            ff1: FeedForward::load(
                h,
                ff,
                vb.pp("feed_forward1"),
            )?,
            norm_self_att: layer_norm(
                h,
                vb.pp("norm_self_att"),
            )?,
            self_attn: RelativeMHA::load(
                h,
                cfg.num_heads,
                vb.pp("self_attn"),
            )?,
            norm_conv: layer_norm(
                h,
                vb.pp("norm_conv"),
            )?,
            conv: ConvModule::load(
                h,
                cfg.conv_kernel_size,
                vb.pp("conv"),
            )?,
            norm_ff2: layer_norm(
                h,
                vb.pp("norm_feed_forward2"),
            )?,
            ff2: FeedForward::load(
                h,
                ff,
                vb.pp("feed_forward2"),
            )?,
            norm_out: layer_norm(
                h,
                vb.pp("norm_out"),
            )?,
        })
    }

    fn forward(
        &self,
        x: &Tensor,
        pos_emb: &Tensor,
    ) -> Result<Tensor> {
        // Half-step FF1
        let residual = x.clone();
        let x = (&residual
            + (self
                .ff1
                .forward(&self.norm_ff1.forward(x)?)?
                * 0.5)?)?;

        // Self-attention with relative pos
        let x = (&x
            + self.self_attn.forward(
                &self.norm_self_att.forward(&x)?,
                pos_emb,
            )?)?;

        // Convolution module
        let x = (&x
            + self
                .conv
                .forward(
                    &self.norm_conv.forward(&x)?,
                )?)?;

        // Half-step FF2
        let x = (&x
            + (self
                .ff2
                .forward(&self.norm_ff2.forward(&x)?)?
                * 0.5)?)?;

        // Final norm
        self.norm_out.forward(&x)
    }
}

// --- CTC Head ---

#[derive(Debug, Clone)]
struct CtcHead {
    conv: Conv1d,
}

impl CtcHead {
    fn load(
        hidden: usize,
        vocab_size: usize,
        vb: VarBuilder,
    ) -> Result<Self> {
        let cfg = Conv1dConfig::default();
        let conv =
            conv1d_with_bias(hidden, vocab_size, 1, cfg, vb)?;
        Ok(Self { conv })
    }

    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // x: (B, T, H) -> (B, H, T) for conv
        let x = x.transpose(1, 2)?;
        let x = self.conv.forward(&x)?;
        // -> (B, vocab, T) -> (B, T, vocab)
        x.transpose(1, 2)
    }
}

// --- Full Parakeet Model ---

#[derive(Debug, Clone)]
pub struct Parakeet {
    subsampling: Subsampling,
    inv_freq: Tensor,
    blocks: Vec<ConformerBlock>,
    ctc_head: CtcHead,
    hidden_size: usize,
}

impl Parakeet {
    pub fn load(
        vb: &VarBuilder,
        cfg: &ParakeetConfig,
    ) -> Result<Self> {
        let vb_enc = vb.pp("encoder");

        let subsampling = Subsampling::load(
            cfg,
            vb_enc.pp("subsampling"),
        )?;

        // NeMo doesn't store inv_freq in the
        // checkpoint — compute from config.
        // NeMo: arange(0, hidden_size, 2) / hidden_size
        // i.e. [0, 2, 4, ..., H-2] / H
        let half = cfg.hidden_size / 2;
        let inv_freq_vec: Vec<f32> = (0..half)
            .map(|i| {
                1.0 / (10000f32.powf(
                    (2 * i) as f32
                        / cfg.hidden_size as f32,
                ))
            })
            .collect();
        let inv_freq = Tensor::new(
            inv_freq_vec,
            vb.device(),
        )?;

        let blocks = (0..cfg.num_layers)
            .map(|i| {
                ConformerBlock::load(
                    cfg,
                    vb_enc.pp(format!("layers.{i}")),
                )
            })
            .collect::<Result<Vec<_>>>()?;

        let ctc_head = CtcHead::load(
            cfg.hidden_size,
            cfg.vocab_size,
            vb.pp("ctc_ctc_head"),
        )?;

        Ok(Self {
            subsampling,
            inv_freq,
            blocks,
            ctc_head,
            hidden_size: cfg.hidden_size,
        })
    }

    pub fn forward(
        &self,
        mel: &Tensor,
    ) -> Result<Tensor> {
        // mel: (B, n_mel, time) → transpose to
        // (B, time, n_mel) to match HF/NeMo
        // Conv2d convention (time=height, freq=width)
        let mel = mel.transpose(1, 2)?;

        // Subsampling (8x downsample)
        let mut x =
            self.subsampling.forward(&mel)?;

        // Scale input by sqrt(hidden_size)
        let scale = (self.hidden_size as f64).sqrt();
        x = (x * scale)?;

        // Compute positional embeddings
        let seq_len = x.dim(1)?;
        let pos_emb = compute_pos_emb(
            &self.inv_freq,
            seq_len,
            x.device(),
        )?;

        // Conformer blocks
        for (i, block) in
            self.blocks.iter().enumerate()
        {
            x = block.forward(&x, &pos_emb)?;
            if i < 3 || i == self.blocks.len() - 1 {
                let flat = x.flatten_all()?;
                let vals: Vec<f32> =
                    flat.to_vec1()?;
                let mean = vals.iter().sum::<f32>()
                    / vals.len() as f32;
                let max = vals
                    .iter()
                    .cloned()
                    .fold(f32::NEG_INFINITY, f32::max);
                let min = vals
                    .iter()
                    .cloned()
                    .fold(f32::INFINITY, f32::min);
                let has_nan =
                    vals.iter().any(|v| v.is_nan());
                log_err(&format!(
                    "[parakeet] block {}: \
                     mean={:.4} min={:.4} \
                     max={:.4} nan={}",
                    i, mean, min, max, has_nan,
                ));
            }
        }

        // CTC head
        self.ctc_head.forward(&x)
    }
}

// --- CTC Greedy Decoding ---

/// Greedy CTC decode: argmax, collapse duplicates,
/// remove blank token. Returns token IDs.
pub fn ctc_greedy_decode(
    logits: &Tensor,
    blank_id: u32,
) -> Result<Vec<u32>> {
    // logits: (1, T, vocab) -> take batch 0
    let logits = logits.squeeze(0)?; // (T, vocab)
    let predictions = logits.argmax(D::Minus1)?; // (T,)
    let pred_vec: Vec<u32> =
        predictions.to_vec1::<u32>()?;

    // Collapse consecutive duplicates, remove blanks
    let mut result = Vec::new();
    let mut prev = blank_id;
    for &tok in &pred_vec {
        if tok != prev && tok != blank_id {
            result.push(tok);
        }
        prev = tok;
    }
    Ok(result)
}
