// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs')
const program = require('commander')
const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('../lib/logging')
const depotTools = require('../lib/depotTools')
const syncUtil = require('../lib/syncUtils')

program
  .version(process.env.npm_package_version)
  .option('--gclient_file <file>', 'gclient config file location')
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'comma-separated target OS list')
  .option('--target_arch <target_arch>', 'comma-separated target architecture list')
  .option('--target_android_base <target_android_base>', 'target Android OS level for apk or aab (classic, modern, mono)')
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--fetch_all', 'fetch all tags and branch heads')
  .option('--sync_chromium [arg]', 'force or skip chromium sync (true/false/1/0)', JSON.parse)
  .option('--ignore_chromium', 'do not update chromium version even if it is stale [deprecated, use --sync_chromium=false]')
  .option('-D, --delete_unused_deps', 'delete from the working copy any dependencies that have been removed since the last sync')
  .option('--nohooks', 'Do not run hooks after updating')

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

  // --target_os, --target_arch as lists make sense only for `init/sync`
  // commands. Handle comma-separated values here and only pass the first value
  // to the config.update() call.
  const targetOSList = program.target_os?.split(',')
  if (targetOSList) {
    program.target_os = targetOSList[0]
  }
  const targetArchList = program.target_arch?.split(',')
  if (targetArchList) {
    program.target_arch = targetArchList[0]
  }

  config.update(program)

  if (program.ignore_chromium) {
    Log.warn(
        '--ignore_chromium is deprecated, please replace with ' +
        '--sync_chromium=false')
    program.sync_chromium = false
  }

  depotTools.installDepotTools()

  if (program.init || !fs.existsSync(config.defaultGClientFile)) {
    syncUtil.buildDefaultGClientConfig(targetOSList, targetArchList)
  } else if (program.target_os) {
    Log.warn(
        '--target_os is ignored. If you are attempting to sync with ' +
        'a different target_os argument from that used originally via init ' +
        '(and specified in the .gclient file), then you will likely not end ' +
        'up with the correct dependency projects. Specify new target_os ' +
        'values with --init, or edit .gclient manually before running sync ' +
        'again.')
  }

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
