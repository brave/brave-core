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

use crate::model::NemotronConfig;

// ---- FFT primitives ----
// Adapted from candle's Whisper audio module. Specialized for f32 and
// single-threaded (WASM).

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
        out[2 * (k + n / 2) + 1] =
            even_fft[2 * k + 1] - re * im_odd - im * re_odd;
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

// ---- Nemotron preprocessing ----
// Reimplements NeMo's AudioToMelSpectrogramPreprocessor in Rust with
// the parameters declared in nemotron-speech-streaming-en-0.6b's
// model_config.yaml:
//   sample_rate=16000, n_fft=512, win_length=400 (25 ms),
//   hop=160 (10 ms), 128 mel bins, preemph=0.97,
//   normalize="NA" (no per-feature mean/var normalization),
//   log_zero_guard 2^-24.
// Implemented by referencing NeMo's Python implementation at
// https://github.com/NVIDIA-NeMo/NeMo/blob/main/nemo/collections/asr/parts/preprocessing/features.py
// (FilterbankFeatures + AudioToMelSpectrogramPreprocessor classes).
//
// Differences from candle_parakeet's audio.rs:
//   - n_mels is taken from cfg (128 here vs 80 for parakeet-ctc-110m)
//   - the per-feature mean/std normalization step is gated on
//     cfg.normalize, since nemotron-speech-streaming trains with
//     normalize="NA"
//   - dither is omitted (training-only; the log-zero guard prevents
//     log(0) at inference)

// NeMo's log_zero_guard_value = 2^-24.
const LOG_GUARD: f32 = 5.960_464_5e-8;

/// Apply preemphasis: x[n] = x[n] - preemph * x[n-1].
fn apply_preemphasis(samples: &[f32], preemph: f32) -> Vec<f32> {
    if samples.is_empty() {
        return Vec::new();
    }
    let mut out = Vec::with_capacity(samples.len());
    out.push(samples[0]);
    for i in 1..samples.len() {
        out.push(samples[i] - preemph * samples[i - 1]);
    }
    out
}

/// Compute a log-mel spectrogram matching NeMo's
/// AudioToMelSpectrogramPreprocessor pipeline.
///
/// Inputs:
///   - `samples`: mono f32 PCM at `cfg.sample_rate` Hz.
///   - `filters`: precomputed mel filterbank as a flat
///     (n_mels, n_fft/2 + 1) row-major f32 matrix, generated with
///     librosa.filters.mel(htk=False, norm='slaney').
///   - `cfg`: NemotronConfig — drives n_mels, n_fft, win_length,
///     hop_length, preemph, and the optional per-feature normalize.
///
/// Output: flat (n_mels, n_frames) row-major f32 vector.
pub fn pcm_to_mel(
    samples: &[f32],
    filters: &[f32],
    cfg: &NemotronConfig,
) -> Vec<f32> {
    let n_fft = cfg.n_fft;
    let win_length = cfg.win_length;
    let hop_length = cfg.hop_length;
    let n_mel = cfg.num_mel_bins;
    let preemph = cfg.preemph;

    let samples = apply_preemphasis(samples, preemph);

    // Center padding: NeMo STFT uses center=True with reflect padding
    // around the signal.
    let pad = n_fft / 2;
    let padded_len = samples.len() + 2 * pad;
    let mut padded = vec![0.0f32; padded_len];
    for i in 0..pad {
        padded[pad - 1 - i] = samples[(i + 1).min(samples.len() - 1)];
    }
    padded[pad..pad + samples.len()].copy_from_slice(&samples);
    for i in 0..pad {
        let src = samples
            .len()
            .saturating_sub(2 + i)
            .min(samples.len() - 1);
        padded[pad + samples.len() + i] = samples[src];
    }

    // Hann window, non-periodic (matches NeMo's
    // torch.hann_window(win_length, periodic=False)).
    let two_pi = 2.0 * PI;
    let win_denom = (win_length - 1) as f32;
    let hann: Vec<f32> = (0..win_length)
        .map(|i| 0.5 * (1.0 - (two_pi * i as f32 / win_denom).cos()))
        .collect();

    let n_fft_bins = 1 + n_fft / 2;
    let n_frames = (padded_len - n_fft) / hop_length + 1;
    let win_offset = (n_fft - win_length) / 2;

    let mut mel = vec![0.0f32; n_mel * n_frames];
    let mut fft_in = vec![0.0f32; n_fft];

    for frame in 0..n_frames {
        let offset = frame * hop_length + win_offset;

        fft_in.fill(0.0);
        let avail = win_length.min(padded_len.saturating_sub(offset));
        for j in 0..avail {
            fft_in[j] = hann[j] * padded[offset + j];
        }

        let fft_out = fft(&fft_in);
        let mut power = vec![0.0f32; n_fft_bins];
        for j in 0..n_fft_bins {
            let re = fft_out[2 * j];
            let im = fft_out[2 * j + 1];
            power[j] = re * re + im * im;
        }

        for m in 0..n_mel {
            let mut sum = 0.0f32;
            let filter_row =
                &filters[m * n_fft_bins..(m + 1) * n_fft_bins];
            for j in 0..n_fft_bins {
                sum += power[j] * filter_row[j];
            }
            mel[m * n_frames + frame] = (sum + LOG_GUARD).ln();
        }
    }

    // Per-feature normalization. NeMo's preprocessor declares this
    // via `normalize`: "per_feature" subtracts each mel bin's mean
    // and divides by std (Bessel N-1); "NA" skips this step.
    // nemotron-speech-streaming-en-0.6b ships with "NA", so by
    // default we leave the log-mel values untouched.
    if cfg.normalize == "per_feature" {
        for m in 0..n_mel {
            let start = m * n_frames;
            let end = start + n_frames;
            let slice = &mel[start..end];

            let mean = slice.iter().sum::<f32>() / n_frames as f32;
            let ddof = if n_frames > 1 { n_frames - 1 } else { 1 };
            let var = slice
                .iter()
                .map(|&v| (v - mean) * (v - mean))
                .sum::<f32>()
                / ddof as f32;
            let std = (var + 1e-5).sqrt();

            for v in &mut mel[start..end] {
                *v = (*v - mean) / std;
            }
        }
    }

    mel
}
