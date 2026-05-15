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

// Radix-2 FFT helper shared by the TDT and CTC audio modules.
// Adapted from candle's Whisper audio module at
// https://github.com/huggingface/candle/blob/main/candle-transformers/src/models/whisper/audio.rs
// Specialized for f32 and single-threaded (WASM).

use std::f32::consts::PI;

pub fn fft(inp: &[f32]) -> Vec<f32> {
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
