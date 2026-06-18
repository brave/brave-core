// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'webpack-dev-server'
import fs from 'fs'
import path from 'path'
import webpack from 'webpack'
import CopyPlugin from 'copy-webpack-plugin'
import genTsConfig from '../../../../build/commands/lib/genTsConfig'
import { genPath } from '../../../../build/commands/lib/guessConfig'
import {
  deterministicOptimization,
  deterministicIdsPlugins,
} from '../../../webpack/deterministic-output'
import generatePathMap from '../../../webpack/path-map'
import {
  provideNodeGlobals,
  chromePrefixReplacers,
} from '../../../webpack/plugins'
import { baseResolve, withMockOverrides } from '../../../webpack/resolve'
import {
  cssRules,
  tsLoaderRule,
  fileLoaderRule,
  braveUiFullySpecifiedRule,
  htmlAssetRule,
} from '../../../webpack/rules'

if (!fs.existsSync(genPath)) {
  throw new Error(
    "Failed to find build output 'gen' folder! Have you run a brave-core build yet with the specified (or default) configuration?",
  )
}
console.log(`Using brave-core generated dependency path of '${genPath}'`)

// Mock browser-privileged functionality (e.g. string pluralization,
// createSanitizedImageUrl) with the web-compatible implementations we share
// with Storybook.
const storybookDir = path.resolve(__dirname, '../../../../.storybook')
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
    path.resolve(__dirname, '../../../../tsconfig-webpack.json'),
  )

  const entry: webpack.Configuration['entry'] = {
    'render_conversation': path.join(__dirname, 'render_conversation.tsx'),
  }

  if (isDevMode) {
    entry.demo = path.join(__dirname, 'demo.tsx')
  }

  const output: webpack.Configuration['output'] = {
    path: outputPath,
    filename: '[name].js',
    chunkFilename: '[name].chunk.js',
    publicPath: '/',
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
      from: path.join(__dirname, 'demo.html'),
      to: 'demo.html',
    })
  }

  return {
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
      ...deterministicIdsPlugins(),
      provideNodeGlobals,
      ...chromePrefixReplacers(pathMap),
      new CopyPlugin({
        patterns: copyPluginPatterns,
      }),
    ],
    module: {
      rules: [
        ...cssRules({ isDevMode }),
        tsLoaderRule({ configFile: tsConfigPath }),
        fileLoaderRule(),
        htmlAssetRule,
      ],
    },
  }
}
