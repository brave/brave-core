// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import { useLocalSpeech } from './use_local_speech'

const original = window.AudioContext
let playback: {
  buffer: AudioBuffer | null
  connect: jest.Mock
  disconnect: jest.Mock
  start: jest.Mock
  stop: jest.Mock
  onended: (() => void) | null
}
let copyToChannel: jest.Mock
let createBuffer: jest.Mock

beforeEach(() => {
  playback = {
    buffer: null,
    connect: jest.fn(),
    disconnect: jest.fn(),
    start: jest.fn(),
    stop: jest.fn(),
    onended: null,
  }
  copyToChannel = jest.fn()
  createBuffer = jest.fn(() => ({ copyToChannel }))
  window.AudioContext = jest.fn(() => ({
    resume: jest.fn().mockResolvedValue(undefined),
    close: jest.fn().mockResolvedValue(undefined),
    createBuffer,
    createBufferSource: jest.fn(() => playback),
    destination: {},
  })) as unknown as typeof AudioContext
})

afterEach(() => {
  window.AudioContext = original
})

function response() {
  const bytes = new Uint8Array(8)
  const data = new DataView(bytes.buffer)
  data.setFloat32(0, 0.25, true)
  data.setFloat32(4, -0.5, true)
  return { audioData: Array.from(bytes), sampleRate: 24000, error: '' }
}

it('decodes mono float32 audio and waits until playback ends', async () => {
  const handler = {
    synthesizeSpeech: jest.fn().mockResolvedValue(response()),
    cancelSpeech: jest.fn(),
  }
  const { result } = renderHook(() => useLocalSpeech(handler))
  let completed = false
  let speech!: Promise<void>
  await act(async () => {
    result.current.prepare()
    speech = result.current.speak('hello Leo').then(() => {
      completed = true
    })
    await Promise.resolve()
  })
  expect(handler.synthesizeSpeech).toHaveBeenCalledWith('hello Leo')
  expect(createBuffer).toHaveBeenCalledWith(1, 2, 24000)
  expect(copyToChannel).toHaveBeenCalledWith(new Float32Array([0.25, -0.5]), 0)
  expect(playback.start).toHaveBeenCalledTimes(1)
  expect(completed).toBe(false)
  await act(async () => {
    playback.onended!()
    await speech
  })
  expect(completed).toBe(true)
})

it('ignores a synthesis response that arrives after cancellation', async () => {
  let completeResponse!: (value: ReturnType<typeof response>) => void
  const handler = {
    synthesizeSpeech: jest.fn(
      () =>
        new Promise<ReturnType<typeof response>>((resolve) => {
          completeResponse = resolve
        }),
    ),
    cancelSpeech: jest.fn(),
  }
  const { result } = renderHook(() => useLocalSpeech(handler))
  let speech!: Promise<void>
  act(() => {
    speech = result.current.speak('stale reply')
  })
  act(() => result.current.cancel())
  await act(async () => {
    completeResponse(response())
    await speech
  })
  expect(playback.start).not.toHaveBeenCalled()
  expect(handler.cancelSpeech).toHaveBeenCalled()
})

it('reports local server errors without playing audio', async () => {
  const handler = {
    synthesizeSpeech: jest.fn().mockResolvedValue({
      audioData: [],
      sampleRate: 0,
      error: 'Local TTS request failed',
    }),
    cancelSpeech: jest.fn(),
  }
  const { result } = renderHook(() => useLocalSpeech(handler))
  await expect(result.current.speak('hello')).rejects.toThrow(
    'Local TTS request failed',
  )
  expect(playback.start).not.toHaveBeenCalled()
})
