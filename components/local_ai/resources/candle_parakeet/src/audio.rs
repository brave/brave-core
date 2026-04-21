/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/* The fft() and dft() helpers below incorporate work covered by the
 * following copyright and permission notice:
 *
 * Copyright 2022 Hugging Face
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

use std::f32::consts::PI;

// ---- FFT primitives ----
// Adapted from candle's Whisper audio module at
// https://github.com/huggingface/candle/blob/main/candle-transformers/src/models/whisper/audio.rs
// Specialized for f32 and single-threaded (WASM).

fn fft(inp: &[f32]) -> Vec<f32> {
    let n = inp.len();
    if n == 1 {
        return vec![inp[0], 0.0];
    }
    if n % 2 == 1 {
        return dft(inp);
    }
    let mut out = vec![0.0f32; n * 2];

    let mut even = Vec::with_capacity(n / 2);
    let mut odd = Vec::with_capacity(n / 2);

    for (i, &val) in inp.iter().enumerate() {
        if i % 2 == 0 {
            even.push(val);
        } else {
            odd.push(val);
        }
    }

    let even_fft = fft(&even);
    let odd_fft = fft(&odd);

    let two_pi = 2.0 * PI;
    let n_f = n as f32;
    for k in 0..n / 2 {
        let theta = two_pi * k as f32 / n_f;
        let re = theta.cos();
        let im = -theta.sin();

        let re_odd = odd_fft[2 * k];
        let im_odd = odd_fft[2 * k + 1];

        out[2 * k] = even_fft[2 * k] + re * re_odd - im * im_odd;
        out[2 * k + 1] = even_fft[2 * k + 1] + re * im_odd + im * re_odd;

        out[2 * (k + n / 2)] = even_fft[2 * k] - re * re_odd + im * im_odd;
        out[2 * (k + n / 2) + 1] = even_fft[2 * k + 1] - re * im_odd - im * re_odd;
    }
    out
}

fn dft(inp: &[f32]) -> Vec<f32> {
    let n = inp.len();
    let two_pi = 2.0 * PI;
    let n_f = n as f32;

    let mut out = Vec::with_capacity(2 * n);
    for k in 0..n {
        let k_f = k as f32;
        let mut re = 0.0f32;
        let mut im = 0.0f32;

        for (j, &val) in inp.iter().enumerate() {
            let angle = two_pi * k_f * j as f32 / n_f;
            re += val * angle.cos();
            im -= val * angle.sin();
        }

        out.push(re);
        out.push(im);
    }
    out
}

// ---- Parakeet preprocessing ----
// Reimplements NeMo's AudioToMelSpectrogramPreprocessor in Rust:
//   N_FFT=512, win_length=400, hop=160, 80 mel bins, preemph 0.97,
//   ln(x + 2^-24), per-feature mean/var normalization.
// Implemented by referencing NeMo's Python implementation at
// https://github.com/NVIDIA-NeMo/NeMo/blob/v2.7.2/nemo/collections/asr/parts/preprocessing/features.py
// (FilterbankFeatures + AudioToMelSpectrogramPreprocessor classes).
//
// NeMo stages intentionally skipped (irrelevant for Parakeet-CTC-110M
// inference):
//   - Dither (~1e-5 noise): LOG_GUARD already prevents log(0), and inference
//     typically runs with dither disabled.
//   - Per-sample waveform normalization: Parakeet-CTC-110M uses
//     normalize="per_feature" (implemented below), not "per_sample".
//   - SpecAugment masks: training-only.

const N_FFT: usize = 512;
const WIN_LENGTH: usize = 400;
const HOP_LENGTH: usize = 160;
const N_MEL: usize = 80;
const PREEMPHASIS: f32 = 0.97;
// NeMo's log_zero_guard_value = 2^-24
const LOG_GUARD: f32 = 5.960_464_5e-8;

/// Apply preemphasis: x[n] = x[n] - 0.97 * x[n-1]
fn apply_preemphasis(samples: &[f32]) -> Vec<f32> {
    if samples.is_empty() {
        return Vec::new();
    }
    let mut out = Vec::with_capacity(samples.len());
    out.push(samples[0]);
    for i in 1..samples.len() {
        out.push(samples[i] - PREEMPHASIS * samples[i - 1]);
    }
    out
}

/// Compute a log-mel spectrogram matching NeMo's
/// AudioToMelSpectrogramPreprocessor pipeline.
///
/// Input:
///   - `samples`: mono f32 PCM at 16 kHz. Length unconstrained.
///   - `filters`: precomputed mel filterbank as a flat (N_MEL, n_fft/2 + 1) =
///     (80, 257) row-major f32 matrix. Must be generated with Slaney-normalized
///     triangular filters over the HTK mel scale at sr=16000, n_fft=512,
///     fmin=0, fmax=8000. Standard librosa call: librosa.filters.mel(sr=16000,
///     n_fft=512, n_mels=80, fmin=0, fmax=8000, htk=False, norm='slaney') Using
///     different construction (htk=True, norm=None, different range) will
///     silently produce degraded features. Parakeet-CTC-110M was trained
///     against this exact filterbank.
///
/// Output: flat (N_MEL, n_frames) = (80, n_frames) row-major f32.
pub fn pcm_to_mel(samples: &[f32], filters: &[f32]) -> Vec<f32> {
    let samples = apply_preemphasis(samples);

    // Center padding: NeMo STFT uses center=True, which pads n_fft/2 on
    // each side. PyTorch defaults to reflect padding.
    let pad = N_FFT / 2;
    let padded_len = samples.len() + 2 * pad;
    let mut padded = vec![0.0f32; padded_len];
    // Reflect-pad left
    for i in 0..pad {
        padded[pad - 1 - i] = samples[(i + 1).min(samples.len() - 1)];
    }
    // Copy original
    padded[pad..pad + samples.len()].copy_from_slice(&samples);
    // Reflect-pad right
    for i in 0..pad {
        let src = samples.len().saturating_sub(2 + i).min(samples.len() - 1);
        padded[pad + samples.len() + i] = samples[src];
    }

    // Symmetric Hann window of length 400 (matches NeMo's
    // torch.hann_window(win_length, periodic=False): 0.5 * (1 - cos(2πn /
    // (N-1))) for n in 0..N).
    let two_pi = 2.0 * PI;
    let win_denom = (WIN_LENGTH - 1) as f32;
    let hann: Vec<f32> =
        (0..WIN_LENGTH).map(|i| 0.5 * (1.0 - (two_pi * i as f32 / win_denom).cos())).collect();

    let n_fft_bins = 1 + N_FFT / 2; // 257
    let n_frames = (padded_len - N_FFT) / HOP_LENGTH + 1;
    // torch.stft centers the win_length window inside the n_fft frame.
    // Offset by (n_fft - win_length) / 2 = 56 samples.
    let win_offset = (N_FFT - WIN_LENGTH) / 2;

    let mut mel = vec![0.0f32; N_MEL * n_frames];
    let mut fft_in = vec![0.0f32; N_FFT];

    for frame in 0..n_frames {
        let offset = frame * HOP_LENGTH + win_offset;

        // Apply 400-sample window, zero-pad to 512
        fft_in.fill(0.0);
        let avail = WIN_LENGTH.min(padded_len.saturating_sub(offset));
        for j in 0..avail {
            fft_in[j] = hann[j] * padded[offset + j];
        }

        // FFT -> power spectrum
        let fft_out = fft(&fft_in);
        let mut power = vec![0.0f32; n_fft_bins];
        for j in 0..n_fft_bins {
            let re = fft_out[2 * j];
            let im = fft_out[2 * j + 1];
            power[j] = re * re + im * im;
        }

        // Mel filterbank + natural log
        for m in 0..N_MEL {
            let mut sum = 0.0f32;
            let filter_row = &filters[m * n_fft_bins..(m + 1) * n_fft_bins];
            for j in 0..n_fft_bins {
                sum += power[j] * filter_row[j];
            }
            mel[m * n_frames + frame] = (sum + LOG_GUARD).ln();
        }
    }

    // Per-feature normalization: subtract mean, divide by std for each mel
    // bin.
    for m in 0..N_MEL {
        let start = m * n_frames;
        let end = start + n_frames;
        let slice = &mel[start..end];

        let mean = slice.iter().sum::<f32>() / n_frames as f32;
        // Bessel's correction (N-1) to match torch.std() used in NeMo
        // training.
        let ddof = if n_frames > 1 { n_frames - 1 } else { 1 };
        let var = slice.iter().map(|&v| (v - mean) * (v - mean)).sum::<f32>() / ddof as f32;
        let std = (var + 1e-5).sqrt();

        for v in &mut mel[start..end] {
            *v = (*v - mean) / std;
        }
    }

    mel
}
