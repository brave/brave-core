// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const chalk = require('chalk')
const config = require('./config')
const fs = require('fs')
const path = require('path')
const Log = require('./logging')
const util = require('./util')

function toGClientConfigItem(name, value, pretty = true) {
  // Convert value to json and replace "%True%" -> True, "%False%" -> False,
  // "%None%" -> None.
  const pythonLikeValue =
      JSON.stringify(value, null, pretty ? 2 : 0).replace(/"%(.*?)%"/gm, '$1')
  return `${name} = ${pythonLikeValue}\n`
}

function buildDefaultGClientConfig(
  targetOSList, targetArchList, onlyChromium = false) {
  const items = [
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
    }
  ]

  if (!onlyChromium) {
    items.push({
      managed: '%False%',
      name: 'src/brave',
      // We do not use gclient to manage brave-core, so this should not
      // actually get used.
      url: 'https://github.com/brave/brave-core.git'
    })
  }

  let out = toGClientConfigItem('solutions', items);

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
  const syncWithForce = program.init || program.force
  const syncChromiumValue = program.sync_chromium
  const deleteUnusedDeps = program.delete_unused_deps

  const requiredChromiumRef = config.getProjectRef('chrome')
  let args = [
    'sync', '--nohooks', '--revision', 'src@' + requiredChromiumRef, '--reset',
    '--upstream'
  ];

  if (program.fetch_all) {
    args.push('--with_tags')
    args.push('--with_branch_heads')
  }

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
  if (!shouldSyncChromium && !syncChromiumValue) {
    if (deleteUnusedDeps && !config.isCI) {
      Log.warn(
        '--delete_unused_deps is ignored for src/ dir because Chromium sync ' +
        'is required. Pass --sync_chromium to force it.')
    }
    return false
  }

  if (deleteUnusedDeps) {
    if (util.isGitExclusionExists(config.srcDir, 'brave/')) {
      args.push('-D')
    } else if (!config.isCI) {
      Log.warn(
          '--delete_unused_deps is ignored because sync has not yet added ' +
          'the exclusion for the src/brave/ directory, likely because sync ' +
          'has not previously successfully run before.')
    }
  }

  if (syncChromiumValue !== undefined) {
    if (!syncChromiumValue) {
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

async function checkInternalDepsEndpoint() {
  if (!config.useBraveHermeticToolchain) {
    return true
  }

  try {
    const response = await fetch(
      `${config.internalDepsUrl}/windows-hermetic-toolchain/test.txt`,
      { method: 'HEAD', signal: AbortSignal.timeout(5000), redirect: 'manual' }
    )
    return response.status == 302
  } catch (error) {
    return false
  }
}

module.exports = {
  buildDefaultGClientConfig,
  syncChromium,
  checkInternalDepsEndpoint
}
