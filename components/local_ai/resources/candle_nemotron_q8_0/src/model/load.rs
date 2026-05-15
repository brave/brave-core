/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Loader-side helpers shared by every sub-module of the nemotron
// model. They consume from a `HashMap<String, QTensor>` populated by
// the chunked GGUF reader and either:
//   - keep weights quantized as `QTensor` and wrap in `QLinear` so
//     matmul dispatches through `QMatMul::from_qtensor` (the bulk of
//     RAM savings comes from this), or
//   - dequantize to F32 once at load time for small / shape-sensitive
//     tensors (LayerNorm, biases, depthwise conv, attention pos bias,
//     conv-subsampling Conv2d weights, embedding table).
//
// For F32 GGUF inputs the same code paths still work: `QTensor`
// happily wraps `GgmlDType::F32` payloads and `QMatMul` falls through
// to plain F32 matmul.

use std::collections::HashMap;

use candle_core::quantized::{QMatMul, QTensor};
use candle_core::{Device, Module, Result, Tensor};
use candle_nn::{
    Conv1d, Conv1dConfig, Conv2d, Conv2dConfig, Embedding, LayerNorm,
    LayerNormConfig,
};

pub type TensorMap = HashMap<String, QTensor>;

// ---- Shape adjustment --------------------------------------------

/// Drop a trailing kernel-1 dim from a 3D pointwise-conv weight so it
/// can feed a `QMatMul` (which expects a 2D matrix). The GGUF stores
/// these as Conv1d weights of shape `(out, in, 1)`; mathematically
/// they're Linears. Falls through unchanged if already 2D.
///
/// Goes through F32 to rebuild the QTensor at the same GgmlDType. For
/// F32/F16 sources this is loss-free; for Q8_0/Q4_K it introduces a
/// tiny re-quant rounding error -- negligible vs the on-disk quant
/// already in effect, and the alternative (changing the on-disk
/// shape) would break the existing nemotron F32 GGUF on disk.
pub fn squeeze_kernel_dim(qt: QTensor) -> Result<QTensor> {
    let dims = qt.shape().dims().to_vec();
    if dims.len() != 3 || dims[2] != 1 {
        return Ok(qt);
    }
    let dtype = qt.dtype();
    let t = qt.dequantize(&Device::Cpu)?.squeeze(2)?.contiguous()?;
    QTensor::quantize(&t, dtype)
}

// ---- Take-by-name helpers ----------------------------------------

pub fn take_qt(t: &mut TensorMap, name: &str) -> Result<QTensor> {
    t.remove(name).ok_or_else(|| {
        candle_core::Error::Msg(format!("missing tensor: {name}"))
    })
}

pub fn take_f32(t: &mut TensorMap, name: &str) -> Result<Tensor> {
    take_qt(t, name)?.dequantize(&Device::Cpu)
}

pub fn take_f32_opt(
    t: &mut TensorMap,
    name: &str,
) -> Result<Option<Tensor>> {
    match t.remove(name) {
        Some(qt) => Ok(Some(qt.dequantize(&Device::Cpu)?)),
        None => Ok(None),
    }
}

// ---- QLinear wrapper ---------------------------------------------

/// Quantized linear via candle's `QMatMul`. The weight stays in its
/// loaded GGUF dtype; an optional F32 bias is added after the matmul.
pub struct QLinear {
    inner: QMatMul,
    bias: Option<Tensor>,
}

impl QLinear {
    pub fn new(w: QTensor, bias: Option<Tensor>) -> Result<Self> {
        Ok(Self {
            inner: QMatMul::from_qtensor(w)?,
            bias,
        })
    }

    pub fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = x.contiguous()?;
        let y = self.inner.forward(&x)?;
        match &self.bias {
            Some(b) => y.broadcast_add(b),
            None => Ok(y),
        }
    }
}

pub fn take_ql(t: &mut TensorMap, prefix: &str) -> Result<QLinear> {
    let w = take_qt(t, &format!("{prefix}.weight"))?;
    let b = take_f32_opt(t, &format!("{prefix}.bias"))?;
    QLinear::new(w, b)
}

pub fn take_ql_nb(
    t: &mut TensorMap,
    prefix: &str,
) -> Result<QLinear> {
    let w = take_qt(t, &format!("{prefix}.weight"))?;
    QLinear::new(w, None)
}

// ---- LayerNorm + Conv + Embedding (all F32) ----------------------

pub fn take_ln(
    t: &mut TensorMap,
    prefix: &str,
    cfg: LayerNormConfig,
) -> Result<LayerNorm> {
    let w = take_f32(t, &format!("{prefix}.weight"))?;
    let b = take_f32(t, &format!("{prefix}.bias"))?;
    Ok(LayerNorm::new(w, b, cfg.eps))
}

pub fn take_conv1d(
    t: &mut TensorMap,
    prefix: &str,
    cfg: Conv1dConfig,
    has_bias: bool,
) -> Result<Conv1d> {
    let w = take_f32(t, &format!("{prefix}.weight"))?;
    let b = if has_bias {
        Some(take_f32(t, &format!("{prefix}.bias"))?)
    } else {
        None
    };
    Ok(Conv1d::new(w, b, cfg))
}

pub fn take_conv2d(
    t: &mut TensorMap,
    prefix: &str,
    cfg: Conv2dConfig,
    has_bias: bool,
) -> Result<Conv2d> {
    let w = take_f32(t, &format!("{prefix}.weight"))?;
    let b = if has_bias {
        Some(take_f32(t, &format!("{prefix}.bias"))?)
    } else {
        None
    };
    Ok(Conv2d::new(w, b, cfg))
}

pub fn take_embedding(
    t: &mut TensorMap,
    prefix: &str,
    hidden_size: usize,
) -> Result<Embedding> {
    let w = take_f32(t, &format!("{prefix}.weight"))?;
    Ok(Embedding::new(w, hidden_size))
}
