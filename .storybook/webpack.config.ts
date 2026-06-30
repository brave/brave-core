// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'path'
import webpack, { Configuration } from 'webpack'
import fs from 'fs'
import ForkTsCheckerWebpackPlugin from 'fork-ts-checker-webpack-plugin'
import genTsConfig from '../build/commands/lib/genTsConfig'
import { genPath } from '../build/commands/lib/guessConfig'
import { fallback } from '../build/webpack/polyfill'
import generatePathMap from '../build/webpack/path-map'
import { withMockOverrides } from '../build/webpack/resolve'
import { cssRules, tsLoaderRule, ifdefLoaderRule } from '../build/webpack/rules'
import { provideNodeGlobals, chromePrefixReplacers } from '../build/webpack/plugins'
import { forkTsChecker } from './options'

if (!fs.existsSync(genPath)) {
  throw new Error("Failed to find build output 'gen' folder! Have you run a brave-core build yet with the specified (or default) configuration?")
}
console.log(`Using brave-core generated dependency path of '${genPath}'`)

// Override the path map we use in the browser with some additional mock
// directories, so that we can replace things in Storybook.
const pathMap = withMockOverrides(generatePathMap(genPath), {
  chromeResourcesMockDir: path.resolve(__dirname, 'chrome-resources-mock'),
  webCommonMockDir: path.resolve(__dirname, 'web-common-mock'),
  genPath
})

const buildFlags = JSON.parse(fs.readFileSync(path.join(genPath, 'brave/build_flags.json'), 'utf8'))
buildFlags.is_storybook = true

/**
 * Attempts to use mock implementations of a provided module name
 * the mocked implementation should live in a `__mocks__` folder adjacent to the
 * module (`./bridge` -> `./__mocks__/bridge`)
 *
 * Names of the modules to use mock implementations for
 * @param {string[]} moduleNames
 */
function useMockedModules(moduleNames: string[]) {
  if (!Array.isArray(moduleNames)) {
    throw new Error('moduleNames must be an array of strings')
  }

  const moduleNamesGroup = moduleNames.join('|')

  // Match paths containing any of the module name
  // but not if they are preceded by "__mocks__/"
  const moduleRegex = new RegExp(
    `^(?!.*__mocks__\/(${
      moduleNamesGroup //
    })(?:\.ts)?$).*\/(${
      moduleNamesGroup //
    })(?:\.ts)?$`
  )

  return new webpack.NormalModuleReplacementPlugin(moduleRegex, (resource) => {
    const foldersAndFile = resource.request.split('/')
    const fileName = foldersAndFile[foldersAndFile.length - 1]
    const mockedFile = `__mocks__/${fileName}`
    // Modify the resource path
    resource.request = resource.request.replace(fileName, mockedFile)
  })
}

// Export a function. Accept the base config as the only param.
export default async ({ config, mode }: { config: Configuration, mode: string }) => {
  const tsConfigPath = await genTsConfig(genPath, 'tsconfig-storybook.json', genPath, path.resolve(__dirname, '../tsconfig-storybook.json'))
  console.log(`Using generated tsconfig path of '${tsConfigPath}'`)
  const isDevMode = mode.toLowerCase() === 'development'
  // Make whatever fine-grained changes you need
  config.module!.rules!.push(
    // Narrow to .scss so we don't clobber Storybook's built-in .css handling.
    ...cssRules({ isDevMode, test: /\.scss$/ }),
    tsLoaderRule({ configFile: tsConfigPath, transpileOnly: forkTsChecker }),
    ifdefLoaderRule(buildFlags),
    {
      test: /\.avif$/,
      loader: 'file-loader'
    }
  )

  config.resolve!.alias = pathMap
  config.resolve!.fallback! = fallback

  config.plugins!.push(
    provideNodeGlobals,
    useMockedModules(['bridge', 'brave_rewards_api_proxy']),
    ...chromePrefixReplacers(pathMap)
  )

  // By default, Webpack will use the "web" externals preset, which will include
  // an externals plugin that treats module specifiers beginning with `//` as an
  // external module. Disable the preset so that these modules can be bundled
  // for storybook.
  config.externalsPresets = { web: false }

  // When we aren't running on CI we separate the build and Typecheck phases.
  // This results in significantly faster builds (up to 7x faster). On CI we
  // join the two stages together so errors break the build.
  // ForkTsCheckerWebpackPlugin ensures the Typescript errors are still shown at
  // development time.
  if (forkTsChecker) {
    config.plugins!.push(new ForkTsCheckerWebpackPlugin({
      typescript: {
        build: true,
        configFile: tsConfigPath
      }
    }))
  }
  config.resolve!.extensions!.push('.ts', '.tsx', '.scss')
  return config
}
