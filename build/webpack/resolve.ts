// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import type { ResolveOptions } from 'webpack'
import { fallback } from './polyfill.ts'

type PathMap = Record<string, string | string[]>

/**
 * The module-resolution config shared by every Brave webpack build. Pass the
 * (possibly mock-augmented) path map to use as aliases.
 */
export const baseResolve = (alias: PathMap): ResolveOptions => ({
  extensions: ['.js', '.tsx', '.ts', '.json'],
  symlinks: false, // If symlinks are used, don't use different IDs for them
  alias,
  modules: ['node_modules'],
  fallback,
})

/**
 * Layers Storybook / standalone-library mock directories on top of the
 * production path map, so browser-privileged modules resolve to web-compatible
 * mocks first and fall back to the real generated files. Shared by the
 * Storybook and AI Chat library builds.
 *
 * @param basePathMap The production path map from `./path-map`.
 * @param mocks.chromeResourcesMockDir Directory of `chrome://resources` mocks.
 * @param mocks.webCommonMockDir Directory of `$web-common` mocks.
 * @param mocks.genPath The build output `gen` directory.
 */
export const withMockOverrides = (
  basePathMap: PathMap,
  mocks: {
    chromeResourcesMockDir: string
    webCommonMockDir: string
    genPath: string
  },
): PathMap => ({
  '//resources/mojo/mojo/public/js/bindings.js': path.join(
    mocks.genPath,
    'mojo/public/js/bindings.js',
  ),
  ...basePathMap,
  'chrome://resources': [
    mocks.chromeResourcesMockDir,
    basePathMap['chrome://resources'] as string,
    // Some mojo bindings have their JS code generated in the gen directory
    // (bindings.js). The type definitions are in the same folder as all the
    // other mojo bindings, so we only need this for mock builds.
    mocks.genPath,
  ],
  '$web-common': [mocks.webCommonMockDir, basePathMap['$web-common'] as string],
})
