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

use candle_core::{Module, Result, Tensor};
use candle_nn::{conv2d, linear, Conv2d, Conv2dConfig, Linear, VarBuilder};

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
