/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Parakeet-CTC-110M model.

mod attention;
mod conformer;
mod decoder;
mod encoder;

/// Parakeet-CTC-110M architecture hyperparameters. Mirrors the fields of
/// the `config.json` that ships alongside `model.gguf`.
///
/// Each field corresponds to a value in NeMo's `model_config.yaml` for
/// the upstream `nvidia/parakeet-tdt_ctc-110m` checkpoint:
/// - `hidden_size`          = encoder.d_model
/// - `num_heads`            = encoder.n_heads
/// - `num_layers`           = encoder.n_layers
/// - `intermediate_size`    = encoder.ff_expansion_factor * encoder.d_model
/// - `conv_kernel_size`     = encoder.conv_kernel_size
/// - `num_mel_bins`         = preprocessor.features
/// - `vocab_size`           = aux_ctc.decoder.num_classes + 1 (+1 for the CTC
///   blank token appended by Brave's conversion script)
/// - `subsampling_channels` = encoder.subsampling_conv_channels
pub struct ParakeetConfig {
    pub hidden_size: usize,
    pub num_heads: usize,
    pub num_layers: usize,
    pub intermediate_size: usize,
    pub conv_kernel_size: usize,
    pub num_mel_bins: usize,
    pub vocab_size: usize,
    pub subsampling_channels: usize,
}
