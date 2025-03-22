// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

// Check environment before doing anything.
require('../lib/checkEnvironment')

const program = require('commander')
const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('../lib/logging')
const depotTools = require('../lib/depotTools')
const syncUtil = require('../lib/syncUtils')
const JsoncConfig = require('../lib/jsoncConfig')

program
  .version(process.env.npm_package_version)
  .option('--gclient_file <file>', 'gclient config file location')
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'comma-separated target OS list')
  .option('--target_arch <target_arch>', 'comma-separated target architecture list')
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--fetch_all', 'fetch all tags and branch heads')
  .option('-C, --sync_chromium [arg]', 'force or skip chromium sync (true/false/1/0)', JSON.parse)
  .option('-D, --delete_unused_deps', 'delete from the working copy any dependencies that have been removed since the last sync')
  .option('--nohooks', 'Do not run hooks after updating')

const syncConfigTemplate = `{
  // Add your gclient modifications here.
  "gclient": {
    "solutions": [
      {
        // Uncomment to checkout Chromium's most recent clangd.
        // "custom_vars": {
        //   "checkout_clangd": true,
        // }
      }
    ],
    // This is updated by the sync command if provided via --target_os.
    "target_os": [
      // "android"
    ],
    // This is updated by the sync command if provided via --target_arch.
    "target_cpu": [
      // "arm64"
    ],
  },
  // "delete_unused_deps": true, // Uncomment to always delete unused deps.
}`

function syncBrave(program) {
  let args = ['sync', '--nohooks']
  const syncWithForce = program.init || program.force
  if (syncWithForce) {
    args.push('--force')
  }

  if (program.delete_unused_deps) {
    args.push('-D')
  }

  util.runGClient(
      args, {cwd: config.braveCoreDir},
      path.join(config.braveCoreDir, '.brave_gclient'))
}

async function RunCommand() {
  program.parse(process.argv)

  const syncConfig = new JsoncConfig(config.syncConfigFile, syncConfigTemplate)

  // --target_os, --target_arch as lists make sense only for `init/sync`
  // commands. Handle comma-separated values here and only pass the first value
  // to the config.update() call.
  const targetOSList = program.target_os?.split(',').filter(Boolean)
  if (targetOSList) {
    program.target_os = targetOSList[0]
    syncConfig.set('gclient.target_os', targetOSList)
  } else if (syncConfig.config.gclient?.target_os?.length) {
    program.target_os = syncConfig.config.gclient.target_os[0]
  }

  const targetArchList = program.target_arch?.split(',').filter(Boolean)
  if (targetArchList) {
    program.target_arch = targetArchList[0]
    syncConfig.set('gclient.target_cpu', targetArchList)
  } else if (syncConfig.config.gclient?.target_cpu?.length) {
    program.target_arch = syncConfig.config.gclient.target_cpu[0]
  }

  if (
    program.delete_unused_deps === undefined &&
    syncConfig.config.delete_unused_deps !== undefined
  ) {
    program.delete_unused_deps = syncConfig.config.delete_unused_deps
  }

  config.update(program)

  depotTools.installDepotTools()
  syncUtil.buildDefaultGClientConfig(syncConfig.config.gclient)

  if (config.isCI) {
    program.delete_unused_deps = true
  }

  Log.progressScope('gclient sync', () => {
    const didSyncChromium = syncUtil.syncChromium(program)
    if (!didSyncChromium || program.delete_unused_deps) {
      // If no Chromium sync was done, run sync inside `brave` to sync Brave DEPS.
      syncBrave(program)
    }
  })

  depotTools.optOutOfBuildTelemetry()

  await util.applyPatches()

  if (!program.nohooks) {
    if (!await syncUtil.checkInternalDepsEndpoint()) {
      Log.warn(
        'The internal dependencies endpoint is unreachable, which may block toolchain downloads. Please check your VPN connection.'
      )
    }
    // Run hooks for the root .gclient, this will include Chromium and Brave
    // hooks. Don't cache the result, just always rerun this step, because it's
    // pretty quick in a no-op scenario.
    Log.progressScope('gclient runhooks', () => {
      util.runGClient(['runhooks'])
    })
  }
}

RunCommand()
.catch((err) => {
  Log.error('Brave Browser Sync ERROR:')
  console.error(err)
  process.exit(1)
})
