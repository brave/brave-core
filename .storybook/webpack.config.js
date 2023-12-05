// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const webpack = require('webpack')
const fs = require('fs')
const { fallback, provideNodeGlobals } = require('../components/webpack/polyfill')

const buildConfigs = ['Component', 'Static', 'Debug', 'Release']
const extraArchitectures = ['arm64', 'x86']

function getBuildOuptutPathList(buildOutputRelativePath) {
  return buildConfigs.flatMap((config) => [
    path.resolve(__dirname, `../../out/${config}/${buildOutputRelativePath}`),
    ...extraArchitectures.map((arch) =>
      path.resolve(
        __dirname,
        `../../out/${config}_${arch}/${buildOutputRelativePath}`
      )
    )
  ])
}

const genFolder = getBuildOuptutPathList('gen')
  .filter(a => fs.existsSync(a))
  .sort((a, b) => fs.statSync(b).mtime - fs.statSync(a).mtime)[0]
if (!genFolder) {
  throw new Error("Failed to find build output folder!")
}

const basePathMap = require('../components/webpack/path-map')(genFolder)

// Override the path map we use in the browser with some additional mock
// directories, so that we can replace things in Storybook.
const pathMap = {
  ...basePathMap,
  'chrome://resources': [
    // As we mock some chrome://resources, insert our mock directory as the first
    // place to look.
    path.resolve(__dirname, 'chrome-resources-mock'),
    basePathMap['chrome://resources']
  ]
}

/**
 * Maps a prefix to a corresponding path. We need this as Webpack5 dropped
 * support for scheme prefixes (like chrome://)
 * 
 * Note: This prefixReplacer is slightly different from the one we use in proper
 * builds, as it takes the first match from any build folder, rather than
 * specifying one - we don't know what the user built last, so we just see what
 * we can find.
 * 
 * This isn't perfect, and in future it'd be good to pass the build folder in
 * via an environment variable. For now though, this works well.
 * @param {string} prefix The prefix
 * @param {string[] | string} replacements The real path options
 */
const prefixReplacer = (prefix, replacements) => {
  if (!Array.isArray(replacements)) replacements = [replacements]

  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, resource => {
    resource.request = resource.request.replace(regex, (_, subpath) => {
      if (!subpath) {
        throw new Error("Subpath is undefined")
      }

      const match = replacements.find((dir) => fs.existsSync(path.join(dir, subpath))) ?? replacements[0]
      const result = path.join(match, subpath)
      return result
    })
  })
}

// Export a function. Accept the base config as the only param.
module.exports = async ({ config, mode }) => {
  const isDevMode = mode === 'development'
  // Make whatever fine-grained changes you need
  config.module.rules.push(
    {
      test: /\.scss$/,
      include: [/\.global\./],
      use: [{ loader: 'style-loader' }, { loader: 'css-loader' }]
    },
    {
      test: /\.scss$/,
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
                ? '[path][name]__[local]--[hash:base64:5]'
                : '[hash:base64]'
            }
          }
        },
        { loader: 'sass-loader' }
      ]
    },
    {
      test: /\.(ts|tsx)$/,
      loader: 'ts-loader',
      options: {
        // TODO(petemill): point to the tsconfig in gen/[target] that
        // is made during build-time, or generate a new one. For both those
        // options, use a cli arg or environment variable to obtain the correct
        // build target.
        configFile: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
        getCustomTransformers: path.join(
          __dirname,
          '../components/webpack/webpack-ts-transformers.js'
        )
      }
    },
    {
      test: /\.avif$/,
      loader: 'file-loader'
    }
  )

  config.resolve.alias = pathMap
  config.resolve.fallback = fallback

  config.plugins.push(provideNodeGlobals,
    ...Object.keys(pathMap)
      .filter(prefix => prefix.startsWith('chrome://'))
      .map(prefix => prefixReplacer(prefix, pathMap[prefix])))
  config.resolve.extensions.push('.ts', '.tsx', '.scss')
  return config
}
