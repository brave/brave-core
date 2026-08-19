// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// These tests check the numerical correctness and streaming invariance of the audio
// frontend, including Hann windowing, FFT power calculation, mel feature
// generation, and independence from input packet boundaries.

import { describe, expect, it } from '@jest/globals'

import {
  hannWindow,
  initFftPower,
  fftPower,
  StreamingMelFrontend,
} from './audio_features'

import * as config from './configs'
import type { FftSize } from './configs'

describe('FrontEndProcessing', () => {
  describe('hannWindow', () => {
    // Test to verify that -
    // (a) Hann window formula is correct
    // (b) Window is zero at beginning and ending
    // (c) Window is symmetric
    it('matches the symmetric Hann definition', () => {
      const hann = hannWindow()

      expect(hann.length).toBe(config.WIN_LENGTH)

      for (let i = 0; i < config.WIN_LENGTH; i++) {
        const expected =
          0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (config.WIN_LENGTH - 1))

        expect(hann[i]).toBeCloseTo(expected, 6) // 6 decimal places precision
      }
    })

    it('is symmetric and zero at both ends', () => {
      const hann = hannWindow()

      expect(hann[0]).toBeCloseTo(0)
      expect(hann[hann.length - 1]).toBeCloseTo(0)

      for (let i = 0; i < hann.length; i++) {
        expect(hann[i]).toBeCloseTo(hann[hann.length - 1 - i], 6)
      }
    })
  })
  describe('FFTCalculations', () => {
    // Test to verify that -
    // (a) FFT of an impulse signal is correctly calculated
    // (b) same about a constant signal
    // (c) FFT puts energy in the correct freq. bin
    it('computes the power spectrum of an impulse', () => {
      const fft = initFftPower(config.N_FFT as FftSize)

      // impulse has 1 at index 0, and 0 at all other indices
      fft.frame.fill(0)
      fft.frame[0] = 1

      const power = fftPower(fft)

      // FFT of impulse is 1 at all freq points
      expect(power.length).toBe(config.N_FFT / 2 + 1)

      for (const p of power) {
        expect(p).toBeCloseTo(1, 5)
      }
    })

    it('computes the power spectrum of a constant signal', () => {
      const fft = initFftPower(config.N_FFT as FftSize)

      // constant signal is 1 for all indices
      fft.frame.fill(1)

      const power = fftPower(fft)

      // FFT of constant is non-zero only at freq 0, the DC component
      expect(power[0]).toBeCloseTo(config.N_FFT * config.N_FFT, 2)

      for (let k = 1; k < power.length; k++) {
        expect(power[k]).toBeCloseTo(0, 2)
      }
    })

    it('puts a bin-centred sinusoid in the expected FFT bin', () => {
      const fft = initFftPower(config.N_FFT as FftSize)
      const bin = 7

      for (let n = 0; n < config.N_FFT; n++) {
        fft.frame[n] = Math.sin((2 * Math.PI * bin * n) / config.N_FFT)
      }

      const power = fftPower(fft)

      let maxBin = 0

      for (let k = 1; k < power.length; k++) {
        if (power[k] > power[maxBin]) {
          maxBin = k
        }
      }

      expect(maxBin).toBe(bin)
    })
  })
  describe('MelFrontEnd', () => {
    // Test to verify that -
    // (a) mel features must depend on the audio, not on the Web Speech packet boundaries
    it('produces the same mel features regardless of audio packet boundaries', () => {
      const fbank = createTestMelFilterbank()
      const makeFrontend = () =>
        new StreamingMelFrontend(
          fbank,
          hannWindow(),
          initFftPower(config.N_FFT as FftSize),
          config.TARGET_SAMPLE_RATE,
        )

      const frontendAllAtOnce = makeFrontend()
      const frontendChunked = makeFrontend()

      // Enough audio to produce at least one complete encoder chunk.
      const sampleCount =
        config.NEMO_CHUNK * config.HOP_LENGTH + config.STREAMING_RIGHT_CONTEXT

      // Deterministic non-trivial signal.
      const audio = new Float32Array(sampleCount)

      for (let i = 0; i < audio.length; i++) {
        audio[i] =
          0.2 * Math.sin((2 * Math.PI * 440 * i) / config.TARGET_SAMPLE_RATE)
          + 0.1 * Math.sin((2 * Math.PI * 1000 * i) / config.TARGET_SAMPLE_RATE)
      }

      // Frontend A gets the entire stream in one call.
      frontendAllAtOnce.appendAudioSamples(audio)

      // Frontend B gets exactly the same samples, but with arbitrary
      // packet boundaries.
      const packetSizes = [17, 101, 503, 37, 1000, 211]

      let offset = 0
      let packet = 0

      while (offset < audio.length) {
        const size = packetSizes[packet % packetSizes.length]
        const end = Math.min(offset + size, audio.length)

        frontendChunked.appendAudioSamples(audio.subarray(offset, end))

        offset = end
        packet++
      }

      expect(frontendAllAtOnce.hasFullChunk()).toBe(true)
      expect(frontendChunked.hasFullChunk()).toBe(true)

      const expected = frontendAllAtOnce.makeNextEncoderInput()

      const actual = frontendChunked.makeNextEncoderInput()

      expect(actual.length).toBe(expected.length)

      for (let i = 0; i < expected.length; i++) {
        expect(actual[i]).toBeCloseTo(expected[i], 6)
      }
    })
  })
})

// Helper functions to compute a Slaney-style mel filterbank

// function to convert freq. from hz to mel scale
function hzToMelSlaney(hz: number) {
  const fSp = 200.0 / 3.0
  const minLogHz = 1000.0
  const minLogMel = minLogHz / fSp
  const logStep = 0.06875177742094912
  return hz < minLogHz
    ? hz / fSp
    : minLogMel + Math.log(hz / minLogHz) / logStep
}

// function to convert freq. from mel scale to hz
function melToHzSlaney(mel: number) {
  const fSp = 200.0 / 3.0
  const minLogHz = 1000.0
  const minLogMel = minLogHz / fSp
  const logStep = 0.06875177742094912
  return mel < minLogMel
    ? mel * fSp
    : minLogHz * Math.exp((mel - minLogMel) * logStep)
}

function createMelFilterbank() {
  const freqBins = config.N_FFT / 2 + 1
  const filterbank = Array.from(
    { length: config.N_MELS },
    () => new Float32Array(freqBins),
  )
  const fmax = config.TARGET_SAMPLE_RATE / 2
  const melMin = hzToMelSlaney(0)
  const melMax = hzToMelSlaney(fmax)
  const melPoints = []

  for (let i = 0; i <= config.N_MELS + 1; i++) {
    melPoints.push(
      melToHzSlaney(melMin + ((melMax - melMin) * i) / (config.N_MELS + 1)),
    )
  }

  const fftFreqs = Array.from(
    { length: freqBins },
    (_, i) => (i * config.TARGET_SAMPLE_RATE) / config.N_FFT,
  )
  const fdiff = melPoints.slice(1).map((v, i) => v - melPoints[i])

  for (let i = 0; i < config.N_MELS; i++) {
    const enorm = 2.0 / (melPoints[i + 2] - melPoints[i])
    for (let k = 0; k < freqBins; k++) {
      const lower = (fftFreqs[k] - melPoints[i]) / fdiff[i]
      const upper = (melPoints[i + 2] - fftFreqs[k]) / fdiff[i + 1]
      filterbank[i][k] = Math.max(0, Math.min(lower, upper)) * enorm
    }
  }

  return filterbank
}

// Flatten it for structural compatibility with the filterbank.bin used in production code
function createTestMelFilterbank(): Float32Array {
  const filterbank2d = createMelFilterbank()
  const freqBins = config.N_FFT / 2 + 1

  const flat = new Float32Array(config.N_MELS * freqBins)

  for (let m = 0; m < config.N_MELS; m++) {
    flat.set(filterbank2d[m], m * freqBins)
  }

  return flat
}
