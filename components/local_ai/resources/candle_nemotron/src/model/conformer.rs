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

use candle_core::{Module, Result, Tensor, D};
use candle_nn::{
    conv1d_no_bias, layer_norm, linear_no_bias, ops, Conv1d,
    Conv1dConfig, LayerNorm, LayerNormConfig, Linear, VarBuilder,
};

use super::attention::RelPositionMultiHeadAttention;
use super::NemotronConfig;

/// Position-wise feed-forward. Bias-free linears match the
/// upstream `use_bias=false` setting for nemotron-streaming.
pub struct ConformerFeedForward {
    linear1: Linear,
    linear2: Linear,
}

impl ConformerFeedForward {
    pub fn new(cfg: &NemotronConfig, vb: VarBuilder) -> Result<Self> {
        Ok(Self {
            linear1: linear_no_bias(
                cfg.hidden_size,
                cfg.intermediate_size,
                vb.pp("linear1"),
            )?,
            linear2: linear_no_bias(
                cfg.intermediate_size,
                cfg.hidden_size,
                vb.pp("linear2"),
            )?,
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
    pointwise_conv1: Conv1d,
    depthwise_conv: Conv1d,
    norm: LayerNorm,
    pointwise_conv2: Conv1d,
    cache_frames: usize,
}

impl ConformerConvolution {
    pub fn new(cfg: &NemotronConfig, vb: VarBuilder) -> Result<Self> {
        let d = cfg.hidden_size;
        let k = cfg.conv_kernel_size;

        // Pointwise convs: in NeMo the depthwise_conv uses bias but
        // pointwise_conv1/2 do not (the saved dump only has `.weight`
        // for the pointwise pair). Reflect that.
        let pointwise_cfg = Conv1dConfig {
            padding: 0,
            stride: 1,
            dilation: 1,
            groups: 1,
            cudnn_fwd_algo: None,
        };
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

        Ok(Self {
            pointwise_conv1: conv1d_no_bias(
                d,
                2 * d,
                1,
                pointwise_cfg,
                vb.pp("pointwise_conv1"),
            )?,
            depthwise_conv: conv1d_no_bias(
                d,
                d,
                k,
                depthwise_cfg,
                vb.pp("depthwise_conv"),
            )?,
            // Despite the state-dict name `batch_norm.*`, the
            // upstream config sets `conv_norm_type=layer_norm` and
            // the dump only contains `weight` and `bias` -- LN
            // parameters, not BN running stats.
            norm: layer_norm(
                d,
                LayerNormConfig::default(),
                vb.pp("batch_norm"),
            )?,
            pointwise_conv2: conv1d_no_bias(
                d,
                d,
                1,
                pointwise_cfg,
                vb.pp("pointwise_conv2"),
            )?,
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

        // (B, T, D) -> (B, D, T) for Conv1d.
        let x = x.transpose(1, 2)?.contiguous()?;

        // Expand channels to 2D, gate via GLU to get back to D.
        let x = self.pointwise_conv1.forward(&x)?;
        let x = Self::glu(&x)?;

        // Prepend the cached frames along time. The depthwise conv
        // then sees [cache, x] with no internal padding.
        let extended = Tensor::cat(&[cache, &x], D::Minus1)?;

        // The next-step cache is the LAST `cache_frames` time steps
        // of the pre-conv extended sequence -- equivalently, the
        // last `cache_frames` columns of the current input alone if
        // `T >= cache_frames`, else a mix of cached and current.
        // `narrow` returns a non-contiguous view; force contiguous so
        // the next chunk's `Tensor::cat([cache, x], -1)` reads the
        // right bytes for the conv input prefix.
        let total_t = extended.dim(D::Minus1)?;
        let keep = kvc.min(total_t);
        let cache_next = extended
            .narrow(D::Minus1, total_t - keep, keep)?
            .contiguous()?;

        // Depthwise conv with no padding; output time = total_t - (k-1).
        let conv_out = self.depthwise_conv.forward(&extended)?;
        // Sanity: conv_out time dim should equal T (since we prepended
        // exactly k-1 frames).
        let out_t = conv_out.dim(D::Minus1)?;
        debug_assert_eq!(out_t, t, "depthwise conv output time mismatch");

        // LayerNorm operates on the channel dim. candle's LayerNorm
        // normalizes over the LAST axis, so we transpose to
        // (B, T, D), apply LN, transpose back.
        let conv_bt_d = conv_out.transpose(1, 2)?.contiguous()?;
        let normed = self.norm.forward(&conv_bt_d)?;
        let normed = normed.transpose(1, 2)?.contiguous()?;

        let activated = ops::silu(&normed)?;
        let projected = self.pointwise_conv2.forward(&activated)?;

        // Back to (B, T, D).
        let out = projected.transpose(1, 2)?.contiguous()?;

        // Ensure cache_next has full width even on the first
        // chunk where T < cache_frames -- pad with zeros on the
        // left so subsequent calls always see (B, D, cache_frames).
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

    /// Gated Linear Unit along the channel dimension. Input
    /// `(B, 2D, T)`, output `(B, D, T)`.
    fn glu(x: &Tensor) -> Result<Tensor> {
        let dim = x.dim(1)?;
        assert!(dim % 2 == 0, "GLU requires an even channel dim");
        let half = dim / 2;
        let a = x.narrow(1, 0, half)?;
        let b = x.narrow(1, half, half)?;
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
    pub fn new(cfg: &NemotronConfig, vb: VarBuilder) -> Result<Self> {
        let d = cfg.hidden_size;
        let ln_cfg = LayerNormConfig::default();
        Ok(Self {
            norm_feed_forward1: layer_norm(
                d,
                ln_cfg,
                vb.pp("norm_feed_forward1"),
            )?,
            feed_forward1: ConformerFeedForward::new(
                cfg,
                vb.pp("feed_forward1"),
            )?,
            norm_self_att: layer_norm(
                d,
                ln_cfg,
                vb.pp("norm_self_att"),
            )?,
            self_attn: RelPositionMultiHeadAttention::new(
                cfg,
                vb.pp("self_attn"),
            )?,
            norm_conv: layer_norm(d, ln_cfg, vb.pp("norm_conv"))?,
            conv: ConformerConvolution::new(cfg, vb.pp("conv"))?,
            norm_feed_forward2: layer_norm(
                d,
                ln_cfg,
                vb.pp("norm_feed_forward2"),
            )?,
            feed_forward2: ConformerFeedForward::new(
                cfg,
                vb.pp("feed_forward2"),
            )?,
            norm_out: layer_norm(d, ln_cfg, vb.pp("norm_out"))?,
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
