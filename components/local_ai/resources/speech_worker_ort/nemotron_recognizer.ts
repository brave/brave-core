// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Nemotron 0.6B streaming RNN-T over onnxruntime-web. Mojo-free: model
// loading from raw bytes plus a per-utterance streaming session that emits
// transcripts through a callback. The mojo transport glue that drives this
// lives in speech_worker_ort.ts.

import { argmax, computeNemoMel } from './audio_features'
import { disposeOrt, ensureOrt } from './ort_env'
import type { Ort, OrtSession, OrtSessionOptions, OrtTensor } from './ort_env'

// Parse a tokens.txt ("<token> <id>" per line, id-ordered, ▁ == space) into
// an id-indexed vocab array.
export function parseTokens(buf: Uint8Array): string[] {
  const vocab: string[] = []
  for (const line of new TextDecoder().decode(buf).split('\n')) {
    if (!line) {
      continue
    }
    const sp = line.lastIndexOf(' ')
    vocab[parseInt(line.slice(sp + 1), 10)] = line.slice(0, sp)
  }
  return vocab
}

// Free the model graph's JS backing store once ORT has copied it into its
// WASM heap. (ORT-Web references its own WASM copy, so the source is no
// longer needed.) transfer(0) detaches it immediately instead of waiting on
// GC of the mojo-mapped shared-memory region.
function releaseBytes(bytes: Uint8Array): void {
  try {
    const buffer = bytes.buffer as ArrayBuffer & {
      transfer?: (newLength?: number) => ArrayBuffer
    }
    buffer.transfer?.(0)
  } catch {
    // Backing store isn't transferable; GC will reclaim it eventually.
  }
}

// Nemotron 0.6B int8 — cache-aware streaming RNN-T. A fixed 560 ms chunk
// per step (56 new mel frames + 9 pre-encode cache frames) with conformer
// caches fed forward, so per-chunk encoder cost is constant.
const NEMO_CHUNK = 56 // new mel frames per step (560 ms @ 10 ms hop)
const NEMO_PRECACHE = 9 // pre-encode mel cache frames prepended
const NEMO_BLANK = 1024
const NEMO_VOCAB = 1025
const NEMO_MAX_SYM = 10
const NEMO_HOP = 160 // mel hop in samples (10 ms @ 16 kHz)
const NEMO_FRAMES = NEMO_PRECACHE + NEMO_CHUNK // 65: fixed encoder input length

// On finish, append this many chunks of silence so the cache-aware encoder
// and RNN-T decoder flush the final partial chunk — without it the trailing
// word(s) can be dropped to right-context / emission lag. The silence frames
// decode to RNN-T blanks, so the transcript is unaffected.
const SILENCE_FLUSH_CHUNKS = 3

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
  // .onnx + .onnx.data (external-data) export is supported for memory
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

// One Nemotron ASR session. Accumulates PCM, recomputes mel on the buffer,
// and feeds newly-available 560 ms chunks through the cache-aware encoder +
// RNN-T decoder, carrying conformer caches and LSTM state forward across the
// engine's ~500 ms flushes. Per-chunk encoder cost is constant. Mojo-free:
// transcripts are delivered through the onResult callback.
export class NemotronStreamSession {
  private readonly onResult: (text: string, isFinal: boolean) => void
  private readonly model: OrtNemotronModel
  private pcm: Float32Array = new Float32Array(0)
  private consumed = 0 // mel frames already fed to the encoder
  // Cache-aware conformer caches + RNN-T LSTM state, held as plain typed
  // arrays (not OrtTensors). ORT-Web output tensors share/free WASM heap
  // across run() calls, so carrying them forward as the next inputs is
  // unsafe; instead we copy each output's `.data` out and build fresh input
  // tensors every step. Cache layout is this export's native layers-first
  // [num_layers, batch, cache_len, d_model].
  private cacheCh = new Float32Array(24 * 70 * 1024)
  private cacheTime = new Float32Array(24 * 1024 * 8)
  private cacheLen = BigInt64Array.from([BigInt(0)])
  private readonly preMel = new Float32Array(128 * NEMO_PRECACHE)
  private st1 = new Float32Array(2 * 640)
  private st2 = new Float32Array(2 * 640)
  private prevToken = NEMO_BLANK // RNN-T predictor seed (BLANK, as in NeMo)
  private readonly hyp: number[] = []
  private closed = false
  private inflight: Promise<void> = Promise.resolve()

  constructor(
    model: OrtNemotronModel,
    sampleRateHz: number,
    onResult: (text: string, isFinal: boolean) => void,
  ) {
    this.model = model
    this.onResult = onResult
  }

  addAudio(samples: Float32Array) {
    if (this.closed) {
      return
    }
    const merged = new Float32Array(this.pcm.length + samples.length)
    merged.set(this.pcm)
    merged.set(samples, this.pcm.length)
    this.pcm = merged
    this.inflight = this.inflight.then(() => this.processAvailable(false))
  }

  // Flush the trailing words and emit the final transcript. Appends silence
  // so the streaming encoder/RNN-T drains the last partial chunk into full
  // 65-frame chunks (see SILENCE_FLUSH_CHUNKS). Idempotent: after the first
  // call addAudio() no-ops, so the final flush runs exactly once.
  finish() {
    if (this.closed) {
      return
    }
    this.closed = true
    const flush = SILENCE_FLUSH_CHUNKS * NEMO_CHUNK * NEMO_HOP
    const merged = new Float32Array(this.pcm.length + flush)
    merged.set(this.pcm) // trailing `flush` samples stay zero (silence)
    this.pcm = merged
    this.inflight = this.inflight.then(() => this.processAvailable(true))
  }

  // Process every full 560 ms chunk that has become available. finish()
  // appends silence before calling with final=true, so the trailing words
  // drain through the same full-chunk path. Emits an interim/final transcript.
  private async processAvailable(final: boolean) {
    const { ort, enc, dec, fbank, tokens } = this.model
    const { data: mel, T } = computeNemoMel(this.pcm, fbank)
    let emitted = false
    // Only ever feed full, fixed-size NEMO_FRAMES (65) chunks: 9 pre-encode
    // cache frames + 56 new. finish() appends silence so the trailing real
    // frames form a full chunk, keeping the encoder input shape invariant and
    // consumed frames clear of the mel's reflect-padded tail.
    while (T - this.consumed >= NEMO_CHUNK) {
      const start = this.consumed
      const sig = new Float32Array(128 * NEMO_FRAMES)
      for (let m = 0; m < 128; m++) {
        for (let j = 0; j < NEMO_PRECACHE; j++) {
          sig[m * NEMO_FRAMES + j] = this.preMel[m * NEMO_PRECACHE + j]
        }
        for (let j = 0; j < NEMO_CHUNK; j++) {
          sig[m * NEMO_FRAMES + NEMO_PRECACHE + j] = mel[m * T + start + j]
        }
      }
      // Fresh input tensors every step (never reuse/carry ORT tensors).
      const eo = await enc.run(
        {
          processed_signal: new ort.Tensor('float32', sig, [
            1,
            128,
            NEMO_FRAMES,
          ]),
          processed_signal_length: new ort.Tensor(
            'int64',
            BigInt64Array.from([BigInt(NEMO_FRAMES)]),
            [1],
          ),
          cache_last_channel: new ort.Tensor(
            'float32',
            this.cacheCh,
            [24, 1, 70, 1024],
          ),
          cache_last_time: new ort.Tensor(
            'float32',
            this.cacheTime,
            [24, 1, 1024, 8],
          ),
          cache_last_channel_len: new ort.Tensor('int64', this.cacheLen, [1]),
        },
        ENC_FETCHES,
      )
      // Copy every output's data into independent JS arrays BEFORE disposing
      // the ORT tensors. `encoded` is [batch, d_model, time]: the data stride
      // stays the full time (nTime), and the decode count is clamped to
      // `encoded_len` (trailing silence from finish() decodes to blanks).
      const encOut = (eo.encoded.data as Float32Array).slice()
      const nTime = eo.encoded.dims[2] as number
      const nEnc = Math.min(
        Number((eo.encoded_len.data as BigInt64Array)[0]),
        nTime,
      )
      this.cacheCh = (eo.cache_last_channel_next.data as Float32Array).slice()
      this.cacheTime = (eo.cache_last_time_next.data as Float32Array).slice()
      this.cacheLen = (
        eo.cache_last_channel_len_next.data as BigInt64Array
      ).slice()
      disposeOrt([], eo as unknown as Record<string, OrtTensor>)
      // pre-encode mel cache = last 9 consumed frames.
      for (let m = 0; m < 128; m++) {
        for (let j = 0; j < NEMO_PRECACHE; j++) {
          const sf = start + NEMO_CHUNK - NEMO_PRECACHE + j
          this.preMel[m * NEMO_PRECACHE + j] = sf >= 0 ? mel[m * T + sf] : 0
        }
      }
      // RNN-T greedy decode over this chunk's encoder frames.
      for (let fi = 0; fi < nEnc; fi++) {
        const frame = new Float32Array(1024)
        for (let c = 0; c < 1024; c++) {
          frame[c] = encOut[c * nTime + fi]
        }
        let sym = 0
        while (sym < NEMO_MAX_SYM) {
          const dout = await dec.run({
            encoder_outputs: new ort.Tensor('float32', frame, [1, 1024, 1]),
            targets: new ort.Tensor(
              'int32',
              Int32Array.from([this.prevToken]),
              [1, 1],
            ),
            target_length: new ort.Tensor('int32', Int32Array.from([1]), [1]),
            input_states_1: new ort.Tensor('float32', this.st1, [2, 1, 640]),
            input_states_2: new ort.Tensor('float32', this.st2, [2, 1, 640]),
          })
          const tok = argmax(dout.outputs.data as Float32Array, 0, NEMO_VOCAB)
          if (tok !== NEMO_BLANK) {
            this.hyp.push(tok)
            this.prevToken = tok
            this.st1 = (dout.output_states_1.data as Float32Array).slice()
            this.st2 = (dout.output_states_2.data as Float32Array).slice()
          }
          disposeOrt([], dout as unknown as Record<string, OrtTensor>)
          if (tok === NEMO_BLANK) {
            break
          }
          sym++
        }
      }
      this.consumed += NEMO_CHUNK
      emitted = true
    }
    // Bound memory: computeNemoMel runs over the whole pcm each step, so
    // drop audio well before the consumed boundary (keeping a margin so the
    // next chunk's STFT context is intact) and realign `consumed`. The
    // pre-encode cache (preMel) and conformer caches are kept separately,
    // so only the raw pcm is shifted.
    const KEEP_FRAMES = 32 // ~320 ms; larger than the STFT window
    if (!final && this.consumed > KEEP_FRAMES) {
      this.pcm = this.pcm.slice((this.consumed - KEEP_FRAMES) * 160) // HOP
      this.consumed = KEEP_FRAMES
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
}
