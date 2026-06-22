// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'fs'
import path from 'path'
import type { Configuration } from 'webpack'
import {
  deterministicOptimization,
  deterministicIdsPlugins,
} from './deterministic-output.ts'
import generatePathMap from './path-map.js'
import { provideNodeGlobals, chromePrefixReplacers } from './plugins.ts'
import { baseResolve } from './resolve.ts'
import {
  cssRules,
  tsLoaderRule,
  ifdefLoaderRule,
  fileLoaderRule,
  braveUiFullySpecifiedRule,
  htmlAssetRule,
} from './rules.ts'
import GenerateDepfilePlugin from './webpack-plugin-depfile.js'
import XHRCompileAsyncWasmPlugin from './xhr-compile-async-wasm-plugin.js'

const rootGenDir = process.env.ROOT_GEN_DIR as string
const pathMap = generatePathMap(rootGenDir)
const buildFlags = JSON.parse(
  fs.readFileSync(path.join(rootGenDir, 'brave/build_flags.json'), 'utf8'),
)
const tsConfigPath = path.join(rootGenDir, 'tsconfig-webpack.json')

export default async function (env: any, argv: any): Promise<Configuration> {
  const isDevMode = argv.mode === 'development'
  // webpack-cli no longer allows specifying entry name in cli args, so use
  // a custom env param and parse ourselves.
  const entry: Record<string, string> = {}
  if (!env.brave_entries) {
    throw new Error(
      'Entry point(s) must be provided via env.brave_entries param.',
    )
  }
  const entryInput = env.brave_entries.split(',').sort()
  for (const entryInputItem of entryInput) {
    const entryInputItemParts = entryInputItem.split('=')
    if (entryInputItemParts.length !== 2) {
      throw new Error(
        'Brave Webpack config could not parse entry env param item: '
          + entryInputItem,
      )
    }
    entry[entryInputItemParts[0]] = entryInputItemParts[1]
  }

  const resolve = baseResolve(pathMap)

  if (env.extra_modules) {
    const extraModules = env.extra_modules.split(',')
    resolve.modules = [...extraModules, ...(resolve.modules as string[])]
  }

  if (env.webpack_aliases) {
    resolve.aliasFields = env.webpack_aliases.split(',')
  }

  const output: Configuration['output'] = {
    iife: !env.no_iife,
    path: process.env.TARGET_GEN_DIR,
    filename: '[name].bundle.js',
    chunkFilename: '[name].chunk.js',
    publicPath: '/',
  }

  if (env.output_module) {
    output.library = { type: 'module' }
    output.iife = false
  }

  if (env.output_public_path) {
    output.publicPath = env.output_public_path
  }

  const experiments: Configuration['experiments'] = {
    outputModule: Boolean(env.output_module),
  }
  if (env.sync_wasm) {
    experiments.syncWebAssembly = true
  } else {
    experiments.asyncWebAssembly = true
    output.enabledWasmLoadingTypes = ['xhr']
    output.wasmLoading = 'xhr'
  }

  // Treats `chrome://` and `chrome-untrusted://` specifiers as external `module`
  // imports rather than bundling them. Only applies when emitting a module
  // bundle. Returns a single externals function; add it to the `externals` array.
  //
  const chromeUrlExternals = function (
    { request }: { context?: string; request?: string },
    callback: (err?: null | Error, result?: string) => void,
  ) {
    if (
      env.output_module
      && request
      && /^chrome(-untrusted)?:\/\//.test(request)
    ) {
      return callback(null, 'module ' + request)
    }
    callback()
  }

  // Serve common libraries from shared resources
  const reactExternals: Configuration['externals'] =
    env.output_module && !('no_externals' in env)
      ? {
          // React and ReactDOM ship in a single bundle (see
          // brave/ui/webui/resources/react/initialize_bundle.ts).
          'react': ['module //resources/brave/react.bundle.js', 'React'],
          'react-dom': ['module //resources/brave/react.bundle.js', 'ReactDOM'],
        }
      : {}

  return {
    entry,
    devtool: isDevMode ? 'inline-source-map' : false,
    output,
    resolve,
    optimization: deterministicOptimization,
    experiments,
    externals: [chromeUrlExternals, reactExternals],
    plugins: [
      process.env.DEPFILE_SOURCE_NAME
        && new GenerateDepfilePlugin({
          depfilePath: process.env.DEPFILE_PATH,
          depfileSourceName: process.env.DEPFILE_SOURCE_NAME,
        }),
      ...deterministicIdsPlugins(),
      provideNodeGlobals,
      ...chromePrefixReplacers(pathMap),
      !env.sync_wasm && new XHRCompileAsyncWasmPlugin(),
    ],
    module: {
      rules: [
        ...cssRules({ isDevMode }),
        tsLoaderRule({ configFile: tsConfigPath }),
        ifdefLoaderRule(buildFlags),
        fileLoaderRule(),
        // web-discovery-project is built as CommonJS but may be classified
        // as ESM by webpack. Force auto-detection for correct CJS handling.
        {
          test: /\.js$/,
          include: /web-discovery-project/,
          type: 'javascript/auto',
        },
        braveUiFullySpecifiedRule,
        htmlAssetRule,
      ],
    },
  }
}
