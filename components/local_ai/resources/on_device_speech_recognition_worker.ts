// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  OnDeviceSpeechRecognitionService,
  SpeechRecognitionFactoryReceiver,
  SpeechRecognitionFactoryInterface,
  SpeechRecognitionModelFiles,
} from 'gen/brave/components/local_ai/core/on_device_speech_recognition.mojom.m.js'

import {
  AsrStreamInputReceiver,
  AsrStreamInputInterface,
  AsrStreamInputPendingReceiver,
  AsrStreamResponderRemote,
  AsrStreamOptions,
  AudioData,
  SpeechRecognitionResult,
} from 'gen/services/on_device_model/public/mojom/on_device_model.mojom.m.js'

// Minimal type for the WASM WhisperRecognizer class
interface WasmWhisperRecognizer {
  add_audio(audio: Float32Array): string
  mark_done(): string
  reset(): void
  free(): void
}

// Initialize connection to the browser-side
// OnDeviceSpeechRecognitionService
const service = OnDeviceSpeechRecognitionService.getRemote()

// Helper to extract data from a Mojo BigBuffer.
function extractBigBuffer(buffer: any, name: string): Uint8Array | null {
  if (buffer.bytes) {
    return new Uint8Array(buffer.bytes)
  } else if (buffer.sharedMemory) {
    const sharedMem = buffer.sharedMemory
    const mapResult = sharedMem.bufferHandle.mapBuffer(0, sharedMem.size)
    if (!mapResult.buffer) {
      console.error(`Failed to map ${name} shared buffer`)
      return null
    }
    return new Uint8Array(mapResult.buffer)
  }
  console.error(`Invalid ${name} BigBuffer`)
  return null
}

class AsrStreamInputImpl implements AsrStreamInputInterface {
  private recognizer: WasmWhisperRecognizer
  private responder: AsrStreamResponderRemote
  private receiver: AsrStreamInputReceiver

  constructor(
    recognizer: WasmWhisperRecognizer,
    pendingReceiver: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
    onDisconnect: () => void,
  ) {
    this.recognizer = recognizer
    this.responder = responder
    this.receiver = new AsrStreamInputReceiver(this)
    this.receiver.$.bindHandle(pendingReceiver.handle)
    this.receiver.onConnectionError.addListener(() => {
      // Pipe closed = end of stream. Flush remaining
      // audio and send final results.
      this.flush()
      onDisconnect()
    })
  }

  async addAudioChunk(data: AudioData) {
    try {
      const audio = new Float32Array(data.data)
      const resultJson = this.recognizer.add_audio(audio)
      this.maybeSendResults(resultJson)
    } catch (e) {
      console.error('Whisper addAudioChunk error:', e)
    }
  }

  private flush() {
    try {
      const resultJson = this.recognizer.mark_done()
      this.maybeSendResults(resultJson)
    } catch (e) {
      console.error('Whisper flush error:', e)
    }
  }

  private maybeSendResults(json: string) {
    if (json && this.responder) {
      const r = JSON.parse(json)
      const result: SpeechRecognitionResult = {
        transcript: r.transcript,
        isFinal: r.is_final,
      }
      this.responder.onResponse([result])
    }
  }
}

// Single WASM recognizer instance, created once during
// init and reused across sessions. Avoids re-parsing
// model weights and growing WASM memory each bind().
let wasmRecognizer: WasmWhisperRecognizer | null = null

class SpeechRecognitionFactoryImpl
  implements SpeechRecognitionFactoryInterface
{
  receiver: SpeechRecognitionFactoryReceiver
  streamCount = 0
  isInitialized = false

  constructor() {
    this.receiver = new SpeechRecognitionFactoryReceiver(this)
  }

  async init(modelFiles: SpeechRecognitionModelFiles) {
    if (this.isInitialized) {
      return { success: true }
    }

    try {
      const weights = extractBigBuffer(modelFiles.weights, 'weights')
      const tokenizer = extractBigBuffer(modelFiles.tokenizer, 'tokenizer')
      const melFilters = extractBigBuffer(modelFiles.melFilters, 'mel_filters')
      const config = extractBigBuffer(modelFiles.config, 'config')

      if (!weights || !tokenizer || !melFilters || !config) {
        console.error('[whisper-worker] Failed to extract model files')
        return { success: false }
      }

      console.log(
        `[whisper-worker] Model files received: `
          + `weights=${weights.byteLength}, `
          + `tokenizer=${tokenizer.byteLength}, `
          + `mel_filters=${melFilters.byteLength}, `
          + `config=${config.byteLength}`,
      )

      // Load the candle_whisper WASM module
      console.log('[whisper-worker] Loading WASM module...')
      const module = await import(
        // @ts-ignore - Served by the same WebUI data source
        'chrome-untrusted://whisper-worker/' + 'candle_whisper.bundle.js'
      )

      // Create the single WASM recognizer. Model bytes are
      // only needed during construction and will be GC'd
      // after this function returns.
      const startTime = performance.now()
      wasmRecognizer = new module.WhisperRecognizer(
        weights,
        tokenizer,
        melFilters,
        config,
      )
      const elapsed = performance.now() - startTime
      console.log(
        `[whisper-worker] Recognizer created in ` + `${elapsed.toFixed(0)}ms`,
      )

      this.isInitialized = true
      return { success: true }
    } catch (e) {
      console.error('[whisper-worker] Failed to initialize:', e)
      return { success: false }
    }
  }

  createSession(
    _options: AsrStreamOptions,
    stream: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
  ) {
    if (!wasmRecognizer) {
      console.error('Cannot create stream: model not loaded')
      return
    }

    try {
      // Reset decoder state between sessions.
      wasmRecognizer.reset()

      this.streamCount++
      new AsrStreamInputImpl(wasmRecognizer, stream, responder, () => {
        this.streamCount--
        if (this.streamCount === 0) {
          service.notifySpeechRecognitionIdle()
        }
      })
    } catch (e) {
      console.error('Failed to create ASR stream:', e)
    }
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }
}

// Register factory immediately on page load — the service
// pushes model files via init() when ready.
const factory = new SpeechRecognitionFactoryImpl()
service.registerSpeechRecognitionFactory(factory.getPendingRemote())

console.log('[whisper-worker] Factory registered, ready for init')
