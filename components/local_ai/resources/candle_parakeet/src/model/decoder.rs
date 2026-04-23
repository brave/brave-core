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

// CTC head for Parakeet-CTC-110M.

use candle_core::{Module, Result, Tensor};
use candle_nn::{conv1d, Conv1d, Conv1dConfig, VarBuilder};

use super::ParakeetConfig;

/// CTC output head. A single 1×1 Conv1d that maps each encoder
/// frame's hidden vector to per-token logits:
///
/// ```text
/// encoder_output:  (B, T/8, hidden_size)
///   transpose ->   (B, hidden_size, T/8)
///   Conv1d(hidden_size -> vocab_size, k=1)
///   transpose ->   (B, T/8, vocab_size)
/// logits
/// ```
///
/// For Parakeet-CTC-110M: `vocab_size = 1025` (1024 BPE subword
/// pieces + 1 CTC blank token at ID 1024).
///
/// Returns raw logits rather than `log_softmax(logits)`. Greedy
/// CTC decoding takes an argmax per frame, which is unchanged by
/// the monotonic log_softmax, so the extra op buys nothing. A
/// beam-search caller can apply log_softmax explicitly if needed.
///
/// A 1×1 Conv1d is mathematically identical to a `Linear` applied
/// at each time step — we use Conv1d here to keep tensor names
/// byte-identical with NeMo (`decoder_layers.0.weight` has shape
/// `(vocab_size, hidden_size, 1)` because NeMo wraps the Conv1d
/// in an `nn.Sequential` at index 0).
///
/// Tensor names under `ctc_decoder.`:
/// ```text
/// decoder_layers.0.weight     # Conv1d weight (vocab_size, hidden_size, 1)
/// decoder_layers.0.bias       # Conv1d bias  (vocab_size,)
/// ```
///
/// From NeMo v2.7.2 at
/// <https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/modules/conv_asr.py>
/// — implements `ConvASRDecoder`. Adapter hooks and the optional
/// `temperature` divisor are skipped (training / experimentation
/// knobs we don't need at inference).
pub struct ConvASRDecoder {
    conv: Conv1d,
}

impl ConvASRDecoder {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        let conv_cfg =
            Conv1dConfig { padding: 0, stride: 1, dilation: 1, groups: 1, cudnn_fwd_algo: None };
        Ok(Self {
            conv: conv1d(
                cfg.hidden_size,
                cfg.vocab_size,
                1,
                conv_cfg,
                vb.pp("decoder_layers").pp("0"),
            )?,
        })
    }

    /// `x` shape: `(B, T, hidden_size)`. Returns logits of shape
    /// `(B, T, vocab_size)`.
    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // Conv1d expects (B, C, T); encoder emits (B, T, C).
        let x = x.transpose(1, 2)?.contiguous()?;
        let x = self.conv.forward(&x)?;
        x.transpose(1, 2)?.contiguous()
    }
}
