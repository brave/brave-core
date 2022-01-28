// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const fs = require('fs')
const program = require('commander')
const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('../lib/sync/logging')

program
  .version(process.env.npm_package_version)
  .option('--gclient_file <file>', 'gclient config file location')
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'target OS')
  .option('--target_arch <target_arch>', 'target architecture')
  .option('--target_android_base <target_android_base>', 'target Android OS level for apk or aab (classic, modern, mono)')
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--ignore_chromium', 'do not update chromium version even if it is stale')
  .option('--nohooks', 'Do not run hooks after updating')

const maybeInstallDepotTools = (options = config.defaultOptions) => {
  options.cwd = config.braveCoreDir

  if (!fs.existsSync(config.depotToolsDir)) {
    Log.progress('Install Depot Tools...')
    fs.mkdirSync(config.depotToolsDir)
    util.run('git', ['-C', config.depotToolsDir, 'clone', 'https://chromium.googlesource.com/chromium/tools/depot_tools.git', '.'], options)
    Log.progress('Done Depot Tools...')
  }

  const ninjaLogCfgPath = path.join(config.depotToolsDir, 'ninjalog.cfg');
  if (!fs.existsSync(ninjaLogCfgPath)) {
    // Create a ninja config to prevent (auto)ninja from calling goma_auth
    // each time. See for details:
    // https://chromium.googlesource.com/chromium/tools/depot_tools/+/main/ninjalog.README.md
    const ninjaLogCfgConfig = {
      'is-googler': false,
      'version': 3,
      'countdown': 10,
      'opt-in': false
    };
    fs.writeFileSync(ninjaLogCfgPath, JSON.stringify(ninjaLogCfgConfig))
  }
}

async function RunCommand () {
  program.parse(process.argv)
  config.update(program)

  let braveCoreRef = program.args[0]
  if (braveCoreRef && !program.init) {
    Log.error('[ref] option requies --init to work correctly')
    process.exit(1)
  }

  maybeInstallDepotTools()

  if (program.init) {
    util.buildGClientConfig()
  }

  braveCoreRef = program.init ? config.getProjectVersion('brave-core') : null

  if (program.init || program.force) {
    // we're doing a reset of brave-core so try to stash any changes
    Log.progress('Stashing any local changes')
    util.runGit(config.braveCoreDir, ['stash'], true)
  }

  Log.progress('Running gclient sync...')
  const result = util.gclientSync(program.init || program.force, program.init, braveCoreRef, !program.ignore_chromium)
  const postSyncBraveCoreRef = util.getGitReadableLocalRef(config.braveCoreDir)
  if (braveCoreRef) {
    Log.status(`Brave Core is now at ${postSyncBraveCoreRef || '[unknown]'}`)
  }
  if (result.didUpdateChromium) {
    const postSyncChromiumRef = util.getGitReadableLocalRef(config.srcDir)
    Log.status(`Chromium is now at ${postSyncChromiumRef || '[unknown]'}`)
  }
  Log.progress('...gclient sync done')

  await util.applyPatches()

  if (!program.nohooks) {
    util.gclientRunhooks()
  }
}

Log.progress('Brave Browser Sync starting')
RunCommand()
.then(() => {
  Log.progress('Brave Browser Sync complete')
})
.catch((err) => {
  Log.error('Brave Browser Sync ERROR:')
  console.error(err)
  process.exit(1)
})
