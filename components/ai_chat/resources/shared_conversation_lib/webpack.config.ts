// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'webpack-dev-server'
import assert from 'node:assert'
import fs from 'fs'
import path from 'path'
import webpack from 'webpack'
import CopyPlugin from 'copy-webpack-plugin'
import { isCI } from '../../../../build/commands/lib/ciDetect.ts'
import { writeTsConfig } from '../../../../build/webpack/ts-config.ts'
import {
  deterministicOptimization,
  deterministicIdsPlugins,
} from '../../../../build/webpack/deterministic-output.ts'
import { generatePathMapWithWebMocks } from '../../../../build/webpack/path-map.ts'
import {
  provideNodeGlobals,
  chromePrefixReplacers,
} from '../../../../build/webpack/plugins.ts'
import { baseResolve } from '../../../../build/webpack/resolve.ts'
import {
  cssRules,
  tsLoaderRule,
  fileLoaderRule,
  htmlAssetRule,
} from '../../../../build/webpack/rules.ts'
import GenerateDepfilePlugin from '../../../../build/webpack/webpack-plugin-depfile.js'

// style-loader inlines the source of its `insert` option in to every generated
// CSS module (it literally calls `.toString()` on the function), so that
// function cannot reference anything this bundle imports. ProvidePlugin bridges
// the gap: it rewrites this free identifier, in each of those generated modules,
// to style_loader.ts's default export.
// TODO(https://github.com/brave/brave-browser/issues/57626): Updated style-loader
// might avoid the need for ProvidePlugin by insert being able to reference
// a module.
declare const _INSERT_STYLE_ELEMENT: (element: HTMLStyleElement) => void

type WebpackEnv = {
  root_gen_dir?: string
  depfile_path?: string
  depfile_source_name?: string
}

async function resolveGenPath(env: WebpackEnv): Promise<string> {
  if (env.root_gen_dir) {
    return path.resolve(env.root_gen_dir)
  }

  assert(!isCI, 'guessConfig is not allowed on CI')
  const { genPath } = await import(
    '../../../../build/commands/lib/guessConfig.js'
  )
  return genPath
}

export default async function (
  env: WebpackEnv = {},
  argv: any,
): Promise<webpack.Configuration> {
  const isDevMode = argv.mode === 'development'

  const genPath = await resolveGenPath(env)
  if (!fs.existsSync(genPath)) {
    throw new Error(
      "Failed to find build output 'gen' folder! Have you run a brave-core build yet with the specified (or default) configuration?",
    )
  }

  // Mock browser-privileged functionality (e.g. string pluralization,
  // createSanitizedImageUrl) with the web-compatible implementations we share
  // with Storybook.
  const pathMap = generatePathMapWithWebMocks(genPath)

  const outputPath = path.join(genPath, 'ai-chat-shared-conversation-lib')

  const tsConfigPath = await writeTsConfig(
    pathMap,
    genPath,
    'tsconfig-ai-chat-shared-conversation.json',
    path.resolve(import.meta.dirname, '../../../../tsconfig-webpack.json'),
  )

  const entry: webpack.Configuration['entry'] = {
    'render_conversation': path.join(
      import.meta.dirname,
      'render_conversation.tsx',
    ),
  }

  if (isDevMode) {
    entry.demo = path.join(import.meta.dirname, 'demo.tsx')
  }

  const output: webpack.Configuration['output'] = {
    path: outputPath,
    filename: '[name].js',
    chunkFilename: '[name].chunk.js',
    // Need publicPath: 'auto' to have file-loader resolve to script bundle URL,
    // not the loading-websites URL.
    publicPath: 'auto',
    clean: true,
    library: { type: 'module' },
    iife: false,
  }

  const copyPluginPatterns: CopyPlugin.Pattern[] = [
    {
      from: path.join(genPath, 'brave/ui/webui/resources/icons'),
      to: 'nala-icons',
    },
    {
      from: path.resolve(
        import.meta.dirname,
        '../../../../node_modules/@brave/leo/tokens/css/variables.css',
      ),
      to: 'nala.css',
      transform: {
        // TODO(https://github.com/brave/leo/issues/1433): Remove this transform
        // when nala supports `:host`.
        transformer(content: Buffer, absoluteFrom: string) {
          return content.toString().replaceAll(':root', ':host')
        },
      },
    },
  ]

  if (isDevMode) {
    copyPluginPatterns.push({
      from: path.join(import.meta.dirname, 'demo.html'),
      to: 'demo.html',
    })
  }

  const plugins: webpack.Configuration['plugins'] = []
  if (env.depfile_path) {
    assert(
      env.depfile_source_name,
      'depfile_source_name is required when depfile_path is set',
    )
    plugins.push(
      new GenerateDepfilePlugin({
        depfilePath: env.depfile_path,
        depfileSourceName: env.depfile_source_name,
      }),
    )
  }
  plugins.push(
    ...deterministicIdsPlugins(genPath),
    provideNodeGlobals,
    ...chromePrefixReplacers(pathMap),
    new CopyPlugin({
      patterns: copyPluginPatterns,
    }),
    new webpack.ProvidePlugin({
      _INSERT_STYLE_ELEMENT: [
        path.join(import.meta.dirname, 'style_loader.ts'),
        'default',
      ],
    }),
  )

  return {
    target: 'web',
    context: path.resolve(import.meta.dirname, '../../../..'),
    entry,
    stats: isDevMode ? 'normal' : 'errors-only',
    devtool: isDevMode ? 'inline-source-map' : false,
    devServer: {
      historyApiFallback: true,
      hot: false,
      static: false,

      devMiddleware: {
        writeToDisk: true,
        index: 'demo.html',
      },
    },
    output,
    resolve: baseResolve(pathMap),
    optimization: deterministicOptimization,
    experiments: {
      outputModule: true,
    },
    plugins,
    module: {
      parser: {
        // Leave import.meta.url untransformed so that we can use the runtime
        // value instead of webpack's hardcoded build-time value of the local
        // file path. Needed for loading assets from the output bundle path.
        javascript: {
          importMeta: false,
        },
      },
      rules: [
        ...cssRules({
          isDevMode,
          styleLoaderOptions: {
            insert: (element: HTMLStyleElement) =>
              _INSERT_STYLE_ELEMENT(element),
          },
        }),
        tsLoaderRule({ configFile: tsConfigPath }),
        fileLoaderRule(),
        htmlAssetRule,
      ],
    },
  }
}
