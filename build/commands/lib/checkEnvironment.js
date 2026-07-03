/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import fs from 'node:fs'
import path from 'node:path'

checkWorkingDirectoryChainOnWindows()

// Check that the working directory and all parent directories are not symlinks
// or junctions on Windows. Upstream doesn't support such setup, so we check
// this early to avoid cryptic errors down the line.
function checkWorkingDirectoryChainOnWindows() {
  if (process.platform !== 'win32') {
    return
  }

  let currentDir = process.cwd()
  const workingDirectoryDev = fs.statSync(currentDir).dev

  while (currentDir && currentDir !== path.dirname(currentDir)) {
    // Use lstat to not follow symlinks.
    const stats = fs.lstatSync(currentDir)

    // Check if it's a symlink.
    if (stats.isSymbolicLink()) {
      console.error(
        `Directory chain contains a symlink: ${currentDir}. This is not supported.`,
      )
      process.exit(1)
    }

    // Check if directory device ID does not match the working directory device
    // ID. This can happen if a drive is attached as a directory. ReFS (Dev
    // Drive) has `dev` value change for each directory, but NTFS has it set to
    // 0 for on-device directories. Check only NTFS drives as there's no native
    // way of checking if a drive is attached as a directory in Node.js.
    if (workingDirectoryDev === 0 && stats.dev !== workingDirectoryDev) {
      console.error(
        `Is ${currentDir} a junction pointing to a different drive than ${process.cwd()}? `
          + 'This is not supported.',
      )
      process.exit(1)
    }

    // Check the parent directory.
    currentDir = path.dirname(currentDir)
  }
}
