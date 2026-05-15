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

// Cache-aware relative-positional self-attention for nemotron.
// Differs from candle_parakeet's attention.rs in two places:
//   - all linear projections are bias-free (`use_bias: false` in
//     the upstream yaml).
//   - forward takes separate `q_in` and `kv_in` tensors so the layer
//     above can pass the cached frames concatenated onto the new
//     frames as the key/value source.

use candle_core::{Result, Tensor, D};

use super::load::{take_f32, take_ql_nb, QLinear, TensorMap};
use super::NemotronConfig;

pub struct RelPositionMultiHeadAttention {
    linear_q: QLinear,
    linear_k: QLinear,
    linear_v: QLinear,
    linear_out: QLinear,
    linear_pos: QLinear,
    pos_bias_u: Tensor,
    pos_bias_v: Tensor,
    num_heads: usize,
    head_dim: usize,
    scale: f64,
}

impl RelPositionMultiHeadAttention {
    pub fn load(
        cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        let d = cfg.hidden_size;
        let h = cfg.num_heads;
        assert!(d % h == 0, "hidden_size must be divisible by num_heads");
        let d_k = d / h;
        Ok(Self {
            linear_q: take_ql_nb(t, &format!("{prefix}.linear_q"))?,
            linear_k: take_ql_nb(t, &format!("{prefix}.linear_k"))?,
            linear_v: take_ql_nb(t, &format!("{prefix}.linear_v"))?,
            linear_out: take_ql_nb(
                t,
                &format!("{prefix}.linear_out"),
            )?,
            linear_pos: take_ql_nb(
                t,
                &format!("{prefix}.linear_pos"),
            )?,
            pos_bias_u: take_f32(
                t,
                &format!("{prefix}.pos_bias_u"),
            )?,
            pos_bias_v: take_f32(
                t,
                &format!("{prefix}.pos_bias_v"),
            )?,
            num_heads: h,
            head_dim: d_k,
            scale: (d_k as f64).sqrt(),
        })
    }

    /// Forward with separate query and key/value inputs (the
    /// key/value side has the layer cache prepended along time).
    ///
    /// Shapes:
    ///   q_in:    (B, T_q, D)
    ///   kv_in:   (B, T_kv, D)   T_kv = T_q + cache_len
    ///   pos_emb: (1, T_kv + T_q - 1, D) -- one row per relative
    ///            distance from a query position to a kv position.
    ///
    /// Output: (B, T_q, D)
    pub fn forward(
        &self,
        q_in: &Tensor,
        kv_in: &Tensor,
        pos_emb: &Tensor,
    ) -> Result<Tensor> {
        let (b, t_q, _) = q_in.dims3()?;
        let t_kv = kv_in.dim(1)?;
        let h = self.num_heads;
        let d_k = self.head_dim;

        let q = self
            .linear_q
            .forward(q_in)?
            .reshape((b, t_q, h, d_k))?;
        let k = self
            .linear_k
            .forward(kv_in)?
            .reshape((b, t_kv, h, d_k))?
            .transpose(1, 2)?
            .contiguous()?;
        let v = self
            .linear_v
            .forward(kv_in)?
            .reshape((b, t_kv, h, d_k))?
            .transpose(1, 2)?
            .contiguous()?;

        let pos_len = pos_emb.dim(1)?;
        let p = self
            .linear_pos
            .forward(pos_emb)?
            .reshape((1, pos_len, h, d_k))?
            .transpose(1, 2)?
            .contiguous()?;

        let u = self.pos_bias_u.reshape((1, 1, h, d_k))?;
        let v_bias = self.pos_bias_v.reshape((1, 1, h, d_k))?;
        let q_with_u =
            q.broadcast_add(&u)?.transpose(1, 2)?.contiguous()?;
        let q_with_v =
            q.broadcast_add(&v_bias)?.transpose(1, 2)?.contiguous()?;

        let k_t = k.transpose(D::Minus2, D::Minus1)?.contiguous()?;
        let matrix_ac = q_with_u.matmul(&k_t)?;

        let p_t = p.transpose(D::Minus2, D::Minus1)?.contiguous()?;
        let matrix_bd = q_with_v.broadcast_matmul(&p_t)?;
        let matrix_bd = Self::rel_shift(&matrix_bd, t_kv)?;
        let matrix_bd = matrix_bd.narrow(D::Minus1, 0, t_kv)?;

        let scores = ((matrix_ac + matrix_bd)? / self.scale)?;
        let attn = candle_nn::ops::softmax(&scores, D::Minus1)?;
        let out = attn.matmul(&v)?;

        let out = out
            .transpose(1, 2)?
            .contiguous()?
            .reshape((b, t_q, h * d_k))?;
        self.linear_out.forward(&out)
    }

    /// Transformer-XL relative-shift trick generalized for the
    /// cache-aware case. Input `(B, H, T_q, pos_len)` with
    /// `pos_len = T_kv + T_q - 1`; output `(B, H, T_q, T_kv)` (after
    /// the narrow that follows this call in `forward`).
    ///
    /// We zero-pad a column on the left, reshape so the padded
    /// matrix's `(T_q, pos_len+1)` is read as `(pos_len+1, T_q)`,
    /// drop the first row, and reshape back. Each row ends up
    /// rotated relative to the row above by exactly one position,
    /// which is the relative-position rearrangement Transformer-XL
    /// Appendix B describes.
    fn rel_shift(x: &Tensor, t_kv: usize) -> Result<Tensor> {
        let (b, h, t_q, pos_len) = x.dims4()?;
        let pad =
            Tensor::zeros((b, h, t_q, 1), x.dtype(), x.device())?;
        let x = Tensor::cat(&[&pad, x], D::Minus1)?;
        let x = x.reshape((b, h, pos_len + 1, t_q))?;
        // The narrow that drops the first row produces a NON-
        // contiguous view. The subsequent reshape from (pos_len, t_q)
        // back to (t_q, pos_len) is the load-bearing step of the
        // rel-shift trick -- it RELIES on reading the underlying
        // bytes in row-major order, so without contiguous() candle
        // is reading scrambled bytes and the trick silently
        // produces garbage. (This is the cache-streaming bug.)
        let x = x.narrow(2, 1, pos_len)?.contiguous()?;
        let x = x.reshape((b, h, t_q, pos_len))?;
        let keep = t_kv.min(pos_len);
        x.narrow(D::Minus1, 0, keep)
    }
}
