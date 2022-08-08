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
const chalk = require('chalk')

program
  .version(process.env.npm_package_version)
  .option('--gclient_file <file>', 'gclient config file location')
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'target OS')
  .option('--target_arch <target_arch>', 'target architecture')
  .option('--target_android_base <target_android_base>', 'target Android OS level for apk or aab (classic, modern, mono)')
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--sync_src [arg]', 'force or skip src sync (true/false/1/0)', JSON.parse)
  .option('--cleanup', 'remove obsolete gclient dirs')
  .option('--nohooks', 'Do not run hooks after updating')

function maybeInstallDepotTools() {
  if (!fs.existsSync(config.depotToolsDir)) {
    Log.progress('Install Depot Tools...')
    fs.mkdirSync(config.depotToolsDir)
    const options = util.mergeOptionsWithDefault({cwd: config.depotToolsDir})
    util.run(
        'git',
        [
          'clone',
          'https://chromium.googlesource.com/chromium/tools/depot_tools.git',
          '.'
        ],
        options)
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
      'opt-in': false,
    };
    fs.writeFileSync(ninjaLogCfgPath, JSON.stringify(ninjaLogCfgConfig))
  }
}

function buildDefaultGClientConfig() {
  function toGClientConfigItem(name, value, pretty = true) {
    // Convert value to json and replace "%True%" -> True, "%False%" -> False,
    // "%None%" -> None.
    const pythonLikeValue =
        JSON.stringify(value, null, pretty ? 2 : 0).replace(/"%(.*?)%"/gm, '$1')
    return `${name} = ${pythonLikeValue}\n`
  }

  let out = toGClientConfigItem('solutions', [
    {
      managed: '%False%',
      name: 'src',
      url: config.chromiumRepo,
      custom_deps: {
        'src/third_party/WebKit/LayoutTests': '%None%',
        'src/chrome_frame/tools/test/reference_build/chrome': '%None%',
        'src/chrome_frame/tools/test/reference_build/chrome_win': '%None%',
        'src/chrome/tools/test/reference_build/chrome': '%None%',
        'src/chrome/tools/test/reference_build/chrome_linux': '%None%',
        'src/chrome/tools/test/reference_build/chrome_mac': '%None%',
        'src/chrome/tools/test/reference_build/chrome_win': '%None%'
      },
      custom_vars: {
        'checkout_pgo_profiles': config.isBraveReleaseBuild() ? '%True%' :
                                                                '%False%'
      }
    },
    {
      managed: '%False%',
      name: 'src/brave',
      // We do not use gclient to manage brave-core, so this should not
      // actually get used.
      url: 'https://github.com/brave/brave-core.git'
    }
  ])

  if (process.env.GIT_CACHE_PATH) {
    out += toGClientConfigItem('cache_dir', process.env.GIT_CACHE_PATH)
  }
  if (config.targetOS) {
    out += toGClientConfigItem('target_os', [config.targetOS], false)
  }

  fs.writeFileSync(config.defaultGClientFile, out)
}

function shouldUpdateChromium(latestSuccessfulSync, expectedSuccessfulSync) {
  const chromiumRef = expectedSuccessfulSync.chromiumRef
  const headSHA = util.runGit(config.srcDir, ['rev-parse', 'HEAD'], true)
  const targetSHA = util.runGit(config.srcDir, ['rev-parse', chromiumRef], true)
  const needsUpdate = targetSHA !== headSHA || (!headSHA && !targetSHA) ||
      JSON.stringify(latestSuccessfulSync) !==
          JSON.stringify(expectedSuccessfulSync)
  if (needsUpdate) {
    const currentRef = util.getGitReadableLocalRef(config.srcDir)
    console.log(
        `Chromium repo ${chalk.blue.bold('needs sync')}.\n  target is ${
            chalk.italic(chromiumRef)} at commit ${
            targetSHA || '[missing]'}\n  current commit is ${
            chalk.italic(currentRef || '[unknown]')} at commit ${
            chalk.inverse(
                headSHA || '[missing]')}\n  latest successful sync is ${
                  JSON.stringify(latestSuccessfulSync, null, 4)}`)
  } else {
    console.log(
        chalk.green.bold(`Chromium repo does not need sync as it is already ${
            chalk.italic(
                chromiumRef)} at commit ${targetSHA || '[missing]'}.`))
  }
  return needsUpdate
}

function syncChromium(program) {
  const requiredChromiumRef = config.getProjectRef('chrome')
  let args = [
    'sync', '--nohooks', '--reset', '--revision',
    'src@' + requiredChromiumRef, '--with_tags',
    '--with_branch_heads', '--upstream'
  ];

  const syncWithForce = program.init || program.force
  if (syncWithForce) {
    args.push('--force')
  }
  if (program.cleanup) {
    if (util.isGitExclusionExists(config.srcDir, 'brave/')) {
      args.push('-D')
    } else {
      Log.warn(
          '--cleanup is ignored, because brave dir is not excluded from src')
    }
  }

  const latestSuccessfulSyncFile =
      path.join(config.rootDir, '.brave_latest_successful_sync.json')
  const latestSuccessfulSync = util.readJSON(latestSuccessfulSyncFile)
  const expectedSuccessfulSync = {
    chromiumRef: requiredChromiumRef,
    gClientTimestamp: fs.statSync(config.gClientFile).mtimeMs.toString()
  }

  const chromiumNeedsUpdate =
      shouldUpdateChromium(latestSuccessfulSync, expectedSuccessfulSync)
  const shouldSyncSrc = chromiumNeedsUpdate || syncWithForce ||
      program.sync_src || program.cleanup
  if (!shouldSyncSrc) {
    return false
  }

  if (program.sync_src !== undefined && !program.sync_src) {
    Log.warn(chalk.yellow.bold(
        'Chromium needed sync but received the flag to skip performing the ' +
        'update. Working directory may not compile correctly.'))
    return false
  }

  util.runGClient(args, {cwd: config.rootDir}, config.gClientFile)
  util.addGitExclusion(config.srcDir, 'brave/')
  util.writeJSON(latestSuccessfulSyncFile, expectedSuccessfulSync)

  const postSyncChromiumRef = util.getGitReadableLocalRef(config.srcDir)
  Log.status(`Chromium is now at ${postSyncChromiumRef || '[unknown]'}`)
  return true
}

function syncBrave(program) {
  let args = ['sync', '--nohooks']
  const syncWithForce = program.init || program.force
  if (syncWithForce) {
    args.push('--force')
  }
  if (program.cleanup) {
    args.push('-D')
  }

  // Don't pass gClientFile here, let gclient find it automatically, which
  // should be brave/.gclient.
  util.runGClient(args, {cwd: config.braveCoreDir}, null)
}

function gclientRunhooks() {
  util.runGClient(['runhooks'], {cwd: config.rootDir}, config.gClientFile)
}

async function RunCommand () {
  program.parse(process.argv)
  config.update(program)

  if (program.init || !fs.existsSync(config.depotToolsDir)) {
    maybeInstallDepotTools()
  }

  if (program.init || !fs.existsSync(config.defaultGClientFile)) {
    buildDefaultGClientConfig()
  } else if (program.target_os) {
    Log.warn(
        '--target_os is ignored. Use with --init or edit .gclient manually')
  }

  Log.progress('Running gclient sync...')
  const didSyncChromium = syncChromium(program)
  if (!didSyncChromium) {
    // If no Chromium sync was done, run sync inside `brave` to sync Brave DEPS.
    syncBrave(program)
  }
  Log.progress('...gclient sync done.')

  await util.applyPatches()

  if (!program.nohooks) {
    // Run hooks for the root .gclient, this will include Chromium and Brave
    // hooks. Don't cache the result, just always rerun this step, because it's
    // pretty quick in a no-op scenario.
    Log.progress('Running gclient runhooks...')
    gclientRunhooks()
    Log.progress('...gclient runhooks done.')
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
