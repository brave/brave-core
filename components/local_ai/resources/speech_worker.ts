// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  OnDeviceSpeechRecognitionService,
  SpeechRecognitionFactoryInterface,
  SpeechRecognitionFactoryReceiver,
  SpeechRecognitionInitFiles,
} from 'gen/brave/components/local_ai/core/local_ai.mojom.m.js'
import {
  AsrStreamInputInterface,
  AsrStreamInputPendingReceiver,
  AsrStreamInputReceiver,
  AsrStreamOptions,
  AsrStreamResponderRemote,
  AudioData,
} from 'gen/services/on_device_model/public/mojom/on_device_model.mojom.m.js'

// Minimal typing for the WASM ParakeetTranscriber class exported from
// candle_parakeet.bundle.js. The bundle itself handles mojo-unrelated
// details.
interface ParakeetTranscriberInstance {
  load_weight_chunk(chunk: Uint8Array): void
  finalize(): void
  add_audio(pcm: Float32Array): string
  mark_done(): string
  reset(): void
}

interface ParakeetTranscriberCtor {
  new (
    weights: Uint8Array,
    tokenizer: Uint8Array,
    mel_filters: Uint8Array,
    config: Uint8Array,
  ): ParakeetTranscriberInstance
}

// BigBuffer is a mojo union: either inlined bytes or a shared-memory
// handle the renderer maps in.
function readBigBuffer(
  buffer: {
    bytes?: number[]
    sharedMemory?: {
      bufferHandle: {
        mapBuffer(o: number, s: number): { buffer: ArrayBuffer | null }
      }
      size: number
    }
  },
  name: string,
): Uint8Array {
  if (buffer.bytes) {
    return new Uint8Array(buffer.bytes)
  }
  if (buffer.sharedMemory) {
    const mapped = buffer.sharedMemory.bufferHandle.mapBuffer(
      0,
      buffer.sharedMemory.size,
    )
    if (!mapped.buffer) {
      throw new Error(`Failed to map ${name} shared buffer`)
    }
    return new Uint8Array(mapped.buffer)
  }
  throw new Error(`Invalid ${name} BigBuffer`)
}

// Audio input for a single ASR session. Forwards PCM frames to the
// shared transcriber and ships results back to the browser via the
// responder remote.
class AsrStreamInputImpl implements AsrStreamInputInterface {
  private readonly receiver: AsrStreamInputReceiver
  private readonly responder: AsrStreamResponderRemote
  private readonly transcriber: ParakeetTranscriberInstance

  constructor(
    transcriber: ParakeetTranscriberInstance,
    receiver: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
  ) {
    this.transcriber = transcriber
    this.responder = responder
    // The transcriber is shared across sessions; clear the residual
    // audio buffer / accumulated transcript from any prior session
    // so this one starts fresh.
    this.transcriber.reset()
    this.receiver = new AsrStreamInputReceiver(this)
    this.receiver.$.bindHandle(receiver.handle)
    // The engine signals end-of-stream by closing the input pipe.
    // Flush the transcriber and emit the final transcript.
    this.receiver.onConnectionError.addListener(() => {
      const json = this.transcriber.mark_done()
      if (!json) {
        return
      }
      const parsed = JSON.parse(json)
      this.responder.onResponse([
        { transcript: parsed.transcript, isFinal: true },
      ])
    })
  }

  addAudioChunk(data: AudioData) {
    const samples = new Float32Array(data.data)
    const json = this.transcriber.add_audio(samples)
    if (!json) {
      return
    }
    const parsed = JSON.parse(json)
    this.responder.onResponse([
      { transcript: parsed.transcript, isFinal: parsed.is_final },
    ])
  }
}

class SpeechRecognitionFactoryImpl
  implements SpeechRecognitionFactoryInterface
{
  private readonly receiver: SpeechRecognitionFactoryReceiver
  private transcriber: ParakeetTranscriberInstance | null = null
  private transcriberCtor: ParakeetTranscriberCtor | null = null

  constructor() {
    this.receiver = new SpeechRecognitionFactoryReceiver(this)
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }

  async init(files: SpeechRecognitionInitFiles) {
    try {
      const gguf_header = readBigBuffer(files.ggufHeader, 'GgufHeader')
      const tokenizer = readBigBuffer(files.tokenizer, 'Tokenizer')
      const mel_filters = readBigBuffer(files.melFilters, 'MelFilters')
      const config = readBigBuffer(files.config, 'Config')

      if (!this.transcriberCtor) {
        const mod = await import(
          // @ts-ignore — full URL because dynamic imports aren't
          // relative-resolved through webpack at runtime.
          'chrome-untrusted://on-device-speech-recognition-worker/candle_parakeet.bundle.js'
        )
        this.transcriberCtor =
          mod.ParakeetTranscriber as ParakeetTranscriberCtor
      }
      this.transcriber = new this.transcriberCtor(
        gguf_header,
        tokenizer,
        mel_filters,
        config,
      )
      return { success: true }
    } catch (err) {
      console.error('[speech-worker] init failed:', err)
      return { success: false }
    }
  }

  async loadWeightChunk(chunk: {
    bytes?: number[]
    sharedMemory?: {
      bufferHandle: {
        mapBuffer(o: number, s: number): { buffer: ArrayBuffer | null }
      }
      size: number
    }
  }) {
    if (!this.transcriber) {
      return { success: false }
    }
    try {
      const bytes = readBigBuffer(chunk, 'Chunk')
      this.transcriber.load_weight_chunk(bytes)
      return { success: true }
    } catch (err) {
      console.error('[speech-worker] loadWeightChunk failed:', err)
      return { success: false }
    }
  }

  async finalize() {
    if (!this.transcriber) {
      return { success: false }
    }
    try {
      this.transcriber.finalize()
      return { success: true }
    } catch (err) {
      console.error('[speech-worker] finalize failed:', err)
      return { success: false }
    }
  }

  createAsrStream(
    options: AsrStreamOptions,
    stream: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
  ) {
    void options
    if (!this.transcriber) {
      console.error('[speech-worker] createAsrStream before finalize')
      return
    }
    new AsrStreamInputImpl(this.transcriber, stream, responder)
  }
}

const service = OnDeviceSpeechRecognitionService.getRemote()
const factory = new SpeechRecognitionFactoryImpl()
service.registerSpeechRecognitionFactory(factory.getPendingRemote())
console.debug('[speech-worker] factory registered')
