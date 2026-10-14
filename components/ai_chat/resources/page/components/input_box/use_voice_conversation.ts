// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSpeechRecognition } from './use_speech_recognition'

type Phase = 'off' | 'listening' | 'waiting' | 'speaking'

interface VoiceResponseSpeech {
  speak: (isCancelled: () => boolean) => Promise<void>
  cancel: () => void
}

/** Listens for one utterance, then waits for Leo's response before restarting. */
export function useVoiceConversation(
  isBusy: boolean,
  onTranscript: (transcript: string) => void,
  onUtterance: (transcript: string) => void,
  responseSpeech?: VoiceResponseSpeech,
) {
  const [phase, setPhase] = React.useState<Phase>('off')
  const phaseRef = React.useRef<Phase>('off')
  const responseStarted = React.useRef(false)
  const speechRef = React.useRef(responseSpeech)
  speechRef.current = responseSpeech
  const generation = React.useRef(0)
  const [playbackError, setPlaybackError] = React.useState<string | null>(null)

  const changePhase = (next: Phase) => {
    phaseRef.current = next
    setPhase(next)
  }

  const recognition = useSpeechRecognition(onTranscript, {
    continuous: true,
    inactivityTimeoutMs: 2000,
    onComplete: (transcript) => {
      if (phaseRef.current !== 'listening' || !transcript.trim()) {
        return
      }
      responseStarted.current = false
      changePhase('waiting')
      onUtterance(transcript)
    },
  })

  const cancelRecognition = recognition.cancel
  const stop = React.useCallback(() => {
    generation.current++
    speechRef.current?.cancel()
    phaseRef.current = 'off'
    setPhase('off')
    cancelRecognition()
  }, [cancelRecognition])

  React.useEffect(() => stop, [stop])

  const { error, isListening, start: startRecognition } = recognition

  React.useEffect(() => {
    if (phase === 'off') {
      return
    }
    if (error && error !== 'no-speech') {
      phaseRef.current = 'off'
      setPhase('off')
      cancelRecognition()
      return
    }
    if (phase === 'waiting') {
      if (isBusy) {
        responseStarted.current = true
      } else if (responseStarted.current) {
        responseStarted.current = false
        if (!speechRef.current) {
          changePhase('listening')
        } else {
          changePhase('speaking')
          const currentGeneration = generation.current
          void speechRef.current
            .speak(() => generation.current !== currentGeneration)
            .then(
              () => {
                if (generation.current === currentGeneration) {
                  changePhase('listening')
                }
              },
              () => {
                if (generation.current === currentGeneration) {
                  setPlaybackError('tts-unavailable')
                  stop()
                }
              },
            )
        }
      }
      return
    }
    if (isBusy) {
      generation.current++
      speechRef.current?.cancel()
      cancelRecognition()
      responseStarted.current = true
      phaseRef.current = 'waiting'
      setPhase('waiting')
      return
    }
    if (phase === 'speaking') {
      return
    }
    if (!isListening) {
      // Avoid spinning if the recognizer ends without hearing any speech.
      const timer = window.setTimeout(startRecognition, 250)
      return () => window.clearTimeout(timer)
    }
    return undefined
  }, [
    phase,
    isBusy,
    error,
    isListening,
    startRecognition,
    cancelRecognition,
    stop,
  ])

  return {
    isActive: phase !== 'off',
    isWaiting: phase === 'waiting' || phase === 'speaking',
    isSupported: recognition.isSupported,
    error: playbackError || recognition.error,
    start: () => {
      if (phaseRef.current !== 'off' || isBusy) {
        return
      }
      changePhase('listening')
      setPlaybackError(null)
      recognition.start()
    },
    stop,
  }
}
