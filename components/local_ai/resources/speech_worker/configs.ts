// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Nemotron 0.6B INT4 — cache-aware streaming RNN-T.
// Each encoder step advances by 560 ms: 56 new mel frames plus 9
// pre-encode left-context frames. Conformer caches are carried between
// steps, and fixed input/cache shapes keep the encoder cost constant per step.

//////////////// BEGIN: Shared audio frontend for both English only and multi-lingual Nemotron //////////////////////////////

// frontend constants
export type FftSize =
  | 32
  | 64
  | 128
  | 256
  | 512
  | 1024
  | 2048
  | 4096
  | 8192
  | 16384
  | 32768 // should be a power of 2
export type FftState = {
  n: number
  bitRev: Uint16Array
  real: Float32Array
  imag: Float32Array
  power: Float32Array
  frame: Float32Array
}

export const TARGET_SAMPLE_RATE = 16000
export const WIN_LENGTH: number = 400
export const N_FFT: number = 512
export const HOP_LENGTH: number = 160 // mel hop in samples (10 ms @ 16 kHz)
export const N_MELS: number = 128
export const PREEMPH: number = 0.97
export const LOG_ZERO_GUARD: number = 5.9604645e-8

// PAD BEGIN //
// Zero padding applied to the raw audio before framing for FFT caculations.
// PAD num of left-padding is applied once per audio stream, at the beginning.
// This is for doing 'centered framing' for the first FFT frame. Otherwise the first few raw audio samples
// would lie near the left edge of the Hann window,mwhere the window coefficients are close to zero,
// strongly attenuating their contribution to the resulting spectrum.

export const PAD = N_FFT >> 1 // 256

// PAD END //

// OFFSET BEGIN //
// We take WIN_LENGTH real audio samples, multiply them by a WIN_LENGTH point Hann window
// and place the result in the middle of an FFT buffer of N_FFT num. of elements.
// OFF tells us where the real WIN_LENGTH-sample window begins inside the FFT buffer.

export const OFF = (N_FFT - WIN_LENGTH) >> 1 // 56

// So, a 512-point FFT buffer would be:
// | 56 zeros | 400 windowed audio samples | 56 zeros |
// 0         OFF                 (WIN_LENGTH + OFF)  N_FFT
// OFFSET END //

// A streaming mel frame is stable once the right edge of the real Hann window
// is available. Currently the WIN_LENGTH window at OFF
// inside the N_FFT frame, so raw window is:
// [f * HOP_LENGTH - PAD + OFF, f * HOP_LENGTH - PAD + OFF + WIN_LENGTH)
export const STREAMING_RIGHT_CONTEXT: number = OFF + WIN_LENGTH - PAD

//////////////// END: Shared audio frontend for both English only and multi-lingual Nemotron //////////////////////////////

export type NemotronModelType = 'english' | 'multilingual'

// model specific constants

const NEMO_CHUNK = 56 // new mel frames per step (560 ms @ 10 ms hop)
const NEMO_PRECACHE = 9 // pre-encode mel cache frames prepended

export const COMMON_NEMOTRON_CONFIG = {
  NEMO_CHUNK,
  NEMO_PRECACHE,
  NEMO_FRAMES: NEMO_PRECACHE + NEMO_CHUNK, // 65: fixed encoder input length
  NEMO_MAX_SYM: 10,
  NEMO_NUM_ENCODER_LAYERS: 24,
  NEMO_HIDDEN_DIM: 1024,
  NEMO_DECODER_LSTM_DIM: 640,
  // its num of frames in the left context of encoder's conv blocks'
  // conv_kernel_size = 9, so causal context = kernel_size - 1 = 8.
  NEMO_CONV_CONTEXT: 8,
  // On finish, append this many chunks of silence so the cache-aware encoder
  // and RNN-T decoder flush the final partial chunk — without it the trailing
  // word(s) can be dropped to right-context / emission lag. The silence frames
  // decode to RNN-T blanks, so the transcript is unaffected.
  SILENCE_FLUSH_CHUNKS: 3,
} as const

export const ENGLISH_NEMOTRON_CONFIG = {
  ...COMMON_NEMOTRON_CONFIG,
  NEMO_BLANK: 1024,
  NEMO_VOCAB: 1025, // 1024 + blank
  // note that this is different from left context at mel level;
  // its num of frames in the left context of encoder's attention blocks'
  NEMO_LEFT_CONTEXT: 70,
} as const

export const MULTILINGUAL_NEMOTRON_CONFIG = {
  ...COMMON_NEMOTRON_CONFIG,
  NEMO_BLANK: 13087,
  NEMO_VOCAB: 13088,
  NEMO_LEFT_CONTEXT: 56,
} as const

// supported languages

export const ENGLISH_SUPPORTED_LANGUAGES = [
  'en-US',
  'en-GB',
] as const

export const MULTILINGUAL_SUPPORTED_LANGUAGES = [
  'es-ES',
  'es-US',
  'hi-IN',
  'fr-FR',
  'de-DE',
  'ja-JP',
  // ...
] as const

export type SupportedLanguage =
  | typeof ENGLISH_SUPPORTED_LANGUAGES[number]
  | typeof MULTILINGUAL_SUPPORTED_LANGUAGES[number]
