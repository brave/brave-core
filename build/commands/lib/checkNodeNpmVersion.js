/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const assert = require('assert')
const Log = require('./logging')

// Define the required versions.
const requiredNodeVersionRange = ['20.0.0', '*']
const requiredNpmVersionRange = ['10.0.0', '*']

// Get the actual versions.
const nodeVersion = process.versions.node
const npmVersion = process.env.npm_config_npm_version

// Check Node.js version.
checkVersionInRange('node', nodeVersion, requiredNodeVersionRange)

// Check npm version if it's defined. It can be undefined if Yarn is used.
if (npmVersion !== undefined) {
  checkVersionInRange('npm', npmVersion, requiredNpmVersionRange)
}

// Check if a version is within a min-max range (excluding max unless max is
// '*').
function checkVersionInRange(type, version, range) {
  const extractNumericVersion = (version) =>
    version.split('.').map((part) => parseInt(part, 10))

  const [min, max] = range
  const versionParts = extractNumericVersion(version)
  assert(versionParts.length === 3)
  const minParts = extractNumericVersion(min)
  assert(minParts.length === 3)

  // Check if version is greater than or equal to min.
  for (let i = 0; i < 3; i++) {
    if (versionParts[i] > minParts[i]) {
      break
    } else if (versionParts[i] < minParts[i]) {
      Log.error(
        `Error: ${type} version must be at least ${min}. Current version: ${version}`
      )
      process.exit(1)
    }
  }

  // If max is '*', no need to check further.
  if (max === '*') {
    return
  }

  const maxParts = extractNumericVersion(max)
  assert(maxParts.length === 3)

  // Check if version is less than max.
  for (let i = 0; i < 3; i++) {
    if (versionParts[i] < maxParts[i]) {
      break
    } else if (versionParts[i] > maxParts[i]) {
      Log.error(
        `Error: ${type} version must be less than ${max}. Current version: ${version}`
      )
      process.exit(1)
    }
  }
}
