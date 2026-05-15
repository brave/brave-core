/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Cache state for nemotron's cache-aware streaming encoder.
//
// NeMo's `ConformerEncoder.forward_for_export` threads two cache
// tensors through every layer:
//   - cache_last_channel: the post-norm pre-attention input X kept
//     for self-attention; shape (num_layers, batch, left_ctx, D).
//     On each chunk, the cache is prepended to the new X along the
//     time axis before K/V projection, then the LAST left_ctx frames
//     of the extended sequence are saved back as the next cache.
//   - cache_last_time: the depthwise-conv state, shape
//     (num_layers, batch, D, conv_ctx). conv_ctx = kernel_size - 1.
//     Same prepend/slice idea, but along time = -1 axis (channel-
//     first).
//
// We hold them as per-layer Vec<Tensor> rather than a 4D tensor so
// each layer's cache can be updated in place without re-stacking.

use candle_core::{Device, Result, Tensor};

use super::NemotronConfig;

/// Pre-encode mel cache size, in mel frames. NeMo's
/// ConvSubsampling sets `_max_cache_len = subsampling_factor + 1
/// = 9` -- the number of prior-chunk mel frames the next chunk
/// needs to prepend so the subsampling conv stack has enough
/// left context.
const PRE_ENCODE_CACHE_FRAMES: usize = 9;

pub struct EncoderCache {
    /// Self-attention input cache, one tensor per encoder layer.
    /// Each is shape (batch, time, hidden_size) and grows from 0
    /// to `left_context_frames` as chunks accumulate.
    pub last_channel: Vec<Tensor>,
    /// Depthwise-conv state cache, one tensor per encoder layer.
    /// Each is shape (batch, hidden_size, conv_cache_frames).
    pub last_time: Vec<Tensor>,
    /// Pre-encode mel cache: the last `PRE_ENCODE_CACHE_FRAMES`
    /// mel frames from the previous chunk's input, prepended to
    /// the current chunk's mel before ConvSubsampling.
    /// Shape: (batch, n_mels, PRE_ENCODE_CACHE_FRAMES).
    pub pre_encode: Tensor,
    /// True until the first chunk has been processed. Determines
    /// whether `drop_extra_pre_encoded` frames need to be dropped
    /// from the encoder output (NeMo only drops them when cache
    /// was non-empty before the call).
    pub has_processed_chunk: bool,
}

impl EncoderCache {
    /// Initialize all per-layer caches with zero-width tensors. The
    /// first chunk's forward sees an empty cache and produces the
    /// initial state itself.
    pub fn new(cfg: &NemotronConfig, batch: usize) -> Result<Self> {
        let device = Device::Cpu;
        let mut last_channel = Vec::with_capacity(cfg.num_layers);
        let mut last_time = Vec::with_capacity(cfg.num_layers);
        for _ in 0..cfg.num_layers {
            last_channel.push(Tensor::zeros(
                (batch, 0, cfg.hidden_size),
                candle_core::DType::F32,
                &device,
            )?);
            last_time.push(Tensor::zeros(
                (batch, cfg.hidden_size, cfg.conv_cache_frames),
                candle_core::DType::F32,
                &device,
            )?);
        }
        let pre_encode = Tensor::zeros(
            (batch, cfg.num_mel_bins, PRE_ENCODE_CACHE_FRAMES),
            candle_core::DType::F32,
            &device,
        )?;
        Ok(Self {
            last_channel,
            last_time,
            pre_encode,
            has_processed_chunk: false,
        })
    }

    /// Reset the cache to its zero state (called from
    /// `NemotronTranscriber::reset` between utterances).
    pub fn clear(&mut self, cfg: &NemotronConfig) -> Result<()> {
        let device = Device::Cpu;
        for slot in self.last_channel.iter_mut() {
            *slot = Tensor::zeros(
                (1, 0, cfg.hidden_size),
                candle_core::DType::F32,
                &device,
            )?;
        }
        for slot in self.last_time.iter_mut() {
            *slot = Tensor::zeros(
                (1, cfg.hidden_size, cfg.conv_cache_frames),
                candle_core::DType::F32,
                &device,
            )?;
        }
        self.pre_encode = Tensor::zeros(
            (1, cfg.num_mel_bins, PRE_ENCODE_CACHE_FRAMES),
            candle_core::DType::F32,
            &device,
        )?;
        self.has_processed_chunk = false;
        Ok(())
    }

    pub fn pre_encode_frames() -> usize {
        PRE_ENCODE_CACHE_FRAMES
    }
}
