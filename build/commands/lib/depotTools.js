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

// This guard is used to ensure that we don't have a partial or broken checkout
// of depot_tools.
const depotToolsGuard = new ActionGuard(
  path.join(config.rootDir, 'depot_tools_install.guard'),
  () => {
    if (fs.existsSync(config.depotToolsDir)) {
      Log.status('detected incomplete depot_tools install. Cleaning up...')
      fs.rmSync(config.depotToolsDir, { recursive: true })
    }
  }
)

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

// Reads the enforced depot_tools ref from .disable_auto_update file. If the
// file does not exist or the ref is not valid, returns null.
function readEnforcedDepotToolsRef() {
  if (!fs.existsSync(disableAutoUpdateFilePath)) {
    return null
  }
  const ref = fs.readFileSync(disableAutoUpdateFilePath, 'utf8').trim()
  const isRefValid = /^[a-f0-9]{40}$|^refs\/tags\/.*$|^origin\/.*$/.test(ref);
  return isRefValid ? ref : null
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

function installDepotTools(options = config.defaultOptions) {
  options.cwd = config.braveCoreDir

  const enforcedDepotToolsRef = config.getProjectRef('depot_tools', null)

  // If we're modifying depot_tools reference, it's important to ensure a clean
  // checkout to re-bootstrap CIPD, Python and other dependencies.
  if (enforcedDepotToolsRef !== readEnforcedDepotToolsRef() &&
      fs.existsSync(config.depotToolsDir)) {
    Log.status(
      'depot_tools ref is updated. Removing existing checkout to ' +
      're-bootstrap all dependencies.'
    )
    fs.rmSync(config.depotToolsDir, { recursive: true })
  }

  if (!fs.existsSync(config.depotToolsDir) || depotToolsGuard.isDirty()) {
    Log.progressScope('install depot_tools', () => {
      depotToolsGuard.ensureClean(() => {
        util.run(
          'git',
          ['clone', config.depotToolsRepo, config.depotToolsDir],
          options
        )
      })
    })
  }

  if (enforcedDepotToolsRef !== readEnforcedDepotToolsRef()) {
    if (enforcedDepotToolsRef) {
      Log.progressScope(`switch depot_tools to ${enforcedDepotToolsRef}`, () => {
        depotToolsGuard.with(() => {
          util.fetchAndCheckoutRef(config.depotToolsDir, enforcedDepotToolsRef)
          // Write the enforced depot_tools ref to .disable_auto_update file.
          writeEnforcedDepotToolsRef(enforcedDepotToolsRef)
          // Bootstrap CIPD, Python and other dependencies manually.
          if (process.platform === 'win32') {
            util.run(path.join(config.depotToolsDir, 'cipd_bin_setup.bat'), [], options)
            util.run(path.join(config.depotToolsDir, 'bootstrap', 'win_tools.bat'), [], options)
          } else {
            util.run(path.join(config.depotToolsDir, 'ensure_bootstrap'), [], options)
          }
        })
      })
    } else {
      // Remove the .disable_auto_update file to re-enable auto-updates.
      writeEnforcedDepotToolsRef(null)
    }
  }

  const ninjaLogCfgPath = path.join(config.depotToolsDir, 'ninjalog.cfg');
  if (!fs.existsSync(ninjaLogCfgPath)) {
    // Create a ninja config to prevent autoninja from calling "cipd auth-info"
    // each time. See for details:
    // https://chromium.googlesource.com/chromium/tools/depot_tools/+/main/ninjalog.README.md
    const ninjaLogCfgConfig = {
      'is-googler': false,
      'version': 3,
      'countdown': 10,
      'opt-in': false,
    };
    fs.writeFileSync(ninjaLogCfgPath, JSON.stringify(ninjaLogCfgConfig))
  }
}


module.exports = {
  installDepotTools,
}
