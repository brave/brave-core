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

// RNN-T joint network for nemotron-speech-streaming. Combines one
// encoder frame `f_t` (1, encoder_hidden) with one predictor output
// `g_u` (1, pred_hidden), producing logits over `vocab_size + 1`
// tokens (blank is the +1).
//
// Joint structure per NeMo's `RNNTJoint` with `jointnet.activation
// = relu` and `joint_hidden = 640`:
//
//   z = relu(enc(f_t) + pred(g_u))             # (1, joint_hidden)
//   logits = joint_net.2(z)                    # (1, vocab_size + 1)
//
// joint_net.0 in NeMo is the activation (relu); joint_net.1 is
// dropout. Only joint_net.2 has parameters and shows up in the
// state dict.

use candle_core::{Result, Tensor};

use super::load::{take_ql, QLinear, TensorMap};
use super::NemotronConfig;

pub struct RnntJoint {
    enc: QLinear,
    pred: QLinear,
    joint_net_2: QLinear,
}

impl RnntJoint {
    pub fn load(
        _cfg: &NemotronConfig,
        t: &mut TensorMap,
        prefix: &str,
    ) -> Result<Self> {
        // The yaml config also includes the auxiliary `relu` /
        // `dropout` sub-modules under joint_net.{0,1}, but those have
        // no parameters. Only joint_net.2 is in the state dict.
        Ok(Self {
            enc: take_ql(t, &format!("{prefix}.enc"))?,
            pred: take_ql(t, &format!("{prefix}.pred"))?,
            joint_net_2: take_ql(
                t,
                &format!("{prefix}.joint_net.2"),
            )?,
        })
    }

    /// Forward. `f_t` shape `(1, encoder_hidden)`, `g_u` shape
    /// `(1, pred_hidden)`. Returns logits `(1, vocab_size)` (where
    /// vocab_size in config already includes the +1 blank).
    pub fn forward(
        &self,
        f_t: &Tensor,
        g_u: &Tensor,
    ) -> Result<Tensor> {
        let a = self.enc.forward(f_t)?;
        let b = self.pred.forward(g_u)?;
        let z = (a + b)?.relu()?;
        self.joint_net_2.forward(&z)
    }
}
