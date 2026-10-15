// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import { useVoiceCapture } from './use_voice_capture'

const original = Object.getOwnPropertyDescriptor(navigator, 'mediaDevices')
const originalAudioContext = window.AudioContext
let level: number
let close: jest.Mock
let track: MediaStreamTrack
let microphone: MediaStream
let getUserMedia: jest.Mock
beforeEach(() => {
  jest.useFakeTimers()
  level = 0
  close = jest.fn().mockResolvedValue(undefined)
  window.AudioContext = jest.fn(() => ({
    resume: jest.fn().mockResolvedValue(undefined),
    close,
    createMediaStreamSource: () => ({ connect: jest.fn() }),
    createAnalyser: () => ({
      fftSize: 1024,
      connect: jest.fn(),
      getFloatTimeDomainData: (samples: Float32Array) => samples.fill(level),
    }),
    createGain: () => ({ gain: { value: 1 }, connect: jest.fn() }),
    destination: {},
  })) as unknown as typeof AudioContext
  track = {
    stop: jest.fn(),
    getSettings: () => ({ echoCancellation: true }),
    applyConstraints: jest.fn(),
  } as unknown as MediaStreamTrack
  microphone = {
    getTracks: () => [track],
    getAudioTracks: () => [track],
  } as unknown as MediaStream
  getUserMedia = jest.fn().mockResolvedValue(microphone)
  Object.defineProperty(navigator, 'mediaDevices', {
    configurable: true,
    value: { getUserMedia },
  })
})
afterEach(() => {
  window.AudioContext = originalAudioContext
  jest.useRealTimers()
  if (original) {
    Object.defineProperty(navigator, 'mediaDevices', original)
  } else {
    Reflect.deleteProperty(navigator, 'mediaDevices')
  }
})

it('selects echo cancellation at startup and accepts ordinary AEC fallback', async () => {
  const { result, unmount } = renderHook(() => useVoiceCapture())
  await expect(result.current.start()).resolves.toBe(track)
  expect(getUserMedia).toHaveBeenCalledWith({
    audio: {
      echoCancellation: true,
      noiseSuppression: true,
      autoGainControl: true,
      channelCount: 1,
      advanced: [
        { echoCancellation: { exact: 'all' } },
        { voiceIsolation: { exact: true } },
      ],
    },
    video: false,
  })
  expect(track.applyConstraints).not.toHaveBeenCalled()
  unmount()
  expect(track.stop).toHaveBeenCalledTimes(1)
})

it('accepts all-playback cancellation', async () => {
  track.getSettings = () =>
    ({ echoCancellation: 'all' }) as unknown as MediaTrackSettings
  const { result } = renderHook(() => useVoiceCapture())
  await expect(result.current.start()).resolves.toBe(track)
})

it('releases capture that resolves after voice mode stops', async () => {
  let resolve!: (stream: MediaStream) => void
  getUserMedia.mockImplementation(
    () =>
      new Promise<MediaStream>((done) => {
        resolve = done
      }),
  )
  const { result } = renderHook(() => useVoiceCapture())
  const pending = result.current.start()
  act(() => result.current.stop())
  resolve(microphone)
  await expect(pending).resolves.toBeNull()
  expect(track.stop).toHaveBeenCalledTimes(1)
})

it('rejects unprocessed audio and releases its microphone', async () => {
  track.getSettings = () => ({ echoCancellation: false })
  const { result } = renderHook(() => useVoiceCapture())
  await expect(result.current.start()).rejects.toThrow(
    'Echo cancellation is unavailable',
  )
  expect(track.stop).toHaveBeenCalledTimes(1)
})

it('detects sustained microphone activity before any transcript and tracks actual silence', async () => {
  const onSpeech = jest.fn()
  const { result } = renderHook(() => useVoiceCapture(onSpeech))
  await result.current.start()
  level = 0.05
  act(() => jest.advanceTimersByTime(100))
  expect(onSpeech).not.toHaveBeenCalled()
  act(() => jest.advanceTimersByTime(60))
  expect(onSpeech).toHaveBeenCalledTimes(1)
  act(() => jest.advanceTimersByTime(3000))
  expect(result.current.getSpeechIdleMs()).toBeLessThan(20)
  expect(onSpeech).toHaveBeenCalledTimes(1)
  level = 0
  act(() => jest.advanceTimersByTime(2100))
  expect(result.current.getSpeechIdleMs()).toBeGreaterThanOrEqual(2100)
  level = 0.05
  act(() => jest.advanceTimersByTime(160))
  expect(onSpeech).toHaveBeenCalledTimes(2)
  act(() => result.current.stop())
  expect(close).toHaveBeenCalledTimes(1)
  act(() => jest.advanceTimersByTime(1000))
  expect(onSpeech).toHaveBeenCalledTimes(2)
})
