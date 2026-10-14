// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { MAX_SPEECH_CHUNK_LENGTH, SpeechQueue } from './speech_queue'

it('speaks the first complete sentence before the reply finishes', async () => {
  const speak = jest.fn().mockResolvedValue(undefined)
  const queue = new SpeechQueue(speak)
  queue.append('First sentence. Still generating')
  await queue.wait()
  expect(speak.mock.calls).toEqual([['First sentence.']])
  queue.append('First sentence. Still generating another sentence.')
  await queue.wait()
  expect(speak.mock.calls).toEqual([
    ['First sentence.'],
    ['Still generating another sentence.'],
  ])
  queue.append(
    'First sentence. Still generating another sentence. Last words',
    true,
  )
  await queue.wait()
  expect(speak).toHaveBeenLastCalledWith('Last words')
})

it('holds incomplete sentences and does not repeat unchanged updates', async () => {
  const speak = jest.fn().mockResolvedValue(undefined)
  const queue = new SpeechQueue(speak)
  queue.append('Still thinking')
  await queue.wait()
  expect(speak).not.toHaveBeenCalled()
  queue.append('Still thinking.')
  queue.append('Still thinking.')
  queue.append('Still thinking.', true)
  await queue.wait()
  expect(speak).toHaveBeenCalledTimes(1)
})

it('bounds long sentences and unbroken text without dropping characters', async () => {
  const speak = jest.fn().mockResolvedValue(undefined)
  const queue = new SpeechQueue(speak)
  const text = 'A long sentence '.repeat(100) + 'x'.repeat(700) + '.'
  queue.append(text, true)
  await queue.wait()
  const chunks: string[] = speak.mock.calls.map(([chunk]) => chunk)
  expect(chunks.every((chunk) => chunk.length <= MAX_SPEECH_CHUNK_LENGTH)).toBe(
    true,
  )
  expect(chunks.join('').replaceAll(' ', '')).toBe(text.replaceAll(' ', ''))
})

it('waits for each playback before starting the next', async () => {
  const finishes: Array<() => void> = []
  const speak = jest.fn(
    () =>
      new Promise<void>((resolve) => {
        finishes.push(resolve)
      }),
  )
  const queue = new SpeechQueue(speak)
  queue.append('One. Two. Three.')
  await Promise.resolve()
  expect(speak).toHaveBeenCalledTimes(1)
  finishes.shift()!()
  // Flush the nested playback promises before checking the next item.
  for (let i = 0; i < 8; i++) {
    await Promise.resolve()
  }
  expect(speak).toHaveBeenCalledTimes(2)
  finishes.shift()!()
  for (let i = 0; i < 8; i++) {
    await Promise.resolve()
  }
  expect(speak).toHaveBeenCalledTimes(3)
  finishes.shift()!()
  await queue.wait()
})

it('cancels queued sentences without replaying them in a new reply', async () => {
  let finish!: () => void
  const speak = jest
    .fn()
    .mockImplementationOnce(
      () =>
        new Promise<void>((resolve) => {
          finish = resolve
        }),
    )
    .mockResolvedValue(undefined)
  const queue = new SpeechQueue(speak)
  queue.append('Old first. Old second.')
  await Promise.resolve()
  queue.reset()
  queue.append('New reply.', true)
  finish()
  await queue.wait()
  expect(speak.mock.calls).toEqual([['Old first.'], ['New reply.']])
})

it('retries oversized audio as smaller chunks and preserves order', async () => {
  const played: string[] = []
  const queue = new SpeechQueue(async (text) => {
    if (text.length > 12) {
      throw new Error('tts-audio-too-large')
    }
    played.push(text)
  })
  const text = 'An unusually slow voice reads this sentence.'
  queue.append(text, true)
  await queue.wait()
  expect(played.every((chunk) => chunk.length <= 12)).toBe(true)
  expect(played.join('')).toBe(text)
})

it('reports synthesis failures and skips remaining chunks', async () => {
  const speak = jest.fn().mockRejectedValue(new Error('Server unavailable'))
  const queue = new SpeechQueue(speak)
  queue.append('One. Two.')
  await expect(queue.wait()).rejects.toThrow('Server unavailable')
  expect(speak).toHaveBeenCalledTimes(1)
})

it('keeps code out of speech as fenced blocks stream in', async () => {
  const speak = jest.fn().mockResolvedValue(undefined)
  const queue = new SpeechQueue(speak)
  queue.append('Here is code. ```ts\nconst x = 1.')
  await queue.wait()
  expect(speak.mock.calls).toEqual([['Here is code.']])
  queue.append('Here is code. ```ts\nconst x = 1.\n``` Explanation.', true)
  await queue.wait()
  expect(speak.mock.calls).toEqual([['Here is code.'], ['Explanation.']])
})
