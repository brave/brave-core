// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import chalk from 'chalk'
import config from './config.ts'
import { isCI } from './ciDetect.ts'
import fs from 'node:fs'
import path from 'node:path'
import * as Log from './log.ts'
import util from './util.js'

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
  /** @type {Record<string, any>} */
  const gclientConfig = {
    solutions: [
      {
        managed: false,
        name: 'src',
        url: config.chromiumRepo,
        custom_deps: config.chromiumCustomDeps,
        custom_vars: config.chromiumCustomVars,
      },
    ],
    cache_dir: config.gitCachePath,
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

    const braveGclientConfig = {
      solutions: [
        {
          managed: false,
          name: '.',
          url: 'https://github.com/brave/brave-core.git',
        },
      ],
      cache_dir: config.gitCachePath,
      target_os: targetOSList,
      target_cpu: targetArchList,
      ...config.gclientGlobalVars,
    }

    writeGclientConfigFile(
      braveGclientConfig,
      path.join(config.braveCoreDir, '.brave_gclient'),
      '# Auto-updated on each sync.\n\n',
    )
  }

  // Generate the gclient config file.
  writeGclientConfigFile(
    gclientConfig,
    config.gclientFile,
    `# Auto-updated on each sync.
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

`,
  )
}

function writeGclientConfigFile(gclientConfig, filePath, header) {
  let out = header
  for (const [key, value] of Object.entries(gclientConfig)) {
    const singleLineValue = toGClientConfigItem(key, value, false)
    if (singleLineValue.length > 80) {
      out += toGClientConfigItem(key, value, true)
    } else {
      out += singleLineValue
    }
  }

  if (util.writeFileIfModified(filePath, out)) {
    Log.status(`${filePath} has been updated`)
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

// This is a lean fetch command as chromium/src is really large, and certain
// types of fetch can easily traverse the whole history, resulting in a stall.
const chromiumFetchCmd = [
  '-c',
  'advice.fetchShowForcedUpdates=false',
  'fetch',
  '--no-show-forced-updates',
  '--no-tags',
]

// Checks that a given ref exists in the Chromium repository.
function chromiumRefExists(ref) {
  return (
    util.runGit(config.srcDir, ['rev-parse', '--verify', '--quiet', ref], true)
    !== ''
  )
}

// Path of the shared chromium/src mirror (only for git-cache).
function gitCacheMirrorDir(url) {
  const result = util.run(
    'git',
    ['cache', 'exists', '--quiet', '--cache-dir', config.gitCachePath, url],
    util.mergeWithDefault({
      stdio: 'pipe',
      encoding: 'utf8',
      continueOnFail: true,
    }),
  )
  return result.status === 0 ? result.stdout.toString().trim() : ''
}

// Fetches ref into src/ through git-cache
function fetchChromiumRef(ref) {
  let remote = 'origin'

  if (config.gitCachePath) {
    util.run(
      'git',
      [
        'cache',
        'populate',
        '--cache-dir',
        config.gitCachePath,
        // Only the requested ref is wanted, not all of Chromium's tags.
        '--no-fetch-tags',
        '--ref',
        ref,
        // Otherwise the mirror keeps a refspec for every ref ever populated.
        '--reset-fetch-config',
        config.chromiumRepo,
      ],
      util.mergeWithDefault({ cwd: config.rootDir }),
    )
    // This should always have a valid value, as we just populated the cache.
    remote = gitCacheMirrorDir(config.chromiumRepo) || remote
  }

  util.run(
    'git',
    [...chromiumFetchCmd, remote, `${ref}:${ref}`],
    util.mergeWithDefault({ cwd: config.srcDir }),
  )
}

// Tries to do a lean checkout of the given Chromium ref.
function checkoutChromiumRef(ref) {
  if (!ref.startsWith('refs/')) {
    // We do not attempt a lean checkout for random refs.
    Log.warn(
      `${ref} is not a fully-qualified ref, letting gclient check it out.`,
    )
    return false
  }

  // We assume that chromium tags never move, which is the case for the project.
  if (!ref.startsWith('refs/tags/') || !chromiumRefExists(ref)) {
    fetchChromiumRef(ref)
  }

  util.runGit(config.srcDir, ['reset', '--hard', ref])
  return true
}

function syncChromium(program) {
  const syncWithForce = program.init || program.force
  const syncChromiumValue = program.sync_chromium
  const deleteUnusedDeps = program.delete_unused_deps
  let tryLeanCheckout = config.leanSync

  const requiredChromiumRef = config.getProjectRef('chrome')
  let args = ['sync', '--nohooks', '--reset', '--upstream']

  if (program.fetch_all) {
    args.push('--with_tags')
    args.push('--with_branch_heads')
  }

  if (syncWithForce) {
    args.push('--force')
  }

  if (program.bootstrap === false) {
    if (isCI) {
      Log.error('--no-boostrap is not allowed on CI')
      process.exit(1)
    }
    args.push('--no-bootstrap')
  }

  if (program.history === false) {
    args.push('--no-history')
  }

  const latestSyncInfoFilePath = path.join(
    config.rootDir,
    '.brave_latest_successful_sync.json',
  )
  // @ts-ignore
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
    if (deleteUnusedDeps && !isCI) {
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
    } else if (!isCI) {
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

  if (tryLeanCheckout && (syncWithForce || chromiumNeedsUpdate)) {
    // A lean checkout can only be done if we already have cloned chromium/src.
    tryLeanCheckout =
      fs.existsSync(path.join(config.srcDir, 'chrome', 'VERSION'))
      && checkoutChromiumRef(requiredChromiumRef)
  }

  if (!tryLeanCheckout) {
    args.push('--revision')
    args.push('src@' + requiredChromiumRef)
  }

  util.runGclient(args)
  util.modifyGitExclusions(config.srcDir, {
    // @ts-ignore
    remove: ['brave/', 'brave_origin/'],
    // @ts-ignore
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

export default {
  writeGclientConfig,
  readGclientConfig,
  syncChromium,
  checkInternalDepsEndpoint,
}
