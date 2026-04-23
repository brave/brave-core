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
// Parakeet-CTC-110M. See per-struct doc for NeMo source.

use candle_core::{Module, ModuleT, Result, Tensor};
use candle_nn::{
    batch_norm, conv1d, layer_norm, linear, ops, BatchNorm, BatchNormConfig, Conv1d, Conv1dConfig,
    LayerNorm, LayerNormConfig, Linear, VarBuilder,
};

use super::attention::RelPositionMultiHeadAttention;
use super::ParakeetConfig;

/// Position-wise feed-forward: Linear(D -> D_ff) -> Swish ->
/// Linear(D_ff -> D). Applied independently at each time frame.
/// Parakeet-CTC-110M uses `D=512`, `D_ff=2048` (4× expansion).
///
/// Inputs and outputs are `(B, T, D)`.
///
/// Tensor names under each `encoder.layers.N.feed_forward{1,2}`:
/// ```text
/// linear1.{weight,bias}
/// linear2.{weight,bias}
/// ```
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/submodules/conformer_modules.py>
/// — implements `ConformerFeedForward`. Dropout is omitted
/// (no-op at inference).
pub struct ConformerFeedForward {
    linear1: Linear,
    linear2: Linear,
}

impl ConformerFeedForward {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        Ok(Self {
            linear1: linear(cfg.hidden_size, cfg.intermediate_size, vb.pp("linear1"))?,
            linear2: linear(cfg.intermediate_size, cfg.hidden_size, vb.pp("linear2"))?,
        })
    }

    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = self.linear1.forward(x)?;
        let x = ops::silu(&x)?;
        self.linear2.forward(&x)
    }
}

/// Conformer convolution module: captures local acoustic structure
/// alongside the self-attention that looks globally. Full sub-block
/// layout (all Conv1d operate over the time axis):
///
/// ```text
/// x: (B, T, D)
///   transpose -> (B, D, T)
///   pointwise_conv1: Conv1d(D -> 2D, k=1)
///   GLU: split the 2D channels into first D and second D; output
///        is first * sigmoid(second), shape (B, D, T)
///   depthwise_conv: Conv1d(D -> D, k=conv_kernel_size,
///                          groups=D, pad=(k-1)/2)
///   BatchNorm1d(D) (inference path: fixed per-channel affine)
///   Swish
///   pointwise_conv2: Conv1d(D -> D, k=1)
///   transpose -> (B, T, D)
/// ```
///
/// For Parakeet-CTC-110M: `D=512`, `k=conv_kernel_size=9`.
///
/// Tensor names under each `encoder.layers.N.conv`:
/// ```text
/// pointwise_conv1.{weight,bias}
/// depthwise_conv.{weight,bias}
/// batch_norm.{weight,bias}             # gamma, beta
/// batch_norm.{running_mean,running_var}
/// pointwise_conv2.{weight,bias}
/// ```
///
/// # Port notes
///
/// - **CausalConv1D -> plain Conv1d.** NeMo parameterizes the depthwise conv
///   via `CausalConv1D`, but Parakeet-CTC-110M uses a symmetric kernel (same
///   padding on both sides), so the causal wrapper degenerates to a regular
///   convolution. We instantiate Conv1d directly.
/// - **No cache / no pad mask.** Brave's candle_parakeet runs batch size 1 with
///   chunk-based (not per-frame incremental) streaming, so there is no cache to
///   update across calls and no padded positions to mask.
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/submodules/conformer_modules.py>
/// — implements `ConformerConvolution` with `pointwise_activation=
/// 'glu_'` and `norm_type='batch_norm'` (the variant Parakeet-CTC-
/// 110M's `model_config.yaml` selects).
pub struct ConformerConvolution {
    pointwise_conv1: Conv1d,
    depthwise_conv: Conv1d,
    batch_norm: BatchNorm,
    pointwise_conv2: Conv1d,
}

impl ConformerConvolution {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let d = cfg.hidden_size;
        let k = cfg.conv_kernel_size;
        assert!((k - 1) % 2 == 0, "conv_kernel_size must be odd");
        let pad = (k - 1) / 2;

        let pointwise =
            Conv1dConfig { padding: 0, stride: 1, dilation: 1, groups: 1, cudnn_fwd_algo: None };
        let depthwise =
            Conv1dConfig { padding: pad, stride: 1, dilation: 1, groups: d, cudnn_fwd_algo: None };

        Ok(Self {
            pointwise_conv1: conv1d(d, 2 * d, 1, pointwise, vb.pp("pointwise_conv1"))?,
            depthwise_conv: conv1d(d, d, k, depthwise, vb.pp("depthwise_conv"))?,
            batch_norm: batch_norm(d, BatchNormConfig::default(), vb.pp("batch_norm"))?,
            pointwise_conv2: conv1d(d, d, 1, pointwise, vb.pp("pointwise_conv2"))?,
        })
    }

    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // (B, T, D) -> (B, D, T) for Conv1d.
        let x = x.transpose(1, 2)?.contiguous()?;

        // Expand channels to 2D, gate via GLU to get back to D.
        let x = self.pointwise_conv1.forward(&x)?;
        let x = Self::glu(&x)?;

        // Depthwise conv, BatchNorm, Swish, pointwise conv.
        let x = self.depthwise_conv.forward(&x)?;
        let x = self.batch_norm.forward_t(&x, false)?;
        let x = ops::silu(&x)?;
        let x = self.pointwise_conv2.forward(&x)?;

        // Back to (B, T, D).
        x.transpose(1, 2)?.contiguous()
    }

    /// Gated Linear Unit along the channel dimension: split the
    /// channel axis into two equal halves, return first half times
    /// sigmoid of second half. Input `(B, 2D, T)`, output `(B, D, T)`.
    fn glu(x: &Tensor) -> Result<Tensor> {
        let dim = x.dim(1)?;
        assert!(dim % 2 == 0, "GLU requires an even channel dim");
        let half = dim / 2;
        let a = x.narrow(1, 0, half)?;
        let b = x.narrow(1, half, half)?;
        a * ops::sigmoid(&b)?
    }
}

/// One Conformer encoder layer. Macaron-style feed-forward
/// sandwich around self-attention and convolution, with pre-norm
/// LayerNorm on every residual branch and a final per-layer
/// LayerNorm:
///
/// ```text
/// x = x + 0.5 * FFN1(LN(x))
/// x = x + MHA(LN(x), pos_emb)
/// x = x + Conv(LN(x))
/// x = x + 0.5 * FFN2(LN(x))
/// x = LN(x)
/// ```
///
/// Parakeet-CTC-110M stacks 17 of these after `ConvSubsampling` +
/// `RelPositionalEncoding` to form the encoder body.
///
/// Tensor names under each `encoder.layers.N`:
/// ```text
/// norm_feed_forward1.{weight,bias}
/// feed_forward1.*                          # ConformerFeedForward
/// norm_self_att.{weight,bias}
/// self_attn.*                              # see attention.rs
/// norm_conv.{weight,bias}
/// conv.*                                   # ConformerConvolution
/// norm_feed_forward2.{weight,bias}
/// feed_forward2.*
/// norm_out.{weight,bias}
/// ```
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/submodules/conformer_modules.py>
/// — implements `ConformerLayer` in the `rel_pos` configuration.
/// Dropout, adapters, access hooks, and caches are omitted (see
/// `ConformerFeedForward` / `ConformerConvolution` port notes).
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
}

impl ConformerLayer {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let d = cfg.hidden_size;
        let ln_cfg = LayerNormConfig::default();
        Ok(Self {
            norm_feed_forward1: layer_norm(d, ln_cfg, vb.pp("norm_feed_forward1"))?,
            feed_forward1: ConformerFeedForward::new(cfg, vb.pp("feed_forward1"))?,
            norm_self_att: layer_norm(d, ln_cfg, vb.pp("norm_self_att"))?,
            self_attn: RelPositionMultiHeadAttention::new(cfg, vb.pp("self_attn"))?,
            norm_conv: layer_norm(d, ln_cfg, vb.pp("norm_conv"))?,
            conv: ConformerConvolution::new(cfg, vb.pp("conv"))?,
            norm_feed_forward2: layer_norm(d, ln_cfg, vb.pp("norm_feed_forward2"))?,
            feed_forward2: ConformerFeedForward::new(cfg, vb.pp("feed_forward2"))?,
            norm_out: layer_norm(d, ln_cfg, vb.pp("norm_out"))?,
        })
    }

    pub fn forward(&self, x: &Tensor, pos_emb: &Tensor) -> Result<Tensor> {
        // Half-step FFN1.
        let ff1 = self.feed_forward1.forward(&self.norm_feed_forward1.forward(x)?)?;
        let x = (x + ff1.affine(0.5, 0.0)?)?;

        // Self-attention.
        let att = self.self_attn.forward(&self.norm_self_att.forward(&x)?, pos_emb)?;
        let x = (&x + att)?;

        // Convolution.
        let cv = self.conv.forward(&self.norm_conv.forward(&x)?)?;
        let x = (&x + cv)?;

        // Half-step FFN2.
        let ff2 = self.feed_forward2.forward(&self.norm_feed_forward2.forward(&x)?)?;
        let x = (&x + ff2.affine(0.5, 0.0)?)?;

        self.norm_out.forward(&x)
    }
}
