// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const ActionGuard = require('./actionGuard')
const config = require('./config')
const fs = require('fs')
const path = require('path')
const Log = require('./logging')
const util = require('./util')

// The .disable_auto_update file in the depot_tools directory, regardless of
// its content, disables auto-updates. If a specific depot_tools reference is
// enforced, we use this file for two purposes: 1) disable auto-updates; 2)
// store the enforced reference. Note: The presence of this file also prevents
// the auto-bootstrapping of CIPD (Chrome Infrastructure Package Deployment),
// Python, and other dependencies.
const disableAutoUpdateFilePath = path.join(
  config.depotToolsDir,
  '.disable_auto_update'
)

function isDepotToolsRefValid(ref) {
  // Only support git revision enforcement, because tags and branches may
  // require fetch and checkout on each sync which we don't really want to do.
  return /^[a-f0-9]{40}$/.test(ref)
}

// Reads the enforced depot_tools ref from .disable_auto_update file. If the
// file does not exist or the ref is not valid, returns null.
function readEnforcedDepotToolsRef() {
  if (!fs.existsSync(disableAutoUpdateFilePath)) {
    return null
  }
  const ref = fs.readFileSync(disableAutoUpdateFilePath, 'utf8').trim()
  return isDepotToolsRefValid(ref) ? ref : null
}

// Writes the enforced depot_tools ref to .disable_auto_update file. If ref is a
// falsy value, the file is removed.
function writeEnforcedDepotToolsRef(ref) {
  if (ref) {
    fs.writeFileSync(disableAutoUpdateFilePath, ref)
  } else if (fs.existsSync(disableAutoUpdateFilePath)) {
    fs.unlinkSync(disableAutoUpdateFilePath)
  }
}

// Bootstrap CIPD, Python and other dependencies in depot_tools.
function bootstrapDepotTools(options) {
  if (process.platform === 'win32') {
    util.run(path.join(config.depotToolsDir, 'cipd_bin_setup.bat'), [], options)
    util.run(
      path.join(config.depotToolsDir, 'bootstrap', 'win_tools.bat'),
      [],
      options
    )
  } else {
    util.run(path.join(config.depotToolsDir, 'ensure_bootstrap'), [], options)
  }
}

// Remove the depot_tools directory.
function removeDepotTools() {
  if (fs.existsSync(config.depotToolsDir)) {
    fs.rmSync(config.depotToolsDir, { recursive: true })
  }
}

function installDepotTools(options = config.defaultOptions) {
  options.cwd = config.braveCoreDir

  const enforcedDepotToolsRef = config.getProjectRef('depot_tools', null)
  if (enforcedDepotToolsRef && !isDepotToolsRefValid(enforcedDepotToolsRef)) {
    Log.error(
      `Invalid depot_tools ref: ${enforcedDepotToolsRef}. ` +
        'Only full git revision is supported.'
    )
    process.exit(1)
  }

  // This guard is used to ensure that we don't have a partial or broken
  // checkout of depot_tools.
  const depotToolsGuard = new ActionGuard(
    `${config.depotToolsDir}_install.guard`,
    () => {
      Log.status('depot_tools checkout was interrupted. Cleaning up...')
      removeDepotTools()
    }
  )

  depotToolsGuard.run((wasInterrupted) => {
    // If we're modifying depot_tools reference, it's important to ensure a
    // clean checkout to re-bootstrap CIPD, Python and other dependencies.
    if (
      fs.existsSync(config.depotToolsDir) &&
      enforcedDepotToolsRef !== readEnforcedDepotToolsRef()
    ) {
      Log.status(
        'depot_tools ref is updated. Removing existing checkout to ' +
          're-bootstrap all dependencies.'
      )
      removeDepotTools()
    }

    if (!fs.existsSync(config.depotToolsDir) || wasInterrupted) {
      Log.progressScope('install depot_tools', () => {
        util.run(
          'git',
          ['clone', config.depotToolsRepo, config.depotToolsDir],
          options
        )
      })
    }

    if (enforcedDepotToolsRef !== readEnforcedDepotToolsRef()) {
      // Write the enforced ref to .disable_auto_update or remove the file to
      // re-enable auto-updates.
      writeEnforcedDepotToolsRef(enforcedDepotToolsRef)
      if (enforcedDepotToolsRef) {
        Log.progressScope(
          `switch depot_tools to ${enforcedDepotToolsRef}`,
          () => {
            util.fetchAndCheckoutRef(
              config.depotToolsDir,
              enforcedDepotToolsRef
            )
            // Bootstrap CIPD, Python and other dependencies manually.
            bootstrapDepotTools(options)
          }
        )
      }
    }

    const ninjaLogCfgPath = path.join(config.depotToolsDir, 'ninjalog.cfg')
    if (!fs.existsSync(ninjaLogCfgPath)) {
      // Create a ninja config to prevent autoninja from calling "cipd
      // auth-info" each time. See for details:
      // https://chromium.googlesource.com/chromium/tools/depot_tools/+/main/ninjalog.README.md
      const ninjaLogCfgConfig = {
        'is-googler': false,
        'version': 3,
        'countdown': 10,
        'opt-in': false
      }
      fs.writeFileSync(ninjaLogCfgPath, JSON.stringify(ninjaLogCfgConfig))
    }

    if (process.platform === 'win32' && config.isCI) {
      // Bootstrap gsutil on Windows manually to fix random LockFile issues.
      util.run(path.join(config.depotToolsDir, 'gsutil.py.bat'), [], {
        ...options,
        stdio: 'pipe'
      })
    }
  })
}

module.exports = {
  installDepotTools
}
