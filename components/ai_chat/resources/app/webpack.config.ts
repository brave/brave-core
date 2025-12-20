// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs')
const path = require('path')
const webpack = require('webpack')
const CopyPlugin = require('copy-webpack-plugin')
import genTsConfig from '../../../../build/commands/lib/genTsConfig'
import { genPath } from '../../../../build/commands/lib/guessConfig'
const { fallback, provideNodeGlobals } = require('../../../webpack/polyfill')
const generatePathMap = require('../../../webpack/path-map')

if (!fs.existsSync(genPath)) {
  throw new Error(
    "Failed to find build output 'gen' folder! Have you run a brave-core build yet with the specified (or default) configuration?",
  )
}
console.log(`Using brave-core generated dependency path of '${genPath}'`)

const basePathMap = generatePathMap(genPath)

// Override the path map we use in the browser with some additional mock
// directories.
const pathMap = {
  '//resources/mojo/mojo/public/js/bindings.js': path.join(
    genPath,
    'mojo/public/js/bindings.js',
  ),
  ...basePathMap,
  'chrome://resources': [
    // As we mock some chrome://resources, insert our mock directory as the first
    // place to look.
    path.resolve(__dirname, '../../../../.storybook/chrome-resources-mock'),
    basePathMap['chrome://resources'],

    // Some mojo bindings have their JS code generated in the gen directory (bindings.js). The type definitions are in the same folder as all the other mojo bindings, so we only
    // need this for Storybook builds.
    genPath,
  ],
  '$web-common': [
    path.resolve(__dirname, '../../../../.storybook/web-common-mock'),
    basePathMap['$web-common'],
  ],
}

/**
 * Maps a prefix to a corresponding path. We need this as Webpack5 dropped
 * support for scheme prefixes (like chrome://)
 *
 * Note: This prefixReplacer is slightly different from the one we use in proper
 * builds, as some path maps have multiple possible locations (e.g. for mocks).
 *
 * @param {string} prefix The prefix
 * @param {string[] | string} replacements The real path options
 */
const prefixReplacer = (prefix, replacements) => {
  if (!Array.isArray(replacements)) replacements = [replacements]

  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, (resource) => {
    resource.request = resource.request.replace(regex, (_, subpath) => {
      if (!subpath) {
        throw new Error('Subpath is undefined')
      }

      const match =
        replacements.find((dir) => fs.existsSync(path.join(dir, subpath)))
        ?? replacements[0]
      const result = path.join(match, subpath)
      return result
    })
  })
}

export default async function (env, argv) {
  const tsConfigPath = await genTsConfig(
    genPath,
    'tsconfig-ai-chat-app.json',
    genPath,
    path.resolve(__dirname, '../../../../tsconfig-webpack.json'),
  )
  const isDevMode = argv.mode === 'development'

  // webpack-cli no longer allows specifying entry name in cli args, so use
  // a custom env param and parse ourselves.
  const entry = {
    'ai_chat_app': path.join(__dirname, 'ai_chat_app.tsx'),
  }

  // Webpack config object
  const resolve = {
    extensions: ['.js', '.tsx', '.ts', '.json'],
    symlinks: false, // If symlinks are used, don't use different IDs for them
    alias: pathMap,
    modules: ['node_modules'],
    fallback,
  }

  const output = {
    path: path.join(__dirname, 'dist'),
    filename: '[name].bundle.js',
    chunkFilename: '[name].chunk.js',
    publicPath: '/',
  }

  output.library = { type: 'module' }
  output.iife = false

  // if (env.output_public_path) {
  //   output.publicPath = env.output_public_path
  // }

  const experiments = {
    outputModule: true,
  }
  // if (env.sync_wasm) {
  //   experiments.syncWebAssembly = true
  // } else {
  //   experiments.asyncWebAssembly = true
  //   output.enabledWasmLoadingTypes = [ 'xhr' ]
  //   output.wasmLoading = 'xhr'
  // }

  return {
    entry,
    devtool: isDevMode ? 'inline-source-map' : false,
    devServer: {
      historyApiFallback: true,
      hot: false,
      static: false,
      devMiddleware: {
        writeToDisk: true,
      },
    },
    output,
    resolve,
    optimization: {
      // We are providing chunk and module IDs via a plugin instead of a default
      chunkIds: false,
      moduleIds: false,
      // Define NO_CONCATENATE for analyzing module size.
      concatenateModules: !process.env.NO_CONCATENATE,
    },
    experiments,
    externals: [
      function ({ context, request }, callback) {
        if (env.output_module) {
          if (/^chrome(\-untrusted)?:\/\//.test(request)) {
            return callback(null, 'module ' + request)
          }
        }
        callback()
      },
    ],
    plugins: [
      // Generate module IDs relative to the gen dir since we want identical
      // output per-platform for mac Universal builds. If they remain relative
      // to the src/brave working directory then the paths could include the
      // platform architecture and therefore be different,
      // e.g. ../out/Release_arm64/gen
      // We can't use DeterministicModuleIdsPlugin because the
      // concatenated modules still use a relative path from the source module
      // it will be concatenated with, which will result in file content
      // differing with different build configurations. Webpack's
      // ConcatenatedModule class doesn't use this context configuration for
      // it's identifier construction.
      new webpack.ids.NamedModuleIdsPlugin({
        context: process.env.ROOT_GEN_DIR,
      }),
      // NamedChunkIdsPlugin doesn't seem to care if we don't give a common
      // context - it might if the chunk is directly loaded from the an output
      // path, so it's being provided anyway. Otherwise, it relies on the IDs of
      // the chunk's included modules.
      new webpack.ids.NamedChunkIdsPlugin({
        context: process.env.ROOT_GEN_DIR,
      }),
      provideNodeGlobals,
      ...Object.keys(pathMap)
        .filter((p) => p.startsWith('chrome://'))
        .map((p) => prefixReplacer(p, pathMap[p])),
      new CopyPlugin({
        patterns: [
          {
            from: path.join(genPath, 'brave/ui/webui/resources/icons'),
            to: 'nala-icons',
          },
          { from: path.join(__dirname, 'index.html') },
        ],
      }),
    ],
    module: {
      rules: [
        {
          // CSS imported from node_modules or in a x.global.css file
          // is just regular css converted to JS and injected to style elements
          test: /\.s?css$/,
          include: [/\.global\./, /node_modules/],
          use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
        },
        {
          // CSS imported in the source tree can use sass and css modules
          // syntax.
          test: /\.s?css$/,
          exclude: [/\.global\./, /node_modules/],
          use: [
            // Injects the result into the DOM as a style block
            { loader: 'style-loader' },
            // Converts the resulting CSS to Javascript to be bundled
            // (modules:true to rename CSS classes in output to cryptic identifiers,
            // except if wrapped in a :global(...) pseudo class).
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
            // First, convert SASS to CSS
            { loader: 'sass-loader' },
          ],
        },
        {
          test: /\.tsx?$/,
          loader: 'ts-loader',
          options: {
            getCustomTransformers: path.join(
              __dirname,
              '../../../webpack/webpack-ts-transformers.js',
            ),
            // Use generated tsconfig so that we can point at gen/ output in the
            // correct build configuration output directory.
            configFile: tsConfigPath,
          },
        },
        {
          test: /\.(ttf|eot|ico|svg|woff2|png|jpg|jpeg|gif|webp)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          loader: 'file-loader',
        },
        // Unfortunately, brave-ui is compiled as a "module" so Webpack5 expects
        // it to provide file extensions (which it does not), so we need to
        // special case it here.
        {
          test: (p) =>
            p.includes(path.join('@brave', 'brave-ui')) && p.endsWith('.js'),
          resolve: {
            fullySpecified: false,
          },
        },
        {
          test: /\.html/,
          type: 'asset/source',
        },
      ],
    },
  }
}
