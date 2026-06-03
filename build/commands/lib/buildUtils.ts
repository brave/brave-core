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
