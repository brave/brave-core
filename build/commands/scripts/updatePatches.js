// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import { glob } from 'node:fs/promises'
import path from 'node:path'
import config from '../lib/config.ts'
import {
  getPatchedRepositories,
  joinSourcePath,
  splitSourcePath,
} from '../lib/repositories.ts'
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

// Builds, per repository, a predicate that tells whether one of its source
// paths has its patch managed by a plaster file. In such case
// `update_patches` by-default skips the given patch.
async function loadPlasterPathFilters(repositories, rewriteDir) {
  // Keyed by label, the name a repository is identified by, so nothing here
  // has to know how its path is spelled.
  /** @type {Map<string, Set<string>>} */
  const managedSources = new Map(
    repositories.map((repo) => [repo.label, new Set()]),
  )
  const sourcesFor = (repo) => managedSources.get(repo.label) ?? new Set()

  // The second pattern matches plaster files whose name starts with a dot (e.g.
  // a plaster for a dotfile like `.rustfmt.toml.yaml`); glob's `*` skips
  // leading dots.
  const patterns = [`**/*${plasterExtension}`, `**/.*${plasterExtension}`]
  for await (const file of glob(patterns, { cwd: rewriteDir })) {
    // `<source>.yaml` -> `<source>`, normalized to posix separators so it
    // matches the paths reported by git.
    const source = file
      .split(path.sep)
      .join('/')
      .slice(0, -plasterExtension.length)
    // The path is relative to `src/`, so its leading directories name the
    // repository holding the source, exactly as plaster resolves it.
    const split = splitSourcePath(repositories, source)
    sourcesFor(split.repository).add(split.relativePath)
  }

  return (repo) => {
    const sources = sourcesFor(repo)
    return (s) => sources.has(s)
  }
}

export default async function RunCommand(filePaths, options) {
  config.update(options)

  const repositories = getPatchedRepositories()

  // Plaster is passed a filter per repository it supports, since a plaster
  // file's patch has to be left to plaster wherever that patch lives.
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
  const plasterPathFilterFor = skipPlasterCheck
    ? () => undefined
    : await loadPlasterPathFilters(
        repositories,
        path.join(config.braveCoreDir, 'rewrite'),
      )

  // Every repository is updated the same way, from the directories the
  // repositories module resolves for it. Only chromium carries the exclusions
  // filter, the other repositories having no paths to exclude.
  Promise.all(
    repositories.map((repo) =>
      updatePatches(
        repo.path,
        repo.patchDir,
        filePaths,
        repo.isChromium ? chromiumPathFilter : undefined,
        [],
        plasterPathFilterFor(repo),
      ).then((outdated) =>
        // Reported paths are repository-relative, so they are named the way
        // the checkout sees them, which is how the plaster file that owns
        // each one is found under `rewrite/`.
        (outdated ?? []).map((source) => joinSourcePath(repo, source)),
      ),
    ),
  )
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
