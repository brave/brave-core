// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'webpack-dev-server'
import fs from 'fs'
import path from 'path'
import webpack from 'webpack'
import CopyPlugin from 'copy-webpack-plugin'
import genTsConfig from '../../../../build/commands/lib/genTsConfig.js'
import { genPath } from '../../../../build/commands/lib/guessConfig.js'
import {
  deterministicOptimization,
  deterministicIdsPlugins,
} from '../../../../build/webpack/deterministic-output.ts'
import generatePathMap from '../../../../build/webpack/path-map.js'
import {
  provideNodeGlobals,
  chromePrefixReplacers,
} from '../../../../build/webpack/plugins.ts'
import {
  baseResolve,
  withMockOverrides,
} from '../../../../build/webpack/resolve.ts'
import {
  cssRules,
  tsLoaderRule,
  fileLoaderRule,
  htmlAssetRule,
} from '../../../../build/webpack/rules.ts'

if (!fs.existsSync(genPath)) {
  throw new Error(
    "Failed to find build output 'gen' folder! Have you run a brave-core build yet with the specified (or default) configuration?",
  )
}
console.log(`Using brave-core generated dependency path of '${genPath}'`)

// Mock browser-privileged functionality (e.g. string pluralization,
// createSanitizedImageUrl) with the web-compatible implementations we share
// with Storybook.
const storybookDir = path.resolve(import.meta.dirname, '../../../../.storybook')
const pathMap = withMockOverrides(generatePathMap(genPath), {
  chromeResourcesMockDir: path.join(storybookDir, 'chrome-resources-mock'),
  webCommonMockDir: path.join(storybookDir, 'web-common-mock'),
  genPath,
})

export default async function (
  env: any,
  argv: any,
): Promise<webpack.Configuration> {
  const isDevMode = argv.mode === 'development'

  const outputPath = path.join(genPath, 'ai-chat-shared-conversation-lib')
  if (!isDevMode) {
    console.log('Output path is', outputPath)
  }

  const tsConfigPath = await genTsConfig(
    genPath,
    'tsconfig-ai-chat-shared-conversation.json',
    genPath,
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
  ]

  if (isDevMode) {
    copyPluginPatterns.push({
      from: path.join(import.meta.dirname, 'demo.html'),
      to: 'demo.html',
    })
  }

  return {
    target: 'web',
    entry,
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
    plugins: [
      ...deterministicIdsPlugins(genPath),
      provideNodeGlobals,
      ...chromePrefixReplacers(pathMap),
      new CopyPlugin({
        patterns: copyPluginPatterns,
      }),
    ],
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
        ...cssRules({ isDevMode }),
        tsLoaderRule({ configFile: tsConfigPath }),
        fileLoaderRule(),
        htmlAssetRule,
      ],
    },
  }
}
