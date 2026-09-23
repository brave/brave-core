// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'
import util from './util.js'
import assert from 'node:assert'
import fs from 'node:fs'
import path from 'node:path'
import { isCI } from './ciDetect.ts'
import * as Log from './log.ts'

// Helper to ensure vs_files mount is valid. We found that sometimes ciopfs
// mount may disappear after a while. It's not clear why, but this helper
// ensures it's always valid.
export function ensureVsFilesMount() {
  if (
    isCI
    && config.useBraveHermeticToolchain
    && config.targetOS === 'win'
    && config.hostOS !== 'win'
  ) {
    const vsToolchainScript = path.join(
      config.srcDir,
      'build',
      'vs_toolchain.py',
    )
    // Sanity check to ensure upstream did not move the script.
    assert(fs.existsSync(vsToolchainScript), `${vsToolchainScript} not found`)

    const vsFilesDir = path.join(
      config.srcDir,
      'third_party',
      'depot_tools',
      'win_toolchain',
      'vs_files',
    )

    const hasVsToolchain =
      fs.existsSync(vsFilesDir)
      && fs
        .readdirSync(vsFilesDir, { withFileTypes: true })
        .some((entry) => entry.isDirectory())

    if (!hasVsToolchain) {
      util.run(
        'vpython3',
        [vsToolchainScript, 'update', '--force'],
        config.defaultOptions,
      )
    }
  }
}

// Ensures the src/chrome/VERSION matches brave-core's package.json version.
export function checkVersionsMatch() {
  const srcChromeVersionDir = path.resolve(
    path.join(config.srcDir, 'chrome', 'VERSION'),
  )
  const versionData = fs.readFileSync(srcChromeVersionDir, 'utf8')
  const re = /MAJOR=(\d+)\s+MINOR=(\d+)\s+BUILD=(\d+)\s+PATCH=(\d+)/
  const found = versionData.match(re)
  if (!found) {
    throw new Error('Failed to parse version from src/chrome/VERSION')
  }
  const braveVersionFromChromeFile = `${found[2]}.${found[3]}.${found[4]}`
  if (braveVersionFromChromeFile !== config.braveVersion) {
    // Only a warning. The CI environment will choose to proceed or not within
    // its own script.
    Log.warn(
      `Version files do not match!\n`
        + `src/chrome/VERSION: ${braveVersionFromChromeFile}\n`
        + `brave-core configured version: ${config.braveVersion}\n`
        + `Did you forget to sync?`,
    )
  }
}
