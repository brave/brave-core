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

function buildGClientConfig() {
  function replacer(key, value) {
    return value;
  }

  const solutions = [
    {
      managed: "%False%",
      name: "src",
      url: config.chromiumRepo,
      custom_deps: {
        "src/third_party/WebKit/LayoutTests": "%None%",
        "src/chrome_frame/tools/test/reference_build/chrome": "%None%",
        "src/chrome_frame/tools/test/reference_build/chrome_win": "%None%",
        "src/chrome/tools/test/reference_build/chrome": "%None%",
        "src/chrome/tools/test/reference_build/chrome_linux": "%None%",
        "src/chrome/tools/test/reference_build/chrome_mac": "%None%",
        "src/chrome/tools/test/reference_build/chrome_win": "%None%"
      },
      custom_vars: {
        "checkout_pgo_profiles": config.isBraveReleaseBuild() ? "%True%" : "%False%"
      }
    },
    {
      managed: "%False%",
      name: "src/brave",
      // We do not use gclient to manage brave-core, so this should
      // not actually get used.
      url: 'https://github.com/brave/brave-core.git'
    }
  ]

  let cache_dir = process.env.GIT_CACHE_PATH ? ('\ncache_dir = "' + process.env.GIT_CACHE_PATH + '"\n') : '\n'

  let out = 'solutions = ' + JSON.stringify(solutions, replacer, 2)
    .replace(/"%None%"/g, "None").replace(/"%False%"/g, "False").replace(/"%True%"/g, "True") + cache_dir

  if (config.targetOS) {
    out = out + "target_os = [ '" + config.targetOS + "' ]"
  }

  fs.writeFileSync(config.defaultGClientFile, out)
}

function shouldUpdateChromium(chromiumRef = config.getProjectRef('chrome')) {
  const headSHA = util.runGit(config.srcDir, ['rev-parse', 'HEAD'], true)
  const targetSHA = util.runGit(config.srcDir, ['rev-parse', chromiumRef], true)
  const needsUpdate = ((targetSHA !== headSHA) || (!headSHA && !targetSHA))
  if (needsUpdate) {
    const currentRef = util.getGitReadableLocalRef(config.srcDir)
    console.log(`Chromium repo ${chalk.blue.bold('needs update')}. Target is ${chalk.italic(chromiumRef)} at commit ${targetSHA || '[missing]'} but current commit is ${chalk.italic(currentRef || '[unknown]')} at commit ${chalk.inverse(headSHA || '[missing]')}.`)
  } else {
    console.log(chalk.green.bold(`Chromium repo does not need update as it is already ${chalk.italic(chromiumRef)} at commit ${targetSHA || '[missing]'}.`))
  }
  return needsUpdate
}

function gclientSync(forceReset = false, cleanup = false, shouldCheckChromiumVersion = true, options = {}) {
  let reset = forceReset

  // base args
  const initialArgs = ['sync', '--nohooks']
  const chromiumArgs = ['--revision', 'src@' + config.getProjectRef('chrome')]
  const resetArgs = ['--reset', '--with_tags', '--with_branch_heads', '--upstream']

  let args = [...initialArgs]
  let didUpdateChromium = false

  if (!shouldCheckChromiumVersion) {
    const chromiumNeedsUpdate = shouldUpdateChromium()
    if (chromiumNeedsUpdate) {
      console.warn(chalk.yellow.bold('Chromium needed update but received the flag to skip performing the update. Working directory may not compile correctly.'))
    }
  } else if (forceReset || shouldUpdateChromium()) {
    args = [...args, ...chromiumArgs]
    reset = true
    didUpdateChromium = true
  }

  if (forceReset) {
    args = args.concat(['--force'])
    if (cleanup) {
      // temporarily ignored until we can figure out how not to delete src/brave in the process
      // args = args.concat(['-D'])
    }
  }

  if (reset) {
    args = [...args, ...resetArgs]
  }

  util.runGClient(args, options)

  return {
    didUpdateChromium
  }
}

function gclientRunhooks(options = {}) {
  Log.progress('Running gclient hooks...')
  util.runGClient(['runhooks'], options)
  Log.progress('Done running gclient hooks.')
}

async function RunCommand () {
  program.parse(process.argv)
  config.update(program)

  if (program.all || program.run_hooks || program.run_sync) {
    Log.warn('--all, --run_hooks and --run_sync are deprecated. Will behave as if flag was not passed. Please update your command to `npm run sync` in the future.')
  }

  if (program.init || !fs.existsSync(config.depotToolsDir)) {
    maybeInstallDepotTools()
  }

  if (program.init) {
    buildGClientConfig()
  }

  Log.progress('Running gclient sync...')
  const result = gclientSync(program.init || program.force, program.init, !program.ignore_chromium)
  if (result.didUpdateChromium) {
    const postSyncChromiumRef = util.getGitReadableLocalRef(config.srcDir)
    Log.status(`Chromium is now at ${postSyncChromiumRef || '[unknown]'}`)
  }
  Log.progress('...gclient sync done')

  await util.applyPatches()

  if (!program.nohooks) {
    gclientRunhooks()
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
