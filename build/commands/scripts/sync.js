// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
require('../lib/checkEnvironment')

const fs = require('fs')
const program = require('commander')
const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('../lib/logging')
const depotTools = require('../lib/depotTools')
const syncUtil = require('../lib/syncUtils')
const gn = require('../lib/gn')

program
  .version(process.env.npm_package_version)
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'comma-separated target OS list')
  .option(
    '--target_arch <target_arch>',
    'comma-separated target architecture list',
  )
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--no-history', 'performs a shallow clone') // NOTE: sets program.history = false
  .option('--fetch_all', 'fetch all tags and branch heads')
  .option(
    '-C, --sync_chromium [arg]',
    'force or skip chromium sync (true/false/1/0)',
    JSON.parse,
  )
  .option(
    '-D, --delete_unused_deps',
    'delete from the working copy any dependencies that have been removed since the last sync',
  )
  .option('--nohooks', 'Do not run hooks after updating')
  .option(
    '--with_issue_44921',
    'Do not pass --revision to gclient to avoid process hanging on jenkins. https://github.com/brave/brave-browser/issues/44921',
  )

function syncBrave(program) {
  let args = ['sync', '--nohooks']
  const syncWithForce = program.init || program.force
  if (syncWithForce) {
    args.push('--force')
  }

  if (program.delete_unused_deps) {
    args.push('-D')
  }

  if (program.history === false) {
    args.push('--no-history')
  }

  util.runGclient(
    args,
    { cwd: config.braveCoreDir },
    path.join(config.braveCoreDir, '.brave_gclient'),
  )
}

async function RunCommand() {
  program.parse(process.argv)

  // Install depot_tools early to make Python available.
  depotTools.installDepotTools()

  // Read the existing .gclient config to reuse some values from it if they are
  // not provided.
  const existingGclientConfig = program.init ? {} : syncUtil.readGclientConfig()

  // --target_os, --target_arch as lists make sense only for `init/sync`
  // commands. Handle comma-separated values here and only pass the first value
  // to the config.update() call.
  const targetOSList = commaSeparatedToList(
    program.target_os,
    existingGclientConfig.target_os || [],
  )
  if (targetOSList.length > 0) {
    program.target_os = targetOSList[0]
  }
  const targetArchList = commaSeparatedToList(
    program.target_arch,
    existingGclientConfig.target_cpu || [],
  )
  if (targetArchList.length > 0) {
    program.target_arch = targetArchList[0]
  }

  config.update(program)

  if (
    config.disableGclientConfigUpdate
    && fs.existsSync(config.gclientFile)
    && !program.init
  ) {
    Log.warn(
      `Skipping ${config.gclientFile} update (disable_gclient_config_update=true)`,
    )
  } else {
    syncUtil.writeGclientConfig(targetOSList, targetArchList)
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

  depotTools.optOutOfBuildTelemetry()

  await util.applyPatches()

  if (!program.nohooks) {
    if (!(await syncUtil.checkInternalDepsEndpoint())) {
      Log.warn(
        'The internal dependencies endpoint is unreachable, which may block toolchain downloads. Please check your VPN connection.',
      )
    }
    // Run hooks for the root .gclient, this will include Chromium and Brave
    // hooks. Don't cache the result, just always rerun this step, because it's
    // pretty quick in a no-op scenario.
    Log.progressScope('gclient runhooks', () => {
      util.runGclient(['runhooks'])
    })
  }

  await Log.progressScopeAsync('building patched gn', async () => {
    await gn.gen()
    await gn.build()
  })
}

function commaSeparatedToList(value, defaultValue) {
  return value?.split(',').filter(Boolean) || defaultValue
}

RunCommand().catch((err) => {
  Log.error('Brave Browser Sync ERROR:')
  console.error(err)
  process.exit(1)
})
