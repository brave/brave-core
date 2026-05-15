/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Greedy streaming RNN-T decode for nemotron-speech-streaming.

use candle_core::{IndexOp, Result, Tensor, D};

use crate::model::{Nemotron, NemotronConfig, PredictorState};

/// State that persists across `add_audio` calls so the predictor
/// stays consistent chunk-to-chunk. Reset between utterances.
pub struct DecodeState {
    pub predictor: PredictorState,
    pub prev_token: u32,
}

impl DecodeState {
    pub fn new(cfg: &NemotronConfig) -> Result<Self> {
        Ok(Self {
            predictor: PredictorState::new(cfg)?,
            prev_token: cfg.blank_id as u32,
        })
    }
}

/// Greedy streaming step over one chunk's encoder output.
///
/// Mirrors NeMo v2.4.0rc0
/// `nemo/collections/asr/parts/submodules/rnnt_greedy_decoding.py
/// GreedyRNNTInfer._greedy_decode`:
///
///   for each encoder frame f:
///     not_blank = True
///     symbols_added = 0
///     while not_blank and symbols_added < max_symbols:
///       g, hidden_prime = pred_step(last_token, dec_state)
///       k = argmax(joint(f, g))
///       if k == blank: not_blank = False
///       else:          emit k; dec_state = hidden_prime;
///                      last_token = k
///       symbols_added += 1
///
/// CRITICAL: the predictor state advances ONLY on non-blank
/// emission. Caller commits the candidate state returned by
/// `predictor.step` only when the joint did not predict blank.
pub fn rnnt_greedy_step(
    model: &Nemotron,
    enc_out: &Tensor,
    state: &mut DecodeState,
    cfg: &NemotronConfig,
) -> Result<Vec<u32>> {
    let max_symbols_per_step: usize = 10;
    let blank_id = cfg.blank_id as u32;

    let t = enc_out.dim(1)?;
    let mut emitted = Vec::new();

    for frame_idx in 0..t {
        // f_t: (1, encoder_hidden)
        let f_t = enc_out.i((.., frame_idx, ..))?;
        let mut not_blank = true;
        let mut symbols_added = 0;
        while not_blank && symbols_added < max_symbols_per_step {
            let (g_u, candidate_state) = model
                .predictor
                .step(state.prev_token, &state.predictor)?;
            let logits = model.joint.forward(&f_t, &g_u)?;
            let pred = logits.squeeze(0)?.argmax(D::Minus1)?;
            let tok = pred.to_scalar::<u32>()?;
            if tok == blank_id {
                not_blank = false;
            } else {
                emitted.push(tok);
                state.predictor = candidate_state;
                state.prev_token = tok;
            }
            symbols_added += 1;
        }
    }

    Ok(emitted)
}
