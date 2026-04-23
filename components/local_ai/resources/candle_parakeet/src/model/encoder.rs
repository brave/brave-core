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

// FastConformer encoder components for Parakeet-CTC-110M. Each struct
// below cites its NeMo source in its own doc comment.

use candle_core::{Device, Module, Result, Tensor};
use candle_nn::{conv2d, linear, Conv2d, Conv2dConfig, Linear, VarBuilder};

use super::conformer::ConformerLayer;
use super::ParakeetConfig;

/// Convolutional subsampling frontend. `dw_striding` variant with
/// `subsampling_factor=8`: 8x time downsample and project to the encoder
/// hidden dimension.
///
/// Input:  `(B, T, num_mel_bins)` — time-major mel features
/// Output: `(B, T/8, hidden_size)` — subsampled hidden states
///
/// Layer sequence (mirrors NeMo's `MaskedConvSequential` for
/// `dw_striding`, `subsampling_factor=8`; indices correspond to positions
/// in the saved state dict at `encoder.pre_encode.conv.{i}`):
/// ```text
/// conv.0     Conv2d(1 -> C, k=3, s=2, p=1)
/// [ReLU]
/// conv.2     Conv2d(C -> C, k=3, s=2, p=1, groups=C)   depthwise
/// conv.3     Conv2d(C -> C, k=1)                         pointwise
/// [ReLU]
/// conv.5     Conv2d(C -> C, k=3, s=2, p=1, groups=C)   depthwise
/// conv.6     Conv2d(C -> C, k=1)                         pointwise
/// [ReLU]
/// out        Linear(C * (num_mel_bins / 8) -> hidden_size)
/// ```
/// where `C = cfg.subsampling_channels`.
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/submodules/subsampling.py>
/// — implements only the `dw_striding` variant of NeMo's `ConvSubsampling`
/// class (the variant Parakeet-CTC-110M's `model_config.yaml` selects).
pub struct ConvSubsampling {
    conv0: Conv2d,
    conv2_dw: Conv2d,
    conv3_pw: Conv2d,
    conv5_dw: Conv2d,
    conv6_pw: Conv2d,
    out: Linear,
}

impl ConvSubsampling {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let c = cfg.subsampling_channels;

        let stride2 =
            Conv2dConfig { padding: 1, stride: 2, dilation: 1, groups: 1, cudnn_fwd_algo: None };
        let stride2_dw =
            Conv2dConfig { padding: 1, stride: 2, dilation: 1, groups: c, cudnn_fwd_algo: None };
        let pointwise =
            Conv2dConfig { padding: 0, stride: 1, dilation: 1, groups: 1, cudnn_fwd_algo: None };

        let vb_conv = vb.pp("conv");
        Ok(Self {
            conv0: conv2d(1, c, 3, stride2, vb_conv.pp("0"))?,
            conv2_dw: conv2d(c, c, 3, stride2_dw, vb_conv.pp("2"))?,
            conv3_pw: conv2d(c, c, 1, pointwise, vb_conv.pp("3"))?,
            conv5_dw: conv2d(c, c, 3, stride2_dw, vb_conv.pp("5"))?,
            conv6_pw: conv2d(c, c, 1, pointwise, vb_conv.pp("6"))?,
            // After 3x stride-2 conv, the num_mel_bins axis is 8x smaller
            // (80 -> 10 for the shipped config).
            out: linear(c * (cfg.num_mel_bins / 8), cfg.hidden_size, vb.pp("out"))?,
        })
    }

    /// Forward pass. `x` shape: `(B, T, num_mel_bins)`.
    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // Add a singleton channel axis for Conv2d: (B, 1, T, F).
        let x = x.unsqueeze(1)?;

        let x = self.conv0.forward(&x)?.relu()?;
        let x = self.conv2_dw.forward(&x)?;
        let x = self.conv3_pw.forward(&x)?.relu()?;
        let x = self.conv5_dw.forward(&x)?;
        let x = self.conv6_pw.forward(&x)?.relu()?;

        // (B, C, T/8, F/8) -> (B, T/8, C, F/8) -> (B, T/8, C * F/8)
        let (b, c, t, f) = x.dims4()?;
        let x = x.transpose(1, 2)?.contiguous()?.reshape((b, t, c * f))?;

        self.out.forward(&x)
    }
}

/// Sinusoidal relative positional encoding (Transformer-XL style).
///
/// Emits one sinusoidal vector per relative distance in `[-(T-1), T-1]`,
/// consumed by each Conformer layer's self-attention alongside the
/// usual content Q/K/V. Also applies the `xscaling` input scale
/// (`x * sqrt(hidden_size)`) that the NeMo Conformer encoder routes
/// through this module when `xscaling=true` — which is the case for
/// Parakeet-CTC-110M.
///
/// Input:  `x: (B, T, hidden_size)` — subsampled encoder states
/// Output: `(x_scaled, pos_emb)`
///   - `x_scaled: (B, T, hidden_size)` — `x * sqrt(hidden_size)`
///   - `pos_emb:  (1, 2T-1, hidden_size)` — sinusoidal vectors for relative
///     positions `T-1, T-2, ..., 0, ..., -(T-1)` in that order
///
/// Has no weights to load from the GGUF: the sinusoidal pattern is
/// fixed math, not trained. `new` takes only `&ParakeetConfig` — no
/// `VarBuilder`. This matches NeMo, which registers `pe` as a
/// non-persistent buffer so PyTorch excludes it from `state_dict()`.
/// Unlike NeMo, which caches `pe` on a buffer and extends it on
/// demand, we recompute per forward: Parakeet frame counts are small
/// (25 frames for 2 s audio) so the sin/cos cost is negligible, and
/// avoiding the cache keeps this module stateless.
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/submodules/multi_head_attention.py>
/// — implements `RelPositionalEncoding`. NeMo's two dropout calls on
/// `x` and `pos_emb` are omitted: dropout is a no-op at inference
/// (`model.eval()`), which is all we ever run.
pub struct RelPositionalEncoding {
    d_model: usize,
    xscale: f64,
}

impl RelPositionalEncoding {
    pub fn new(cfg: &ParakeetConfig) -> Self {
        Self { d_model: cfg.hidden_size, xscale: (cfg.hidden_size as f64).sqrt() }
    }

    /// Forward pass. `x` shape: `(B, T, hidden_size)`. Returns
    /// `(x * sqrt(hidden_size), pos_emb)` with `pos_emb` shape
    /// `(1, 2T-1, hidden_size)`.
    pub fn forward(&self, x: &Tensor) -> Result<(Tensor, Tensor)> {
        let t = x.dim(1)?;
        let pos_emb = self.build_pos_emb(t, x.device())?;
        let x_scaled = x.affine(self.xscale, 0.0)?;
        Ok((x_scaled, pos_emb))
    }

    fn build_pos_emb(&self, t: usize, device: &Device) -> Result<Tensor> {
        // Positions run from +(T-1) down to -(T-1), inclusive. Even
        // dims carry sin, odd dims carry cos — standard Transformer
        // formulation with base 10000.
        let d = self.d_model;
        let len = 2 * t - 1;
        let log_base = (10000.0_f32).ln();
        let d_f = d as f32;

        let mut pe = vec![0.0_f32; len * d];
        for row in 0..len {
            let p = (t as i64 - 1 - row as i64) as f32;
            for i in 0..(d / 2) {
                let div = (-(2.0 * i as f32) * log_base / d_f).exp();
                let arg = p * div;
                pe[row * d + 2 * i] = arg.sin();
                pe[row * d + 2 * i + 1] = arg.cos();
            }
        }
        Tensor::from_vec(pe, (1, len, d), device)
    }
}

/// Top-level Parakeet-CTC-110M encoder: runs log-mel features
/// through ConvSubsampling, RelPositionalEncoding, then a stack
/// of `num_layers` ConformerLayer instances, all sharing the same
/// pos_emb produced by the positional-encoding module.
///
/// ```text
/// input mel  (B, T, num_mel_bins)  @ 10 ms/frame
///   -> ConvSubsampling               8× time downsample
///   -> RelPositionalEncoding         (x_scaled, pos_emb)
///   -> num_layers × ConformerLayer   each reads shared pos_emb
///   -> encoder_output (B, T/8, hidden_size)  @ 80 ms/frame
/// ```
///
/// Tensor name prefixes under `encoder.`:
/// ```text
/// pre_encode.*                 ConvSubsampling
/// layers.{0..num_layers-1}.*   ConformerLayer instances
/// ```
/// `pos_enc` has no loaded weights (sinusoidal is fixed math),
/// so no tensors live under that prefix.
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/modules/conformer_encoder.py>
/// — implements `ConformerEncoder` inference with
/// `self_attention_model='rel_pos'`. The `feat_out` projection,
/// streaming caches, pad mask, stochastic depth, adapters, and
/// access hooks are omitted (Parakeet-CTC-110M doesn't use
/// `feat_out`, and the rest aren't needed for chunk-based
/// batch-size-1 inference).
pub struct ConformerEncoder {
    pre_encode: ConvSubsampling,
    pos_enc: RelPositionalEncoding,
    layers: Vec<ConformerLayer>,
}

impl ConformerEncoder {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let pre_encode = ConvSubsampling::new(cfg, vb.pp("pre_encode"))?;
        let pos_enc = RelPositionalEncoding::new(cfg);
        let vb_layers = vb.pp("layers");
        let mut layers = Vec::with_capacity(cfg.num_layers);
        for i in 0..cfg.num_layers {
            layers.push(ConformerLayer::new(cfg, vb_layers.pp(i.to_string()))?);
        }
        Ok(Self { pre_encode, pos_enc, layers })
    }

    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = self.pre_encode.forward(x)?;
        let (mut x, pos_emb) = self.pos_enc.forward(&x)?;
        for layer in &self.layers {
            x = layer.forward(&x, &pos_emb)?;
        }
        Ok(x)
    }
}
