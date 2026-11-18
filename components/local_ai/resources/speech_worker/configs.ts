// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Nemotron 0.6B INT4 — cache-aware streaming RNN-T.
// Each encoder step advances by 560 ms: 56 new mel frames plus 9
// pre-encode left-context frames. Conformer caches are carried between
// steps, and fixed input/cache shapes keep the encoder cost constant per step.

// BEGIN: Shared audio frontend for both English only and multi-lingual Nemotron //

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

// END: Shared audio frontend for both English only and multi-lingual Nemotron //

export type NemotronModelType = 'english' | 'multilingual'

// model specific constants
export const NEMO_CHUNK: number = 56 // new mel frames per step (560 ms @ 10 ms hop)
export const NEMO_PRECACHE: number = 9 // pre-encode mel cache frames prepended

export const NEMO_MAX_SYM: number = 10
export const NEMO_FRAMES: number = NEMO_PRECACHE + NEMO_CHUNK // 65: fixed encoder input length
export const NEMO_NUM_ENCODER_LAYERS: number = 24
export const NEMO_HIDDEN_DIM: number = 1024

// its num of frames in the left context of encoder's conv blocks'
export const NEMO_CONV_CONTEXT: number = 8
export const NEMO_DECODER_LSTM_DIM: number = 640

export interface NemotronModelConfig {
  NEMO_BLANK: number
  NEMO_VOCAB: number
  // note that this is different from left context at mel level;
  // its num of frames in the left context of encoder's attention blocks'
  NEMO_LEFT_CONTEXT: number
}

export const MODEL_CONFIGS: Record<NemotronModelType, NemotronModelConfig> = {
  english: {
    NEMO_BLANK: 1024,
    NEMO_VOCAB: 1025, // 1024 + blank
    NEMO_LEFT_CONTEXT: 70,
  },
  multilingual: {
    NEMO_BLANK: 13087,
    NEMO_VOCAB: 13088, // 13087 + blank
    NEMO_LEFT_CONTEXT: 56,
  },
}

export type NemotronLocale =
  | { model: 'english'; promptId: null }
  | { model: 'multilingual'; promptId: number }

// supported languages

// The locales that the worker serves, with the prompt ID of each in the
// multilingual model. One row for each row of `kLocales` in
// components/local_ai/core/on_device_speech_locales.cc, in the same order. The
// browser resolves each page tag to one of these tags, so the worker gets no
// other tag. The prompt IDs come from `prompt_dictionary` in the multilingual
// model's config.json. The multilingual model serves every locale. The English
// model serves only en-US, with no prompt.
export const LOCALES = {
  'en-US': 0,
  'ar': 7,
  'de-DE': 9,
  'es-ES': 2,
  'es-US': 3,
  'fr-FR': 8,
  'fr-CA': 100,
  'hi-IN': 6,
  'it-IT': 15,
  'ja-JP': 10,
  'ko-KR': 14,
  'nl-NL': 16,
  'pt-BR': 12,
  'pt-PT': 13,
  'ru-RU': 11,
  'tr-TR': 18,
  'uk-UA': 19,
  'vi-VN': 33,
} as const satisfies Record<string, number>

// A language tag token, such as <es-ES>. The multilingual model can emit one
// after the terminal punctuation. The English vocab has no such tokens.
export const LANGUAGE_TAG_TOKEN = /^<[a-z]{2}-[A-Z]{2}>$/

// On finish, append this many chunks of silence so the cache-aware encoder
// and RNN-T decoder flush the final partial chunk — without it the trailing
// word(s) can be dropped to right-context / emission lag. The silence frames
// decode to RNN-T blanks, so the transcript is unaffected.
export const SILENCE_FLUSH_CHUNKS: number = 3

// Silence fed before the first real sample. The exported ONNX encoder graph
// drops the first 2 time frames of its pre-encode output. This happens inside
// the graph, not in our streaming code. It shifts the output grid off the
// start of the audio, so a short word that begins within the first ~80 ms of
// the stream can be lost, whatever the audio source. NVIDIA's Nemotron
// streaming release notes recommend this same lead-in.
export const LEADING_SILENCE_MS: number = 80
