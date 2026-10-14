// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'

type SpeechHandler = Pick<
  Mojom.AIChatUIHandlerRemote,
  'synthesizeSpeech' | 'cancelSpeech'
>

/** Plays replies from the local Pocket TTS server and cancels stale audio. */
export function useLocalSpeech(handler: SpeechHandler) {
  const context = React.useRef<AudioContext | undefined>(undefined)
  const source = React.useRef<AudioBufferSourceNode | undefined>(undefined)
  const generation = React.useRef(0)
  const finishPlayback = React.useRef<(() => void) | undefined>(undefined)

  const prepare = React.useCallback(() => {
    context.current ??= new AudioContext()
    // Called from the voice button to unlock audio under a user gesture.
    void context.current.resume().catch(() => {})
  }, [])

  const cancel = React.useCallback(() => {
    generation.current++
    handler.cancelSpeech()
    source.current?.stop()
    source.current?.disconnect()
    source.current = undefined
    finishPlayback.current?.()
    finishPlayback.current = undefined
  }, [handler])

  const speak = React.useCallback(
    async (text: string) => {
      if (!text.trim()) {
        return
      }
      const currentGeneration = generation.current
      const { audioData, sampleRate, error } =
        await handler.synthesizeSpeech(text)
      if (currentGeneration !== generation.current) {
        return
      }
      if (error || !audioData.length || audioData.length % 4 !== 0) {
        throw new Error(error || 'Invalid local TTS audio')
      }
      const bytes = new Uint8Array(audioData)
      const view = new DataView(bytes.buffer)
      const samples = new Float32Array(bytes.length / 4)
      for (let i = 0; i < samples.length; i++) {
        samples[i] = view.getFloat32(i * 4, true)
        if (!Number.isFinite(samples[i])) {
          throw new Error('Invalid local TTS sample')
        }
      }
      context.current ??= new AudioContext()
      const audioContext = context.current
      await audioContext.resume()
      if (currentGeneration !== generation.current) {
        return
      }
      const buffer = audioContext.createBuffer(1, samples.length, sampleRate)
      buffer.copyToChannel(samples, 0)
      const playback = audioContext.createBufferSource()
      playback.buffer = buffer
      playback.connect(audioContext.destination)
      source.current = playback
      await new Promise<void>((resolve) => {
        finishPlayback.current = resolve
        playback.onended = () => {
          playback.disconnect()
          if (source.current === playback) {
            source.current = undefined
            finishPlayback.current = undefined
          }
          resolve()
        }
        playback.start()
      })
    },
    [handler],
  )

  React.useEffect(() => {
    return () => {
      cancel()
      void context.current?.close()
      context.current = undefined
    }
  }, [cancel])

  return { prepare, speak, cancel }
}
