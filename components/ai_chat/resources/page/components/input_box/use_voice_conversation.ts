// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSpeechRecognition } from './use_speech_recognition'
import { useVoiceCapture } from './use_voice_capture'

type Phase =
  | 'off'
  | 'starting'
  | 'listening'
  | 'waiting'
  | 'speaking'
  | 'interrupting'
  | 'submitting'

interface VoiceResponseSpeech {
  speak: (isCancelled: () => boolean) => Promise<void>
  cancel: () => void
  interrupt?: () => Promise<void>
  pause?: () => void
  resume?: () => void
}

function isAcknowledgement(text: string) {
  const words = text
    .toLowerCase()
    .replace(/[^\p{L}\s]/gu, ' ')
    .trim()
  return /^(?:(?:h+m+|m+h*m+|u+m+|u+h+|uh huh)(?:\s+|$))+$/.test(words)
}

/** Listens through replies and cancels playback when the user speaks again. */
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

  const track = React.useRef<MediaStreamTrack | undefined>(undefined)
  const interruption = React.useRef<Promise<void>>(Promise.resolve())
  const tentativeTimer = React.useRef<number | undefined>(undefined)
  const ignoredAcknowledgement = React.useRef(false)
  const handsFree = !!responseSpeech?.interrupt

  const changePhase = (next: Phase) => {
    phaseRef.current = next
    setPhase(next)
  }

  const resumeReply = () => {
    window.clearInterval(tentativeTimer.current)
    tentativeTimer.current = undefined
    speechRef.current?.resume?.()
  }
  const interrupt = () => {
    if (
      handsFree
      && (phaseRef.current === 'waiting' || phaseRef.current === 'speaking')
    ) {
      window.clearInterval(tentativeTimer.current)
      tentativeTimer.current = undefined
      ignoredAcknowledgement.current = false
      generation.current++
      const currentGeneration = generation.current
      speechRef.current?.cancel()
      responseStarted.current = false
      changePhase('interrupting')
      interruption.current = Promise.resolve().then(() =>
        speechRef.current?.interrupt?.(),
      )
      void interruption.current.catch(() => {
        if (generation.current === currentGeneration) {
          setPlaybackError('interruption-failed')
        }
      })
    }
  }
  const capture = useVoiceCapture(() => {
    if (
      !handsFree
      || (phaseRef.current !== 'waiting' && phaseRef.current !== 'speaking')
    ) {
      return
    }
    speechRef.current?.pause?.()
    window.clearInterval(tentativeTimer.current)
    tentativeTimer.current = window.setInterval(() => {
      const idle = capture.getSpeechIdleMs()
      if (idle !== undefined && idle >= 500) {
        resumeReply()
      }
    }, 100)
  })

  const recognition = useSpeechRecognition(
    (transcript) => {
      if (
        phaseRef.current === 'waiting'
        || phaseRef.current === 'speaking'
        || ignoredAcknowledgement.current
      ) {
        if (isAcknowledgement(transcript)) {
          ignoredAcknowledgement.current = true
          resumeReply()
          return
        }
      }
      if (transcript.trim()) {
        interrupt()
      }
      if (
        phaseRef.current === 'listening'
        || phaseRef.current === 'interrupting'
      ) {
        onTranscript(transcript)
      }
    },
    {
      continuous: true,
      getAudioTrack: () => track.current,
      getSpeechIdleMs: capture.getSpeechIdleMs,
      inactivityTimeoutMs: 2000,
      onComplete: (transcript) => {
        if (ignoredAcknowledgement.current && isAcknowledgement(transcript)) {
          ignoredAcknowledgement.current = false
          return
        }
        ignoredAcknowledgement.current = false
        if (
          (phaseRef.current !== 'listening'
            && phaseRef.current !== 'interrupting')
          || !transcript.trim()
        ) {
          return
        }
        responseStarted.current = false
        if (phaseRef.current === 'interrupting') {
          changePhase('submitting')
          const currentGeneration = generation.current
          void interruption.current.then(
            () => {
              if (generation.current === currentGeneration) {
                changePhase('waiting')
                onUtterance(transcript)
              }
            },
            () => {
              if (generation.current === currentGeneration) {
                setPlaybackError('interruption-failed')
              }
            },
          )
        } else {
          changePhase('waiting')
          onUtterance(transcript)
        }
      },
    },
  )

  const cancelRecognition = recognition.cancel
  const stopCapture = capture.stop
  const stop = React.useCallback(() => {
    window.clearInterval(tentativeTimer.current)
    tentativeTimer.current = undefined
    ignoredAcknowledgement.current = false
    generation.current++
    speechRef.current?.cancel()
    phaseRef.current = 'off'
    setPhase('off')
    cancelRecognition()
    stopCapture()
    track.current = undefined
  }, [cancelRecognition, stopCapture])

  React.useEffect(() => stop, [stop])

  const { error, isListening, start: startRecognition } = recognition

  React.useEffect(() => {
    if (phase === 'off' || phase === 'starting') {
      return
    }
    if (
      (error && error !== 'no-speech')
      || playbackError === 'interruption-failed'
    ) {
      stop()
      return
    }
    if (phase === 'submitting') {
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
      if (!handsFree) {
        return
      }
    }
    if (isBusy && phase === 'listening') {
      generation.current++
      speechRef.current?.cancel()
      cancelRecognition()
      responseStarted.current = true
      phaseRef.current = 'waiting'
      setPhase('waiting')
      return
    }
    if (phase === 'speaking' && !handsFree) {
      return
    }
    if (!isListening) {
      if (handsFree && track.current && error !== 'no-speech') {
        startRecognition()
        return
      }
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
    handsFree,
    playbackError,
  ])

  return {
    isActive: phase !== 'off',
    isWaiting:
      phase === 'waiting' || phase === 'speaking' || phase === 'submitting',
    isSupported: recognition.isSupported,
    error: playbackError || recognition.error,
    start: () => {
      if (phaseRef.current !== 'off' || isBusy) {
        return
      }
      setPlaybackError(null)
      if (!handsFree) {
        changePhase('listening')
        recognition.start()
        return
      }
      changePhase('starting')
      const currentGeneration = generation.current
      void capture.start().then(
        (audioTrack) => {
          if (generation.current !== currentGeneration || !audioTrack) {
            return
          }
          track.current = audioTrack
          changePhase('listening')
          startRecognition()
        },
        () => {
          if (generation.current === currentGeneration) {
            setPlaybackError('audio-capture')
            stop()
          }
        },
      )
    },
    stop,
  }
}
