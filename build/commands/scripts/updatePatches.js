// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import { glob } from 'node:fs/promises'
import path from 'node:path'
import config from '../lib/config.ts'
import updatePatches from '../lib/updatePatches.js'

function loadChromiumPathFilter(filePath) {
  const configLines = fs
    .readFileSync(filePath, 'utf-8')
    .split('\n')
    // @ts-ignore
    .map((line) => line.split('#')[0].trim()) // Removing comments.
    .filter((line) => line.length > 0)

  const prefixes = []
  const suffixes = []
  const exactMatches = new Set()

  // This should be revisited in the future once `path.matchesGlob` is stable
  // and available in node to use, as this current implementation is a bit
  // naive.
  for (const line of configLines) {
    if (line.startsWith('*')) {
      suffixes.push(line.slice(1))
    } else if (line.endsWith('*')) {
      prefixes.push(line.slice(0, -1))
    } else {
      exactMatches.add(line)
    }
  }

  return (s) => {
    if (s.length === 0) {
      return false
    }
    if (exactMatches.has(s)) {
      return false
    }
    if (prefixes.some((prefix) => s.startsWith(prefix))) {
      return false
    }
    if (suffixes.some((suffix) => s.endsWith(suffix))) {
      return false
    }
    return true
  }
}

const chromiumPathFilter = loadChromiumPathFilter(
  path.join(config.braveCoreDir, 'build', 'update_patches_exclusions.cfg'),
)

// Extension of plaster files under `rewrite/`. Keep in sync with
// `PLASTER_EXTENSION` in tools/cr/plaster.py.
const plasterExtension = '.yaml'

// A file that when present indicates that we are doing a lift with brockit.
const versionUpgradeFile = '.version_upgrade'

// Builds a predicate that tells whether a Chromium source path's patch is owned
// by a plaster file. A plaster file at `rewrite/<source>.yaml` is responsible
// for generating the patch for `<source>`, so `update_patches` must not
// regenerate it.
async function loadPlasterPathFilter(rewriteDir) {
  const managedSources = new Set()

  // The second pattern matches plaster files whose name starts with a dot (e.g.
  // `chrome/updater/mac/.install.sh.yaml`); glob's `*` skips leading dots.
  const patterns = [`**/*${plasterExtension}`, `**/.*${plasterExtension}`]
  for await (const file of glob(patterns, { cwd: rewriteDir })) {
    // `<source>.yaml` -> `<source>`, normalized to posix separators so it
    // matches the paths reported by git.
    const source = file
      .split(path.sep)
      .join('/')
      .slice(0, -plasterExtension.length)
    managedSources.add(source)
  }

  return (s) => managedSources.has(s)
}

export default async function RunCommand(filePaths, options) {
  config.update(options)

  const chromiumDir = config.srcDir
  const v8Dir = path.join(config.srcDir, 'v8')
  const catapultDir = path.join(config.srcDir, 'third_party', 'catapult')
  const devtoolsFrontendDir = path.join(
    config.srcDir,
    'third_party',
    'devtools-frontend',
    'src',
  )
  const searchEngineDataDir = path.join(
    config.srcDir,
    'third_party',
    'search_engines_data',
    'resources',
  )
  const ffmpegDir = path.join(config.srcDir, 'third_party', 'ffmpeg')
  const depotToolsDir = path.join(config.srcDir, 'third_party', 'depot_tools')
  const patchDir = path.join(config.braveCoreDir, 'patches')
  const v8PatchDir = path.join(patchDir, 'v8')
  const catapultPatchDir = path.join(patchDir, 'third_party', 'catapult')
  const devtoolsFrontendPatchDir = path.join(
    patchDir,
    'third_party',
    'devtools-frontend',
    'src',
  )
  const searchEngineDataPatchDir = path.join(
    patchDir,
    'third_party',
    'search_engines_data',
    'resources',
  )
  const ffmpegPatchDir = path.join(patchDir, 'third_party', 'ffmpeg')
  const depotToolsPatchDir = path.join(patchDir, 'third_party', 'depot_tools')

  // Plaster only applies to sources in Chromium's `src` repo, so the filter is
  // passed to the chromium update only.
  //
  // The filter is skipped when a brockit lift is in progress, or when
  // `--no-plaster-check` is passed, as in both cases we want update_patches to
  // regenerate plaster-managed patches like any other patch.
  const duringBrockitLift = fs.existsSync(
    path.join(config.braveCoreDir, versionUpgradeFile),
  )
  const noPlasterCheckFlag = options.plasterCheck === false

  // Warn when the check is disabled implicitly by a detected brockit lift, as
  // opposed to the user explicitly asking for it via `--no-plaster-check`.
  if (duringBrockitLift && !noPlasterCheckFlag) {
    console.warn(
      `Warning: a brockit lift is in progress (${versionUpgradeFile} present). `
        + 'Enabling `--no-plaster-check` to prevent plaster checks.',
    )
  }

  const skipPlasterCheck = duringBrockitLift || noPlasterCheckFlag
  const plasterPathFilter = skipPlasterCheck
    ? undefined
    : await loadPlasterPathFilter(path.join(config.braveCoreDir, 'rewrite'))

  Promise.all([
    // chromium
    updatePatches(
      chromiumDir,
      patchDir,
      filePaths,
      chromiumPathFilter,
      [],
      plasterPathFilter,
    ),
    // v8
    updatePatches(v8Dir, v8PatchDir, filePaths),
    // third_party/catapult
    updatePatches(catapultDir, catapultPatchDir, filePaths),
    // third_party/devtools-frontend/src
    updatePatches(devtoolsFrontendDir, devtoolsFrontendPatchDir, filePaths),
    // third_party/search_engines_data
    updatePatches(searchEngineDataDir, searchEngineDataPatchDir, filePaths),
    // third_party/ffmpeg
    updatePatches(ffmpegDir, ffmpegPatchDir, filePaths),
    // third_party/depot_tools
    updatePatches(depotToolsDir, depotToolsPatchDir, filePaths),
  ])
    .then((results) => {
      const outdatedPlasterPaths = results.flat().filter(Boolean)
      if (outdatedPlasterPaths.length) {
        console.error('\nPlaster patches that could not be updated:')
        for (const source of outdatedPlasterPaths) {
          console.error(
            `  - ${source} (managed by rewrite/${source}${plasterExtension})`,
          )
        }
        console.error("\nRun 'tools/cr/plaster.py apply' to regenerate them.")
        process.exitCode = 1
      }
      console.log('Done.')
    })
    .catch((err) => {
      console.error('Error updating patch files:')
      console.error(err)
      process.exitCode = 1
    })
}
