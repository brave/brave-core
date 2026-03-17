// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

import Log from './logging.js'
import config from './config.js'
import isCI from './isCI.ts'
import fs from 'node:fs'
import path from 'node:path'
import util from './util.js'

/**
 * Writes `.sisorc` configuration file. The `.sisorc` format is:
 * ```
 * <siso_flags>
 * <siso_subcommand> <flags>
 * ```
 *
 * When a siso subcommand (e.g., `ninja`) is invoked, the depot_tools siso
 * invoker automatically reads this file and applies the corresponding flags to
 * that subcommand. This allows for automatic configuration without manual flag
 * passing.
 */
function writeSisoRc() {
  const ninjaFlags = []

  if (config.rbeService) {
    // Keep exec stream alive during remote execution. Otherwise siso terminates
    // it each minute, which aborts remote exec in EngFlow backend.
    ninjaFlags.push('-reapi_keep_exec_stream')

    // Increase fs_min_flush_timeout to allow for more time for blobs to be
    // downloaded on slow connection.
    ninjaFlags.push('-fs_min_flush_timeout 300s')

    // Use byte stream for most files as it is compression-aware.
    ninjaFlags.push('-reapi_byte_stream_read_threshold 1024')

    // Enable googlechrome config to build most targets with RBE. This is
    // disabled by default in Chromium to make only clang actions be
    // RBE-buildable, but we can build most targets in Brave RBE infra.
    ninjaFlags.push('-config googlechrome')

    // Configure local disk cache.
    const sisoCacheDir = config.sisoCacheDir
    if (!sisoCacheDir) {
      Log.warn(
        'siso_cache_dir not set: re-builds and other checkouts will be slower with more traffic.',
      )
    } else if (sisoCacheDir.endsWith('none')) {
      // Explicitly disabled, do nothing.
    } else {
      // Enable local disk cache.
      ninjaFlags.push(`-local_cache_enable`)
      // Set cache directory.
      ninjaFlags.push(`-cache_dir "${sisoCacheDir}"`)
      fs.mkdirSync(sisoCacheDir, { recursive: true })
    }

    if (!isCI) {
      // Set reapi_priority to 4 for interactive builds according to EngFlow's
      // recommended priority system:
      // https://blog.engflow.com/2025/04/07/not-all-builds-are-made-equal-using-priorities-to-expedite-remote-execution-of-the-builds-and-tests-that-matter-most/#from-workflow-to-priorities
      ninjaFlags.push(`-reapi_priority 4`)
    }
  }

  const sisoRcPath = path.join(config.srcDir, 'build/config/siso/.sisorc')
  const sisoRcContent = ninjaFlags.length
    ? `ninja ${ninjaFlags.join(' ')}\n`
    : ''
  util.writeFileIfModified(sisoRcPath, sisoRcContent)
}

export default {
  writeSisoRc,
}
