/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/* This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (c) NVIDIA CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Conformer self-attention with relative positional encoding for
// Parakeet-CTC-110M. See per-struct doc for NeMo source.

use candle_core::{Module, Result, Tensor, D};
use candle_nn::{linear, linear_no_bias, Linear, VarBuilder};

use super::ParakeetConfig;

/// Multi-head self-attention with Transformer-XL-style relative
/// positional encoding (one instance per Conformer layer). Adds two
/// things on top of standard attention: the position table `pos_emb`
/// is projected through `linear_pos`, and per-head learned biases
/// `pos_bias_u` / `pos_bias_v` are added to the query to produce
/// separate content-content (`matrix_ac`) and content-position
/// (`matrix_bd`) score terms that sum before softmax. `rel_shift`
/// rearranges `matrix_bd` columns so entry `(i, j)` holds the score
/// for relative distance `i - j` (Transformer-XL Appendix B trick).
///
/// Input:
///   - `x: (B, T, hidden_size)` — subsampled encoder states
///   - `pos_emb: (1, 2T-1, hidden_size)` from `RelPositionalEncoding`
/// Output: `(B, T, hidden_size)`
///
/// Tensor names under each `encoder.layers.N.self_attn`:
/// ```text
/// linear_q.{weight,bias}
/// linear_k.{weight,bias}
/// linear_v.{weight,bias}
/// linear_out.{weight,bias}
/// linear_pos.weight          # no bias
/// pos_bias_u                 # (num_heads, head_dim)
/// pos_bias_v                 # (num_heads, head_dim)
/// ```
///
/// # Port notes
///
/// Everything above describes the layer itself. The bullets below
/// describe choices specific to this Rust port, tied to how Brave
/// runs Parakeet — a general Parakeet deployment could need some
/// of these.
///
/// - **Manual-compute attention.** NeMo can compute attention via PyTorch's
///   fused `scaled_dot_product_attention`; candle has no equivalent, so we
///   always compute matmul → softmax → matmul separately.
/// - **No KV cache / padding mask / attention dropout.** Brave's
///   candle_parakeet runs batch size 1 with chunk-based (not per-frame
///   incremental) streaming, so each forward is a fresh full pass with no
///   cross-call state to cache and no padded positions to mask. Dropout is a
///   no-op at inference.
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/submodules/multi_head_attention.py>
/// — implements `RelPositionMultiHeadAttention`. Paper reference
/// for relative positional encoding:
/// <https://arxiv.org/abs/1901.02860> (Transformer-XL, Section 3.3).
pub struct RelPositionMultiHeadAttention {
    linear_q: Linear,
    linear_k: Linear,
    linear_v: Linear,
    linear_out: Linear,
    linear_pos: Linear,
    pos_bias_u: Tensor,
    pos_bias_v: Tensor,
    num_heads: usize,
    head_dim: usize,
    scale: f64,
}

impl RelPositionMultiHeadAttention {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let d = cfg.hidden_size;
        let h = cfg.num_heads;
        assert!(d % h == 0, "hidden_size must be divisible by num_heads");
        let d_k = d / h;
        Ok(Self {
            linear_q: linear(d, d, vb.pp("linear_q"))?,
            linear_k: linear(d, d, vb.pp("linear_k"))?,
            linear_v: linear(d, d, vb.pp("linear_v"))?,
            linear_out: linear(d, d, vb.pp("linear_out"))?,
            linear_pos: linear_no_bias(d, d, vb.pp("linear_pos"))?,
            pos_bias_u: vb.get((h, d_k), "pos_bias_u")?,
            pos_bias_v: vb.get((h, d_k), "pos_bias_v")?,
            num_heads: h,
            head_dim: d_k,
            scale: (d_k as f64).sqrt(),
        })
    }

    pub fn forward(&self, x: &Tensor, pos_emb: &Tensor) -> Result<Tensor> {
        let (b, t, _) = x.dims3()?;
        let h = self.num_heads;
        let d_k = self.head_dim;

        // Q stays as (B, T, H, d_k) so pos_bias can be broadcast-added
        // on the H/d_k tail; K and V move to (B, H, T, d_k) for matmul.
        let q = self.linear_q.forward(x)?.reshape((b, t, h, d_k))?;
        let k = self.linear_k.forward(x)?.reshape((b, t, h, d_k))?.transpose(1, 2)?.contiguous()?;
        let v = self.linear_v.forward(x)?.reshape((b, t, h, d_k))?.transpose(1, 2)?.contiguous()?;

        // Project the relative-position table: (1, 2T-1, D) -> (1, H, 2T-1, d_k).
        let pos_len = pos_emb.dim(1)?;
        let p = self
            .linear_pos
            .forward(pos_emb)?
            .reshape((1, pos_len, h, d_k))?
            .transpose(1, 2)?
            .contiguous()?;

        // q + pos_bias_{u,v}, then transpose to (B, H, T, d_k).
        let u = self.pos_bias_u.reshape((1, 1, h, d_k))?;
        let v_bias = self.pos_bias_v.reshape((1, 1, h, d_k))?;
        let q_with_u = q.broadcast_add(&u)?.transpose(1, 2)?.contiguous()?;
        let q_with_v = q.broadcast_add(&v_bias)?.transpose(1, 2)?.contiguous()?;

        // matrix_ac: content × content, (B, H, T, T).
        let k_t = k.transpose(D::Minus2, D::Minus1)?.contiguous()?;
        let matrix_ac = q_with_u.matmul(&k_t)?;

        // matrix_bd: content × position. Shape (B, H, T, 2T-1) after
        // matmul, then rel_shift rearranges it so column j holds the
        // score for distance i-j, then we keep the first T columns.
        let p_t = p.transpose(D::Minus2, D::Minus1)?.contiguous()?;
        let matrix_bd = q_with_v.broadcast_matmul(&p_t)?;
        let matrix_bd = Self::rel_shift(&matrix_bd)?;
        let matrix_bd = matrix_bd.narrow(D::Minus1, 0, t)?;

        let scores = ((matrix_ac + matrix_bd)? / self.scale)?;
        let attn = candle_nn::ops::softmax(&scores, D::Minus1)?;
        let out = attn.matmul(&v)?;

        let out = out.transpose(1, 2)?.contiguous()?.reshape((b, t, h * d_k))?;
        self.linear_out.forward(&out)
    }

    /// See the "rel_shift trick" section on the struct for what and
    /// why. Input `(B, H, T, 2T-1)`, output same shape, rotated so
    /// row `i` reads relative distances `i-(T-1), ..., i, ..., i+(T-1)`
    /// in consecutive columns.
    fn rel_shift(x: &Tensor) -> Result<Tensor> {
        let (b, h, t, pos_len) = x.dims4()?;
        let pad = Tensor::zeros((b, h, t, 1), x.dtype(), x.device())?;
        let x = Tensor::cat(&[&pad, x], D::Minus1)?;
        let x = x.reshape((b, h, pos_len + 1, t))?;
        let x = x.narrow(2, 1, pos_len)?;
        x.reshape((b, h, t, pos_len))
    }
}
