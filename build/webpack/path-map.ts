// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import assert from 'node:assert'
import path from 'node:path'
import dirName from './dirName.cjs'

export type PathMap = Record<string, string | string[]>

const allPrefixes = (resourcePath: string, searchPath: string): PathMap =>
  ['chrome://', 'chrome-untrusted://', ''].reduce<PathMap>((acc, prefix) => {
    acc[prefix + resourcePath] = searchPath
    return acc
  }, {})

/**
 * @param genPath The path to the generated files in the build output.
 * @returns A map of path aliases
 */
export function generatePathMap(genPath: string): PathMap {
  return {
    // Find files in the current build configurations /gen directory
    'gen': genPath,
    // Generated resources at this path are available at chrome://resources and
    // whilst webpack will still bundle, we keep the alias to the served path
    // to minimize knowledge of specific gen/ paths and easily allow us to not
    // bundle
    // them in the future for certain build configuration, just like chromium.
    'chrome://resources/brave/fonts': path.join(
      genPath,
      'brave/ui/webui/resources/fonts',
    ),
    'chrome://resources/brave': path.join(
      genPath,
      'brave/ui/webui/resources/tsc',
    ),
    ...allPrefixes(
      '//resources/mojo',
      path.join(genPath, 'ui/webui/resources/tsc/mojo'),
    ),
    'chrome://resources': path.join(genPath, 'ui/webui/resources/tsc'),
    // We import brave-ui direct from source and not from package repo, so we need
    // direct path to the src/ directory.
    'brave-ui': path.resolve(dirName, '../../node_modules/@brave/brave-ui'),
    // Force same styled-components module for brave-core and brave-ui
    // which ensure both repos code use the same singletons, e.g. ThemeContext.
    'styled-components': path.resolve(
      dirName,
      '../../node_modules/styled-components',
    ),
    // More helpful path for local web-components
    // TODO(petemill): Rename 'brave/components/common' dir to
    // 'brave/components/web-common'
    '$web-common': path.resolve(dirName, '../../components/common'),
    '$wallet': path.resolve(dirName, '../../components/brave_wallet_ui'),
    // react-markdown uses this deep in its tree, and the browser variant uses innerHTML, conflicting with WebUIs that requires Trusted Types
    // We redirect to an alternative implementation that uses a lookup table to decode named chars instead of innerHTML
    'decode-named-character-reference': path.resolve(
      dirName,
      '../../node_modules/decode-named-character-reference/index.js',
    ),
  }
}

/**
 * Builds the production path map for `genPath`, then layers web mock
 * directories on top so browser-privileged modules resolve to web-compatible
 * mocks first and fall back to the real generated files. Shared by Storybook
 * and other non-browser webpack builds (e.g. AI Chat shared conversation lib).
 */
export const generatePathMapWithWebMocks = (genPath: string): PathMap => {
  const basePathMap = generatePathMap(genPath)
  const expandPathMap = (alias: string): string[] => {
    const target = basePathMap[alias]
    assert(target, `Path map alias ${alias} is not defined`)
    return Array.isArray(target) ? target : [target]
  }
  const webMocksRoot = path.resolve(dirName, '../../web/mocks')

  return {
    '//resources/mojo/mojo/public/js/bindings.js': path.join(
      genPath,
      'mojo/public/js/bindings.js',
    ),
    ...basePathMap,
    'chrome://resources': [
      path.join(webMocksRoot, 'chrome-resources'),
      ...expandPathMap('chrome://resources'),
      // Some mojo bindings have their JS code generated in the gen directory
      // (bindings.js). The type definitions are in the same folder as all the
      // other mojo bindings, so we only need this for mock builds.
      genPath,
    ],
    '$web-common': [
      path.join(webMocksRoot, 'web-common'),
      ...expandPathMap('$web-common'),
    ],
  }
}
