// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Owns loading and configuring the onnxruntime-web environment for this
// worker. The loader is bundled into this worker's script by webpack, and its
// pthread worker glue and threaded .wasm are emitted as webpack assets served
// from this origin.
import * as ort from 'onnxruntime-web/wasm'

// The threaded .wasm is located by ORT only through ort.env.wasm.wasmPaths at
// runtime, a string webpack cannot trace, so it is not emitted on its own.
// Import it explicitly so webpack emits it and returns its URL for wasmPaths.
import wasmUrl from 'onnxruntime-web/ort-wasm-simd-threaded.wasm'

let ortConfigured = false

export type Ort = typeof import('onnxruntime-web')
export type OrtTensor = import('onnxruntime-web').Tensor
export type OrtSession = import('onnxruntime-web').InferenceSession
export type OrtSessionOptions =
  import('onnxruntime-web').InferenceSession.SessionOptions

// The webpack-emitted worker glue ORT starts as a pthread Worker, named
// ort.wasm.bundle.min.<contenthash>.js. It is the only URL that should reach
// the Trusted Types createScriptURL hook, pinned by prefix and suffix since
// the content hash varies per build.
const ORT_WORKER_SCRIPT_PREFIX = '/ort.wasm.bundle.min.'
const ORT_WORKER_SCRIPT_SUFFIX = '.js'

const NUM_THREADS = Math.min(4, self.navigator.hardwareConcurrency || 4)

// Trusted Types default policy: onnxruntime-web creates its thread-pool
// workers via `new Worker(<url string>)`, which Trusted Types routes
// through the default policy. Allow ONLY our own same-origin worker glue and
// reject everything else.
export function installTrustedTypesPolicy() {
  const tt = (
    self as unknown as {
      trustedTypes?: {
        createPolicy: (name: string, rules: object) => unknown
      }
    }
  ).trustedTypes
  if (!tt) {
    return
  }
  tt.createPolicy('default', {
    createScriptURL: (url: string) => {
      const u = new URL(url, location.href)
      // Pin to the same-origin webpack-emitted worker glue
      // (ort.wasm.bundle.min.<contenthash>.js). search and hash are ignored so
      // a cache-busting query cannot change the decision. Everything else
      // (other files, cross-origin, blob: URLs) is rejected.
      if (
        u.origin === location.origin
        && u.pathname.startsWith(ORT_WORKER_SCRIPT_PREFIX)
        && u.pathname.endsWith(ORT_WORKER_SCRIPT_SUFFIX)
      ) {
        return url
      }
      throw new TypeError('Trusted Types: blocked script URL ' + url)
    },
  })
}

// Configure the threaded WASM backend once and return the bundled ORT
// namespace. wasmPaths.wasm points ORT at the webpack-emitted .wasm asset.
export async function ensureOrt(): Promise<Ort> {
  if (!ortConfigured) {
    ort.env.wasm.numThreads = NUM_THREADS
    ort.env.wasm.simd = true
    ort.env.wasm.wasmPaths = { wasm: wasmUrl }
    ortConfigured = true
  }
  return ort
}

// ORT-Web tensors back their data with WASM memory that is NOT freed by
// JS GC alone — dispose inputs and outputs after each inference or they
// accumulate in the WASM heap.
export function disposeOrt(
  inputs: OrtTensor[],
  out?: Record<string, OrtTensor>,
) {
  for (const t of inputs) {
    try {
      t.dispose()
    } catch {
      /* ignore */
    }
  }
  if (out) {
    for (const k in out) {
      try {
        out[k].dispose()
      } catch {
        /* ignore */
      }
    }
  }
}
