// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import { useSpeechRecognition } from './use_speech_recognition'

it('ignores queued transcripts after cancellation and allows a new session', () => {
  const original = window.SpeechRecognition
  const abort = jest.fn()
  const instances: InstanceType<
    NonNullable<typeof window.SpeechRecognition>
  >[] = []
  window.SpeechRecognition = jest.fn().mockImplementation(() => {
    const instance = {
      lang: '',
      continuous: false,
      interimResults: false,
      onresult: null,
      onerror: null,
      onend: null,
      start: jest.fn(),
      stop: jest.fn(),
      abort,
    }
    instances.push(instance)
    return instance
  })

  try {
    const onTranscript = jest.fn()
    const { result } = renderHook(() => useSpeechRecognition(onTranscript))
    act(() => result.current.start())
    const queuedResult = instances[0].onresult!
    const event = { results: [{ 0: { transcript: 'hello Leo' } }] }
    act(() => queuedResult(event))
    expect(onTranscript).toHaveBeenLastCalledWith('hello Leo')

    act(() => result.current.cancel())
    onTranscript.mockClear()
    act(() => queuedResult(event))
    expect(onTranscript).not.toHaveBeenCalled()
    expect(abort).toHaveBeenCalledTimes(1)
    expect(result.current.isListening).toBe(false)

    act(() => result.current.start())
    act(() => queuedResult(event))
    expect(onTranscript).not.toHaveBeenCalled()
    act(() => instances[1].onresult!(event))
    expect(onTranscript).toHaveBeenCalledWith('hello Leo')
  } finally {
    window.SpeechRecognition = original
  }
})
