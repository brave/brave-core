// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'
import path from 'node:path'
import fs from 'node:fs'

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
  target_arch: process.env.TARGET_ARCH,
  target_os: process.env.TARGET_OS,
  target_environment: process.env.TARGET_ENVIRONMENT,
  target: process.env.TARGET,
  build_config: process.env.BUILD_CONFIG,
})

function getBuildOutputPathList() {
  return buildConfigs.flatMap((buildConfig) => [
    path.resolve(config.srcDir, 'out', buildConfig),
    ...extraArchitectures.map((arch) =>
      path.resolve(config.srcDir, 'out', `${buildConfig}_${arch}`),
    ),
  ])
}

export let outputPath = config.outputDir

if (!fs.existsSync(outputPath)) {
  const outDirectories = getBuildOutputPathList()
    .filter((a) => fs.existsSync(a))
    .sort(
      (a, b) => fs.statSync(b).mtime.getTime() - fs.statSync(a).mtime.getTime(),
    )
  if (!outDirectories.length || outDirectories[0] === undefined) {
    throw new Error(
      'Cannot find any brave-core build output directories. Have you run a brave-core build yet with the specified (or default) configuration?',
    )
  }
  outputPath = outDirectories[0]
}

console.log(
  'Assuming precompiled dependencies can be found at the path:',
  outputPath,
)

export const genPath = path.join(outputPath, 'gen')
