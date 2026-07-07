// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Nemotron 0.6B streaming RNN-T over onnxruntime-web. Mojo-free: model
// loading from raw bytes plus a per-utterance streaming session that emits
// transcripts through a callback. The mojo transport glue that drives this
// lives in speech_worker_ort.ts.

import { ensureOrt } from './ort_env'
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

// One streaming ASR session. Accepts PCM via addAudio() and delivers
// interim/final transcripts through the onResult callback. finish() flushes
// the final transcript at end-of-utterance.
export class NemotronStreamSession {
  constructor(
    model: OrtNemotronModel,
    sampleRateHz: number,
    onResult: (text: string, isFinal: boolean) => void,
  ) {}

  // Feed mono 16 kHz PCM as it arrives; streams interim transcripts.
  addAudio(samples: Float32Array): void {
    throw new Error('not implemented')
  }

  // End of utterance: flush and emit the final transcript via onResult.
  finish(): void {
    throw new Error('not implemented')
  }
}
