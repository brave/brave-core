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
  if (value === undefined) {
    return ''
  }

  const valueMap = {
    true: '%True%',
    false: '%False%',
    null: '%None%',
  }

  const replacer = (_, value) => valueMap[value] || value

  const pythonLikeValue = JSON.stringify(
    value,
    replacer,
    pretty ? 2 : 0,
  ).replace(/"%(.*?)%"/gm, '$1')
  return `${name} = ${pythonLikeValue}\n`
}

function writeGclientConfig(
  targetOSList,
  targetArchList,
  onlyChromium = false,
) {
  const gclientConfig = {
    solutions: [
      {
        managed: false,
        name: 'src',
        url: config.chromiumRepo,
        custom_deps: {
          ...config.chromiumCustomDeps,
        },
        custom_vars: {
          ...config.chromiumCustomVars,
        },
      },
    ],
    cache_dir: process.env.GIT_CACHE_PATH,
    target_os: targetOSList,
    target_cpu: targetArchList,
    ...config.gclientGlobalVars,
  }

  // Add brave-core as a non-managed solution to handle DEPS.
  if (!onlyChromium) {
    gclientConfig.solutions.push({
      managed: false,
      name: 'src/brave',
      // We do not use gclient to manage brave-core, so this should not
      // actually get used.
      url: 'https://github.com/brave/brave-core.git',
    })
  }

  // Generate the gclient config file.
  let out = `# Auto-updated on each sync.
#
# Customize via brave/.env:
#   - projects_chrome_custom_deps: Override chromium solution's custom_deps
#       Example: projects_chrome_custom_deps={"src/third_party/some_dep": null}
#   - projects_chrome_custom_vars: Override chromium solution's custom_vars
#       Example: projects_chrome_custom_vars={"checkout_clangd": true}
#   - gclient_global_vars: Override top-level .gclient variables
#       Example: gclient_global_vars={"delete_unversioned_trees": true}
#
# Key prefixes can be used to override specific values:
#   projects_chrome_custom_vars_checkout_clangd=true
#   gclient_global_vars_delete_unversioned_trees=true
#
# Multiline values can be defined using single quotes:
#   projects_chrome_custom_vars='{
#     "checkout_clang_tidy": true,
#     "checkout_clangd": true
#   }'
#
# Note: target_os and target_cpu persist unless set via CLI.

`
  for (const [key, value] of Object.entries(gclientConfig)) {
    const singleLineValue = toGClientConfigItem(key, value, false)
    if (singleLineValue.length > 80) {
      out += toGClientConfigItem(key, value, true)
    } else {
      out += singleLineValue
    }
  }

  if (util.writeFileIfModified(config.gclientFile, out)) {
    Log.status(`${config.gclientFile} has been updated`)
  }
}

function readGclientConfig() {
  if (!fs.existsSync(config.gclientFile)) {
    return {}
  }

  try {
    const script = `
import json
out = {}
path = r"""${config.gclientFile}"""
exec(compile(open(path, 'r').read(), path, 'exec'), None, out)
print(json.dumps(out))
`
    const result = util.run(
      'python3',
      ['-'],
      util.mergeWithDefault({
        skipLogging: true,
        stdio: 'pipe',
        input: script,
        encoding: 'utf8',
        continueOnFail: true,
      }),
    )
    if (result.status !== 0) {
      throw new Error(result.stderr.toString().trim())
    }
    return JSON.parse(result.stdout.toString().trim())
  } catch (error) {
    Log.error(`Failed to read ${config.gclientFile}:\n${error}`)
    process.exit(1)
  }
}

function shouldUpdateChromium(latestSyncInfo, expectedSyncInfo) {
  const chromiumRef = expectedSyncInfo.chromiumRef
  const headSHA = util.runGit(config.srcDir, ['rev-parse', 'HEAD'], true)
  const targetSHA = util.runGit(config.srcDir, ['rev-parse', chromiumRef], true)
  const needsUpdate =
    targetSHA !== headSHA
    || (!headSHA && !targetSHA)
    || JSON.stringify(latestSyncInfo) !== JSON.stringify(expectedSyncInfo)
  if (needsUpdate) {
    const currentRef = util.getGitReadableLocalRef(config.srcDir)
    console.log(
      `Chromium repo ${chalk.blue.bold(
        'needs sync',
      )}.\n  target is ${chalk.italic(chromiumRef)} at commit ${
        targetSHA || '[missing]'
      }\n  current commit is ${chalk.italic(
        currentRef || '[unknown]',
      )} at commit ${chalk.inverse(
        headSHA || '[missing]',
      )}\n  latest successful sync is ${JSON.stringify(
        latestSyncInfo,
        null,
        4,
      )}`,
    )
  } else {
    console.log(
      chalk.green.bold(
        `Chromium repo does not need sync as it is already ${chalk.italic(
          chromiumRef,
        )} at commit ${targetSHA || '[missing]'}.`,
      ),
    )
  }
  return needsUpdate
}

function syncChromium(program) {
  const syncWithForce = program.init || program.force
  const syncChromiumValue = program.sync_chromium
  const deleteUnusedDeps = program.delete_unused_deps
  const gclientWithoutRevision = program.with_issue_44921

  const requiredChromiumRef = config.getProjectRef('chrome')
  let args = ['sync', '--nohooks', '--reset', '--upstream']

  if (!gclientWithoutRevision) {
    args.push('--revision')
    args.push('src@' + requiredChromiumRef)
  }

  if (program.fetch_all) {
    args.push('--with_tags')
    args.push('--with_branch_heads')
  }

  if (syncWithForce) {
    args.push('--force')
  }

  if (program.history === false) {
    args.push('--no-history')
  }

  const latestSyncInfoFilePath = path.join(
    config.rootDir,
    '.brave_latest_successful_sync.json',
  )
  const latestSyncInfo = util.readJSON(latestSyncInfoFilePath, {})
  const expectedSyncInfo = {
    chromiumRef: requiredChromiumRef,
    gclientTimestamp: fs.statSync(config.gclientFile).mtimeMs.toString(),
  }

  const chromiumNeedsUpdate = shouldUpdateChromium(
    latestSyncInfo,
    expectedSyncInfo,
  )
  const shouldSyncChromium = chromiumNeedsUpdate || syncWithForce
  if (!shouldSyncChromium && !syncChromiumValue) {
    if (deleteUnusedDeps && !config.isCI) {
      Log.warn(
        '--delete_unused_deps is ignored for src/ dir because Chromium sync '
          + 'is required. Pass --sync_chromium to force it.',
      )
    }
    return false
  }

  if (deleteUnusedDeps) {
    if (util.isGitExclusionExists(config.srcDir, '/brave/')) {
      args.push('-D')
    } else if (!config.isCI) {
      Log.warn(
        '--delete_unused_deps is ignored because sync has not yet added '
          + 'the exclusion for the src/brave/ directory, likely because sync '
          + 'has not previously successfully run before.',
      )
    }
  }

  if (syncChromiumValue !== undefined) {
    if (!syncChromiumValue) {
      Log.warn(
        'Chromium needed sync but received the flag to skip performing the '
          + 'update. Working directory may not compile correctly.',
      )
      return false
    } else if (!shouldSyncChromium) {
      Log.warn(
        "Chromium doesn't need sync but received the flag to do it anyway.",
      )
    }
  }

  if (
    gclientWithoutRevision
    && (syncWithForce || chromiumNeedsUpdate)
    && fs.existsSync(path.join(config.srcDir, 'chrome', 'VERSION'))
  ) {
    // Checking out chromium manually if necessary, as no `--revsion` flag is
    // being passed to gclient.
    if (
      util.runGit(config.srcDir, ['rev-parse', requiredChromiumRef], true)
      == null
    ) {
      util.runGit(config.srcDir, [
        'fetch',
        'origin',
        requiredChromiumRef + ':' + requiredChromiumRef,
      ])
    }
    util.runGit(config.srcDir, ['reset', '--hard', requiredChromiumRef])
  }

  util.runGclient(args)
  util.modifyGitExclusions(config.srcDir, {
    remove: ['brave/', 'brave_origin/'],
    add: ['/brave/'],
  })
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
      { method: 'HEAD', signal: AbortSignal.timeout(5000), redirect: 'manual' },
    )
    return response.status === 302
  } catch (error) {
    return false
  }
}

module.exports = {
  writeGclientConfig,
  readGclientConfig,
  syncChromium,
  checkInternalDepsEndpoint,
}
