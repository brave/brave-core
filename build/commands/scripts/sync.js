// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const fs = require('fs')
const program = require('commander')
const config = require('../lib/config')
const util = require('../lib/util')
const Log = require('../lib/sync/logging')
const os = require('os')
const path = require('path')

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

  if (config.isCI && config.getCachePath()) {
    util.runGClient([])
    if (process.platform === 'win32' || process.platform === 'linux') {
      util.runGit(
          config.rootDir, ['config', '--global', 'checkout.workers', 16], false)
    }
    console.log(
        'gitcache size before sync',
        util.runPython3([
              path.join(config.braveCoreDir, 'script/dir_size.py'),
              config.getCachePath()
            ])
            .stdout.toString()
            .trim())
  }

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

  if (config.isCI && config.getCachePath()) {
    console.log(
        'gitcache size after sync',
        util.runPython3([
              path.join(config.braveCoreDir, 'script/dir_size.py'),
              config.getCachePath()
            ])
            .stdout.toString()
            .trim())
  }

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
