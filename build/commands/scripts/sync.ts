// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import fs from 'node:fs'
import { program } from 'commander'
import path from 'node:path'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import * as Log from '../lib/log.ts'
import depotTools from '../lib/depotTools.js'
import { isCI } from '../lib/ciDetect.ts'
import syncUtil from '../lib/syncUtils.js'
import sisoUtils from '../lib/sisoUtils.js'

program
  .version(process.env.npm_package_version || 'unknown')
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'comma-separated target OS list')
  .option(
    '--target_arch <target_arch>',
    'comma-separated target architecture list',
  )
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--no-history', 'performs a shallow clone') // NOTE: sets options.history = false
  .option('--no-bootstrap', "Don't bootstrap from Google Storage.")
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
  .action(sync)
  .parse()

function syncBrave(options) {
  let args = ['sync', '--nohooks']
  const syncWithForce = options.init || options.force
  if (syncWithForce) {
    args.push('--force')
  }

  if (options.delete_unused_deps) {
    args.push('-D')
  }

  if (options.bootstrap === false) {
    if (isCI) {
      Log.error('--no-boostrap is not allowed on CI')
      process.exit(1)
    }
    args.push('--no-bootstrap')
  }

  if (options.history === false) {
    args.push('--no-history')
  }

  util.runGclient(
    args,
    { cwd: config.braveCoreDir },
    path.join(config.braveCoreDir, '.brave_gclient'),
  )
}

async function sync(options) {
  // Install depot_tools early to make Python available.
  depotTools.installDepotTools()

  // Read the existing .gclient config to reuse some values from it if they are
  // not provided.
  const existingGclientConfig = options.init ? {} : syncUtil.readGclientConfig()

  // --target_os, --target_arch as lists make sense only for `init/sync`
  // commands. Handle comma-separated values here and only pass the first value
  // to the config.update() call.
  const targetOSList = commaSeparatedToList(
    options.target_os,
    existingGclientConfig.target_os || [],
  )
  if (targetOSList.length > 0) {
    options.target_os = targetOSList[0]
  }
  const targetArchList = commaSeparatedToList(
    options.target_arch,
    existingGclientConfig.target_cpu || [],
  )
  if (targetArchList.length > 0) {
    options.target_arch = targetArchList[0]
  }

  config.update(options)

  if (
    config.disableGclientConfigUpdate
    && fs.existsSync(config.gclientFile)
    && !options.init
  ) {
    Log.warn(
      `Skipping ${config.gclientFile} update (disable_gclient_config_update=true)`,
    )
  } else {
    syncUtil.writeGclientConfig(targetOSList, targetArchList)
  }

  if (isCI) {
    options.delete_unused_deps = true
  }

  Log.progressScope('gclient sync', () => {
    const didSyncChromium = syncUtil.syncChromium(options)
    if (!didSyncChromium || options.delete_unused_deps) {
      // If no Chromium sync was done, run sync inside `brave` to sync Brave DEPS.
      syncBrave(options)
    }
  })

  depotTools.optOutOfBuildTelemetry()

  await util.applyPatches()

  if (!options.nohooks) {
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

  sisoUtils.writeSisoRc()
}

function commaSeparatedToList(value, defaultValue) {
  return value?.split(',').filter(Boolean) || defaultValue
}
