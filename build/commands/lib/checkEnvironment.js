/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const fs = require('fs')
const path = require('path')
const semver = require('semver')
const Log = require('./logging')

checkNodeVersion()
checkPnpmVersion()
checkWorkingDirectoryChainOnWindows()

function checkNodeVersion() {
  const nodeVersion = process.versions.node
  const requiredNodeVersion = process.env.npm_package_engines_node
  const upgradeInstructions =
    'You can upgrade Node.js by downloading it from https://nodejs.org/'

  checkVersion('node', nodeVersion, requiredNodeVersion, upgradeInstructions)
}

function checkPnpmVersion() {
  const pnpmVersion =
    process.env.npm_config_user_agent?.match(/pnpm\/(\S+)/)?.[1]
  const requiredPnpmVersion = process.env.npm_package_engines_pnpm
  const upgradeInstructions =
    'You can upgrade pnpm by running "pnpm self-update" or '
    + '"npm install -g pnpm"'

  // Check pnpm version if it's defined.
  if (pnpmVersion !== undefined && requiredPnpmVersion !== undefined) {
    checkVersion('pnpm', pnpmVersion, requiredPnpmVersion, upgradeInstructions)
  }
}

/**
 * Check if a version satisfies semver range.
 * @param {string} type The program the version check is for.
 * @param {string} version The version of the program.
 * @param {string} requiredVersion The version (range) the program must be in.
 * @param {string} instruction Instructions on how to upgrade the program.
 */
function checkVersion(type, version, requiredVersion, instruction) {
  if (!semver.satisfies(version, requiredVersion)) {
    Log.error(
      `Error: ${type} version must be "${requiredVersion}". Current version: ${version}\n${instruction}`,
    )
    process.exit(1)
  }
}

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
      Log.error(
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
      Log.error(
        `Is ${currentDir} a junction pointing to a different drive than ${process.cwd()}? `
          + 'This is not supported.',
      )
      process.exit(1)
    }

    // Check the parent directory.
    currentDir = path.dirname(currentDir)
  }
}
