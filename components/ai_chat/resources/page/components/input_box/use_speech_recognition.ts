/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// The DOM typings shipped with the TypeScript used for this build do not
// declare the Web Speech API, so the parts this hook touches are declared
// here. They are module scoped, so a future TypeScript that does ship them
// will not collide with these.
interface SpeechRecognitionResult {
  readonly [index: number]: { readonly transcript: string }
}

interface SpeechRecognitionEvent {
  readonly results: {
    readonly length: number
    readonly [index: number]: SpeechRecognitionResult
  }
}

interface SpeechRecognitionErrorEvent {
  readonly error: string
}

interface SpeechRecognitionInstance {
  lang: string
  continuous: boolean
  interimResults: boolean
  onresult: ((event: SpeechRecognitionEvent) => void) | null
  onerror: ((event: SpeechRecognitionErrorEvent) => void) | null
  onend: (() => void) | null
  start: () => void
  stop: () => void
  abort: () => void
}

type SpeechRecognitionConstructor = new () => SpeechRecognitionInstance

declare global {
  interface Window {
    SpeechRecognition?: SpeechRecognitionConstructor
    webkitSpeechRecognition?: SpeechRecognitionConstructor
  }
}

function getSpeechRecognitionConstructor() {
  return window.SpeechRecognition ?? window.webkitSpeechRecognition
}

// A continuous session keeps every result it has produced, so the whole
// utterance is rebuilt on each event rather than appended to.
function getTranscript(event: SpeechRecognitionEvent) {
  let transcript = ''
  for (let i = 0; i < event.results.length; i++) {
    transcript += event.results[i][0].transcript
  }
  return transcript
}

export interface SpeechRecognitionState {
  /** False when the API is missing, e.g. on a platform without it. */
  isSupported: boolean
  isListening: boolean
  /** The last error code reported by the API, such as 'no-speech'. */
  error: string | null
  start: () => void
  stop: () => void
}

/**
 * Dictation for the composer, driven by the Web Speech API.
 *
 * `processLocally` is deliberately left unset. Blink already asks for
 * on device recognition on every request, which Brave's own engine answers,
 * while setting the attribute would run an upstream availability precheck that
 * only knows about the speech models Brave does not ship.
 *
 * @param onTranscript Called with the full transcript so far, including the
 *     interim words that have not been finalized yet.
 */
export function useSpeechRecognition(
  onTranscript: (transcript: string) => void,
): SpeechRecognitionState {
  const [isListening, setIsListening] = React.useState(false)
  const [error, setError] = React.useState<string | null>(null)
  const recognitionRef = React.useRef<SpeechRecognitionInstance | null>(null)

  // Held in a ref so a new callback each render does not need a restart.
  const onTranscriptRef = React.useRef(onTranscript)
  onTranscriptRef.current = onTranscript

  const start = React.useCallback(() => {
    const SpeechRecognition = getSpeechRecognitionConstructor()
    if (!SpeechRecognition || recognitionRef.current) {
      return
    }

    const recognition = new SpeechRecognition()
    recognition.lang = navigator.language
    recognition.continuous = true
    recognition.interimResults = true
    recognition.onresult = (event) => {
      onTranscriptRef.current(getTranscript(event))
    }
    recognition.onerror = (event) => {
      setError(event.error)
    }
    // Fires for both a normal stop and an error, so it is the single place
    // that returns the button to its idle state.
    recognition.onend = () => {
      recognitionRef.current = null
      setIsListening(false)
    }

    recognitionRef.current = recognition
    setError(null)
    setIsListening(true)
    recognition.start()
  }, [])

  const stop = React.useCallback(() => {
    recognitionRef.current?.stop()
  }, [])

  React.useEffect(() => {
    return () => {
      const recognition = recognitionRef.current
      if (!recognition) {
        return
      }
      // Drop the handlers first so aborting does not set state on an
      // unmounted component.
      recognition.onresult = null
      recognition.onerror = null
      recognition.onend = null
      recognition.abort()
      recognitionRef.current = null
    }
  }, [])

  return {
    isSupported: !!getSpeechRecognitionConstructor(),
    isListening,
    error,
    start,
    stop,
  }
}
