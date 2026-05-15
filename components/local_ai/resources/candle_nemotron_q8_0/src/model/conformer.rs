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

// Conformer feed-forward, convolution, and layer wrapper for
// nemotron-speech-streaming. Differs from candle_parakeet:
//   - `use_bias: false` for the FFN linears (yaml-driven).
//   - `conv_norm_type: layer_norm` so the conv block uses LayerNorm
//     where parakeet used BatchNorm. The state dict still stores
//     the weights under the historical name `batch_norm.*` -- only
//     `weight` and `bias` are present (no `running_mean` /
//     `running_var`), matching LayerNorm semantics.
//   - the depthwise conv supports a per-call cache (last
//     `cache_frames = kernel_size - 1` time steps of its input) and
//     causal padding -- nemotron's `conv_context_size: causal` shifts
//     the kernel so it never reads future frames.
//   - the layer forward exposes the cache plumbing the streaming
//     encoder needs (one `last_channel` cache for self-attention and
//     one `last_time` cache for depthwise conv).

use candle_core::quantized::QMatMul;
use candle_core::{Module, Result, Tensor, D};
use candle_nn::{
    ops, Conv1d, Conv1dConfig, LayerNorm, LayerNormConfig,
};

use super::attention::RelPositionMultiHeadAttention;
use super::load::{
    squeeze_kernel_dim, take_conv1d, take_ln, take_ql_nb, take_qt,
    QLinear, TensorMap,
};
use super::NemotronConfig;

/// Position-wise feed-forward. Bias-free linears match the
/// upstream `use_bias=false` setting for nemotron-streaming.
pub struct ConformerFeedForward {
    linear1: QLinear,
    linear2: QLinear,
}

impl ConformerFeedForward {
    pub fn load(
        _cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        Ok(Self {
            linear1: take_ql_nb(t, &format!("{prefix}.linear1"))?,
            linear2: take_ql_nb(t, &format!("{prefix}.linear2"))?,
        })
    }

    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = self.linear1.forward(x)?;
        let x = ops::silu(&x)?;
        self.linear2.forward(&x)
    }
}

/// Cache-aware Conformer convolution.
///
/// Forward path (matches NeMo `ConformerConvolution` with
/// `pointwise_activation='glu_'` and causal context):
/// ```text
/// x: (B, T, D)
///   transpose -> (B, D, T)
///   pointwise_conv1: Conv1d(D -> 2D, k=1)
///   GLU on channel dim -> (B, D, T)
///   prepend cache (B, D, cache_frames) along time axis
///   depthwise_conv: Conv1d(D -> D, k=kernel, groups=D, no padding;
///                          causal padding comes from the prepended
///                          cache plus zero-pad on the first chunk)
///   slice the output's last T frames (drops the cache-induced ones)
///   LayerNorm over channel dim
///   Swish
///   pointwise_conv2: Conv1d(D -> D, k=1)
///   transpose -> (B, T, D)
/// ```
/// The next cache is the last `cache_frames` time steps of the
/// pre-depthwise-conv signal, i.e. the (B, D, T) slice just before
/// the depthwise call.
pub struct ConformerConvolution {
    // Pointwise convs are stored as 3D Conv1d weights `(out, in, 1)`
    // in the GGUF. We squeeze the kernel dim and wrap in QMatMul so
    // the (Q-)matmul stays on the QTensor (no full dequant at load).
    pointwise_conv1: QMatMul,
    depthwise_conv: Conv1d,
    norm: LayerNorm,
    pointwise_conv2: QMatMul,
    cache_frames: usize,
}

impl ConformerConvolution {
    pub fn load(
        cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        let d = cfg.hidden_size;
        let k = cfg.conv_kernel_size;

        // Depthwise conv: NO internal padding -- the caller
        // pre-pends `cache_frames = k-1` cached time steps and we
        // zero-pad those frames on the first chunk in the
        // EncoderCache initializer. Output time dim therefore comes
        // out to T (matching the input T) for the streaming path.
        let depthwise_cfg = Conv1dConfig {
            padding: 0,
            stride: 1,
            dilation: 1,
            groups: d,
            cudnn_fwd_algo: None,
        };

        let pw1 = squeeze_kernel_dim(take_qt(
            t,
            &format!("{prefix}.pointwise_conv1.weight"),
        )?)?;
        let pw2 = squeeze_kernel_dim(take_qt(
            t,
            &format!("{prefix}.pointwise_conv2.weight"),
        )?)?;

        Ok(Self {
            pointwise_conv1: QMatMul::from_qtensor(pw1)?,
            depthwise_conv: take_conv1d(
                t,
                &format!("{prefix}.depthwise_conv"),
                depthwise_cfg,
                false,
            )?,
            // Despite the state-dict name `batch_norm.*`, the
            // upstream config sets `conv_norm_type=layer_norm` and
            // the dump only contains `weight` and `bias` -- LN
            // parameters, not BN running stats.
            norm: take_ln(
                t,
                &format!("{prefix}.batch_norm"),
                LayerNormConfig::default(),
            )?,
            pointwise_conv2: QMatMul::from_qtensor(pw2)?,
            cache_frames: k - 1,
        })
    }

    /// Forward returning (output, next_cache).
    ///
    /// `x` shape: (B, T, D). `cache` shape: (B, D, cache_frames) with
    /// `cache_frames = kernel_size - 1`.
    pub fn forward(
        &self,
        x: &Tensor,
        cache: &Tensor,
    ) -> Result<(Tensor, Tensor)> {
        let (b, t, _d) = x.dims3()?;
        let kvc = self.cache_frames;

        // Pointwise conv1 as Linear-style matmul. QMatMul takes (..., D)
        // and returns (..., 2D). Stay in (B, T, D)/(B, T, 2D) layout.
        let x = self.pointwise_conv1.forward(x)?;
        // GLU on the channel (last) axis: (B, T, 2D) -> (B, T, D).
        let x = Self::glu(&x)?;

        // Depthwise conv expects (B, D, T). Transpose, then prepend
        // cached time frames so the depthwise sees [cache, x] with no
        // internal padding.
        let x = x.transpose(1, 2)?.contiguous()?;
        let extended = Tensor::cat(&[cache, &x], D::Minus1)?;

        // Next-step cache: the LAST `cache_frames` time steps of the
        // pre-conv extended sequence. narrow yields a non-contiguous
        // view; force contiguous so the next call's cat reads the
        // right bytes.
        let total_t = extended.dim(D::Minus1)?;
        let keep = kvc.min(total_t);
        let cache_next = extended
            .narrow(D::Minus1, total_t - keep, keep)?
            .contiguous()?;

        // Depthwise conv -> (B, D, T).
        let conv_out = self.depthwise_conv.forward(&extended)?;
        let out_t = conv_out.dim(D::Minus1)?;
        debug_assert_eq!(out_t, t, "depthwise conv output time mismatch");

        // LayerNorm over channel dim (candle LN normalizes over the
        // LAST axis, so transpose to (B, T, D)).
        let conv_bt_d = conv_out.transpose(1, 2)?.contiguous()?;
        let normed = self.norm.forward(&conv_bt_d)?;

        // pointwise_conv2 as Linear-style matmul on (B, T, D) -> (B, T, D).
        let activated = ops::silu(&normed)?;
        let out = self.pointwise_conv2.forward(&activated)?;

        // Pad cache_next on the left when the first chunk has fewer
        // than `cache_frames` time steps so every subsequent call
        // sees the full (B, D, cache_frames) shape.
        let cache_next = if keep < kvc {
            let pad = Tensor::zeros(
                (b, cache_next.dim(1)?, kvc - keep),
                cache_next.dtype(),
                cache_next.device(),
            )?;
            Tensor::cat(&[&pad, &cache_next], D::Minus1)?
        } else {
            cache_next
        };

        Ok((out, cache_next))
    }

    /// Gated Linear Unit along the channel (last) dimension. Input
    /// `(B, T, 2D)`, output `(B, T, D)`.
    fn glu(x: &Tensor) -> Result<Tensor> {
        let dim = x.dim(D::Minus1)?;
        assert!(dim % 2 == 0, "GLU requires an even channel dim");
        let half = dim / 2;
        let a = x.narrow(D::Minus1, 0, half)?;
        let b = x.narrow(D::Minus1, half, half)?;
        a * ops::sigmoid(&b)?
    }
}

/// One Conformer layer with cache plumbing for cache-aware
/// streaming. Macaron FFN sandwich around self-attention and
/// convolution, with pre-norm LayerNorm on every residual branch
/// and a final per-layer LayerNorm.
pub struct ConformerLayer {
    norm_feed_forward1: LayerNorm,
    feed_forward1: ConformerFeedForward,
    norm_self_att: LayerNorm,
    self_attn: RelPositionMultiHeadAttention,
    norm_conv: LayerNorm,
    conv: ConformerConvolution,
    norm_feed_forward2: LayerNorm,
    feed_forward2: ConformerFeedForward,
    norm_out: LayerNorm,
    left_context: usize,
}

impl ConformerLayer {
    pub fn load(
        cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        let _d = cfg.hidden_size;
        let ln_cfg = LayerNormConfig::default();
        Ok(Self {
            norm_feed_forward1: take_ln(
                t,
                &format!("{prefix}.norm_feed_forward1"),
                ln_cfg,
            )?,
            feed_forward1: ConformerFeedForward::load(
                cfg,
                t,
                &format!("{prefix}.feed_forward1"),
            )?,
            norm_self_att: take_ln(
                t,
                &format!("{prefix}.norm_self_att"),
                ln_cfg,
            )?,
            self_attn: RelPositionMultiHeadAttention::load(
                cfg,
                t,
                &format!("{prefix}.self_attn"),
            )?,
            norm_conv: take_ln(
                t,
                &format!("{prefix}.norm_conv"),
                ln_cfg,
            )?,
            conv: ConformerConvolution::load(
                cfg,
                t,
                &format!("{prefix}.conv"),
            )?,
            norm_feed_forward2: take_ln(
                t,
                &format!("{prefix}.norm_feed_forward2"),
                ln_cfg,
            )?,
            feed_forward2: ConformerFeedForward::load(
                cfg,
                t,
                &format!("{prefix}.feed_forward2"),
            )?,
            norm_out: take_ln(
                t,
                &format!("{prefix}.norm_out"),
                ln_cfg,
            )?,
            left_context: cfg.left_context_frames,
        })
    }

    /// Streaming forward.
    ///
    /// Inputs:
    ///   x:           (B, T_new, D) -- the new chunk's frames
    ///   pos_emb:     (1, T_kv + T_new - 1, D) where T_kv = T_new +
    ///                cache_in_channel.dim(1)
    ///   cache_in_channel: (B, L, D) -- last L frames of this layer's
    ///                    self-attn input from previous chunks
    ///   cache_in_time: (B, D, K-1) -- depthwise-conv state from
    ///                  previous chunks
    ///
    /// Returns (output, cache_channel_next, cache_time_next).
    pub fn forward(
        &self,
        x: &Tensor,
        pos_emb: &Tensor,
        cache_in_channel: &Tensor,
        cache_in_time: &Tensor,
    ) -> Result<(Tensor, Tensor, Tensor)> {
        // Half-step FFN1.
        let ff1 = self
            .feed_forward1
            .forward(&self.norm_feed_forward1.forward(x)?)?;
        let x = (x + ff1.affine(0.5, 0.0)?)?;

        // Self-attention with cache. Query = new x; key/value = cache
        // concatenated with new x.
        let normed = self.norm_self_att.forward(&x)?;
        let kv_in = if cache_in_channel.dim(1)? > 0 {
            Tensor::cat(&[cache_in_channel, &normed], 1)?
        } else {
            normed.clone()
        };
        let att = self.self_attn.forward(&normed, &kv_in, pos_emb)?;
        let x = (&x + att)?;

        // Save the last `left_context` frames of the extended kv
        // input as the next cache.
        let kv_t = kv_in.dim(1)?;
        let keep = self.left_context.min(kv_t);
        let cache_channel_next =
            kv_in.narrow(1, kv_t - keep, keep)?.contiguous()?;

        // Convolution with depthwise-conv cache.
        let normed_for_conv = self.norm_conv.forward(&x)?;
        let (cv, cache_time_next) =
            self.conv.forward(&normed_for_conv, cache_in_time)?;
        let x = (&x + cv)?;

        // Half-step FFN2.
        let ff2 = self
            .feed_forward2
            .forward(&self.norm_feed_forward2.forward(&x)?)?;
        let x = (&x + ff2.affine(0.5, 0.0)?)?;

        let out = self.norm_out.forward(&x)?;
        Ok((out, cache_channel_next, cache_time_next))
    }
}
