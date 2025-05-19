/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const {
  WEBASSEMBLY_MODULE_TYPE_ASYNC
} = require('webpack/lib/ModuleTypeConstants')
const AsyncWasmLoadingRuntimeModule = require('webpack/lib/wasm-async/AsyncWasmLoadingRuntimeModule')

const PLUGIN_NAME = 'XHRCompileAsyncWasmPlugin'

class XHRCompileAsyncWasmPlugin {
  type = 'xhr'

  apply(compiler) {
    const { webpack } = compiler
    const { RuntimeGlobals, Template, wasm } = webpack
    wasm.EnableWasmLoadingPlugin.setEnabled(compiler, this.type)

    compiler.hooks.thisCompilation.tap(PLUGIN_NAME, (compilation) => {
      const { outputOptions, runtimeTemplate } = compilation
      const globalWasmLoading = outputOptions.wasmLoading

      const isEnabledForChunk = (chunk) => {
        const options = chunk.getEntryOptions()
        const wasmLoading =
          options && options.wasmLoading !== undefined
            ? options.wasmLoading
            : globalWasmLoading
        return wasmLoading === this.type
      }

      const generateLoadBinaryCode = (path) =>
        `(${runtimeTemplate.basicFunction('', [
          'const { promise, resolve } = Promise.withResolvers();',
          'const request = new XMLHttpRequest();',
          `request.onload = ${runtimeTemplate.returningFunction(
            'resolve({ arrayBuffer() { return request.response }})',
            ''
          )};`,
          `request.open("GET", ${RuntimeGlobals.publicPath} + ${path});`,
          'request.responseType = "arraybuffer";',
          'request.send();',
          'return promise;'
        ])})()`

      compilation.hooks.runtimeRequirementInTree
        .for(RuntimeGlobals.instantiateWasm)
        .tap(PLUGIN_NAME, (chunk, set, { chunkGraph }) => {
          if (!isEnabledForChunk(chunk)) return
          if (
            !chunkGraph.hasModuleInGraph(
              chunk,
              (m) => m.type === WEBASSEMBLY_MODULE_TYPE_ASYNC
            )
          ) {
            return
          }
          set.add(RuntimeGlobals.publicPath)
          compilation.addRuntimeModule(
            chunk,
            new (class extends AsyncWasmLoadingRuntimeModule {
              constructor(generateLoadBinaryCode) {
                super({ generateLoadBinaryCode, supportsStreaming: false })
              }
            })(generateLoadBinaryCode)
          )
        })
    })
  }
}

module.exports = XHRCompileAsyncWasmPlugin
