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

////////// PAD BEGIN ////////////
// Zero padding applied to the raw audio before framing for FFT caculations.
// PAD num of left-padding is applied once per audio stream, at the beginning.
// This is for doing 'centered framing' for the first FFT frame. Otherwise the first few raw audio samples
// would lie near the left edge of the Hann window,mwhere the window coefficients are close to zero,
// strongly attenuating their contribution to the resulting spectrum.

export const PAD = N_FFT >> 1 // 256

////////// PAD END ////////////

////////// OFFSET BEGIN ////////////
// We take WIN_LENGTH real audio samples, multiply them by a WIN_LENGTH point Hann window
// and place the result in the middle of an FFT buffer of N_FFT num. of elements.
// OFF tells us where the real WIN_LENGTH-sample window begins inside the FFT buffer.

export const OFF = (N_FFT - WIN_LENGTH) >> 1 // 56

// So, a 512-point FFT buffer would be:
// | 56 zeros | 400 windowed audio samples | 56 zeros |
// 0         OFF                 (WIN_LENGTH + OFF)  N_FFT
////////// OFFSET END ////////////

// A streaming mel frame is stable once the right edge of the real Hann window
// is available. Currently the WIN_LENGTH window at OFF
// inside the N_FFT frame, so raw window is:
// [f * HOP_LENGTH - PAD + OFF, f * HOP_LENGTH - PAD + OFF + WIN_LENGTH)
export const STREAMING_RIGHT_CONTEXT: number = OFF + WIN_LENGTH - PAD

//////////////// END: Shared audio frontend for both English only and multi-lingual Nemotron //////////////////////////////

export type NemotronModelType = 'english' | 'multilingual'

// model specific constants
export const NEMO_CHUNK: number = 56 // new mel frames per step (560 ms @ 10 ms hop)
export const NEMO_PRECACHE: number = 9 // pre-encode mel cache frames prepended

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

// The locales that the worker serves, with the model and the prompt ID of
// each. One row for each row of `kLocales` in
// brave/chromium_src/components/soda/constants.cc, in the same order. The
// browser resolves each page tag to one of these tags, so the worker gets no
// other tag. The prompt IDs come from `prompt_dictionary` in the multilingual
// model's config.json.
export const LOCALES = {
  'en-US': { model: 'english', promptId: null },
  'ar': { model: 'multilingual', promptId: 7 },
  'de-DE': { model: 'multilingual', promptId: 9 },
  'es-ES': { model: 'multilingual', promptId: 2 },
  'es-US': { model: 'multilingual', promptId: 3 },
  'fr-FR': { model: 'multilingual', promptId: 8 },
  'fr-CA': { model: 'multilingual', promptId: 100 },
  'hi-IN': { model: 'multilingual', promptId: 6 },
  'it-IT': { model: 'multilingual', promptId: 15 },
  'ja-JP': { model: 'multilingual', promptId: 10 },
  'ko-KR': { model: 'multilingual', promptId: 14 },
  'nl-NL': { model: 'multilingual', promptId: 16 },
  'pt-BR': { model: 'multilingual', promptId: 12 },
  'pt-PT': { model: 'multilingual', promptId: 13 },
  'ru-RU': { model: 'multilingual', promptId: 11 },
  'tr-TR': { model: 'multilingual', promptId: 18 },
  'uk-UA': { model: 'multilingual', promptId: 19 },
  'vi-VN': { model: 'multilingual', promptId: 33 },
} as const satisfies Record<
  string,
  | { model: 'english'; promptId: null }
  | { model: 'multilingual'; promptId: number }
>

// A language tag token, such as <es-ES>. The multilingual model can emit one
// after the terminal punctuation. The English vocab has no such tokens.
export const LANGUAGE_TAG_TOKEN = /^<[a-z]{2}-[A-Z]{2}>$/

// Silence fed before the first real sample. The exported ONNX encoder graph
// drops the first 2 time frames of its pre-encode output. This happens inside
// the graph, not in our streaming code. It shifts the output grid off the
// start of the audio, so a short word that begins within the first ~80 ms of
// the stream can be lost, whatever the audio source. NVIDIA's Nemotron
// streaming release notes recommend this same lead-in.
export const LEADING_SILENCE_MS: number = 80
