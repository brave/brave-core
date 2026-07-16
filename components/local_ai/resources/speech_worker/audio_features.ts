// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as config from './configs'
import type { FftSize, FftState } from './configs'

export interface StreamingMelFrontendDebugState {
  rawSamplesReceived: number
  rawBaseSample: number
  rawBufferedSamples: number

  melFrameBase: number
  melBufferedFrames: number
  nextMelFrame: number
  nextChunkFrame: number

  stableBacklogFrames: number
  chunksReady: number
  estimatedStableBacklogMs: number
}

// standard symmetric Hann window; following NeMo's implementation
export function hannWindow(): Float32Array {
  return Float32Array.from(
    { length: config.WIN_LENGTH },
    (_, i) => 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (config.WIN_LENGTH - 1)),
  )
}

// Initializes reusable state and working buffers for a radix-2
// Cooley–Tukey FFT, including a bit-reversal lookup table.
export function initFftPower(n: FftSize): FftState {
  if ((n & (n - 1)) !== 0) {
    throw new Error(`N_FFT must be a power of two, got ${n}`)
  }

  const bits = Math.log2(n)
  const bitRev = new Uint16Array(n)

  for (let i = 0; i < n; i++) {
    let x = i
    let y = 0

    for (let b = 0; b < bits; b++) {
      y = (y << 1) | (x & 1)
      x >>= 1
    }

    bitRev[i] = y
  }

  return {
    n,
    bitRev,
    real: new Float32Array(n),
    imag: new Float32Array(n),
    power: new Float32Array(n / 2 + 1),
    frame: new Float32Array(n),
  }
}

// Compute power spectrum from a real-valued N_FFT frame.
function fftPower(frame: Float32Array, fft: FftState): Float32Array {
  const n = fft.n
  const real = fft.real
  const imag = fft.imag
  const bitRev = fft.bitRev

  // Bit-reversed copy of real input.
  // frame is expected to have length N_FFT.
  for (let i = 0; i < n; i++) {
    real[i] = frame[bitRev[i]]
    imag[i] = 0.0
  }

  // Iterative radix-2 Cooley-Tukey FFT.
  // This matches the forward transform:
  // X[k] = sum_n x[n] * exp(-j * 2πkn/N)
  for (let len = 2; len <= n; len <<= 1) {
    const halfLen = len >> 1
    const angle = (-2.0 * Math.PI) / len
    const wLenReal = Math.cos(angle)
    const wLenImag = Math.sin(angle)

    for (let i = 0; i < n; i += len) {
      let wReal = 1.0
      let wImag = 0.0

      for (let j = 0; j < halfLen; j++) {
        const evenIndex = i + j
        const oddIndex = evenIndex + halfLen

        const oddReal = real[oddIndex]
        const oddImag = imag[oddIndex]

        const vReal = oddReal * wReal - oddImag * wImag
        const vImag = oddReal * wImag + oddImag * wReal

        const uReal = real[evenIndex]
        const uImag = imag[evenIndex]

        real[evenIndex] = uReal + vReal
        imag[evenIndex] = uImag + vImag

        real[oddIndex] = uReal - vReal
        imag[oddIndex] = uImag - vImag

        const nextWReal = wReal * wLenReal - wImag * wLenImag
        const nextWImag = wReal * wLenImag + wImag * wLenReal

        wReal = nextWReal
        wImag = nextWImag
      }
    }
  }

  const power = fft.power
  const bins = n / 2 + 1

  for (let k = 0; k < bins; k++) {
    power[k] = real[k] * real[k] + imag[k] * imag[k]
  }

  return power
}

export class StreamingMelFrontend {
  private rawAudio: number[] = []
  private rawBaseSample = 0
  private rawSamplesReceived = 0

  private melFrames: Float32Array[] = []
  private melFrameBase = 0
  private nextMelFrame = 0
  private nextChunkFrame = 0

  constructor(
    private readonly fbank: Float32Array,
    private readonly hann: Float32Array,
    private readonly fftPre: FftState,
  ) {}

  appendAudioSamples(samples: Float32Array): void {
    for (let i = 0; i < samples.length; i++) {
      this.rawAudio.push(samples[i])
    }

    this.rawSamplesReceived += samples.length
    this.appendStableMelFrames()
    this.trimOldBuffers()
  }

  hasFullChunk(): boolean {
    return this.nextMelFrame - this.nextChunkFrame >= config.NEMO_CHUNK
  }

  makeNextEncoderInput(): Float32Array {
    if (!this.hasFullChunk()) {
      throw new Error('Not enough stable mel frames for another encoder chunk')
    }

    const chunk = new Float32Array(config.N_MELS * config.NEMO_FRAMES)
    const mainStart = this.nextChunkFrame

    // Left pre-encode mel cache. For the first chunk, this leaves the first
    // NEMO_PRECACHE columns as zeros.
    const cacheStart = Math.max(0, mainStart - config.NEMO_PRECACHE)
    const cacheFrames = mainStart - cacheStart
    const cacheOffset = config.NEMO_PRECACHE - cacheFrames

    for (let f = 0; f < cacheFrames; f++) {
      this.copyMelFrameToEncoderChunk(
        chunk,
        cacheOffset + f,
        this.getMelFrame(cacheStart + f),
      )
    }

    for (let f = 0; f < config.NEMO_CHUNK; f++) {
      this.copyMelFrameToEncoderChunk(
        chunk,
        config.NEMO_PRECACHE + f,
        this.getMelFrame(mainStart + f),
      )
    }

    return chunk
  }

  consumeChunk(): void {
    this.nextChunkFrame += config.NEMO_CHUNK
    this.trimOldBuffers()
  }

  // Zero every buffer holding audio or acoustic features and reset the
  // frontend to empty. Called at session end so the utterance does not
  // persist in the JS heap until garbage collection.
  wipe(): void {
    this.rawAudio.fill(0)
    this.rawAudio.length = 0
    for (const frame of this.melFrames) {
      frame.fill(0)
    }
    this.melFrames.length = 0
  }

  debugState(sampleRateHz: number): StreamingMelFrontendDebugState {
    const stableBacklogFrames = this.nextMelFrame - this.nextChunkFrame

    return {
      rawSamplesReceived: this.rawSamplesReceived,
      rawBaseSample: this.rawBaseSample,
      rawBufferedSamples: this.rawAudio.length,

      melFrameBase: this.melFrameBase,
      melBufferedFrames: this.melFrames.length,
      nextMelFrame: this.nextMelFrame,
      nextChunkFrame: this.nextChunkFrame,

      stableBacklogFrames,
      chunksReady: Math.floor(stableBacklogFrames / config.NEMO_CHUNK),
      estimatedStableBacklogMs:
        (stableBacklogFrames * config.HOP_LENGTH * 1000) / sampleRateHz,
    }
  }

  private appendStableMelFrames(): void {
    // By 'stable' mel frame, we mean a mel frame whose required raw audio
    // samples are already available, including its right-side context.
    const stableFrames = this.stableMelFrameCountForSamples(
      this.rawSamplesReceived,
    )

    while (this.nextMelFrame < stableFrames) {
      this.melFrames.push(this.computeStreamingMelFrame(this.nextMelFrame))
      this.nextMelFrame += 1
    }
  }

  private stableMelFrameCountForSamples(sampleCount: number): number {
    const count =
      Math.floor(
        (sampleCount - config.STREAMING_RIGHT_CONTEXT) / config.HOP_LENGTH,
      ) + 1

    return Math.max(0, count)
  }

  private computeStreamingMelFrame(frameIndex: number): Float32Array {
    const frame = this.fftPre.frame
    frame.fill(0)

    // Match the full-buffer NeMo layout:
    // window is placed at OFF inside an N_FFT-sized frame, and the raw
    // window starts at frameIndex * HOP_LENGTH - PAD + OFF.
    const rawStart = frameIndex * config.HOP_LENGTH - config.PAD + config.OFF

    for (let i = 0; i < config.WIN_LENGTH; i++) {
      frame[config.OFF + i] =
        this.preemphasizedSampleAt(rawStart + i) * this.hann[i]
    }

    const power = fftPower(frame, this.fftPre)
    const nfFreq = config.N_FFT / 2 + 1
    const melFrame = new Float32Array(config.N_MELS)

    for (let m = 0; m < config.N_MELS; m++) {
      let acc = 0
      const fb = m * nfFreq

      for (let k = 0; k < nfFreq; k++) {
        acc += this.fbank[fb + k] * power[k]
      }

      melFrame[m] = Math.log(acc + config.LOG_ZERO_GUARD)
    }

    return melFrame
  }

  private rawSampleAt(absSampleIndex: number): number {
    if (absSampleIndex < 0) {
      throw new Error(
        `rawSampleAt called with negative index ${absSampleIndex}`,
      )
    }

    // Should only happen if the caller intentionally requests samples beyond
    // the real stream. During normal streaming, stable-frame gating prevents it.
    if (absSampleIndex >= this.rawSamplesReceived) {
      return 0.0
    }

    const rel = absSampleIndex - this.rawBaseSample

    if (rel < 0 || rel >= this.rawAudio.length) {
      throw new Error(
        `Streaming frontend lost raw sample ${absSampleIndex}; `
          + `base=${this.rawBaseSample}, len=${this.rawAudio.length}`,
      )
    }

    return this.rawAudio[rel]
  }

  private preemphasizedOriginalSampleAt(absSampleIndex: number): number {
    const x = this.rawSampleAt(absSampleIndex)

    if (absSampleIndex === 0) {
      return x
    }

    return x - config.PREEMPH * this.rawSampleAt(absSampleIndex - 1)
  }

  private preemphasizedSampleAt(absSampleIndex: number): number {
    // Match reflect-padding semantics at the start of the stream after
    // pre-emphasis. For example, -1 maps to preemphasized sample 1.
    if (absSampleIndex < 0) {
      return this.preemphasizedOriginalSampleAt(-absSampleIndex)
    }

    return this.preemphasizedOriginalSampleAt(absSampleIndex)
  }

  private getMelFrame(absFrameIndex: number): Float32Array {
    const rel = absFrameIndex - this.melFrameBase

    if (rel < 0 || rel >= this.melFrames.length) {
      throw new Error(
        `Streaming frontend lost mel frame ${absFrameIndex}; `
          + `base=${this.melFrameBase}, len=${this.melFrames.length}`,
      )
    }

    return this.melFrames[rel]
  }

  private copyMelFrameToEncoderChunk(
    chunk: Float32Array,
    chunkFrameOffset: number,
    melFrame: Float32Array,
  ): void {
    for (let m = 0; m < config.N_MELS; m++) {
      chunk[m * config.NEMO_FRAMES + chunkFrameOffset] = melFrame[m]
    }
  }

  private trimOldBuffers(): void {
    // Keep only mel frames needed for the next encoder input's 9-frame
    // pre-encode cache.
    const keepFromFrame = Math.max(
      0,
      this.nextChunkFrame - config.NEMO_PRECACHE,
    )

    const dropFrames = keepFromFrame - this.melFrameBase

    if (dropFrames > 0) {
      // Zero each dropped mel frame before releasing it. Mel features are a
      // near complete acoustic representation, so wiping them here bounds the
      // resident acoustics to the live window rather than leaving dropped
      // frames in the JS heap until garbage collection.
      for (let i = 0; i < dropFrames; i++) {
        this.melFrames[i].fill(0)
      }
      this.melFrames.splice(0, dropFrames)
      this.melFrameBase = keepFromFrame
    }

    // Keep enough raw audio to compute the next mel frame, including one
    // previous sample for pre-emphasis continuity.
    const firstRawNeeded = Math.max(
      0,
      this.nextMelFrame * config.HOP_LENGTH - config.PAD + config.OFF - 1,
    )

    const dropSamples = firstRawNeeded - this.rawBaseSample

    if (dropSamples > 0) {
      this.rawAudio.splice(0, dropSamples)
      this.rawBaseSample += dropSamples
    }
  }
}
