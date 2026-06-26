// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The three pieces below all exist for the same reason: producing byte-identical
// bundle output across platforms (notably for mac Universal builds). Keep them
// in lockstep.

import webpack from 'webpack'
import type { Configuration } from 'webpack'

export const deterministicOptimization: NonNullable<
  Configuration['optimization']
> = {
  // We are providing chunk and module IDs via a plugin instead of a default
  chunkIds: false,
  moduleIds: false,
  // Define NO_CONCATENATE for analyzing module size
  concatenateModules: !process.env.NO_CONCATENATE,
}

/**
 * Generate module IDs relative to the gen dir since we want identical output
 * per-platform for mac Universal builds. If they remain relative to the
 * src/brave working directory then the paths could include the platform
 * architecture and therefore be different, e.g. ../out/Release_arm64/gen
 *
 * We can't use DeterministicModuleIdsPlugin because the concatenated modules
 * still use a relative path from the source module it will be concatenated
 * with, which will result in file content differing with different build
 * configurations. Webpack's ConcatenatedModule class doesn't use this context
 * configuration for its identifier construction.
 */
export function deterministicIdsPlugins() {
  return [
    new webpack.ids.NamedModuleIdsPlugin({
      context: process.env.ROOT_GEN_DIR,
    }),
    // NamedChunkIdsPlugin doesn't seem to care if we don't give a common context
    // - it might if the chunk is directly loaded from an output path, so it's
    // being provided anyway. Otherwise, it relies on the IDs of the chunk's
    // included modules.
    new webpack.ids.NamedChunkIdsPlugin({
      context: process.env.ROOT_GEN_DIR,
    }),
  ]
}
