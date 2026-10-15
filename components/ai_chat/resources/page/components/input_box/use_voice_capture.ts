// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

/** Keeps an echo-cancelled microphone track alive across voice turns. */
export function useVoiceCapture(onSpeech?: () => void) {
  const onSpeechRef = React.useRef(onSpeech)
  onSpeechRef.current = onSpeech
  const context = React.useRef<AudioContext | null>(null)
  const timer = React.useRef<number | undefined>(undefined)
  const lastActivity = React.useRef(-Infinity)
  const stream = React.useRef<MediaStream | null>(null)
  const generation = React.useRef(0)
  const stop = React.useCallback(() => {
    generation.current++
    window.clearInterval(timer.current)
    timer.current = undefined
    void context.current?.close()
    context.current = null
    lastActivity.current = -Infinity
    stream.current?.getTracks().forEach((track) => track.stop())
    stream.current = null
  }, [])
  const start = React.useCallback(async () => {
    stop()
    const currentGeneration = generation.current
    const microphone = await navigator.mediaDevices.getUserMedia({
      audio: {
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true,
        channelCount: 1,
        // Active Chromium audio tracks cannot change their processing mode.
        // Optional constraints preserve AEC when voice isolation is unsupported
        // or incompatible with the selected echo cancellation mode.
        advanced: [
          { echoCancellation: { exact: 'all' } },
          { voiceIsolation: { exact: true } },
        ] as unknown as MediaTrackConstraintSet[],
      },
      video: false,
    })
    if (generation.current !== currentGeneration) {
      microphone.getTracks().forEach((track) => track.stop())
      return null
    }
    stream.current = microphone
    const track = microphone.getAudioTracks()[0]
    const settings = track?.getSettings() as
      | { echoCancellation?: boolean | string }
      | undefined
    if (
      !track
      || (settings?.echoCancellation !== true
        && settings?.echoCancellation !== 'all')
    ) {
      stop()
      throw new Error('Echo cancellation is unavailable')
    }
    try {
      const audioContext = new AudioContext()
      context.current = audioContext
      await audioContext.resume()
      if (generation.current !== currentGeneration) {
        return null
      }
      const input = audioContext.createMediaStreamSource(microphone)
      const analyser = audioContext.createAnalyser()
      analyser.fftSize = 1024
      input.connect(analyser)
      const mute = audioContext.createGain()
      mute.gain.value = 0
      analyser.connect(mute)
      mute.connect(audioContext.destination)
      const samples = new Float32Array(analyser.fftSize)
      let speechSince: number | undefined
      let notified = false
      timer.current = window.setInterval(() => {
        analyser.getFloatTimeDomainData(samples)
        let energy = 0
        for (const sample of samples) {
          energy += sample * sample
        }
        const now = performance.now()
        if (Math.sqrt(energy / samples.length) > 0.015) {
          lastActivity.current = now
          speechSince ??= now
          if (!notified && now - speechSince >= 120) {
            notified = true
            onSpeechRef.current?.()
          }
        } else {
          speechSince = undefined
          if (now - lastActivity.current > 250) {
            notified = false
          }
        }
      }, 20)
      return track
    } catch (error) {
      if (generation.current === currentGeneration) {
        stop()
      }
      throw error
    }
  }, [stop])
  React.useEffect(() => stop, [stop])
  const getSpeechIdleMs = React.useCallback(
    () =>
      context.current ? performance.now() - lastActivity.current : undefined,
    [],
  )
  return { start, stop, getSpeechIdleMs }
}
