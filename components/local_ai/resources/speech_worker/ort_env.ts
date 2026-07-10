// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Owns loading and configuring the onnxruntime-web environment for this
// worker. ORT is loaded at runtime as a served ES module rather than
// bundled by webpack: webpack rewrites ORT's internal worker/wasm URLs,
// which breaks multi-threaded pthread worker creation. `import type` keeps
// the types without emitting a runtime import. The absolute
// chrome-untrusted:// URL is externalized by Brave's webpack config (any
// chrome(-untrusted):// import is left as a runtime `import`), so webpack
// never touches ORT.
let ort!: typeof import('onnxruntime-web')
let ortLoaded = false

export type Ort = typeof import('onnxruntime-web')
export type OrtTensor = import('onnxruntime-web').Tensor
export type OrtSession = import('onnxruntime-web').InferenceSession
export type OrtSessionOptions =
  import('onnxruntime-web').InferenceSession.SessionOptions

// The ORT distribution files are bundled into the worker's pak (built by
// the ort_dist_generated target in this folder's BUILD.gn) and served
// from this origin under /ort-dist/.
const ORT_DIST_PATH = '/ort-dist/'

// The single worker-glue script ORT instantiates as a pthread Worker
// (ort.env.wasm.wasmPaths.mjs below). It is the ONLY URL that should ever
// reach the Trusted Types createScriptURL hook — the main bundle is loaded
// via dynamic import() (governed by CSP script-src, not Trusted Types) and
// the .wasm is fetched, not run as a worker. So the policy pins to this
// exact path rather than allowing the whole ort-dist directory.
// Served as .js (not its source .mjs name): WebUIDataSource's GetMimeType has
// no .mjs case and would fall back to text/html, which blocks the module.
const ORT_WORKER_SCRIPT = ORT_DIST_PATH + 'ort-wasm-simd-threaded.js'

const NUM_THREADS = Math.min(4, self.navigator.hardwareConcurrency || 4)

// Trusted Types default policy: onnxruntime-web creates its thread-pool
// workers via `new Worker(<url string>)`, which Trusted Types routes
// through the default policy. Allow ONLY our own same-origin ort-dist
// worker scripts; reject everything else.
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
      // Pin to the one same-origin worker script. search/hash are ignored
      // so a cache-busting query can't change the decision; everything
      // else (other files, cross-origin, blob: URLs) is rejected.
      if (u.origin === location.origin && u.pathname === ORT_WORKER_SCRIPT) {
        return url
      }
      throw new TypeError('Trusted Types: blocked script URL ' + url)
    },
  })
}

// Load ORT (runtime ES module), configure the threaded WASM backend, and
// return the loaded namespace.
export async function ensureOrt(): Promise<Ort> {
  const ortBase = location.origin + ORT_DIST_PATH
  if (!ortLoaded) {
    ort = await import(
      // @ts-expect-error — absolute WebUI URL resolved at runtime, not by
      // webpack (externalized by components/webpack/webpack.config.js).
      'chrome-untrusted://on-device-speech-recognition-worker/ort-dist/ort.wasm.bundle.min.js'
    )
    ortLoaded = true
  }
  ort.env.wasm.numThreads = NUM_THREADS
  ort.env.wasm.simd = true
  ort.env.wasm.wasmPaths = {
    wasm: ortBase + 'ort-wasm-simd-threaded.wasm',
    mjs: location.origin + ORT_WORKER_SCRIPT,
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
