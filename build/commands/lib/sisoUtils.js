// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const Log = require('./logging')
const config = require('./config')
const fs = require('fs')
const util = require('./util')

/**
 * Writes `.sisorc` configuration file. The format is:
 * ```
 * <siso_flags>
 * <subcommand1> <flags>
 * <subcommand2> <flags>
 * ...
 * ```
 *
 * When a command (e.g., `ninja`) is invoked, the depot_tools siso invoker
 * automatically reads this file and applies the corresponding flags to that
 * command. This allows for automatic configuration without manual flag passing.
 */
function writeSisoRc() {
  const ninjaFlags = []

  if (config.rbeService) {
    // `-reapi_keep_exec_stream` to keep exec stream alive during remote
    // execution, otherwise siso terminates it each minute which aborts remote
    // exec in EngFlow backend.
    ninjaFlags.push('-reapi_keep_exec_stream')
    // Increase fs_min_flush_timeout to allow for more time for blobs to be
    // downloaded on slow connection.
    ninjaFlags.push('-fs_min_flush_timeout 300s')
    // Use byte stream for most files as it is compression-aware.
    ninjaFlags.push('-reapi_byte_stream_read_threshold 1024')

    const cacheDir = config.sisoCacheDir
    if (cacheDir) {
      // Enable local disk cache for remote execution.
      ninjaFlags.push(`-cache_dir "${cacheDir}" -local_cache_enable`)
      try {
        fs.mkdirSync(cacheDir, { recursive: true })
      } catch (e) {
        // Ignore errors if cache dir already exists.
      }
    } else {
      Log.warn(
        'siso_cache_dir is not set in brave/.env. Local disk cache is disabled.',
      )
    }

    if (!config.isCI) {
      // Set reapi_priority to 4 for interactive builds according to EngFlow's
      // recommended priority system:
      // https://blog.engflow.com/2025/04/07/not-all-builds-are-made-equal-using-priorities-to-expedite-remote-execution-of-the-builds-and-tests-that-matter-most/#from-workflow-to-priorities
      ninjaFlags.push(`-reapi_priority 4`)
    }
  }

  const sisoRcPath = `${config.srcDir}/build/config/siso/.sisorc`
  const sisoRcContent = ninjaFlags.length
    ? `ninja ${ninjaFlags.join(' ')}\n`
    : ''
  util.writeFileIfModified(sisoRcPath, sisoRcContent)
}

module.exports = {
  writeSisoRc,
}
