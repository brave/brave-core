// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import chalk from 'chalk'
import config from './config.ts'
import { checkoutChromiumRef } from './chromiumFetch.ts'
import { isCI } from './ciDetect.ts'
import fs from 'node:fs'
import path from 'node:path'
import * as Log from './log.ts'
import util from './util.ts'

function toGClientConfigItem(name: string, value: unknown, pretty = true) {
  if (value === undefined) {
    return ''
  }

  const valueMap: Record<string, string> = {
    true: '%True%',
    false: '%False%',
    null: '%None%',
  }

  const replacer = (_: string, value: unknown) =>
    valueMap[String(value)] || value

  const pythonLikeValue = JSON.stringify(
    value,
    replacer,
    pretty ? 2 : 0,
  ).replace(/"%(.*?)%"/gm, '$1')
  return `${name} = ${pythonLikeValue}\n`
}

export function writeGclientConfig(
  targetOSList: string[],
  targetArchList: string[],
  onlyChromium = false,
) {
  const gclientConfig: Record<string, any> = {
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

function writeGclientConfigFile(
  gclientConfig: Record<string, any>,
  filePath: string,
  header: string,
) {
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

export function readGclientConfig() {
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

interface SyncInfo {
  chromiumRef: string
  gclientTimestamp: string
}

function shouldUpdateChromium(
  latestSyncInfo: Partial<SyncInfo>,
  expectedSyncInfo: SyncInfo,
) {
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

// Generates the git config that includes the global one plus redirects of
// upstream fetches to our Gerrit mirrors, then points GIT_CONFIG_GLOBAL at it.
// Fetches authenticate as BRAVE_USE_GERRIT_MIRRORS_USER. An existing file is
// only regenerated when `update` is set. No-op unless that user is set. The
// file is inert without GIT_CONFIG_GLOBAL being set.
function configureGerritMirrors(update: boolean) {
  if (!config.gerritMirrorsUser) {
    return
  }

  const args = [
    path.join(
      config.braveCoreDir,
      'tools',
      'recipes',
      'recipe_modules',
      'brave_core_checkout',
      'resources',
      'mirror_git_config.py',
    ),
    'install',
    '--user',
    config.gerritMirrorsUser,
  ]
  if (update) {
    args.push('--update')
  }
  util.run('vpython3', args, config.defaultOptions)
  config.applyGerritMirrorsGitConfig()
}

type SyncOptions = {
  init?: boolean
  force?: boolean
  sync_chromium?: boolean
  delete_unused_deps?: boolean
  fetch_all?: boolean
  bootstrap?: boolean
  history?: boolean
}

export function syncChromium(program: SyncOptions) {
  const syncWithForce = Boolean(program.init || program.force)
  const syncChromiumValue = program.sync_chromium
  const deleteUnusedDeps = program.delete_unused_deps
  let tryLeanCheckout = config.leanSync

  const requiredChromiumRef = config.getProjectRef('chrome')
  const args = ['sync', '--nohooks', '--reset', '--upstream']

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

  // Before any fetch, so both the Chromium and Brave syncs go through the
  // mirrors when enabled. The mirror list is only refreshed alongside a
  // Chromium update or a forced sync (`init`, `--force`), so no-op syncs stay
  // offline.
  configureGerritMirrors(shouldSyncChromium)

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
    remove: ['brave/', 'brave_origin/'],
    add: ['/brave/'],
  })
  util.writeJSON(latestSyncInfoFilePath, expectedSyncInfo)

  const postSyncChromiumRef = util.getGitReadableLocalRef(config.srcDir)
  Log.status(`Chromium is now at ${postSyncChromiumRef || '[unknown]'}`)
  return true
}

export async function checkInternalDepsEndpoint() {
  if (!config.useBraveHermeticToolchain) {
    return true
  }

  try {
    const response = await fetch(
      `${config.internalDepsUrl}/windows-hermetic-toolchain/test.txt`,
      { method: 'HEAD', signal: AbortSignal.timeout(5000), redirect: 'manual' },
    )
    return response.status === 302
  } catch {
    return false
  }
}
