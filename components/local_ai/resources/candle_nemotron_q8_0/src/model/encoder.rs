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

// Cache-aware FastConformer encoder for nemotron-speech-streaming.
//
// Top-level shape:
//   input mel  (B, T, num_mel_bins)
//     -> ConvSubsampling                       8x time downsample
//     -> RelPositionalEncoding (no xscaling)   (x, pos_emb)
//     -> num_layers x cache-aware ConformerLayer
//     -> encoder_output (B, T/8, hidden_size)

use candle_core::{Module, Result, Tensor};
use candle_nn::{Conv2d, Conv2dConfig};

use super::cache::EncoderCache;
use super::conformer::ConformerLayer;
use super::load::{take_conv2d, take_ql, QLinear, TensorMap};
use super::NemotronConfig;

/// 8x time-downsampling subsampling frontend (`dw_striding` variant).
///
/// Mirrors NeMo's `CausalDwStridingSubsampling` from
/// `subsampling.py`. Three stride-2 Conv2d layers downsample both
/// the time and freq axes. Each Conv2d uses NeMo's CausalConv2D
/// asymmetric padding (left = kernel_size - 1 = 2, right = stride
/// - 1 = 1) applied to BOTH spatial dims (PyTorch's Conv2d only
/// takes symmetric padding, so we F.pad manually and set
/// `Conv2dConfig.padding = 0`). For num_mel_bins=128 this drives
/// the freq output to 17 = ceil((128 + 3 - 3) / 2 / 2 / 2 + 1),
/// matching the saved `pre_encode.out.weight` `[1024, 4352]` shape.
pub struct ConvSubsampling {
    conv0: Conv2d,
    conv2_dw: Conv2d,
    conv3_pw: Conv2d,
    conv5_dw: Conv2d,
    conv6_pw: Conv2d,
    out: QLinear,
}

impl ConvSubsampling {
    pub fn load(
        cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        let c = cfg.subsampling_channels;

        // `padding = 0` everywhere -- we do asymmetric padding
        // manually before each stride-2 conv (see `pad_causal_2d`).
        let stride2 = Conv2dConfig {
            padding: 0,
            stride: 2,
            dilation: 1,
            groups: 1,
            cudnn_fwd_algo: None,
        };
        let stride2_dw = Conv2dConfig {
            padding: 0,
            stride: 2,
            dilation: 1,
            groups: c,
            cudnn_fwd_algo: None,
        };
        let pointwise = Conv2dConfig {
            padding: 0,
            stride: 1,
            dilation: 1,
            groups: 1,
            cudnn_fwd_algo: None,
        };

        let conv_prefix = format!("{prefix}.conv");
        Ok(Self {
            conv0: take_conv2d(
                t,
                &format!("{conv_prefix}.0"),
                stride2,
                true,
            )?,
            conv2_dw: take_conv2d(
                t,
                &format!("{conv_prefix}.2"),
                stride2_dw,
                true,
            )?,
            conv3_pw: take_conv2d(
                t,
                &format!("{conv_prefix}.3"),
                pointwise,
                true,
            )?,
            conv5_dw: take_conv2d(
                t,
                &format!("{conv_prefix}.5"),
                stride2_dw,
                true,
            )?,
            conv6_pw: take_conv2d(
                t,
                &format!("{conv_prefix}.6"),
                pointwise,
                true,
            )?,
            out: take_ql(t, &format!("{prefix}.out"))?,
        })
    }

    /// Forward pass. `x` shape: `(B, T, num_mel_bins)`. Returns
    /// `(B, T_sub, hidden_size)`.
    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // (B, T, F) -> (B, 1, T, F).
        let x = x.unsqueeze(1)?;

        // NeMo's CausalConv2D applies asymmetric pad (left=2,
        // right=1) on BOTH spatial dims of each stride-2 conv.
        let x = pad_causal_2d(&x)?;
        let x = self.conv0.forward(&x)?.relu()?;

        let x = pad_causal_2d(&x)?;
        let x = self.conv2_dw.forward(&x)?;
        let x = self.conv3_pw.forward(&x)?.relu()?;

        let x = pad_causal_2d(&x)?;
        let x = self.conv5_dw.forward(&x)?;
        let x = self.conv6_pw.forward(&x)?.relu()?;

        // (B, C, T_sub, F_sub) -> (B, T_sub, C, F_sub) ->
        // (B, T_sub, C * F_sub).
        let (b, c, t, f) = x.dims4()?;
        let x = x
            .transpose(1, 2)?
            .contiguous()?
            .reshape((b, t, c * f))?;

        self.out.forward(&x)
    }
}

/// NeMo's `CausalConv2D` asymmetric pad: (left = k-1 = 2, right =
/// stride-1 = 1) on each spatial dim. Implemented via
/// `pad_with_zeros` because candle's Conv2d only supports symmetric
/// padding.
fn pad_causal_2d(x: &Tensor) -> Result<Tensor> {
    // dim 2 = time axis (H), dim 3 = freq axis (W) of (B, C, T, F).
    let x = x.pad_with_zeros(2, 2, 1)?;
    x.pad_with_zeros(3, 2, 1)
}

/// Sinusoidal relative positional encoding (Transformer-XL style),
/// extended to cover both the cached frames and the new chunk's
/// frames.
///
/// nemotron-speech-streaming's encoder uses `xscaling=false`, so this
/// module does not scale `x` (the `xscale` factor is identity).
pub struct RelPositionalEncoding {
    d_model: usize,
}

impl RelPositionalEncoding {
    pub fn new(cfg: &NemotronConfig) -> Self {
        Self { d_model: cfg.hidden_size }
    }

    /// Build a positional-encoding table for cache-aware attention.
    ///
    /// Matches NeMo's RelPositionalEncoding.forward, which slices a
    /// precomputed PE table to length `2 * input_len - 1` where
    /// `input_len = t_q + cache_len = t_kv`. The slice covers
    /// relative distances `[-(t_kv - 1), +(t_kv - 1)]` with positive
    /// distances first. The `t_q` parameter is not used here -- it
    /// only matters for downstream rel_shift slicing.
    ///
    /// Reference: NeMo v2.4.0rc0
    /// nemo/collections/asr/parts/submodules/multi_head_attention.py
    /// `RelPositionalEncoding.forward`.
    pub fn build(
        &self,
        _t_q: usize,
        t_kv: usize,
        device: &candle_core::Device,
    ) -> Result<Tensor> {
        let d = self.d_model;
        let len = 2 * t_kv - 1;
        let log_base = (10000.0_f32).ln();
        let d_f = d as f32;

        let mut pe = vec![0.0_f32; len * d];
        // Positions run from +(t_kv - 1) down to -(t_kv - 1).
        for row in 0..len {
            let p = (t_kv as i64 - 1 - row as i64) as f32;
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

/// Top-level encoder. Threads the per-layer cache through every
/// ConformerLayer in order.
pub struct ConformerEncoder {
    pre_encode: ConvSubsampling,
    pos_enc: RelPositionalEncoding,
    layers: Vec<ConformerLayer>,
    subsampling_factor: usize,
}

impl ConformerEncoder {
    pub fn load(
        cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        let pre_encode = ConvSubsampling::load(
            cfg,
            t,
            &format!("{prefix}.pre_encode"),
        )?;
        let pos_enc = RelPositionalEncoding::new(cfg);
        let mut layers = Vec::with_capacity(cfg.num_layers);
        for i in 0..cfg.num_layers {
            layers.push(ConformerLayer::load(
                cfg,
                t,
                &format!("{prefix}.layers.{i}"),
            )?);
        }
        Ok(Self {
            pre_encode,
            pos_enc,
            layers,
            subsampling_factor: cfg.subsampling_factor,
        })
    }

    /// Streaming forward.
    ///
    /// `mel`: (B, T_new, num_mel_bins). `cache` is updated in place.
    /// Returns the new chunk's encoder output: (B, T_sub, hidden).
    ///
    /// Streaming flow matches NeMo's `ConformerEncoder.forward_internal`:
    ///   1. Prepend `pre_encode` mel cache (last 9 mel frames from the
    ///      previous chunk) to the current chunk's mel along the time
    ///      axis. On the first chunk the cache is zeros, which mimics
    ///      the zero-pad CausalConv2D would otherwise apply.
    ///   2. Update the mel cache with the LAST 9 frames of the new
    ///      chunk's mel input -- those are the frames the next chunk
    ///      will reuse.
    ///   3. Run ConvSubsampling on the extended mel.
    ///   4. Drop `drop_extra_pre_encoded` (= 9 // 8 = 1) frames from
    ///      the FRONT of the subsampling output -- they correspond to
    ///      the cached mel region whose conv outputs are needed only
    ///      to give the FIRST "real" frame correct left context.
    ///   5. Build pos_emb covering the extended kv length (cache +
    ///      current).
    ///   6. Thread per-layer K/V and conv-state caches through every
    ///      ConformerLayer in order.
    pub fn forward(
        &self,
        mel: &Tensor,
        cache: &mut EncoderCache,
    ) -> Result<Tensor> {
        let device = mel.device().clone();
        let pre_encode_frames = EncoderCache::pre_encode_frames();
        let drop_extra = pre_encode_frames / self.subsampling_factor;

        // Step 1: prepend mel cache. mel is (B, T_new, F); the cache
        // is stored as (B, F, 9) for fast time-axis concat -- bring
        // both to (B, T, F) for the concat along dim=1.
        let cache_btf =
            cache.pre_encode.transpose(1, 2)?.contiguous()?;
        let extended_mel = Tensor::cat(&[&cache_btf, mel], 1)?;

        // Step 2: refresh the mel cache with the last 9 frames of
        // the NEW chunk's mel (not the extended one -- only the new
        // frames carry forward).
        let t_new = mel.dim(1)?;
        let keep = pre_encode_frames.min(t_new);
        let mel_tail =
            mel.narrow(1, t_new - keep, keep)?.contiguous()?;
        let mel_tail = if keep < pre_encode_frames {
            // Left-pad with zeros when the new chunk itself has
            // fewer than 9 frames (only happens on very short final
            // chunks, never in steady-state streaming).
            let (b, _t, f) = mel_tail.dims3()?;
            let pad = Tensor::zeros(
                (b, pre_encode_frames - keep, f),
                mel_tail.dtype(),
                mel_tail.device(),
            )?;
            Tensor::cat(&[&pad, &mel_tail], 1)?
        } else {
            mel_tail
        };
        cache.pre_encode = mel_tail.transpose(1, 2)?.contiguous()?;

        // Step 3: ConvSubsampling on the extended mel.
        let x = self.pre_encode.forward(&extended_mel)?;

        // Step 4: drop the `drop_extra` frames at the front. NeMo
        // does this only when cache_last_channel was non-empty
        // (i.e. on chunks AFTER the first). We do it from chunk 0
        // onwards too -- the dropped frames at chunk 0 come from
        // the zero-filled mel cache and carry no signal.
        let x = if drop_extra > 0 {
            let t_sub = x.dim(1)?;
            x.narrow(1, drop_extra, t_sub - drop_extra)?
                .contiguous()?
        } else {
            x
        };
        let mut x = x;

        // Step 5: build pos_emb spanning cache + current.
        let t_q = x.dim(1)?;
        let cache_len = cache.last_channel[0].dim(1)?;
        let t_kv = cache_len + t_q;
        let pos_emb = self.pos_enc.build(t_q, t_kv, &device)?;

        // Step 6: thread per-layer caches.
        for (i, layer) in self.layers.iter().enumerate() {
            let (new_x, new_chan, new_time) = layer.forward(
                &x,
                &pos_emb,
                &cache.last_channel[i],
                &cache.last_time[i],
            )?;
            x = new_x;
            cache.last_channel[i] = new_chan;
            cache.last_time[i] = new_time;
        }

        cache.has_processed_chunk = true;
        Ok(x)
    }
}
