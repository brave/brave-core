// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  OnDeviceSpeechRecognitionService,
  AsrStreamInputClientCallbackRouter,
} from 'gen/brave/components/local_ai/core/on_device_speech_recognition.mojom.m.js'

const service = OnDeviceSpeechRecognitionService.getRemote()

function output(msg: string) {
  const el = document.getElementById('output')!
  el.textContent += msg + '\n'
}

// Decode a 16kHz mono WAV file to int16 PCM samples.
function decodeWav(buffer: ArrayBuffer): Int16Array {
  const headerSize = 44
  return new Int16Array(buffer.slice(headerSize))
}

async function transcribeAudio(audio: number[]) {
  output('Getting recognizer...')
  const { recognizer } = await service.getAsrStreamInput()
  if (!recognizer) {
    output('ERROR: No recognizer returned')
    return
  }

  // Wire up a client to receive results.
  const router = new AsrStreamInputClientCallbackRouter()
  const startTime = performance.now()

  const done = new Promise<void>((resolve) => {
    router.onTranscript.addListener((transcript: string, isFinal: boolean) => {
      const elapsed = performance.now() - startTime
      output(
        `Result (${elapsed.toFixed(0)}ms): `
          + `"${transcript}" (final=${isFinal})`,
      )
    })
    router.onStopped.addListener(() => {
      resolve()
    })
  })

  recognizer.setClient(router.$.bindNewPipeAndPassRemote())

  output(
    `Transcribing ${audio.length} samples `
      + `(${(audio.length / 16000).toFixed(1)}s)...`,
  )
  recognizer.addAudio(audio)
  recognizer.markDone()

  await done
  recognizer.$.close()
  output('Done.')
}

async function testSilence() {
  try {
    output('--- Test: silence (1s) ---')
    const silence = new Array(16000).fill(0)
    await transcribeAudio(silence)
  } catch (e) {
    output(`ERROR: ${e}`)
    console.error('testSilence failed:', e)
  }
}

async function testWavFile() {
  try {
    const fileInput = document.getElementById('wav-file') as HTMLInputElement
    const file = fileInput.files?.[0]
    if (!file) {
      output('ERROR: No file selected')
      return
    }

    output(`--- Test: ${file.name} ---`)
    output(`Reading file (${file.size} bytes)...`)
    const buffer = await file.arrayBuffer()
    const samples = decodeWav(buffer)
    output(
      `Decoded ${samples.length} samples `
        + `(${(samples.length / 16000).toFixed(1)}s)`,
    )

    await transcribeAudio(Array.from(samples))
  } catch (e) {
    output(`ERROR: ${e}`)
    console.error('testWavFile failed:', e)
  }
}

// --- Web Speech API test ---

let activeRecognition: any = null

function testWebSpeech() {
  try {
    const SpeechRecognition =
      (window as any).SpeechRecognition
      || (window as any).webkitSpeechRecognition
    if (!SpeechRecognition) {
      output('ERROR: SpeechRecognition API ' + 'not available on this page')
      return
    }

    output('--- Web Speech API ---')
    const recognition = new SpeechRecognition()
    recognition.continuous = true
    recognition.interimResults = true
    recognition.lang = 'en-US'
    activeRecognition = recognition

    const stopBtn = document.getElementById(
      'webspeech-stop-btn',
    ) as HTMLButtonElement
    const startBtn = document.getElementById(
      'webspeech-btn',
    ) as HTMLButtonElement
    stopBtn.disabled = false
    startBtn.disabled = true

    recognition.onstart = () => {
      output('Recognition started (listening...)')
    }

    recognition.onresult = (event: any) => {
      const results = event.results
      for (let i = event.resultIndex; i < results.length; i++) {
        const result = results[i]
        const text = result[0].transcript
        const conf = result[0].confidence
        const tag = result.isFinal ? 'FINAL' : 'interim'
        output(`[${tag}] "${text}" ` + `(confidence=${conf.toFixed(2)})`)
      }
    }

    recognition.onerror = (event: any) => {
      output(`ERROR: ${event.error} - ${event.message}`)
      stopBtn.disabled = true
      startBtn.disabled = false
      activeRecognition = null
    }

    recognition.onend = () => {
      output('Recognition ended.')
      stopBtn.disabled = true
      startBtn.disabled = false
      activeRecognition = null
    }

    recognition.start()
  } catch (e) {
    output(`ERROR: ${e}`)
    console.error('testWebSpeech failed:', e)
  }
}

function stopWebSpeech() {
  if (activeRecognition) {
    activeRecognition.stop()
    output('Stopping recognition...')
  }
}

document.getElementById('test-btn')!.addEventListener('click', testSilence)
document.getElementById('test-wav-btn')!.addEventListener('click', testWavFile)
document
  .getElementById('webspeech-btn')!
  .addEventListener('click', testWebSpeech)
document
  .getElementById('webspeech-stop-btn')!
  .addEventListener('click', stopWebSpeech)
