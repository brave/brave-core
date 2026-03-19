/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* This file incorporates work covered by the following
 * copyright and permission notice:
 *
 * Copyright 2022 Hugging Face
 *
 * Licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the License is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See
 * the License for the specific language governing
 * permissions and limitations under the License.
 */

// Audio processing for Whisper mel spectrogram, adapted
// from https://github.com/huggingface/candle/blob/main/
//   candle-transformers/src/models/whisper/audio.rs
// Specialized for f32 and single-threaded (WASM).

use std::f32::consts::PI;

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

#[allow(clippy::too_many_arguments)]
fn log_mel_spectrogram_w(
    ith: usize,
    hann: &[f32],
    samples: &[f32],
    filters: &[f32],
    fft_size: usize,
    fft_step: usize,
    speed_up: bool,
    n_len: usize,
    n_mel: usize,
    n_threads: usize,
) -> Vec<f32> {
    let n_fft = if speed_up { 1 + fft_size / 4 } else { 1 + fft_size / 2 };

    let mut fft_in = vec![0.0f32; fft_size];
    let mut mel = vec![0.0f32; n_len * n_mel];
    let n_samples = samples.len();
    let end = std::cmp::min(n_samples / fft_step + 1, n_len);

    for i in (ith..end).step_by(n_threads) {
        let offset = i * fft_step;

        // Apply Hanning window
        for j in 0..std::cmp::min(fft_size, n_samples - offset) {
            fft_in[j] = hann[j] * samples[offset + j];
        }

        // Fill the rest with zeros
        if n_samples - offset < fft_size {
            fft_in[n_samples - offset..].fill(0.0);
        }

        // FFT
        let mut fft_out = fft(&fft_in);

        // Calculate modulus^2 of complex numbers
        for j in 0..fft_size {
            fft_out[j] = fft_out[2 * j] * fft_out[2 * j] + fft_out[2 * j + 1] * fft_out[2 * j + 1];
        }
        for j in 1..fft_size / 2 {
            let v = fft_out[fft_size - j];
            fft_out[j] += v;
        }

        if speed_up {
            for j in 0..n_fft {
                fft_out[j] = 0.5 * (fft_out[2 * j] + fft_out[2 * j + 1]);
            }
        }

        // Mel spectrogram
        for j in 0..n_mel {
            let mut sum = 0.0f32;
            let mut k = 0;
            // Unroll loop
            while k < n_fft.saturating_sub(3) {
                sum += fft_out[k] * filters[j * n_fft + k]
                    + fft_out[k + 1] * filters[j * n_fft + k + 1]
                    + fft_out[k + 2] * filters[j * n_fft + k + 2]
                    + fft_out[k + 3] * filters[j * n_fft + k + 3];
                k += 4;
            }
            // Handle remainder
            while k < n_fft {
                sum += fft_out[k] * filters[j * n_fft + k];
                k += 1;
            }
            mel[j * n_len + i] = f32::max(sum, 1e-10).log10();
        }
    }
    mel
}

pub fn log_mel_spectrogram(
    samples: &[f32],
    filters: &[f32],
    fft_size: usize,
    fft_step: usize,
    n_mel: usize,
    speed_up: bool,
) -> Vec<f32> {
    let two_pi = 2.0 * PI;
    let fft_size_f = fft_size as f32;

    let hann: Vec<f32> =
        (0..fft_size).map(|i| 0.5 * (1.0 - (two_pi * i as f32 / fft_size_f).cos())).collect();
    let n_len = samples.len() / fft_step;

    // Pad to multiples of chunk length
    let pad = 100 * super::CHUNK_LENGTH / 2;
    let n_len = if n_len % pad != 0 { (n_len / pad + 1) * pad } else { n_len };
    let n_len = n_len + pad;

    let samples = {
        let mut padded = samples.to_vec();
        let target_len = n_len * fft_step;
        if padded.len() < target_len {
            padded.resize(target_len, 0.0);
        }
        padded
    };

    // Single-threaded for WASM
    let mut mel = log_mel_spectrogram_w(
        0, &hann, &samples, filters, fft_size, fft_step, speed_up, n_len, n_mel, 1,
    );

    // Normalize
    let mmax = mel.iter().cloned().fold(f32::NEG_INFINITY, f32::max) - 8.0;
    for m in mel.iter_mut() {
        let v = f32::max(*m, mmax);
        *m = v / 4.0 + 1.0;
    }
    mel
}

pub fn pcm_to_mel(cfg: &super::Config, samples: &[f32], filters: &[f32]) -> Vec<f32> {
    log_mel_spectrogram(samples, filters, super::N_FFT, super::HOP_LENGTH, cfg.num_mel_bins, false)
}
