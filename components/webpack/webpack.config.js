// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const webpack = require('webpack')
const GenerateDepfilePlugin = require('./webpack-plugin-depfile')
const { fallback, provideNodeGlobals } = require('./polyfill')
const pathMap = require('./path-map')(process.env.ROOT_GEN_DIR)

const tsConfigPath = path.join(process.env.ROOT_GEN_DIR, 'tsconfig-webpack.json')

/**
 * Maps a prefix to a corresponding path. We need this as Webpack5 dropped
 * support for scheme prefixes (like chrome://)
 * @param {string} prefix The prefix
 * @param {string} replacement The real path
 */
const prefixReplacer = (prefix, replacement) => {
  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, resource => {
    resource.request = resource.request.replace(regex, path.join(replacement, '$1'))
  })
}

module.exports = async function (env, argv) {
  const isDevMode = argv.mode === 'development'
  // webpack-cli no longer allows specifying entry name in cli args, so use
  // a custom env param and parse ourselves.
  const entry = {}
  if (!env.brave_entries) {
    throw new Error(
      "Entry point(s) must be provided via env.brave_entries param."
    )
  }
  const entryInput = env.brave_entries.split(',')
  for (const entryInputItem of entryInput) {
    const entryInputItemParts = entryInputItem.split('=')
    if (entryInputItemParts.length !== 2) {
      throw new Error(
        'Brave Webpack config could not parse entry env param item: ' +
        entryInputItem
      )
    }
    entry[entryInputItemParts[0]] = entryInputItemParts[1]
  }

  // Webpack config object
  const resolve = {
    extensions: ['.js', '.tsx', '.ts', '.json'],
    alias: pathMap,
    modules: ['node_modules'],
    fallback
  }

  if (env.extra_modules) {
    const extraModules = env.extra_modules.split(',')
    resolve.modules = [
      ...extraModules,
      ...resolve.modules
    ]
  }

  if (env.webpack_aliases) {
    resolve.aliasFields = env.webpack_aliases.split(',')
  }

  const output = {
    path: process.env.TARGET_GEN_DIR,
    filename: '[name].bundle.js',
    chunkFilename: '[id].chunk.js'
  }
  if (env.output_public_path) {
    output.publicPath = env.output_public_path
  }

  return {
    entry,
    devtool: isDevMode ? 'inline-source-map' : false,
    output,
    resolve,
    optimization: {
      // Define NO_CONCATENATE for analyzing module size.
      concatenateModules: !process.env.NO_CONCATENATE
    },
    experiments: {
      syncWebAssembly: true
    },
    plugins: [
      process.env.DEPFILE_SOURCE_NAME && new GenerateDepfilePlugin({
        depfilePath: process.env.DEPFILE_PATH,
        depfileSourceName: process.env.DEPFILE_SOURCE_NAME
      }),
      provideNodeGlobals,
      ...Object.keys(pathMap)
        .filter(p => p.startsWith('chrome://'))
        .map(p => prefixReplacer(p, pathMap[p])),
    ],
    module: {
      rules: [
        {
          // CSS imported from node_modules or in a x.global.css file
          // is just regular css converted to JS and injected to style elements
          test: /\.s?css$/,
          include: [/\.global\./, /node_modules/],
          use: [
            { loader: "style-loader" },
            { loader: "css-loader" },
          ],
        },
        {
          // CSS imported in the source tree can use sass and css modules
          // syntax.
          test: /\.s?css$/,
          exclude: [/\.global\./, /node_modules/],
          use: [
            // Injects the result into the DOM as a style block
            { loader: "style-loader" },
            // Converts the resulting CSS to Javascript to be bundled
            // (modules:true to rename CSS classes in output to cryptic identifiers,
            // except if wrapped in a :global(...) pseudo class).
            {
              loader: "css-loader",
              options: {
                importLoaders: 3,
                sourceMap: false,
                modules: {
                  localIdentName: isDevMode
                    ? "[path][name]__[local]--[contenthash:base64:5]"
                    : "[contenthash:base64]",
                },
              },
            },
            // First, convert SASS to CSS
            { loader: "sass-loader" },
          ],
        },
        {
          test: /\.tsx?$/,
          loader: 'ts-loader',
          options: {
            getCustomTransformers: path.join(__dirname, './webpack-ts-transformers.js'),
            // Use generated tsconfig so that we can point at gen/ output in the
            // correct build configuration output directory.
            configFile: tsConfigPath
          }
        },
        // Loads font files for Font Awesome
        {
          test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          loader: 'url-loader',
          options: {
            limit: 13000,
            mimetype: 'application/font-woff'
          }
        },
        {
          test: /\.(ttf|eot|ico|svg|png|jpg|jpeg|gif|webp)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          loader: 'file-loader'
        },
        // Unfortunately, brave-ui is compiled as a "module" so Webpack5 expects
        // it to provide file extensions (which it does not), so we need to
        // special case it here.
        {
          test: p => p.includes(path.join('@brave', 'brave-ui')) && p.endsWith('.js'),
          resolve: {
              fullySpecified: false,
          },
        }
      ]
    }
  }
}
