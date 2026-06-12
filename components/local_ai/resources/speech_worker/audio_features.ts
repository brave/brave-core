// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Pure DSP for NeMo-style log-mel features. No ORT, no mojo, no module
// globals — kept dependency-free so it stays trivially testable and the
// recognizer can reuse it across models.

// In-place iterative radix-2 FFT, size 512.
export function fft512(re: Float32Array, im: Float32Array) {
  const n = 512
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1
    for (; j & bit; bit >>= 1) {
      j ^= bit
    }
    j ^= bit
    if (i < j) {
      const tr = re[i]
      re[i] = re[j]
      re[j] = tr
      const ti = im[i]
      im[i] = im[j]
      im[j] = ti
    }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = (-2 * Math.PI) / len
    const wr = Math.cos(ang)
    const wi = Math.sin(ang)
    for (let i = 0; i < n; i += len) {
      let cwr = 1
      let cwi = 0
      for (let k = 0; k < len / 2; k++) {
        const a = i + k
        const b = a + len / 2
        const vr = re[b] * cwr - im[b] * cwi
        const vi = re[b] * cwi + im[b] * cwr
        re[b] = re[a] - vr
        im[b] = im[a] - vi
        re[a] = re[a] + vr
        im[a] = im[a] + vi
        const ncwr = cwr * wr - cwi * wi
        cwi = cwr * wi + cwi * wr
        cwr = ncwr
      }
    }
  }
}

// 128-mel log spectrogram, NeMo FilterbankFeatures, normalize=null (NO
// per-feature normalization). Preemph 0.97, reflect-pad STFT
// n_fft=512/hop=160/win=400 hann-periodic, power spectrum, slaney
// filterbank, log(+2^-24).
export function computeNemoMel(
  wav: Float32Array,
  fbank: Float32Array,
): { data: Float32Array; T: number } {
  const NFFT = 512,
    HOP = 160,
    WIN = 400,
    NMEL = 128,
    NFREQ = 257
  const PRE = 0.97,
    OFF = (NFFT - WIN) >> 1,
    PAD = NFFT >> 1
  const x = new Float32Array(wav.length)
  x[0] = wav[0]
  for (let i = 1; i < wav.length; i++) {
    x[i] = wav[i] - PRE * wav[i - 1]
  }
  const padded = new Float32Array(x.length + 2 * PAD)
  padded.set(x, PAD)
  for (let i = 0; i < PAD; i++) {
    padded[PAD - 1 - i] = x[Math.min(i + 1, x.length - 1)]
    padded[PAD + x.length + i] = x[Math.max(x.length - 2 - i, 0)]
  }
  const hann = new Float32Array(WIN)
  for (let i = 0; i < WIN; i++) {
    hann[i] = 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / WIN)
  }
  const T = 1 + Math.floor(x.length / HOP)
  const mel = new Float32Array(NMEL * T)
  const re = new Float32Array(NFFT)
  const im = new Float32Array(NFFT)
  for (let t = 0; t < T; t++) {
    re.fill(0)
    im.fill(0)
    const base = t * HOP
    for (let i = 0; i < WIN; i++) {
      re[OFF + i] = padded[base + OFF + i] * hann[i]
    }
    fft512(re, im)
    for (let m = 0; m < NMEL; m++) {
      let acc = 0
      const fb = m * NFREQ
      for (let k = 0; k < NFREQ; k++) {
        acc += fbank[fb + k] * (re[k] * re[k] + im[k] * im[k])
      }
      mel[m * T + t] = Math.log(acc + 5.960464477539063e-8) // 2^-24
    }
  }
  return { data: mel, T }
}

// Argmax over a sub-range [start, end) of a flat array; returns the index
// relative to `start`.
export function argmax(a: Float32Array, start: number, end: number): number {
  let best = start
  for (let i = start + 1; i < end; i++) {
    if (a[i] > a[best]) {
      best = i
    }
  }
  return best - start
}
