// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { ResolveOptions } from 'webpack'
import { fallback } from './polyfill.ts'
import type { PathMap } from './path-map.ts'

/**
 * The module-resolution config shared by every Brave webpack build. Pass the
 * (possibly mock-augmented) path map to use as aliases.
 */
export const baseResolve = (alias: PathMap): ResolveOptions => ({
  extensions: ['.js', '.tsx', '.ts', '.json'],
  alias,
  modules: ['node_modules'],
  fallback,
})
