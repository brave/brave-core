// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// ORT-Web speech worker. A cross-origin-isolated renderer that runs the
// Nemotron 0.6B int4 ONNX encoder + RNN-T decoder through onnxruntime-web
// (multi-threaded WASM) and exposes a browser-driven mojo contract: a
// factory that loads the model whole and mints per-utterance ASR streams.
// This file is the mojo transport layer; the ORT environment lives in
// ort_env.ts and the model/inference in nemotron_recognizer.ts.

import {
  OnDeviceSpeechRecognitionService,
  OrtModelFiles,
  SpeechRecognitionFactoryInterface,
  SpeechRecognitionFactoryReceiver,
} from 'gen/brave/components/local_ai/core/local_ai.mojom.m.js'
import {
  AsrStreamInputInterface,
  AsrStreamInputPendingReceiver,
  AsrStreamInputReceiver,
  AsrStreamOptions,
  AsrStreamResponderRemote,
  AudioData,
} from 'gen/services/on_device_model/public/mojom/on_device_model.mojom.m.js'

import { installTrustedTypesPolicy } from './ort_env'
import {
  NemotronStreamSession,
  OrtNemotronModel,
  parseTokens,
} from './nemotron_recognizer'

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

// Thin mojo endpoint for one ASR stream: owns the receiver and responder
// and bridges AsrStreamInput calls to a mojo-free NemotronStreamSession.
// Engine disconnect (~end of utterance) triggers the final flush.
class AsrStreamInputAdapter implements AsrStreamInputInterface {
  private readonly receiver: AsrStreamInputReceiver
  private readonly session: NemotronStreamSession

  constructor(
    model: OrtNemotronModel,
    pending: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
  ) {
    this.session = new NemotronStreamSession(model, (text, isFinal) => {
      responder.onResponse([{ transcript: text, isFinal }])
    })
    this.receiver = new AsrStreamInputReceiver(this)
    this.receiver.$.bindHandle(pending.handle)
    this.receiver.onConnectionError.addListener(() => this.session.finish())
  }

  addAudioChunk(data: AudioData) {
    this.session.addAudio(new Float32Array(data.data))
  }
}

class SpeechRecognitionFactoryImpl
  implements SpeechRecognitionFactoryInterface
{
  private readonly receiver: SpeechRecognitionFactoryReceiver
  private model: OrtNemotronModel | null = null

  constructor() {
    this.receiver = new SpeechRecognitionFactoryReceiver(this)
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }

  async loadModel(files: OrtModelFiles) {
    try {
      this.model = await OrtNemotronModel.buildFromBytes(
        readBigBuffer(files.encoder, 'Encoder'),
        readBigBuffer(files.encoderData, 'EncoderData'),
        readBigBuffer(files.decoder, 'Decoder'),
        readBigBuffer(files.decoderData, 'DecoderData'),
        parseTokens(readBigBuffer(files.tokens, 'Tokens')),
        new Float32Array(
          readBigBuffer(files.melFilters, 'Filterbank').slice().buffer,
        ),
      )
      return { success: true }
    } catch (err) {
      console.error('[speech-worker-ort] loadModel failed:', err)
      return { success: false }
    }
  }

  createAsrStream(
    options: AsrStreamOptions,
    stream: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
  ) {
    if (!this.model) {
      console.error('[speech-worker-ort] createAsrStream before loadModel')
      return
    }
    new AsrStreamInputAdapter(this.model, stream, responder)
  }
}

installTrustedTypesPolicy()
const service = OnDeviceSpeechRecognitionService.getRemote()
const factory = new SpeechRecognitionFactoryImpl()
service.registerSpeechRecognitionFactory(factory.getPendingRemote())
