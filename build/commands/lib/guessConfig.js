// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'
import os from 'node:os'
import path from 'node:path/posix'
import fs from 'node:fs'
import rootDir from './rootDir.cjs'

const buildConfigs = ['Component', 'Static', 'Debug', 'Release']
const extraArchitectures = ['arm64', 'x86']

// Choose which brave-core build directory to look for pre-compiled resource
// dependencies:
// 1. Default for local builds for the actual platform / architecture
// 2. platform / architecture overriden by environment variables
// 3. most recently built - this caters to the common scenario when a
//    non-standard target has been built but no arguments are provided to
//    storybook.

// This uses environment variables as there is currently no way to pass custom
// arguments to the |storybook build| cli.
config.update({
  target_arch: /** @type {any} */ (process.env.TARGET_ARCH),
  target_os: /** @type {any} */ (process.env.TARGET_OS),
  target_environment: /** @type {any} */ (process.env.TARGET_ENVIRONMENT),
  target: /** @type {any} */ (process.env.TARGET),
  build_config: process.env.BUILD_CONFIG,
})

let outputPath = config.outputDir

function getBuildOutputPathList() {
  if (os.platform() === 'win32') {
    return buildConfigs.flatMap((config) => [
      path.win32.resolve(rootDir, `src\\out\\${config}`),
      ...extraArchitectures.map((arch) =>
        path.win32.resolve(rootDir, `src\\out\\${config}_${arch}`),
      ),
    ])
  } else {
    return buildConfigs.flatMap((config) => [
      path.resolve(rootDir, `src/out/${config}`),
      ...extraArchitectures.map((arch) =>
        path.resolve(rootDir, `src/out/${config}_${arch}`),
      ),
    ])
  }
}

if (fs.existsSync(outputPath)) {
  console.log(
    `[guessConfig] Using config-provided build output path: ${outputPath}`,
  )
} else {
  const outDirectories = getBuildOutputPathList()
    .filter((a) => fs.existsSync(a))
    .sort(
      (a, b) => fs.statSync(b).mtime.getTime() - fs.statSync(a).mtime.getTime(),
    )
  if (!outDirectories.length || outDirectories[0] === undefined) {
    throw new Error(
      '[guessConfig] Cannot find any brave-core build output directories. Have you run a brave-core build yet with the specified (or default) configuration?',
    )
  }
  outputPath = outDirectories[0]
  console.log(
    `[guessConfig] Using most recent build output path: ${outputPath}`,
  )
}

const genPath = path.join(outputPath, 'gen')
export { outputPath, genPath }
