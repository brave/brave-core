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
const updateChromeVersion = require('../lib/sync/updateChromeVersion')
const chalk = require('chalk')

program
  .version(process.env.npm_package_version)
  .option('--gclient_file <file>', 'gclient config file location')
  .option('--gclient_verbose', 'verbose output for gclient')
  .option('--target_os <target_os>', 'comma-separated target OS list')
  .option('--target_arch <target_arch>', 'comma-separated target architecture list')
  .option('--target_android_base <target_android_base>', 'target Android OS level for apk or aab (classic, modern, mono)')
  .option('--init', 'initialize all dependencies')
  .option('--force', 'force reset all projects to origin/ref')
  .option('--sync_chromium [arg]', 'force or skip chromium sync (true/false/1/0)', JSON.parse)
  .option('--ignore_chromium', 'do not update chromium version even if it is stale [deprecated, use --sync_chromium=false]')
  .option('-D, --delete_unused_deps', 'delete from the working copy any dependencies that have been removed since the last sync')
  .option('--nohooks', 'Do not run hooks after updating')

function maybeInstallDepotTools(options = config.defaultOptions) {
  options.cwd = config.braveCoreDir

  if (!fs.existsSync(config.depotToolsDir)) {
    Log.progressScope('install depot_tools', () => {
      fs.mkdirSync(config.depotToolsDir)
      util.run(
        'git',
        [
          '-C',
          config.depotToolsDir,
          'clone',
          'https://chromium.googlesource.com/chromium/tools/depot_tools.git',
          '.'
        ],
        options
      )
    })
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

function toGClientConfigItem(name, value, pretty = true) {
  // Convert value to json and replace "%True%" -> True, "%False%" -> False,
  // "%None%" -> None.
  const pythonLikeValue =
      JSON.stringify(value, null, pretty ? 2 : 0).replace(/"%(.*?)%"/gm, '$1')
  return `${name} = ${pythonLikeValue}\n`
}

function buildDefaultGClientConfig(targetOSList, targetArchList) {
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
        'checkout_rust': '%True%',
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
  if (targetOSList) {
    out += toGClientConfigItem('target_os', targetOSList, false)
  }
  if (targetArchList) {
    out += toGClientConfigItem('target_cpu', targetArchList, false)
  }

  fs.writeFileSync(config.defaultGClientFile, out)
}

function shouldUpdateChromium(latestSyncInfo, expectedSyncInfo) {
  const chromiumRef = expectedSyncInfo.chromiumRef
  const headSHA = util.runGit(config.srcDir, ['rev-parse', 'HEAD'], true)
  const targetSHA = util.runGit(config.srcDir, ['rev-parse', chromiumRef], true)
  const needsUpdate = targetSHA !== headSHA || (!headSHA && !targetSHA) ||
      JSON.stringify(latestSyncInfo) !== JSON.stringify(expectedSyncInfo)
  if (needsUpdate) {
    const currentRef = util.getGitReadableLocalRef(config.srcDir)
    console.log(`Chromium repo ${chalk.blue.bold('needs sync')}.\n  target is ${
        chalk.italic(chromiumRef)} at commit ${
        targetSHA || '[missing]'}\n  current commit is ${
        chalk.italic(currentRef || '[unknown]')} at commit ${
        chalk.inverse(headSHA || '[missing]')}\n  latest successful sync is ${
        JSON.stringify(latestSyncInfo, null, 4)}`)
  }
  else {
    console.log(
        chalk.green.bold(`Chromium repo does not need sync as it is already ${
            chalk.italic(chromiumRef)} at commit ${targetSHA || '[missing]'}.`))
  }
  return needsUpdate
}

function syncChromium(program) {
  const requiredChromiumRef = config.getProjectRef('chrome')
  let args = [
    'sync', '--nohooks', '--revision', 'src@' + requiredChromiumRef, '--reset',
    '--with_tags', '--with_branch_heads', '--upstream'
  ];

  const syncWithForce = program.init || program.force
  if (syncWithForce) {
    args.push('--force')
  }

  const latestSyncInfoFilePath =
      path.join(config.rootDir, '.brave_latest_successful_sync.json')
  const latestSyncInfo = util.readJSON(latestSyncInfoFilePath, {})
  const expectedSyncInfo = {
    chromiumRef: requiredChromiumRef,
    gClientTimestamp: fs.statSync(config.gClientFile).mtimeMs.toString(),
  }

  const chromiumNeedsUpdate =
      shouldUpdateChromium(latestSyncInfo, expectedSyncInfo)
  const shouldSyncChromium = chromiumNeedsUpdate || syncWithForce
  if (!shouldSyncChromium && !program.sync_chromium) {
    if (program.delete_unused_deps && !config.isCI) {
      Log.warn(
        '--delete_unused_deps is ignored for src/ dir because Chromium sync ' +
        'is required. Pass --sync_chromium to force it.')
    }
    return false
  }

  if (program.delete_unused_deps) {
    if (util.isGitExclusionExists(config.srcDir, 'brave/')) {
      args.push('-D')
    } else if (!config.isCI) {
      Log.warn(
          '--delete_unused_deps is ignored because sync has not yet added ' +
          'the exclusion for the src/brave/ directory, likely because sync ' +
          'has not previously successfully run before.')
    }
  }

  if (program.sync_chromium !== undefined) {
    if (!program.sync_chromium) {
      Log.warn(
          'Chromium needed sync but received the flag to skip performing the ' +
          'update. Working directory may not compile correctly.')
      return false
    } else if (!shouldSyncChromium) {
      Log.warn(
          'Chromium doesn\'t need sync but received the flag to do it anyway.')
    }
  }

  util.runGClient(args)
  util.addGitExclusion(config.srcDir, 'brave/')
  util.writeJSON(latestSyncInfoFilePath, expectedSyncInfo)

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

  if (program.init || !fs.existsSync(config.depotToolsDir)) {
    maybeInstallDepotTools()
  }

  if (program.init || !fs.existsSync(config.defaultGClientFile)) {
    buildDefaultGClientConfig(targetOSList, targetArchList)
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
    const didSyncChromium = syncChromium(program)
    if (!didSyncChromium || program.delete_unused_deps) {
      // If no Chromium sync was done, run sync inside `brave` to sync Brave DEPS.
      syncBrave(program)
    }
  })

  await util.applyPatches()
  updateChromeVersion(config)

  if (!program.nohooks) {
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
