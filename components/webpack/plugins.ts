// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'fs'
import path from 'path'
import webpack from 'webpack'

// Re-exported so consumers have a single import site for shared webpack pieces.
export { provideNodeGlobals } from './polyfill'

/**
 * Maps a scheme prefix to one or more real paths. We need this as Webpack5
 * dropped support for scheme prefixes (like chrome://).
 *
 * When given multiple replacement directories (e.g. a mock dir plus the real
 * generated files) the first one that actually contains the requested subpath
 * wins, falling back to the first entry.
 */
function prefixReplacer(prefix: string, replacements: string | string[]) {
  const options = Array.isArray(replacements) ? replacements : [replacements]
  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, (resource) => {
    resource.request = resource.request.replace(regex, (_, subpath) => {
      if (!subpath) {
        throw new Error('Subpath is undefined')
      }
      const match =
        options.find((dir) => fs.existsSync(path.join(dir, subpath)))
        ?? options[0]
      return path.join(match, subpath)
    })
  })
}

/**
 * Builds a NormalModuleReplacementPlugin for every `chrome://` alias in the
 * path map, rewriting those imports to their real (or mock) on-disk location.
 */
export function chromePrefixReplacers(
  pathMap: Record<string, string | string[]>,
) {
  return Object.keys(pathMap)
    .filter((p) => p.startsWith('chrome://'))
    .map((p) => prefixReplacer(p, pathMap[p]))
}
