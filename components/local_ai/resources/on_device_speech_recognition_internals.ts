// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AsrSession } from 'gen/brave/components/local_ai/core/local_ai.mojom.m.js'

import {
  AsrStreamInputRemote,
  AsrStreamResponderCallbackRouter,
  AudioData,
} from 'gen/services/on_device_model/public/mojom/on_device_model.mojom.m.js'

function output(msg: string) {
  const el = document.getElementById('output')!
  el.textContent += msg + '\n'
}

// Read the source sample rate from a canonical WAV header (the `fmt ` chunk
// stores it as a little-endian uint32 at byte offset 24). Used for logging.
function readWavSampleRate(buffer: ArrayBuffer): number {
  if (buffer.byteLength < 28) {
    return 0
  }
  return new DataView(buffer).getUint32(24, true)
}

// Decode any WAV (any sample rate, any channel count) to 16 kHz mono float32
// samples, which is what the worker's mel front-end expects. decodeAudioData
// resamples to the AudioContext's sample rate, so a 16 kHz context gives us
// the rate conversion for free, and we average channels down to mono.
async function decodeTo16kMono(buffer: ArrayBuffer): Promise<Float32Array> {
  const ctx = new AudioContext({ sampleRate: 16000 })
  try {
    const decoded = await ctx.decodeAudioData(buffer)
    const frames = decoded.length
    const channels = decoded.numberOfChannels
    const mono = new Float32Array(frames)
    for (let c = 0; c < channels; c++) {
      const data = decoded.getChannelData(c)
      for (let i = 0; i < frames; i++) {
        mono[i] += data[i]
      }
    }
    if (channels > 1) {
      for (let i = 0; i < frames; i++) {
        mono[i] /= channels
      }
    }
    return mono
  } finally {
    await ctx.close()
  }
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
  const options = {
    sampleRateHz: 16000,
  }
  const session = AsrSession.getRemote()
  session.start(
    options,
    stream.$.bindNewPipeAndPassReceiver(),
    responder.$.bindNewPipeAndPassRemote(),
  )

  output(
    `Transcribing ${audio.length} samples `
      + `(${(audio.length / 16000).toFixed(1)}s)...`,
  )

  // Feed audio in 500 ms increments, mirroring the real engine's flush
  // cadence, so the worker streams it (CTC re-encodes a growing window;
  // Nemotron drains constant-cost cache-aware chunks) and reports a per-
  // step inference time for each increment.
  const CHUNK = 8000 // 500 ms @ 16 kHz
  // Space the flushes ~600 ms apart in real time so each interim decode
  // runs before the next flush (matching the real engine's cadence). If we
  // sent them all synchronously the worker would skip every interim and
  // only run the final, hiding the per-step timing.
  for (let off = 0; off < audio.length; off += CHUNK) {
    const slice = audio.subarray(off, Math.min(off + CHUNK, audio.length))
    const data: AudioData = {
      channelCount: 1,
      sampleRate: 16000,
      frameCount: slice.length,
      data: Array.from(slice),
    }
    stream.addAudioChunk(data)
    await new Promise((resolve) => setTimeout(resolve, 600))
  }

  // Let the last interim finish, then close to signal end of audio.
  // The WASM worker flushes and sends the final result.
  await new Promise((resolve) => setTimeout(resolve, 1500))
  stream.$.close()

  // Drop the AsrSession remote to end the session, mirroring the real Web
  // Speech engine dropping it in EndRecognition. The controller decrements
  // off the receiver disconnect and arms its idle timer to free the worker.
  session.$.close()

  // Wait for streaming + final result to arrive (Nemotron's 837 MB load +
  // ~26 chunks can take ~15 s).
  await new Promise((resolve) => setTimeout(resolve, 30000))
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
    const sourceRate = readWavSampleRate(buffer)
    const samples = await decodeTo16kMono(buffer)
    output(
      `Decoded ${samples.length} samples `
        + `(${(samples.length / 16000).toFixed(1)}s) `
        + `from ${sourceRate || 'unknown'} Hz source, resampled to 16 kHz mono`,
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
    // ;(recognition as any).processLocally = true
    // ;(recognition as any).quality = 'conversation'
    activeRecognition = recognition

    const stopBtn = document.getElementById(
      'webspeech-stop-btn',
    ) as HTMLButtonElement
    const startBtn = document.getElementById(
      'webspeech-btn',
    ) as HTMLButtonElement
    stopBtn.disabled = false
    startBtn.disabled = true

    // `onstart` fires immediately on recognition.start(), BEFORE the
    // OS mic stream has actually opened. Audio spoken between
    // recognition.start() and the first captured chunk is lost at
    // the device layer (the mic isn't streaming yet, so there's
    // nothing to queue from). Signal "listening" on `onaudiostart`
    // instead — that fires when the first audio chunk arrives, i.e.
    // when it's actually safe for the user to start speaking. Log
    // both to show the gap, since it's the audio-loss window.
    const startedAt = performance.now()
    recognition.onstart = () => {
      output('Recognition requested (mic opening...)')
    }
    recognition.onaudiostart = () => {
      const delayMs = performance.now() - startedAt
      output(`Listening (mic open after ${delayMs.toFixed(0)} ms).`)
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
