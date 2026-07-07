// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Nemotron 0.6B streaming RNN-T over onnxruntime-web. Mojo-free: model
// loading from raw bytes plus a per-utterance streaming session that emits
// transcripts through a callback. The mojo transport glue that drives this
// lives in speech_worker_ort.ts.

import {
  hannWindow,
  initFftPower,
  StreamingMelFrontend,
} from './audio_features'
import { disposeOrt, ensureOrt } from './ort_env'
import type { Ort, OrtSession, OrtSessionOptions, OrtTensor } from './ort_env'
import * as config from './configs'
import type { FftSize } from './configs'
import { releaseBytes, argmax } from './utils'

// Encoder outputs we consume. `encoded_len` is the count of valid encoder
// frames; the input length is fixed every call so it normally equals the
// full time, but we clamp the decode to it defensively.
const ENC_FETCHES = [
  'encoded',
  'encoded_len',
  'cache_last_channel_next',
  'cache_last_time_next',
  'cache_last_channel_len_next',
]

// Shared, loaded-once Nemotron model: the external-data encoder + RNN-T
// decoder sessions plus tokens and the 128-mel filterbank.
export class OrtNemotronModel {
  readonly ort: Ort
  readonly tokens: string[]
  readonly fbank: Float32Array
  readonly hann: Float32Array

  // The encoder and decoder sessions are private on purpose: every run must go
  // through runEncoder/runDecoder so it is serialized against other streams.
  private enc: OrtSession
  private dec: OrtSession

  // Every run goes through this one queue so runs from different streams never
  // overlap on the shared sessions. onnxruntime-web keeps a single WASM stack
  // per module and does not lock run(), so overlapping runs corrupt that stack.
  private queue: Promise<unknown> = Promise.resolve()

  constructor(
    ort: Ort,
    enc: OrtSession,
    dec: OrtSession,
    tokens: string[],
    fbank: Float32Array,
  ) {
    this.ort = ort
    this.enc = enc
    this.dec = dec
    this.tokens = tokens
    this.fbank = fbank
    this.hann = hannWindow()
  }

  // Serialized encoder/decoder runs. These are the only way to reach the
  // shared sessions, so no two streams' runs ever overlap. A failed run is
  // isolated on the chain so it cannot stall later runs for other streams,
  // while its rejection still propagates to the caller.
  runEncoder(feeds: Record<string, OrtTensor>, fetches: readonly string[]) {
    return this.serialized(() => this.enc.run(feeds, fetches as string[]))
  }

  runDecoder(feeds: Record<string, OrtTensor>) {
    return this.serialized(() => this.dec.run(feeds))
  }

  private serialized<T>(run: () => Promise<T>): Promise<T> {
    const result = this.queue.then(run)
    this.queue = result.catch(() => {})
    return result
  }

  // Build the encoder + decoder sessions from mojo BigBuffers. Only the
  // .onnx + .onnx.data external-data export is supported for memory
  // optimization.
  static async buildFromBytes(
    encoderGraph: Uint8Array,
    encData: Uint8Array,
    decoderGraph: Uint8Array,
    decData: Uint8Array,
    tokens: string[],
    fbank: Float32Array,
  ): Promise<OrtNemotronModel> {
    const ort = await ensureOrt()

    const sessionOptions = (
      data: Uint8Array,
      dataPath: string,
    ): OrtSessionOptions => ({
      executionProviders: ['wasm'],

      // int4 prepacking gives no speedup on the external-data path, so disable
      // it to avoid the extra prepacked-weight copy in WASM memory.
      extra: {
        session: {
          disable_prepacking: '1',
        },
      },

      externalData: [{ path: dataPath, data }],
    })

    const enc = await ort.InferenceSession.create(
      encoderGraph,
      sessionOptions(encData, 'encoder.onnx.data'),
    )
    releaseBytes(encoderGraph)
    releaseBytes(encData)

    const dec = await ort.InferenceSession.create(
      decoderGraph,
      sessionOptions(decData, 'decoder_joint.onnx.data'),
    )
    releaseBytes(decoderGraph)
    releaseBytes(decData)

    return new OrtNemotronModel(ort, enc, dec, tokens, fbank)
  }
}

// One Nemotron ASR session. Uses an incremental streaming mel frontend:
// raw samples are appended, only newly stable mel frames are computed, and
// fixed 65-frame encoder chunks are packed from cached mel frames. Mojo-free:
// transcripts are delivered through the onResult callback.
export class NemotronStreamSession {
  private readonly onResult: (text: string, isFinal: boolean) => void
  private readonly model: OrtNemotronModel
  private readonly sampleRateHz: number
  private readonly frontend: StreamingMelFrontend

  // Attention cache.
  private cacheCh = new Float32Array(
    config.NEMO_NUM_ENCODER_LAYERS
      * config.NEMO_LEFT_CONTEXT
      * config.NEMO_HIDDEN_DIM,
  )

  // Convolution cache.
  private cacheTime = new Float32Array(
    config.NEMO_NUM_ENCODER_LAYERS
      * config.NEMO_HIDDEN_DIM
      * config.NEMO_CONV_CONTEXT,
  )

  // Indicates how much of left context at encoder-output level is valid.
  // This is different from left context at mel-frame level.
  private cacheLen = BigInt64Array.from([BigInt(0)])

  // Decoder LSTM state cache; 2 LSTM layers.
  private st1 = new Float32Array(2 * config.NEMO_DECODER_LSTM_DIM)
  private st2 = new Float32Array(2 * config.NEMO_DECODER_LSTM_DIM)

  // RNN-T predictor seed. NeMo uses BLANK as the initial previous token.
  private prevToken = config.NEMO_BLANK

  // Hypothesis token IDs.
  private readonly hyp: number[] = []

  // ASR stream is closed once finish() is called. After that, addAudio() no-ops.
  private closed = false

  // Prevents overlapping async inference calls within the same stream.
  private inflight: Promise<void> = Promise.resolve()

  // Debug chunk counter.
  private chunkIdx = 0

  constructor(
    model: OrtNemotronModel,
    sampleRateHz: number,
    onResult: (text: string, isFinal: boolean) => void,
  ) {
    this.model = model
    this.sampleRateHz = sampleRateHz
    this.onResult = onResult
    this.frontend = new StreamingMelFrontend(
      model.fbank,
      model.hann,
      initFftPower(config.N_FFT as FftSize),
    )
  }

  addAudio(samples: Float32Array): void {
    if (this.closed) {
      return
    }

    this.frontend.appendAudioSamples(samples)

    if (config.DEBUG) {
      this.debug('audio appended', {
        samples: samples.length,
        ms: this.samplesToMs(samples.length),
        frontend: this.frontend.debugState(this.sampleRateHz),
      })
    }

    this.inflight = this.inflight.then(() => this.processAvailable(false))
  }

  // Flush the trailing words and emit the final transcript. Appends silence
  // so the streaming encoder/RNN-T drains the last partial chunk into full
  // 65-frame chunks. Idempotent: after the first call addAudio() no-ops, so
  // the final flush runs exactly once.
  finish(): void {
    if (this.closed) {
      return
    }

    this.closed = true

    const flush =
      config.SILENCE_FLUSH_CHUNKS * config.NEMO_CHUNK * config.HOP_LENGTH

    this.frontend.appendAudioSamples(new Float32Array(flush))

    if (config.DEBUG) {
      this.debug('finish requested, appended silence', {
        flushSamples: flush,
        flushMs: this.samplesToMs(flush),
        frontend: this.frontend.debugState(this.sampleRateHz),
      })
    }

    this.inflight = this.inflight
      .then(() => this.processAvailable(true))
      .then(() => this.wipe())
  }

  // Zero every per-session buffer holding audio, acoustic features, or
  // transcript once the final result has emitted, so the utterance does not
  // linger in the JS heap until garbage collection. The shared model weights
  // are untouched.
  private wipe(): void {
    this.frontend.wipe()
    this.hyp.fill(0)
    this.hyp.length = 0
  }

  private async processAvailable(final: boolean): Promise<void> {
    const { ort, tokens } = this.model
    let emitted = false

    while (this.frontend.hasFullChunk()) {
      const chunkIdx = this.chunkIdx++

      const frontendBefore = config.DEBUG
        ? this.frontend.debugState(this.sampleRateHz)
        : undefined

      const chunkStarted = performance.now()

      const packStarted = performance.now()
      const sig = this.frontend.makeNextEncoderInput()
      const packMs = performance.now() - packStarted

      // Fresh input tensors every step. Do not reuse/carry ORT tensors.
      const encoderStarted = performance.now()
      const eo = await this.model.runEncoder(
        {
          processed_signal: new ort.Tensor('float32', sig, [
            1,
            config.N_MELS,
            config.NEMO_FRAMES,
          ]),

          processed_signal_length: new ort.Tensor(
            'int64',
            BigInt64Array.from([BigInt(config.NEMO_FRAMES)]),
            [1],
          ),

          cache_last_channel: new ort.Tensor('float32', this.cacheCh, [
            config.NEMO_NUM_ENCODER_LAYERS,
            1,
            config.NEMO_LEFT_CONTEXT,
            config.NEMO_HIDDEN_DIM,
          ]),

          cache_last_time: new ort.Tensor('float32', this.cacheTime, [
            config.NEMO_NUM_ENCODER_LAYERS,
            1,
            config.NEMO_HIDDEN_DIM,
            config.NEMO_CONV_CONTEXT,
          ]),

          cache_last_channel_len: new ort.Tensor('int64', this.cacheLen, [1]),
        },
        ENC_FETCHES,
      )
      const encoderMs = performance.now() - encoderStarted

      // Copy data out before disposing ORT tensors.
      // Encoder output is laid out as [batch, hidden_dim, time].
      const encOut = (eo.encoded.data as Float32Array).slice()

      // Number of time frames at output of encoder.
      const nTime = Number(eo.encoded.dims[2])

      // Number of valid encoder frames to actually decode.
      const nEnc = Math.min(
        Number((eo.encoded_len.data as BigInt64Array)[0]),
        nTime,
      )

      // Update encoder caches.
      this.cacheCh = (eo.cache_last_channel_next.data as Float32Array).slice()

      this.cacheTime = (eo.cache_last_time_next.data as Float32Array).slice()

      // Grows until NEMO_LEFT_CONTEXT and then stays there.
      this.cacheLen = (
        eo.cache_last_channel_len_next.data as BigInt64Array
      ).slice()

      disposeOrt([], eo as unknown as Record<string, OrtTensor>)

      // RNN-T greedy decode over this chunk's encoder frames.
      const decodeStarted = performance.now()
      let decoderCalls = 0
      let emittedTokensThisChunk = 0
      let maxSymHits = 0

      // Outer loop over encoder time index.
      for (let fi = 0; fi < nEnc; fi++) {
        const frame = new Float32Array(config.NEMO_HIDDEN_DIM)

        // Inner loop over channel/hidden-dim index.
        for (let c = 0; c < config.NEMO_HIDDEN_DIM; c++) {
          frame[c] = encOut[c * nTime + fi]
        }

        let sym = 0

        // Decoder processes each time frame, aggregated across all channels.
        while (sym < config.NEMO_MAX_SYM) {
          const dout = await this.model.runDecoder({
            encoder_outputs: new ort.Tensor('float32', frame, [
              1,
              config.NEMO_HIDDEN_DIM,
              1,
            ]),

            targets: new ort.Tensor(
              'int32',
              Int32Array.from([this.prevToken]),
              [1, 1],
            ),

            target_length: new ort.Tensor('int32', Int32Array.from([1]), [1]),

            input_states_1: new ort.Tensor('float32', this.st1, [
              2,
              1,
              config.NEMO_DECODER_LSTM_DIM,
            ]),

            input_states_2: new ort.Tensor('float32', this.st2, [
              2,
              1,
              config.NEMO_DECODER_LSTM_DIM,
            ]),
          })

          decoderCalls++

          const tok = argmax(
            dout.outputs.data as Float32Array,
            0,
            config.NEMO_VOCAB,
          )

          if (tok !== config.NEMO_BLANK) {
            this.hyp.push(tok)
            this.prevToken = tok
            this.st1 = (dout.output_states_1.data as Float32Array).slice()
            this.st2 = (dout.output_states_2.data as Float32Array).slice()
            emittedTokensThisChunk++
          }

          disposeOrt([], dout as unknown as Record<string, OrtTensor>)

          if (tok === config.NEMO_BLANK) {
            break
          }

          sym++
        }

        if (sym === config.NEMO_MAX_SYM) {
          maxSymHits++
        }
      }

      const decodeMs = performance.now() - decodeStarted

      this.frontend.consumeChunk()

      if (config.DEBUG) {
        const modelMs = encoderMs + decodeMs
        const totalMs = performance.now() - chunkStarted
        const chunkAudioMs = this.samplesToMs(
          config.NEMO_CHUNK * config.HOP_LENGTH,
        )

        this.debug('chunk processed', {
          chunkIdx,

          packMs,
          encoderMs,
          decodeMs,
          modelMs,
          totalMs,

          chunkAudioMs,
          modelRtf: modelMs / chunkAudioMs,
          totalRtf: totalMs / chunkAudioMs,

          nTime,
          nEnc,
          decoderCalls,
          emittedTokensThisChunk,
          hypTokensTotal: this.hyp.length,
          maxSymHits,

          cacheLen: Number(this.cacheLen[0]),

          frontendBefore,
          frontendAfter: this.frontend.debugState(this.sampleRateHz),
        })
      }

      emitted = true
    }

    if (emitted || final) {
      const text = this.hyp
        .map((id) => tokens[id] ?? '')
        .join('')
        .replace(/▁/g, ' ')
        .trim()

      if (text || final) {
        this.onResult(text, final)
      }
    }
  }

  private samplesToMs(samples: number): number {
    return (samples * 1000) / this.sampleRateHz
  }

  private debug(message: string, details?: Record<string, unknown>): void {
    if (!config.DEBUG) {
      return
    }

    const json = JSON.stringify(details ?? {}, (_key, value: unknown) =>
      typeof value === 'bigint' ? value.toString() : value,
    )

    console.error(`[NEMO_DEBUG] ${message} ${json}`)
  }
}
