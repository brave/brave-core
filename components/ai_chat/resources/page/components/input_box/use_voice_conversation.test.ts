// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import { useVoiceConversation } from './use_voice_conversation'

type Recognition = InstanceType<NonNullable<typeof window.SpeechRecognition>>

let original: typeof window.SpeechRecognition
let instances: Recognition[]

beforeEach(() => {
  original = window.SpeechRecognition
  instances = []
  jest.useFakeTimers()
  window.SpeechRecognition = jest.fn().mockImplementation(() => {
    const instance: Recognition = {
      lang: '',
      continuous: false,
      interimResults: false,
      onresult: null,
      onerror: null,
      onend: null,
      start: jest.fn(),
      stop: jest.fn(),
      abort: jest.fn(),
    }
    instances.push(instance)
    return instance
  })
})

afterEach(() => {
  window.SpeechRecognition = original
  jest.useRealTimers()
})

it('waits for voice playback before reopening the microphone', async () => {
  let finish!: () => void
  const speech = {
    speak: jest.fn(
      () =>
        new Promise<void>((resolve) => {
          finish = resolve
        }),
    ),
    cancel: jest.fn(),
  }
  const { result, rerender } = renderHook(
    ({ busy }) => useVoiceConversation(busy, jest.fn(), jest.fn(), speech),
    { initialProps: { busy: false } },
  )
  act(() => result.current.start())
  act(() => {
    instances[0].onresult!(transcript('a question'))
    instances[0].onend!()
  })
  rerender({ busy: true })
  rerender({ busy: false })
  expect(speech.speak).toHaveBeenCalledTimes(1)
  expect(result.current.isWaiting).toBe(true)
  act(() => jest.advanceTimersByTime(1000))
  expect(instances).toHaveLength(1)
  await act(async () => finish())
  act(() => jest.advanceTimersByTime(250))
  expect(instances).toHaveLength(2)
})

it('cancels playback and ignores its completion after voice mode stops', async () => {
  let finish!: () => void
  const speech = {
    speak: jest.fn(
      () =>
        new Promise<void>((resolve) => {
          finish = resolve
        }),
    ),
    cancel: jest.fn(),
  }
  const { result, rerender } = renderHook(
    ({ busy }) => useVoiceConversation(busy, jest.fn(), jest.fn(), speech),
    { initialProps: { busy: false } },
  )
  act(() => result.current.start())
  act(() => {
    instances[0].onresult!(transcript('a question'))
    instances[0].onend!()
  })
  rerender({ busy: true })
  rerender({ busy: false })
  act(() => result.current.stop())
  expect(speech.cancel).toHaveBeenCalled()
  await act(async () => finish())
  act(() => jest.advanceTimersByTime(1000))
  expect(result.current.isActive).toBe(false)
  expect(instances).toHaveLength(1)
})

function transcript(text: string) {
  return { results: [{ 0: { transcript: text } }] }
}

it('keeps listening through short pauses and resets the send countdown', () => {
  const onUtterance = jest.fn()
  const { result } = renderHook(() =>
    useVoiceConversation(false, jest.fn(), onUtterance),
  )
  act(() => result.current.start())
  act(() => instances[0].onresult!(transcript('tell me')))
  act(() => jest.advanceTimersByTime(1500))
  expect(instances[0].stop).not.toHaveBeenCalled()
  act(() => instances[0].onresult!(transcript('tell me about the moon')))
  act(() => jest.advanceTimersByTime(1999))
  expect(instances[0].stop).not.toHaveBeenCalled()
  expect(onUtterance).not.toHaveBeenCalled()
  act(() => jest.advanceTimersByTime(1))
  expect(instances[0].stop).toHaveBeenCalledTimes(1)
  act(() => instances[0].onend!())
  expect(onUtterance).toHaveBeenCalledWith('tell me about the moon')
})

it('cancels the send countdown when voice mode is stopped', () => {
  const onUtterance = jest.fn()
  const { result } = renderHook(() =>
    useVoiceConversation(false, jest.fn(), onUtterance),
  )
  act(() => result.current.start())
  act(() => instances[0].onresult!(transcript('unfinished question')))
  act(() => result.current.stop())
  act(() => jest.advanceTimersByTime(3000))
  expect(instances[0].stop).not.toHaveBeenCalled()
  expect(onUtterance).not.toHaveBeenCalled()
})

it('sends when the recognizer repeats unchanged results during silence', () => {
  const onUtterance = jest.fn()
  const { result } = renderHook(() =>
    useVoiceConversation(false, jest.fn(), onUtterance),
  )
  act(() => result.current.start())
  act(() => instances[0].onresult!(transcript('a complete question')))
  const instance = instances[0]
  for (let i = 0; i < 4; i++) {
    act(() => {
      jest.advanceTimersByTime(500)
      instance.onresult!(transcript('a complete question'))
    })
  }
  expect(instances[0].stop).toHaveBeenCalledTimes(1)
  // The final flush may still update the trailing words after stop().
  act(() => instances[0].onresult!(transcript('a complete question please')))
  act(() => jest.advanceTimersByTime(3000))
  expect(instances[0].stop).toHaveBeenCalledTimes(1)
  act(() => instances[0].onend!())
  expect(onUtterance).toHaveBeenCalledTimes(1)
  expect(onUtterance).toHaveBeenCalledWith('a complete question please')
})

it('sends the completed utterance once and resumes only after the response', () => {
  const onTranscript = jest.fn()
  const onUtterance = jest.fn()
  const { result, rerender } = renderHook(
    ({ busy }) => useVoiceConversation(busy, onTranscript, onUtterance),
    { initialProps: { busy: false } },
  )
  act(() => result.current.start())
  expect(instances[0].continuous).toBe(true)
  act(() => instances[0].onresult!(transcript('hello')))
  expect(onTranscript).toHaveBeenLastCalledWith('hello')
  expect(onUtterance).not.toHaveBeenCalled()
  act(() => instances[0].onresult!(transcript('hello Leo')))
  act(() => instances[0].onend!())
  expect(onUtterance).toHaveBeenCalledTimes(1)
  expect(onUtterance).toHaveBeenCalledWith('hello Leo')
  expect(result.current.isWaiting).toBe(true)
  act(() => jest.advanceTimersByTime(1000))
  expect(instances).toHaveLength(1)
  rerender({ busy: true })
  act(() => jest.advanceTimersByTime(1000))
  expect(instances).toHaveLength(1)
  rerender({ busy: false })
  act(() => jest.advanceTimersByTime(250))
  expect(instances).toHaveLength(2)
  act(() => instances[1].onresult!(transcript('next question')))
  act(() => instances[1].onend!())
  expect(onUtterance).toHaveBeenLastCalledWith('next question')
  expect(onUtterance).toHaveBeenCalledTimes(2)
})

it('retries empty sessions but stops on permission errors', () => {
  const onUtterance = jest.fn()
  const { result } = renderHook(() =>
    useVoiceConversation(false, jest.fn(), onUtterance),
  )
  act(() => result.current.start())
  act(() => {
    instances[0].onerror!({ error: 'no-speech' })
    instances[0].onend!()
  })
  act(() => jest.advanceTimersByTime(250))
  expect(instances).toHaveLength(2)
  expect(onUtterance).not.toHaveBeenCalled()
  act(() => instances[1].onerror!({ error: 'not-allowed' }))
  expect(result.current.isActive).toBe(false)
  act(() => jest.advanceTimersByTime(1000))
  expect(instances).toHaveLength(2)
})

it('cancels late results and pending restarts when stopped or unmounted', () => {
  const onUtterance = jest.fn()
  const { result, unmount } = renderHook(() =>
    useVoiceConversation(false, jest.fn(), onUtterance),
  )
  act(() => result.current.start())
  const queuedResult = instances[0].onresult!
  const queuedEnd = instances[0].onend!
  act(() => result.current.stop())
  act(() => {
    queuedResult(transcript('do not send'))
    queuedEnd()
    jest.advanceTimersByTime(1000)
  })
  expect(onUtterance).not.toHaveBeenCalled()
  expect(result.current.isActive).toBe(false)
  expect(instances).toHaveLength(1)
  act(() => result.current.start())
  act(() => instances[1].onend!())
  unmount()
  act(() => jest.advanceTimersByTime(1000))
  expect(instances).toHaveLength(2)
})

it('does not resume after ending voice mode while Leo is responding', () => {
  const onUtterance = jest.fn()
  const { result, rerender } = renderHook(
    ({ busy }) => useVoiceConversation(busy, jest.fn(), onUtterance),
    { initialProps: { busy: false } },
  )
  act(() => result.current.start())
  act(() => {
    instances[0].onresult!(transcript('a question'))
    instances[0].onend!()
  })
  rerender({ busy: true })
  act(() => result.current.stop())
  rerender({ busy: false })
  act(() => jest.advanceTimersByTime(1000))
  expect(instances).toHaveLength(1)
  expect(result.current.isActive).toBe(false)
})

it('handles a synchronous microphone startup failure without retrying', () => {
  const { result } = renderHook(() =>
    useVoiceConversation(false, jest.fn(), jest.fn()),
  )
  window.SpeechRecognition = jest.fn().mockImplementation(() => ({
    start: () => {
      throw new Error('microphone unavailable')
    },
    abort: jest.fn(),
  }))
  act(() => result.current.start())
  expect(result.current.error).toBe('audio-capture')
  expect(result.current.isActive).toBe(false)
  act(() => jest.advanceTimersByTime(1000))
  expect(window.SpeechRecognition).toHaveBeenCalledTimes(1)
})
