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
  OrtModelFiles,
  SpeechRecognitionFactoryHost,
  SpeechRecognitionFactoryInterface,
  SpeechRecognitionFactoryReceiver,
} from 'gen/brave/components/local_ai/core/on_device_speech_recognition.mojom.m.js'
import {
  AsrStreamInputInterface,
  AsrStreamInputPendingReceiver,
  AsrStreamInputReceiver,
  AsrStreamOptions,
  AsrStreamResponderRemote,
  AudioData,
} from 'gen/services/on_device_model/public/mojom/on_device_model.mojom.m.js'

import { installTrustedTypesPolicy } from './ort_env'
import { NemotronStreamSession, OrtNemotronModel } from './nemotron_recognizer'

import { parseTokens, readBigBuffer } from './utils'

// Thin mojo endpoint for one ASR stream: owns the receiver and responder
// and bridges AsrStreamInput calls to a mojo-free NemotronStreamSession.
// Engine disconnect (~end of utterance) triggers the final flush.
class AsrStreamInputAdapter implements AsrStreamInputInterface {
  private readonly receiver: AsrStreamInputReceiver
  private readonly session: NemotronStreamSession

  constructor(
    model: OrtNemotronModel,
    sampleRateHz: number,
    pending: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
    onClose: () => void,
  ) {
    this.session = new NemotronStreamSession(
      model,
      sampleRateHz,
      (text, isFinal) => {
        // Empty transcript means nothing was recognized. Send an empty
        // vector rather than a bogus [FINAL] "" (still ends the session).
        responder.onResponse(text ? [{ transcript: text, isFinal }] : [])
      },
    )
    this.receiver = new AsrStreamInputReceiver(this)
    this.receiver.$.bindHandle(pending.handle)
    this.receiver.onConnectionError.addListener(() => {
      this.session.finish()
      onClose()
    })
  }

  addAudioChunk(data: AudioData) {
    this.session.addAudio(new Float32Array(data.data))
  }
}

class SpeechRecognitionFactoryImpl
  implements SpeechRecognitionFactoryInterface
{
  private readonly receiver: SpeechRecognitionFactoryReceiver
  private readonly streams = new Set<AsrStreamInputAdapter>()
  private model: OrtNemotronModel | null = null

  constructor() {
    this.receiver = new SpeechRecognitionFactoryReceiver(this)
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }

  async init(files: OrtModelFiles) {
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
      console.error('[speech-worker] init failed:', err)
      return { success: false }
    }
  }

  createAsrStream(
    options: AsrStreamOptions,
    stream: AsrStreamInputPendingReceiver,
    responder: AsrStreamResponderRemote,
  ) {
    if (!this.model) {
      console.error('[speech-worker] createAsrStream before init')
      return
    }
    const adapter = new AsrStreamInputAdapter(
      this.model,
      options.sampleRateHz,
      stream,
      responder,
      () => this.streams.delete(adapter),
    )
    this.streams.add(adapter)
  }
}

installTrustedTypesPolicy()
const factoryHost = SpeechRecognitionFactoryHost.getRemote()
const factory = new SpeechRecognitionFactoryImpl()
factoryHost.registerFactory(factory.getPendingRemote())
