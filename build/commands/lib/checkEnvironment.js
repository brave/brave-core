/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const semver = require('semver')
const Log = require('./logging')

checkNodeVersion()
checkNpmVersion()

function checkNodeVersion() {
  const nodeVersion = process.versions.node
  const requiredNodeVersion = process.env.npm_package_engines_node
  const upgradeInstructions =
    'You can upgrade Node.js by downloading it from https://nodejs.org/'

  checkVersion('node', nodeVersion, requiredNodeVersion, upgradeInstructions)
}

function checkNpmVersion() {
  const npmVersion = process.env.npm_config_npm_version
  const requiredNpmVersion = process.env.npm_package_engines_npm
  const upgradeInstructions =
    'You can upgrade npm by running "npm install -g npm"'

  // Check npm version if it's defined. It can be undefined if Yarn is used.
  if (npmVersion !== undefined) {
    checkVersion('npm', npmVersion, requiredNpmVersion, upgradeInstructions)
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
      `Error: ${type} version must be "${requiredVersion}". Current version: ${version}\n${instruction}`
    )
    process.exit(1)
  }
}
