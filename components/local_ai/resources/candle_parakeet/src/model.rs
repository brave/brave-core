/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Parakeet-CTC-110M model.

mod attention;
mod builder;
mod conformer;
mod decoder;
mod encoder;

use candle_core::{Result, Tensor};
use candle_nn::VarBuilder;
use serde::Deserialize;

pub use builder::ModelBuilder;
use decoder::ConvASRDecoder;
use encoder::ConformerEncoder;

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
///
/// `tensor_data_offset` is Brave-specific: the byte offset in
/// `model.gguf` where the tensor-data blob starts. The JS loader
/// splits the file at this offset so the header can be parsed
/// separately from the streamed tensor-data chunks.
#[derive(Deserialize)]
pub struct ParakeetConfig {
    pub hidden_size: usize,
    pub num_heads: usize,
    pub num_layers: usize,
    pub intermediate_size: usize,
    pub conv_kernel_size: usize,
    pub num_mel_bins: usize,
    pub vocab_size: usize,
    pub subsampling_channels: usize,
    pub tensor_data_offset: usize,
}

/// Parakeet-CTC-110M model: the `ConformerEncoder` that reads log-mel
/// features and the `ConvASRDecoder` CTC head that projects encoder
/// hidden states to per-frame token logits.
///
/// ```text
/// input mel:  (B, T, num_mel_bins)  10 ms per frame
///   -> ConformerEncoder
///   -> (B, T/8, hidden_size)        80 ms per frame
///   -> ConvASRDecoder
///   -> logits (B, T/8, vocab_size)
/// ```
///
/// Top-level tensor prefixes loaded from the supplied VarBuilder:
///
/// ```text
/// encoder.pre_encode.*                 ConvSubsampling weights
/// encoder.layers.{0..num_layers-1}.*   num_layers × ConformerLayer
/// ctc_decoder.decoder_layers.0.*       1×1 Conv1d CTC head
/// ```
///
/// Module layout:
///
/// - `encoder` — `ConvSubsampling` + `RelPositionalEncoding` +
///   `ConformerEncoder` (the top-level encoder composition).
/// - `conformer` — one `ConformerLayer` and its two sub-blocks
///   `ConformerFeedForward` + `ConformerConvolution`.
/// - `attention` — `RelPositionMultiHeadAttention` and the Transformer-XL
///   `rel_shift` index trick.
/// - `decoder` — `ConvASRDecoder`, the 1×1 Conv1d CTC head.
///
/// Upstream reference: NeMo v2.7.2
/// (<https://github.com/NVIDIA-NeMo/NeMo/tree/v2.7.2/nemo/collections/asr>),
/// Apache-2.0. Per-struct doc comments cite the exact Python source
/// file they port from.
pub struct Parakeet {
    encoder: ConformerEncoder,
    decoder: ConvASRDecoder,
}

impl Parakeet {
    pub fn new(cfg: &ParakeetConfig, vb: VarBuilder) -> Result<Self> {
        Ok(Self {
            encoder: ConformerEncoder::new(cfg, vb.pp("encoder"))?,
            decoder: ConvASRDecoder::new(cfg, vb.pp("ctc_decoder"))?,
        })
    }

    /// Forward pass. `mel` shape: `(B, T, num_mel_bins)`. Returns
    /// logits of shape `(B, T/8, vocab_size)`.
    pub fn forward(&self, mel: &Tensor) -> Result<Tensor> {
        let encoded = self.encoder.forward(mel)?;
        self.decoder.forward(&encoded)
    }
}
