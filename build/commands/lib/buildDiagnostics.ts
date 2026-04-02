// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import * as Log from './log.ts'

const kReadFileTailLimitBytes = 128 * 1024

/**
 * Dumps the hanged build diagnostics for a build.
 * @param {string} outputDir
 */
export function dumpBuildHangDiagnostics(outputDir: string) {
  const relativePaths = ['siso.INFO', 'siso.exe.INFO', 'siso_output']
  for (const rel of relativePaths) {
    let filePath = path.join(outputDir, rel)
    if (!fs.existsSync(filePath)) {
      const filePathRedirected = path.join(outputDir, rel + '.redirected')
      if (fs.existsSync(filePathRedirected)) {
        // read the redirected file and get the path
        const redirectedContent = fs.readFileSync(filePathRedirected, 'utf8')
        const redirectedPath = redirectedContent.trim()
        if (fs.existsSync(redirectedPath)) {
          filePath = redirectedPath
        } else {
          Log.error(`Redirected file ${redirectedPath} does not exist`)
          continue
        }
      } else {
        continue
      }
    }
    console.log(`--- tail of ${filePath} ---`)
    try {
      console.log(readFileTailUtf8Sync(filePath, kReadFileTailLimitBytes))
    } catch (err) {
      Log.error(err instanceof Error ? err.message : String(err))
    }
  }
}

/**
 * Reads only the last `maxBytes` bytes from a file.
 */
function readFileTailUtf8Sync(filePath: string, maxBytes: number): string {
  const fd = fs.openSync(filePath, 'r')
  try {
    const size = fs.fstatSync(fd).size
    if (size === 0) {
      return ''
    }
    const toRead = Math.min(maxBytes, size)
    const buf = Buffer.alloc(toRead)
    fs.readSync(fd, buf, 0, toRead, size - toRead)
    // May start mid-sequence for UTF-8; Node replaces invalid bytes at slice
    // edge.
    let str = buf.toString('utf8')
    if (toRead < size) {
      const nl = str.indexOf('\n')
      if (nl !== -1) {
        str = str.slice(nl + 1)
      }
    }
    return str
  } finally {
    fs.closeSync(fd)
  }
}
