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
 */

// RNN-T predictor (a.k.a. prediction network) for
// nemotron-speech-streaming. Token-by-token autoregressive LSTM.

use candle_core::{Device, IndexOp, Module, Result, Tensor, D};
use candle_nn::{ops, Embedding, VarBuilder};

use super::NemotronConfig;

/// PyTorch LSTM cell weights for one layer. We store the
/// concatenated (4*H, *) tensors verbatim from the state dict and
/// apply gates inline -- the cell is too simple to justify
/// `candle_nn::rnn::LSTM`, and the streaming path always processes
/// one token at a time so the matmul is small.
struct LstmLayer {
    w_ih: Tensor,
    w_hh: Tensor,
    b_ih: Tensor,
    b_hh: Tensor,
    hidden: usize,
}

impl LstmLayer {
    fn new(
        input_size: usize,
        hidden: usize,
        layer_idx: usize,
        vb: &VarBuilder,
    ) -> Result<Self> {
        let suffix = format!("_l{layer_idx}");
        Ok(Self {
            w_ih: vb.get(
                (4 * hidden, input_size),
                &format!("weight_ih{suffix}"),
            )?,
            w_hh: vb.get(
                (4 * hidden, hidden),
                &format!("weight_hh{suffix}"),
            )?,
            b_ih: vb.get(
                4 * hidden,
                &format!("bias_ih{suffix}"),
            )?,
            b_hh: vb.get(
                4 * hidden,
                &format!("bias_hh{suffix}"),
            )?,
            hidden,
        })
    }

    /// One LSTM step. Inputs are batch-size-1 vectors of shape
    /// `(1, input_size)` and `(1, hidden)`; output `(h_next, c_next)`
    /// each of shape `(1, hidden)`. PyTorch's gate layout is
    /// `[i, f, g, o]` along the gates axis, so we slice the
    /// `(1, 4H)` gate stack into those four blocks before applying
    /// activations.
    fn step(
        &self,
        x: &Tensor,
        h: &Tensor,
        c: &Tensor,
    ) -> Result<(Tensor, Tensor)> {
        // gates = x @ W_ih.T + b_ih + h @ W_hh.T + b_hh
        let gates = x
            .matmul(&self.w_ih.transpose(0, 1)?)?
            .broadcast_add(&self.b_ih)?
            .add(&h.matmul(&self.w_hh.transpose(0, 1)?)?)?
            .broadcast_add(&self.b_hh)?;

        let h_dim = self.hidden;
        let i = gates.narrow(D::Minus1, 0, h_dim)?;
        let f = gates.narrow(D::Minus1, h_dim, h_dim)?;
        let g = gates.narrow(D::Minus1, 2 * h_dim, h_dim)?;
        let o = gates.narrow(D::Minus1, 3 * h_dim, h_dim)?;

        let i = ops::sigmoid(&i)?;
        let f = ops::sigmoid(&f)?;
        let g = g.tanh()?;
        let o = ops::sigmoid(&o)?;

        let c_next = (f.mul(c)? + i.mul(&g)?)?;
        let h_next = o.mul(&c_next.tanh()?)?;
        Ok((h_next, c_next))
    }
}

/// (h, c) for every LSTM layer. The streaming decoder threads this
/// across `add_audio` calls so the predictor stays consistent across
/// chunks of audio.
pub struct PredictorState {
    pub h: Vec<Tensor>,
    pub c: Vec<Tensor>,
}

impl PredictorState {
    pub fn new(cfg: &NemotronConfig) -> Result<Self> {
        let device = Device::Cpu;
        let dtype = candle_core::DType::F32;
        let mut h = Vec::with_capacity(cfg.pred_rnn_layers);
        let mut c = Vec::with_capacity(cfg.pred_rnn_layers);
        for _ in 0..cfg.pred_rnn_layers {
            h.push(Tensor::zeros((1, cfg.pred_hidden), dtype, &device)?);
            c.push(Tensor::zeros((1, cfg.pred_hidden), dtype, &device)?);
        }
        Ok(Self { h, c })
    }
}

/// RNN-T predictor: embed previous token, run LSTM stack, return the
/// (1, pred_hidden) output that the joint network combines with each
/// encoder frame.
///
/// Tensor names under `decoder.prediction`:
/// ```text
/// embed.weight                       # (vocab_size, pred_hidden)
/// dec_rnn.lstm.weight_ih_l{0..1}     # (4*pred_hidden, input_dim)
/// dec_rnn.lstm.weight_hh_l{0..1}     # (4*pred_hidden, pred_hidden)
/// dec_rnn.lstm.bias_ih_l{0..1}       # (4*pred_hidden,)
/// dec_rnn.lstm.bias_hh_l{0..1}       # (4*pred_hidden,)
/// ```
pub struct RnntPredictor {
    embed: Embedding,
    layers: Vec<LstmLayer>,
    blank_id: u32,
    blank_as_pad: bool,
}

impl RnntPredictor {
    pub fn new(cfg: &NemotronConfig, vb: VarBuilder) -> Result<Self> {
        // NeMo's RNNTDecoder stores the embedding as
        // (vocab_size, pred_hidden). `vocab_size` here already
        // includes the blank id (config field is vocab + 1 from the
        // yaml; see derive_config).
        let embed_weight = vb
            .pp("embed")
            .get((cfg.vocab_size, cfg.pred_hidden), "weight")?;
        let embed =
            Embedding::new(embed_weight, cfg.pred_hidden);

        let vb_lstm = vb.pp("dec_rnn").pp("lstm");
        let mut layers = Vec::with_capacity(cfg.pred_rnn_layers);
        for i in 0..cfg.pred_rnn_layers {
            let input_size = if i == 0 {
                cfg.pred_hidden
            } else {
                cfg.pred_hidden
            };
            layers.push(LstmLayer::new(
                input_size,
                cfg.pred_hidden,
                i,
                &vb_lstm,
            )?);
        }

        Ok(Self {
            embed,
            layers,
            blank_id: cfg.blank_id as u32,
            blank_as_pad: cfg.blank_as_pad,
        })
    }

    /// Step the predictor by one token. Returns the top LSTM
    /// layer's hidden state `(1, pred_hidden)` and the *candidate*
    /// next PredictorState.
    ///
    /// The caller MUST commit the returned state only when a
    /// non-blank token is actually emitted -- that's NeMo's
    /// `RNNTGreedyDecoder` contract. Advancing the predictor on a
    /// blank prediction would scramble the autoregressive history
    /// and produce repeat-blank cascades. (This was the cause of
    /// chunks 1+ collapsing to all-blank in streaming.)
    ///
    /// When `prev_token == blank_id` and `blank_as_pad=true`, NeMo
    /// reuses the previous hidden state as g_u without running the
    /// LSTM; mirrored here.
    pub fn step(
        &self,
        prev_token: u32,
        state: &PredictorState,
    ) -> Result<(Tensor, PredictorState)> {
        let last_h = state.h.last().unwrap().clone();
        if self.blank_as_pad && prev_token == self.blank_id {
            return Ok((
                last_h,
                PredictorState {
                    h: state.h.clone(),
                    c: state.c.clone(),
                },
            ));
        }

        let idx = Tensor::from_vec(
            vec![prev_token as i64],
            (1,),
            last_h.device(),
        )?;
        let mut x = self.embed.forward(&idx)?;

        let mut updated_h = Vec::with_capacity(self.layers.len());
        let mut updated_c = Vec::with_capacity(self.layers.len());
        for (i, layer) in self.layers.iter().enumerate() {
            let (h_next, c_next) =
                layer.step(&x, &state.h[i], &state.c[i])?;
            x = h_next.clone();
            updated_h.push(h_next);
            updated_c.push(c_next);
        }

        Ok((x, PredictorState { h: updated_h, c: updated_c }))
    }
}

// The IndexOp import above is currently unused but kept available
// for when we add batched predictor evaluation later.
#[allow(dead_code)]
fn _ensure_index_op_imported(_t: &Tensor) {
    let _ = |t: &Tensor| -> Result<Tensor> { t.i(0) };
}
