/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Parakeet-TDT-0.6B-v3 (FastConformer encoder + LSTM predictor + TDT
// joint network). Token-and-Duration Transducer: like RNN-T but the
// joint network also predicts how many encoder frames to skip,
// allowing variable-rate decoding.
//
// Architecture:
//   Encoder:   24-layer FastConformer (d=1024, 8 heads, FFN=4096)
//   Predictor: Embed(8193, 640) + 2-layer LSTM(640)
//   Joint:     enc(1024 -> 640) + pred(640 -> 640) -> ReLU
//              -> out(640 -> 8198)
//   Output:    8192 vocab + 1 blank + 5 duration values
//
// GGUF tensor names follow cstr/parakeet-tdt-0.6b-v3-GGUF's NeMo-to-GGUF
// convention. Input layout: (B, mel, time) — transposed inside
// QSubsampling before Conv2d.
//
// Quantization: candle's QMatMul dispatches on the loaded GGUF dtype, so
// this same code runs the F16 / Q4_K / Q8_0 variants unchanged. The
// pointwise convs (pw1/pw2 in QConvModule) are converted to F16 QTensor
// at load time and dequantized on-the-fly during forward to keep
// resident memory bounded (~144 MB F16 vs ~288 MB F32 across 24 layers).

use std::collections::HashMap;

use candle_core::quantized::ggml_file::qtensor_from_ggml;
use candle_core::quantized::{gguf_file, GgmlDType, QMatMul, QTensor};
use candle_core::{DType, Device, IndexOp, Module, Result, Tensor, D};
use candle_nn::{
    BatchNorm, Conv1d, Conv1dConfig, Conv2d, Conv2dConfig, LayerNorm, Linear, ModuleT,
};
use serde::Deserialize;

// ============================================================
// Config
// ============================================================

#[derive(Debug, Clone, Deserialize)]
pub struct TdtConfig {
    pub hidden_size: usize,
    pub num_heads: usize,
    pub num_layers: usize,
    pub intermediate_size: usize,
    pub conv_kernel_size: usize,
    pub num_mel_bins: usize,
    pub vocab_size: usize,
    pub blank_id: usize,
    pub subsampling_channels: usize,
    pub pred_hidden: usize,
    pub pred_layers: usize,
    pub joint_hidden: usize,
    pub n_durations: usize,
    #[serde(default = "default_durations")]
    pub durations: Vec<usize>,
    #[serde(default)]
    pub quantized: bool,
}

fn default_durations() -> Vec<usize> {
    vec![0, 1, 2, 3, 4]
}

impl Default for TdtConfig {
    fn default() -> Self {
        Self {
            hidden_size: 1024,
            num_heads: 8,
            num_layers: 24,
            intermediate_size: 4096,
            conv_kernel_size: 9,
            num_mel_bins: 128,
            vocab_size: 8192,
            blank_id: 8192,
            subsampling_channels: 256,
            pred_hidden: 640,
            pred_layers: 2,
            joint_hidden: 640,
            n_durations: 5,
            durations: vec![0, 1, 2, 3, 4],
            quantized: true,
        }
    }
}

// ============================================================
// Relative positional encoding helpers
// ============================================================
// Lifted from the legacy `parakeet_model.rs` shared module so this
// crate is self-contained.

/// Build the sinusoidal relative positional embedding for a sequence
/// of length `seq_len`. Output: `(1, 2*seq_len - 1, hidden)` with
/// interleaved sin/cos pairs (NOT stacked halves).
fn compute_pos_emb(inv_freq: &Tensor, seq_len: usize, device: &Device) -> Result<Tensor> {
    let n_pos = 2 * seq_len - 1;
    let positions: Vec<f32> = (0..n_pos).map(|i| (seq_len as f32 - 1.0) - i as f32).collect();
    let positions = Tensor::new(positions, device)?.unsqueeze(1)?;
    let inv = inv_freq.unsqueeze(0)?;

    let freqs = positions.broadcast_mul(&inv)?;
    let sin = freqs.sin()?;
    let cos = freqs.cos()?;
    let sin = sin.unsqueeze(D::Minus1)?;
    let cos = cos.unsqueeze(D::Minus1)?;
    let interleaved = Tensor::cat(&[sin, cos], D::Minus1)?;
    let (n, half) = freqs.dims2()?;
    interleaved.reshape((n, half * 2))?.unsqueeze(0)
}

/// Shaw-style relative shift. Input `(B, H, S, 2S-1)` -> `(B, H, S, S)`.
fn rel_shift(x: &Tensor) -> Result<Tensor> {
    let (b, h, s, _) = x.dims4()?;
    let zero_pad = Tensor::zeros((b, h, s, 1), x.dtype(), x.device())?;
    let x_padded = Tensor::cat(&[&zero_pad, x], 3)?;
    let x_padded = x_padded.contiguous()?.reshape((b, h, 2 * s, s))?;
    let x_padded = x_padded.narrow(2, 1, 2 * s - 1)?;
    x_padded.contiguous()?.reshape((b, h, s, 2 * s - 1))?.narrow(3, 0, s)
}

// ============================================================
// QLinear: quantized linear via candle QMatMul
// ============================================================

#[derive(Debug, Clone)]
struct QLinear {
    inner: QMatMul,
    bias: Option<Tensor>,
}

impl QLinear {
    fn new(w: QTensor, bias: Option<Tensor>) -> Result<Self> {
        Ok(Self { inner: QMatMul::from_qtensor(w)?, bias })
    }

    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = x.contiguous()?;
        let y = self.inner.forward(&x)?;
        match &self.bias {
            Some(b) => y.broadcast_add(b),
            None => Ok(y),
        }
    }
}

// ============================================================
// Encoder: Quantized FastConformer
// ============================================================

// ----- Subsampling: encoder.pre.conv.{0,2,3,5,6} + encoder.pre.out -----

#[derive(Debug, Clone)]
struct QSubsampling {
    layers: Vec<Conv2d>,
    linear: QLinear,
}

impl QSubsampling {
    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // x: (B, mel, time) → transpose to (B, time, mel), then
        // unsqueeze to (B, 1, time, mel) for Conv2d. NeMo ConvSubsampling
        // expects (B, T, D) input where T=time, D=mel_bins.
        let x = x.transpose(1, 2)?;
        let mut x = x.unsqueeze(1)?;

        x = self.layers[0].forward(&x)?.relu()?;
        x = self.layers[1].forward(&x)?;
        x = self.layers[2].forward(&x)?.relu()?;
        x = self.layers[3].forward(&x)?;
        x = self.layers[4].forward(&x)?.relu()?;

        let (b, c, t_sub, mel_sub) = x.dims4()?;
        x = x.transpose(1, 2)?.contiguous()?.reshape((b, t_sub, c * mel_sub))?;
        self.linear.forward(&x)
    }
}

// ----- FeedForward: encoder.layers.{i}.ff{1,2}.linear{1,2} -----

#[derive(Debug, Clone)]
struct QFeedForward {
    linear1: QLinear,
    linear2: QLinear,
}

impl QFeedForward {
    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        let x = self.linear1.forward(x)?.silu()?;
        self.linear2.forward(&x)
    }
}

// ----- Relative Multi-Head Attention -----
// encoder.layers.{i}.attn.{q,k,v,out,pos,pos_bias_u,pos_bias_v}

#[derive(Debug, Clone)]
struct QRelativeMHA {
    q_proj: QLinear,
    k_proj: QLinear,
    v_proj: QLinear,
    o_proj: QLinear,
    pos_proj: QLinear,
    bias_u: Tensor,
    bias_v: Tensor,
    n_head: usize,
    head_dim: usize,
}

impl QRelativeMHA {
    fn forward(&self, x: &Tensor, pos_emb: &Tensor) -> Result<Tensor> {
        let (b, s, _) = x.dims3()?;
        let h = self.n_head;
        let d = self.head_dim;

        let q = self.q_proj.forward(x)?.reshape((b, s, h, d))?.transpose(1, 2)?;
        let k = self.k_proj.forward(x)?.reshape((b, s, h, d))?.transpose(1, 2)?;
        let v = self.v_proj.forward(x)?.reshape((b, s, h, d))?.transpose(1, 2)?;

        let r = self.pos_proj.forward(pos_emb)?.reshape((1, 2 * s - 1, h, d))?.transpose(1, 2)?;

        let bu = self.bias_u.unsqueeze(0)?.unsqueeze(2)?;
        let bv = self.bias_v.unsqueeze(0)?.unsqueeze(2)?;

        let q_u = q.broadcast_add(&bu)?;
        let matrix_ac = q_u.matmul(&k.transpose(2, 3)?)?;

        let q_v = q.broadcast_add(&bv)?;
        let matrix_bd = q_v.matmul(&r.transpose(2, 3)?)?;
        let matrix_bd = rel_shift(&matrix_bd)?;

        let scale = (d as f64).sqrt();
        let scores = ((matrix_ac + matrix_bd)? / scale)?;

        let attn = candle_nn::ops::softmax_last_dim(&scores)?;
        let out = attn.matmul(&v)?;

        let out = out.transpose(1, 2)?.contiguous()?.reshape((b, s, h * d))?;
        self.o_proj.forward(&out)
    }
}

// ----- Conv Module -----
// encoder.layers.{i}.conv.{pw1,dw,pw2,bn}
// pw1/pw2 stored as 3D [out, in, 1] F16 in GGUF. Kept as QTensor and
// dequantized on-the-fly during forward to save memory (~144 MB F16 vs
// ~288 MB F32 across the 24 layers). dw stays F32 Conv1d.

struct QConvModule {
    pw1_qt: QTensor,
    pw1_bias: Option<Tensor>,
    depthwise_conv: Conv1d,
    norm: BatchNorm,
    pw2_qt: QTensor,
    pw2_bias: Option<Tensor>,
}

impl QConvModule {
    fn forward(&self, x: &Tensor) -> Result<Tensor> {
        // x: (B, T, H). pw1 dequantize F16 → F32 on-the-fly and drop
        // after the matmul so peak memory stays bounded.
        let pw1_w = self.pw1_qt.dequantize(&Device::Cpu)?;
        let x = Linear::new(pw1_w, self.pw1_bias.clone()).forward(x)?;

        // GLU gate
        let chunks = x.chunk(2, 2)?;
        let gate = candle_nn::ops::sigmoid(&chunks[1])?;
        let x = (&chunks[0] * &gate)?;

        // Depthwise conv needs (B, H, T)
        let x = x.transpose(1, 2)?;
        let x = self.depthwise_conv.forward(&x)?;
        let x = self.norm.forward_t(&x, false)?;
        let x = x.silu()?;
        let x = x.transpose(1, 2)?;

        // pw2: dequant F16 → F32 on-the-fly
        let pw2_w = self.pw2_qt.dequantize(&Device::Cpu)?;
        Linear::new(pw2_w, self.pw2_bias.clone()).forward(&x)
    }
}

// ----- Conformer Block (macaron-style: FF1 / MHSA / Conv / FF2) -----

struct QConformerBlock {
    norm_ff1: LayerNorm,
    ff1: QFeedForward,
    norm_attn: LayerNorm,
    self_attn: QRelativeMHA,
    norm_conv: LayerNorm,
    conv: QConvModule,
    norm_ff2: LayerNorm,
    ff2: QFeedForward,
    norm_out: LayerNorm,
}

impl QConformerBlock {
    fn forward(&self, x: &Tensor, pos_emb: &Tensor) -> Result<Tensor> {
        let residual = x.clone();
        let x = (&residual + (self.ff1.forward(&self.norm_ff1.forward(x)?)? * 0.5)?)?;

        let x = (&x + self.self_attn.forward(&self.norm_attn.forward(&x)?, pos_emb)?)?;

        let x = (&x + self.conv.forward(&self.norm_conv.forward(&x)?)?)?;

        let x = (&x + (self.ff2.forward(&self.norm_ff2.forward(&x)?)? * 0.5)?)?;

        self.norm_out.forward(&x)
    }
}

// ============================================================
// Predictor: LSTM prediction network
// ============================================================
// decoder.embed.weight: (vocab_size+1, pred_hidden)
// decoder.lstm.{0,1}.w_ih/b_ih/w_hh/b_hh

struct LstmLayer {
    ih: Linear,
    hh: Linear,
}

pub struct LstmState {
    h: Vec<Tensor>,
    c: Vec<Tensor>,
}

struct LstmPredictor {
    embed: Tensor,
    layers: Vec<LstmLayer>,
    pred_hidden: usize,
}

impl LstmPredictor {
    fn init_state(&self) -> LstmState {
        let n = self.layers.len();
        let h = self.pred_hidden;
        LstmState {
            h: (0..n)
                .map(|_| Tensor::zeros((1, h), DType::F32, &Device::Cpu).unwrap())
                .collect(),
            c: (0..n)
                .map(|_| Tensor::zeros((1, h), DType::F32, &Device::Cpu).unwrap())
                .collect(),
        }
    }

    /// Run one LSTM step. Returns the last layer's hidden state.
    fn step(&self, token_id: u32, state: &mut LstmState) -> Result<Tensor> {
        let x = self.embed.i(token_id as usize)?.unsqueeze(0)?;

        let mut input = x;
        for (i, layer) in self.layers.iter().enumerate() {
            let (h_new, c_new) = lstm_step(layer, &input, &state.h[i], &state.c[i])?;
            state.h[i] = h_new.clone();
            state.c[i] = c_new;
            input = h_new;
        }

        Ok(input)
    }
}

/// Single LSTM cell step.
/// x: (1, input_size), h: (1, H), c: (1, H) → (h_new, c_new) each (1, H).
fn lstm_step(layer: &LstmLayer, x: &Tensor, h: &Tensor, c: &Tensor) -> Result<(Tensor, Tensor)> {
    let hidden = c.dim(1)?;

    // gates = x @ w_ih^T + h @ w_hh^T + bias (combined bias is on ih).
    let gates = (layer.ih.forward(x)? + layer.hh.forward(h)?)?;

    let i_gate = candle_nn::ops::sigmoid(&gates.narrow(1, 0, hidden)?)?;
    let f_gate = candle_nn::ops::sigmoid(&gates.narrow(1, hidden, hidden)?)?;
    let g_gate = gates.narrow(1, 2 * hidden, hidden)?.tanh()?;
    let o_gate = candle_nn::ops::sigmoid(&gates.narrow(1, 3 * hidden, hidden)?)?;

    let c_new = ((&f_gate * c)? + (&i_gate * &g_gate)?)?;
    let h_new = (&o_gate * c_new.tanh()?)?;

    Ok((h_new, c_new))
}

// ============================================================
// Joint Network
// ============================================================
// joint.enc:  Linear(d_model -> joint_hidden)
// joint.pred: Linear(pred_hidden -> joint_hidden)
// joint.out:  Linear(joint_hidden -> vocab+blank+dur)

struct JointNetwork {
    enc_proj: Linear,
    pred_proj: Linear,
    out_proj: Linear,
}

impl JointNetwork {
    /// Combine encoder and predictor outputs. `enc_projected` is already
    /// run through `enc_proj`; `pred_out` is the raw predictor hidden
    /// state. Returns `(1, vocab + blank + dur)` logits.
    fn forward(&self, enc_projected: &Tensor, pred_out: &Tensor) -> Result<Tensor> {
        let g = self.pred_proj.forward(pred_out)?;
        let combined = (enc_projected + g)?.relu()?;
        self.out_proj.forward(&combined)
    }
}

// ============================================================
// Full TDT Model
// ============================================================

/// Maximum tokens emitted per encoder frame (prevents infinite loops).
const MAX_SYMBOLS_PER_STEP: usize = 10;

pub struct TdtParakeet {
    // Encoder
    subsampling: QSubsampling,
    inv_freq: Tensor,
    blocks: Vec<QConformerBlock>,
    hidden_size: usize,
    cached_pos_emb: Option<(usize, Tensor)>,

    // Predictor + Joint
    predictor: LstmPredictor,
    joint: JointNetwork,

    // TDT decode config
    blank_id: u32,
    durations: Vec<usize>,
    /// vocab_size + 1 (blank). Duration logits start at this index in
    /// joint output.
    vocab_plus_blank: usize,
}

impl TdtParakeet {
    /// Run full inference: mel -> encode -> TDT decode. Returns token
    /// IDs (not including blank or duration tokens).
    pub fn transcribe(&mut self, mel: &Tensor) -> Result<Vec<u32>> {
        let enc = self.encode(mel)?;
        self.tdt_decode(&enc)
    }

    /// Encoder forward: subsampling + Conformer blocks. Returns
    /// `(1, T, d_model)`.
    fn encode(&mut self, mel: &Tensor) -> Result<Tensor> {
        let mut x = self.subsampling.forward(mel)?;

        let scale = (self.hidden_size as f64).sqrt();
        x = (x * scale)?;

        let seq_len = x.dim(1)?;
        let pos_emb = self.get_pos_emb(seq_len, x.device())?;

        for block in &self.blocks {
            x = block.forward(&x, &pos_emb)?;
        }

        Ok(x)
    }

    fn get_pos_emb(&mut self, seq_len: usize, device: &Device) -> Result<Tensor> {
        if let Some((cached_len, ref emb)) = self.cached_pos_emb {
            if cached_len == seq_len {
                return Ok(emb.clone());
            }
        }
        let emb = compute_pos_emb(&self.inv_freq, seq_len, device)?;
        self.cached_pos_emb = Some((seq_len, emb.clone()));
        Ok(emb)
    }

    /// TDT greedy decode. For each encoder frame, run the joint
    /// network with the current predictor state. If a non-blank token
    /// is predicted, emit it and feed it back to the predictor. The
    /// duration output determines how many encoder frames to advance.
    fn tdt_decode(&self, enc: &Tensor) -> Result<Vec<u32>> {
        let t_max = enc.dim(1)?;
        let blank = self.blank_id;

        let mut state = self.predictor.init_state();
        let mut pred_out = self.predictor.step(blank, &mut state)?;

        let mut time_idx: usize = 0;
        let mut tokens = Vec::new();

        while time_idx < t_max {
            let enc_frame = enc.i((0, time_idx))?.unsqueeze(0)?;
            let enc_proj = self.joint.enc_proj.forward(&enc_frame)?;

            let mut symbols_added: usize = 0;

            loop {
                let logits = self.joint.forward(&enc_proj, &pred_out)?;
                let logits_vec: Vec<f32> = logits.squeeze(0)?.to_vec1()?;

                let vpb = self.vocab_plus_blank;
                let token = argmax(&logits_vec[..vpb]);
                let dur_idx = argmax(&logits_vec[vpb..]);
                let skip = self.durations[dur_idx];

                if token as u32 == blank {
                    time_idx += if skip > 0 { skip } else { 1 };
                    break;
                }

                tokens.push(token as u32);

                pred_out = self.predictor.step(token as u32, &mut state)?;
                symbols_added += 1;
                time_idx += skip;

                if skip > 0 || symbols_added >= MAX_SYMBOLS_PER_STEP {
                    break;
                }
            }

            if symbols_added >= MAX_SYMBOLS_PER_STEP {
                time_idx += 1;
            }
        }

        Ok(tokens)
    }
}

fn argmax(slice: &[f32]) -> usize {
    let mut best_idx = 0;
    let mut best_val = f32::NEG_INFINITY;
    for (i, &v) in slice.iter().enumerate() {
        if v > best_val {
            best_val = v;
            best_idx = i;
        }
    }
    best_idx
}

// ============================================================
// TdtModelBuilder: chunked GGUF loading
// ============================================================
// Same pattern as the 110M ModelBuilder. Receives tensor data in
// ~8 MB chunks; after all chunks, build() assembles TdtParakeet.

pub struct TdtModelBuilder {
    content: gguf_file::Content,
    config: TdtConfig,
    tensors: HashMap<String, QTensor>,
    bytes_received: usize,
    spill: Vec<u8>,
    spill_offset: usize,
}

impl TdtModelBuilder {
    pub fn new(content: gguf_file::Content, config: &TdtConfig) -> Result<Self> {
        Ok(Self {
            content,
            config: config.clone(),
            tensors: HashMap::new(),
            bytes_received: 0,
            spill: Vec::new(),
            spill_offset: 0,
        })
    }

    /// Placeholder used by `lib.rs` mem::replace during finalize.
    pub fn empty() -> Self {
        Self {
            content: gguf_file::Content {
                magic: gguf_file::VersionedMagic::GgufV2,
                metadata: HashMap::new(),
                tensor_infos: HashMap::new(),
                tensor_data_offset: 0,
            },
            config: TdtConfig::default(),
            tensors: HashMap::new(),
            bytes_received: 0,
            spill: Vec::new(),
            spill_offset: 0,
        }
    }

    /// Process a chunk of tensor data.
    pub fn load_chunk(&mut self, chunk: &[u8]) -> Result<()> {
        let combined: Vec<u8>;
        let (view, view_start) = if self.spill.is_empty() {
            (chunk, self.bytes_received)
        } else {
            combined = [self.spill.as_slice(), chunk].concat();
            let start = self.spill_offset;
            self.spill.clear();
            (combined.as_slice(), start)
        };
        let view_end = view_start + view.len();

        for (name, info) in &self.content.tensor_infos {
            let t_start = info.offset as usize;
            let elems = info.shape.elem_count();
            let bs = info.ggml_dtype.block_size();
            let t_size = elems / bs * info.ggml_dtype.type_size();
            let t_end = t_start + t_size;

            if self.tensors.contains_key(name) {
                continue;
            }
            if t_start >= view_end || t_end <= view_start {
                continue;
            }

            if t_end > view_end {
                let local = t_start.saturating_sub(view_start);
                self.spill = view[local..].to_vec();
                self.spill_offset = t_start;
                continue;
            }

            let local = t_start - view_start;
            let slice = &view[local..local + t_size];
            let qt = qtensor_from_ggml(
                info.ggml_dtype,
                slice,
                info.shape.dims().to_vec(),
                &Device::Cpu,
            )?;
            self.tensors.insert(name.clone(), qt);
        }

        self.bytes_received += chunk.len();
        Ok(())
    }

    /// Build TdtParakeet from accumulated QTensors.
    pub fn build(self) -> Result<TdtParakeet> {
        TdtParakeet::load_from_tensors(self.tensors, &self.config)
    }
}

// ============================================================
// Build TdtParakeet from a QTensor HashMap
// ============================================================

impl TdtParakeet {
    fn load_from_tensors(mut t: HashMap<String, QTensor>, cfg: &TdtConfig) -> Result<Self> {
        // ----- Helpers -----

        fn take_qt(m: &mut HashMap<String, QTensor>, name: &str) -> Result<QTensor> {
            m.remove(name)
                .ok_or_else(|| candle_core::Error::Msg(format!("missing tensor: {name}")))
        }
        fn take_f32(m: &mut HashMap<String, QTensor>, name: &str) -> Result<Tensor> {
            take_qt(m, name)?.dequantize(&Device::Cpu)
        }
        fn take_f32_opt(
            m: &mut HashMap<String, QTensor>,
            name: &str,
        ) -> Result<Option<Tensor>> {
            match m.remove(name) {
                Some(qt) => Ok(Some(qt.dequantize(&Device::Cpu)?)),
                None => Ok(None),
            }
        }
        fn take_ql(m: &mut HashMap<String, QTensor>, prefix: &str) -> Result<QLinear> {
            let w = take_qt(m, &format!("{prefix}.weight"))?;
            let b = take_f32_opt(m, &format!("{prefix}.bias"))?;
            QLinear::new(w, b)
        }
        fn take_ql_nb(m: &mut HashMap<String, QTensor>, prefix: &str) -> Result<QLinear> {
            let w = take_qt(m, &format!("{prefix}.weight"))?;
            QLinear::new(w, None)
        }
        fn take_ln(m: &mut HashMap<String, QTensor>, prefix: &str) -> Result<LayerNorm> {
            let w = take_f32(m, &format!("{prefix}.weight"))?;
            let b = take_f32(m, &format!("{prefix}.bias"))?;
            Ok(LayerNorm::new(w, b, 1e-5))
        }
        fn take_bn(
            m: &mut HashMap<String, QTensor>,
            prefix: &str,
            n: usize,
        ) -> Result<BatchNorm> {
            let w = take_f32(m, &format!("{prefix}.weight"))?;
            let b = take_f32(m, &format!("{prefix}.bias"))?;
            let rm = take_f32(m, &format!("{prefix}.running_mean"))?;
            let rv = take_f32(m, &format!("{prefix}.running_var"))?;
            BatchNorm::new(n, rm, rv, w, b, 1e-5)
        }
        /// Load pointwise conv weight as F16 QTensor for on-the-fly
        /// dequant. Squeeze 3D [out, in, 1] → 2D [out, in].
        fn take_pw_qt(
            m: &mut HashMap<String, QTensor>,
            prefix: &str,
        ) -> Result<(QTensor, Option<Tensor>)> {
            let qt = take_qt(m, &format!("{prefix}.weight"))?;
            let w = qt.dequantize(&Device::Cpu)?;
            let w = if w.dims().len() == 3 { w.squeeze(2)? } else { w };
            let w_half = w.to_dtype(DType::F16)?;
            let qt = QTensor::quantize(&w_half, GgmlDType::F16)?;
            let b = take_f32_opt(m, &format!("{prefix}.bias"))?;
            Ok((qt, b))
        }
        fn take_linear(m: &mut HashMap<String, QTensor>, prefix: &str) -> Result<Linear> {
            let w = take_f32(m, &format!("{prefix}.weight"))?;
            let b = take_f32_opt(m, &format!("{prefix}.bias"))?;
            Ok(Linear::new(w, b))
        }

        // ----- Subsampling -----

        let ch = cfg.subsampling_channels;
        fn take_conv2d(
            m: &mut HashMap<String, QTensor>,
            name: &str,
            conv_cfg: Conv2dConfig,
        ) -> Result<Conv2d> {
            let pfx = "encoder.pre.conv";
            let w = take_f32(m, &format!("{pfx}.{name}.weight"))?;
            let b = take_f32(m, &format!("{pfx}.{name}.bias"))?;
            Ok(Conv2d::new(w, Some(b), conv_cfg))
        }

        let c0 = Conv2dConfig { padding: 1, stride: 2, ..Default::default() };
        let c_dw = Conv2dConfig { padding: 1, stride: 2, groups: ch, ..Default::default() };
        let c_pw = Conv2dConfig::default();

        let subsampling = QSubsampling {
            layers: vec![
                take_conv2d(&mut t, "0", c0)?,
                take_conv2d(&mut t, "2", c_dw)?,
                take_conv2d(&mut t, "3", c_pw)?,
                take_conv2d(&mut t, "5", c_dw)?,
                take_conv2d(&mut t, "6", c_pw)?,
            ],
            linear: take_ql(&mut t, "encoder.pre.out")?,
        };

        // ----- inv_freq (computed, not stored) -----

        let half = cfg.hidden_size / 2;
        let inv_freq_vec: Vec<f32> = (0..half)
            .map(|i| 1.0 / (10000f32.powf((2 * i) as f32 / cfg.hidden_size as f32)))
            .collect();
        let inv_freq = Tensor::new(inv_freq_vec, &Device::Cpu)?;

        // ----- Conformer blocks -----

        let h = cfg.hidden_size;
        let blocks = (0..cfg.num_layers)
            .map(|i| {
                let p = format!("encoder.layers.{i}");
                let kernel = cfg.conv_kernel_size;
                let pad = (kernel - 1) / 2;
                let dw_cfg = Conv1dConfig { padding: pad, groups: h, ..Default::default() };
                let dw_w = take_f32(&mut t, &format!("{p}.conv.dw.weight"))?;
                let dw_b = take_f32_opt(&mut t, &format!("{p}.conv.dw.bias"))?;

                Ok(QConformerBlock {
                    norm_ff1: take_ln(&mut t, &format!("{p}.norm_ff1"))?,
                    ff1: QFeedForward {
                        linear1: take_ql(&mut t, &format!("{p}.ff1.linear1"))?,
                        linear2: take_ql(&mut t, &format!("{p}.ff1.linear2"))?,
                    },
                    norm_attn: take_ln(&mut t, &format!("{p}.norm_attn"))?,
                    self_attn: QRelativeMHA {
                        q_proj: take_ql(&mut t, &format!("{p}.attn.q"))?,
                        k_proj: take_ql(&mut t, &format!("{p}.attn.k"))?,
                        v_proj: take_ql(&mut t, &format!("{p}.attn.v"))?,
                        o_proj: take_ql(&mut t, &format!("{p}.attn.out"))?,
                        pos_proj: take_ql_nb(&mut t, &format!("{p}.attn.pos"))?,
                        bias_u: take_f32(&mut t, &format!("{p}.attn.pos_bias_u"))?,
                        bias_v: take_f32(&mut t, &format!("{p}.attn.pos_bias_v"))?,
                        n_head: cfg.num_heads,
                        head_dim: h / cfg.num_heads,
                    },
                    norm_conv: take_ln(&mut t, &format!("{p}.norm_conv"))?,
                    conv: {
                        let (pw1_qt, pw1_bias) =
                            take_pw_qt(&mut t, &format!("{p}.conv.pw1"))?;
                        let (pw2_qt, pw2_bias) =
                            take_pw_qt(&mut t, &format!("{p}.conv.pw2"))?;
                        QConvModule {
                            pw1_qt,
                            pw1_bias,
                            depthwise_conv: Conv1d::new(dw_w, dw_b, dw_cfg),
                            norm: take_bn(&mut t, &format!("{p}.conv.bn"), h)?,
                            pw2_qt,
                            pw2_bias,
                        }
                    },
                    norm_ff2: take_ln(&mut t, &format!("{p}.norm_ff2"))?,
                    ff2: QFeedForward {
                        linear1: take_ql(&mut t, &format!("{p}.ff2.linear1"))?,
                        linear2: take_ql(&mut t, &format!("{p}.ff2.linear2"))?,
                    },
                    norm_out: take_ln(&mut t, &format!("{p}.norm_out"))?,
                })
            })
            .collect::<Result<Vec<_>>>()?;

        // ----- Predictor (LSTM) -----

        let embed = take_f32(&mut t, "decoder.embed.weight")?;
        let mut lstm_layers = Vec::new();
        for i in 0..cfg.pred_layers {
            let pfx = format!("decoder.lstm.{i}");
            let w_ih = take_f32(&mut t, &format!("{pfx}.w_ih"))?;
            let b_ih = take_f32(&mut t, &format!("{pfx}.b_ih"))?;
            let w_hh = take_f32(&mut t, &format!("{pfx}.w_hh"))?;
            let b_hh = take_f32(&mut t, &format!("{pfx}.b_hh"))?;
            // gates = x@w_ih^T + b_ih + h@w_hh^T + b_hh
            //       = x@w_ih^T + h@w_hh^T + (b_ih + b_hh)
            let bias = (b_ih + b_hh)?;
            lstm_layers.push(LstmLayer {
                ih: Linear::new(w_ih, Some(bias)),
                hh: Linear::new(w_hh, None),
            });
        }
        let predictor =
            LstmPredictor { embed, layers: lstm_layers, pred_hidden: cfg.pred_hidden };

        // ----- Joint network -----

        let joint = JointNetwork {
            enc_proj: take_linear(&mut t, "joint.enc")?,
            pred_proj: take_linear(&mut t, "joint.pred")?,
            out_proj: take_linear(&mut t, "joint.out")?,
        };

        Ok(Self {
            subsampling,
            inv_freq,
            blocks,
            hidden_size: cfg.hidden_size,
            cached_pos_emb: None,
            predictor,
            joint,
            blank_id: cfg.blank_id as u32,
            durations: cfg.durations.clone(),
            vocab_plus_blank: cfg.vocab_size + 1,
        })
    }
}
