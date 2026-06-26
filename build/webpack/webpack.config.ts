// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
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
import type { TranspileWebUiCliOptions } from './transpile-web-ui-command.ts'
import GenerateDepfilePlugin from './webpack-plugin-depfile.js'
import XHRCompileAsyncWasmPlugin from './xhr-compile-async-wasm-plugin.js'

export function createWebpackConfig(
  env: TranspileWebUiCliOptions,
): Configuration {
  const rootGenDir = path.resolve(env.root_gen_dir)
  const pathMap = generatePathMap(rootGenDir)
  const buildFlags = JSON.parse(
    fs.readFileSync(path.join(rootGenDir, 'brave/build_flags.json'), 'utf8'),
  )
  const tsConfigPath = path.join(rootGenDir, 'tsconfig-webpack.json')

  const isDevMode = env.mode === 'development'
  // webpack-cli no longer allows specifying entry name in cli args, so use
  // a custom env param and parse ourselves.
  const entry: Record<string, string> = {}
  for (const entryInputItem of env.entry) {
    const entryInputItemParts = entryInputItem.split('=', 2)
    if (entryInputItemParts.length !== 2) {
      throw new Error(
        'Brave Webpack config could not parse entry env param item: '
          + entryInputItem,
      )
    }
    const [key, value] = entryInputItemParts as [string, string]
    entry[key] = value
  }

  const resolve = baseResolve(pathMap)

  if (env.extra_modules.length > 0) {
    resolve.modules = [...env.extra_modules, ...resolve.modules!]
  }

  if (env.webpack_alias.length > 0) {
    resolve.aliasFields = env.webpack_alias
  }

  const output: NonNullable<Configuration['output']> = {
    iife: !env.no_iife,
    path: path.resolve(env.output_dir), // Must be absolute path
    filename: '[name].bundle.js',
    chunkFilename: '[name].chunk.js',
    publicPath: '/',
  }

  if (env.output_module) {
    output.library = { type: 'module' }
    output.iife = false
  }

  if (env.public_asset_path) {
    output.publicPath = env.public_asset_path
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
    mode: env.mode,
    context: path.resolve(env.webpack_context_dir), // Must be absolute path
    devtool: isDevMode ? 'inline-source-map' : false,
    output,
    resolve,
    optimization: deterministicOptimization,
    experiments,
    externals: [chromeUrlExternals, reactExternals],
    plugins: [
      env.grd_path
        && new GenerateDepfilePlugin({
          depfilePath: env.depfile_path,
          depfileSourceName: env.grd_path,
        }),
      ...deterministicIdsPlugins(rootGenDir),
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
