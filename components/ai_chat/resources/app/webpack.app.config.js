// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Standalone webpack config for the ai_chat_app build.
// Compiles the AI Chat frontend with ai_chat_app=true, pointing at
// a local HTTP/SSE AI backend instead of Chrome Mojo bindings.

'use strict'

const path = require('path')
const fs = require('fs')
const webpack = require('webpack')
const { fallback, provideNodeGlobals } = require('../../../webpack/polyfill')

const __brave = path.resolve(__dirname, '../../../..')

// Find an existing GN build output that has tsconfig-webpack.json and
// the generated AI Chat mojom JS. Accepts ROOT_GEN_DIR env override.
function findGenDir() {
  const candidates = [
    process.env.ROOT_GEN_DIR,
    path.resolve(__brave, '../out/Component/gen'),
    path.resolve(__brave, '../out/Debug/gen'),
    path.resolve(__brave, '../out/Default/gen'),
    path.resolve(__brave, '../out/Release/gen'),
  ].filter(Boolean)

  for (const d of candidates) {
    if (
      fs.existsSync(path.join(d, 'tsconfig-webpack.json')) &&
      fs.existsSync(path.join(d, 'brave/build_flags.json'))
    ) {
      return d
    }
  }
  throw new Error(
    'No GN build output found. Run a GN build first (e.g. npm run build), ' +
      'or set ROOT_GEN_DIR to the gen/ directory of an existing build.',
  )
}

const ROOT_GEN_DIR = findGenDir()
const DIST_DIR =
  process.env.TARGET_GEN_DIR ||
  path.resolve(__brave, 'dist/ai_chat_app')

const pathMap = require('../../../webpack/path-map')(ROOT_GEN_DIR)

// Load the base build flags from the GN output and override ai_chat_app.
const existingFlags = JSON.parse(
  fs.readFileSync(path.join(ROOT_GEN_DIR, 'brave/build_flags.json'), 'utf8'),
)
const buildFlags = { ...existingFlags, ai_chat_app: true }
const tsConfigPath = path.join(ROOT_GEN_DIR, 'tsconfig-webpack.json')

// Mirrors the prefixReplacer helper from webpack.config.js.
const prefixReplacer = (prefix, replacement) => {
  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, (resource) => {
    resource.request = resource.request.replace(
      regex,
      path.join(replacement, '$1'),
    )
  })
}

module.exports = async function (_env, argv) {
  const isDevMode = (argv.mode || 'development') !== 'production'

  return {
    entry: {
      chat_ui: path.join(
        __brave,
        'components/ai_chat/resources/page/chat_ui.tsx',
      ),
      untrusted_conversation_frame: path.join(
        __brave,
        'components/ai_chat/resources/untrusted_conversation_frame/untrusted_conversation_frame.tsx',
      ),
    },
    devtool: isDevMode ? 'inline-source-map' : false,
    output: {
      iife: true,
      path: DIST_DIR,
      filename: '[name].bundle.js',
      chunkFilename: '[name].chunk.js',
      publicPath: '/',
    },
    resolve: {
      extensions: ['.js', '.tsx', '.ts', '.json'],
      symlinks: false,
      alias: pathMap,
      modules: ['node_modules'],
      fallback,
    },
    optimization: {
      chunkIds: false,
      moduleIds: false,
    },
    experiments: {
      asyncWebAssembly: true,
    },
    plugins: [
      new webpack.ids.NamedModuleIdsPlugin({ context: ROOT_GEN_DIR }),
      new webpack.ids.NamedChunkIdsPlugin({ context: ROOT_GEN_DIR }),
      provideNodeGlobals,
      ...Object.keys(pathMap)
        .filter((p) => p.startsWith('chrome://'))
        .map((p) => prefixReplacer(p, pathMap[p])),
    ],
    module: {
      rules: [
        {
          test: /\.s?css$/,
          include: [/\.global\./, /node_modules/],
          use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
        },
        {
          test: /\.s?css$/,
          exclude: [/\.global\./, /node_modules/],
          use: [
            { loader: 'style-loader' },
            {
              loader: 'css-loader',
              options: {
                importLoaders: 3,
                sourceMap: false,
                modules: {
                  localIdentName: isDevMode
                    ? '[path][name]__[local]--[contenthash:base64:5]'
                    : '[contenthash:base64]',
                },
              },
            },
            { loader: 'sass-loader' },
          ],
        },
        {
          test: /\.tsx?$/,
          loader: 'ts-loader',
          options: {
            getCustomTransformers: path.join(
              __brave,
              'components/webpack/webpack-ts-transformers.js',
            ),
            configFile: tsConfigPath,
          },
        },
        {
          test: /\.(js|ts)x?$/,
          loader: path.join(
            __brave,
            'components/webpack/plugins/ifdef-loader.ts',
          ),
          options: buildFlags,
        },
        {
          test: /\.(ttf|eot|ico|svg|png|jpg|jpeg|gif|webp)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          loader: 'file-loader',
        },
        {
          test: /\.html/,
          type: 'asset/source',
        },
        {
          test: (p) =>
            p.includes(path.join('@brave', 'brave-ui')) && p.endsWith('.js'),
          resolve: { fullySpecified: false },
        },
      ],
    },
  }
}
