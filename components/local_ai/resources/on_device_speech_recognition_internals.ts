// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import { OnDeviceSpeechRecognitionService } from 'gen/brave/components/local_ai/core/on_device_speech_recognition.mojom.m.js'

import {
  AsrStreamInputRemote,
  AsrStreamResponderCallbackRouter,
  AudioData,
} from 'gen/services/on_device_model/public/mojom/on_device_model.mojom.m.js'

const service = OnDeviceSpeechRecognitionService.getRemote()

function output(msg: string) {
  const el = document.getElementById('output')!
  el.textContent += msg + '\n'
}

// Decode a 16kHz mono WAV file to float32 samples.
function decodeWav(buffer: ArrayBuffer): Float32Array {
  const headerSize = 44
  const int16 = new Int16Array(buffer.slice(headerSize))
  const float32 = new Float32Array(int16.length)
  for (let i = 0; i < int16.length; i++) {
    float32[i] = int16[i] / 32768.0
  }
  return float32
}

async function transcribeAudio(audio: Float32Array) {
  output('Creating session...')

  const responder = new AsrStreamResponderCallbackRouter()
  const startTime = performance.now()

  responder.onResponse.addListener((results: any[]) => {
    const elapsed = performance.now() - startTime
    for (const r of results) {
      output(
        `Result (${elapsed.toFixed(0)}ms): `
          + `"${r.transcript}" `
          + `(final=${r.isFinal})`,
      )
    }
  })

  const stream = new AsrStreamInputRemote()
  const options = { sampleRateHz: 16000 }
  service.createSession(
    options,
    stream.$.bindNewPipeAndPassReceiver(),
    responder.$.bindNewPipeAndPassRemote(),
  )

  output(
    `Transcribing ${audio.length} samples `
      + `(${(audio.length / 16000).toFixed(1)}s)...`,
  )

  const data: AudioData = {
    channelCount: 1,
    sampleRate: 16000,
    frameCount: audio.length,
    data: Array.from(audio),
  }
  stream.addAudioChunk(data)

  // Close the stream to signal end of audio.
  // The WASM worker will flush and send final results.
  stream.$.close()

  // Wait a bit for results to arrive.
  await new Promise((r) => setTimeout(r, 5000))
  output('Done.')
}

async function testSilence() {
  try {
    output('--- Test: silence (1s) ---')
    const silence = new Float32Array(16000)
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

    await transcribeAudio(samples)
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
