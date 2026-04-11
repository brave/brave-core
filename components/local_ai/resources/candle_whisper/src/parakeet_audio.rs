/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Audio preprocessing for Parakeet speech recognition.
// Matches NeMo's AudioToMelSpectrogramPreprocessor:
// - N_FFT=512, win_length=400, hop=160, 80 mel bins
// - Preemphasis 0.97
// - ln(x + 2^-24) (not log10)
// - Per-feature mean/var normalization

use std::f32::consts::PI;

use crate::audio::fft;

// Parakeet audio parameters
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
        out.push(
            samples[i] - PREEMPHASIS * samples[i - 1],
        );
    }
    out
}

/// Compute log-mel spectrogram matching NeMo's
/// preprocessing pipeline.
/// Returns flattened (n_mel, n_frames) array.
pub fn pcm_to_mel_parakeet(
    samples: &[f32],
    filters: &[f32],
) -> Vec<f32> {
    let samples = apply_preemphasis(samples);

    // Center padding: NeMo STFT uses center=True,
    // which pads n_fft/2 on each side. PyTorch
    // defaults to reflect padding.
    let pad = N_FFT / 2;
    let padded_len = samples.len() + 2 * pad;
    let mut padded =
        vec![0.0f32; padded_len];
    // Reflect-pad left
    for i in 0..pad {
        padded[pad - 1 - i] =
            samples[(i + 1).min(samples.len() - 1)];
    }
    // Copy original
    padded[pad..pad + samples.len()]
        .copy_from_slice(&samples);
    // Reflect-pad right
    for i in 0..pad {
        let src = samples
            .len()
            .saturating_sub(2 + i)
            .min(samples.len() - 1);
        padded[pad + samples.len() + i] =
            samples[src];
    }

    // Hann window of length 400 (not 512).
    let two_pi = 2.0 * PI;
    let win_f = WIN_LENGTH as f32;
    let hann: Vec<f32> = (0..WIN_LENGTH)
        .map(|i| {
            0.5 * (1.0
                - (two_pi * i as f32 / win_f).cos())
        })
        .collect();

    let n_fft_bins = 1 + N_FFT / 2; // 257
    let n_frames =
        (padded_len - N_FFT) / HOP_LENGTH + 1;
    // torch.stft centers the win_length window
    // inside the n_fft frame. Offset by
    // (n_fft - win_length) / 2 = 56 samples.
    let win_offset = (N_FFT - WIN_LENGTH) / 2;

    let mut mel =
        vec![0.0f32; N_MEL * n_frames];
    let mut fft_in = vec![0.0f32; N_FFT];

    for frame in 0..n_frames {
        let offset =
            frame * HOP_LENGTH + win_offset;

        // Apply 400-sample window, zero-pad to 512
        fft_in.fill(0.0);
        let avail = WIN_LENGTH
            .min(padded_len.saturating_sub(offset));
        for j in 0..avail {
            fft_in[j] =
                hann[j] * padded[offset + j];
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
            let filter_row =
                &filters[m * n_fft_bins
                    ..(m + 1) * n_fft_bins];
            for j in 0..n_fft_bins {
                sum += power[j] * filter_row[j];
            }
            mel[m * n_frames + frame] =
                (sum + LOG_GUARD).ln();
        }
    }

    // Per-feature normalization: subtract mean,
    // divide by std for each mel bin.
    for m in 0..N_MEL {
        let start = m * n_frames;
        let end = start + n_frames;
        let slice = &mel[start..end];

        let mean =
            slice.iter().sum::<f32>()
                / n_frames as f32;
        // Bessel's correction (N-1) to match
        // torch.std() used in NeMo training.
        let ddof = if n_frames > 1 {
            n_frames - 1
        } else {
            1
        };
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

    mel
}
