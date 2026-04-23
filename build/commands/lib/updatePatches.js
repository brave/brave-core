// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import fs from 'fs-extra'
import util from './util.js'

const desiredReplacementSeparator = '-'
const patchExtension = '.patch'

const readDirPromise = (pathName) =>
  new Promise((resolve, reject) =>
    fs.readdir(pathName, (err, fileList) => {
      if (err) {
        return reject(err)
      }
      return resolve(fileList)
    }),
  )

/**
 * Reads the patch directory and returns the set of repo-relative paths covered
 * by existing patches (derived from the patch filenames).
 */
async function getExistingPatchPaths(patchDirPath) {
  try {
    return new Set(
      (await readDirPromise(patchDirPath))
        .filter((s) => s.endsWith(patchExtension))
        .map((s) =>
          s
            .slice(0, -patchExtension.length)
            .replace(new RegExp(desiredReplacementSeparator, 'g'), '/'),
        ),
    )
  } catch (err) {
    if (err.code === 'ENOENT') {
      return new Set()
    }
    throw err
  }
}

/**
 * Gets paths in `gitRepoPath` that should be refreshed as patches.
 *
 * Returns both modified files (tracked) and untracked files that correspond to
 * an existing new-file patch. `git apply` creates new files without staging
 * them, so without this second set, round-tripping a new-file patch through
 * apply_patches / update_patches would drop the patch.
 *
 * @returns {Promise<{modified: string[], untracked: string[]}>}
 */
async function getModifiedPaths(gitRepoPath, patchDirPath, filter, onlyFiles) {
  const onlyFilesSet = new Set(onlyFiles)
  const pickFilter = (s) =>
    (onlyFilesSet.size ? onlyFilesSet.has(s) : true)
    && (filter ?? (() => true))(s)

  const modifiedOutput = await util.runAsync(
    'git',
    [
      'diff',
      '--ignore-submodules',
      '--diff-filter=M',
      '--name-only',
      '--ignore-space-at-eol',
    ],
    { cwd: gitRepoPath, verbose: false },
  )
  const modified = modifiedOutput
    .split('\n')
    .filter((s) => s)
    .filter(pickFilter)

  const untrackedOutput = await util.runAsync(
    'git',
    ['ls-files', '--others', '--exclude-standard'],
    { cwd: gitRepoPath, verbose: false },
  )
  const untrackedPaths = untrackedOutput.split('\n').filter((s) => s)
  let untracked = []
  if (untrackedPaths.length) {
    const existingPatchPaths = await getExistingPatchPaths(patchDirPath)
    untracked = untrackedPaths
      .filter((p) => existingPatchPaths.has(p))
      .filter(pickFilter)
  }

  return { modified, untracked }
}

async function writePatchFile(repoPath, patchDirPath, patchFilename, diffArgs) {
  // `git diff --no-index` (used for untracked files) exits with code 1 when a
  // diff exists, which util.runAsync surfaces as an error. Pass
  // continueOnFail so we can read the diff from the error's stdout instead of
  // killing the process.
  let patchContents
  try {
    patchContents = await util.runAsync('git', diffArgs, {
      cwd: repoPath,
      verbose: false,
      continueOnFail: true,
    })
  } catch (err) {
    if (err && typeof err.stdout === 'string' && err.stdout.length > 0) {
      patchContents = err.stdout
    } else {
      throw err
    }
  }
  await fs.writeFile(path.join(patchDirPath, patchFilename), patchContents)
}

function toPatchFilename(repoRelativePath) {
  return (
    repoRelativePath.replace(/\//g, desiredReplacementSeparator)
    + patchExtension
  )
}

async function writePatchFiles(
  modifiedPaths,
  untrackedPaths,
  gitRepoPath,
  patchDirPath,
) {
  // When splitting one large diff into a per-file diff, there are a few ways
  // you can go about it. Because different files can have the same name
  // (by being located in different directories), you need to avoid collisions.
  // Mirroring the directory structure seems undesirable.
  // Prefixing with numbers works but is O(n) volatile for O(1) additions
  // We choose here to flatten the directory structure by replacing separators
  // In practice this will avoid collisions. Should a pathological case ever
  // appear, you can quickly patch this by changing the separator, even
  // to something longer.
  const total = modifiedPaths.length + untrackedPaths.length
  if (total) {
    await fs.ensureDir(patchDirPath)
  }

  const logRepoName = path.basename(gitRepoPath)
  let writeOpsDoneCount = 0
  const logDone = (patchFilename) => {
    writeOpsDoneCount++
    console.log(
      `updatePatches [${logRepoName}] wrote ${writeOpsDoneCount} / ${total}: ${patchFilename}`,
    )
  }

  const modifiedFilenames = modifiedPaths.map(toPatchFilename)
  const modifiedOps = modifiedPaths.map(async (repoPath, i) => {
    const patchFilename = modifiedFilenames[i]
    await writePatchFile(gitRepoPath, patchDirPath, patchFilename, [
      'diff',
      '--src-prefix=a/',
      '--dst-prefix=b/',
      '--default-prefix',
      '--full-index',
      repoPath,
    ])
    logDone(patchFilename)
  })

  // Untracked files: use `git diff --no-index` so the output is a synthetic
  // new-file diff that matches the format produced by `git apply`.
  const untrackedFilenames = untrackedPaths.map(toPatchFilename)
  const untrackedOps = untrackedPaths.map(async (repoPath, i) => {
    const patchFilename = untrackedFilenames[i]
    await writePatchFile(gitRepoPath, patchDirPath, patchFilename, [
      'diff',
      '--src-prefix=a/',
      '--dst-prefix=b/',
      '--default-prefix',
      '--full-index',
      '--no-index',
      '/dev/null',
      repoPath,
    ])
    logDone(patchFilename)
  })

  await Promise.all([...modifiedOps, ...untrackedOps])
  return modifiedFilenames.concat(untrackedFilenames)
}

async function removeStalePatchFiles(
  patchFilenames,
  patchDirPath,
  keepPatchFilenames,
) {
  // grab every existing patch file in the dir (at this point, patchfiles for now-unmodified files live on)
  let existingPathFilenames
  try {
    existingPathFilenames = ((await readDirPromise(patchDirPath)) || []).filter(
      (s) => s.endsWith('.patch'),
    )
  } catch (err) {
    if (err.code === 'ENOENT') {
      console.log(`Path at ${patchDirPath} does not exist.`)
      return
    }
    throw err
  }

  // Subtract to find which patchfiles no longer have diffs, yet still exist
  const validFilenames = patchFilenames.concat(keepPatchFilenames)
  const toRemoveFilenames = existingPathFilenames.filter(
    (x) => !validFilenames.includes(x),
  )

  // regular rm patchfiles whose target is no longer modified
  let removedProgress = 0
  for (const filename of toRemoveFilenames) {
    const fullPath = path.join(patchDirPath, filename)
    fs.removeSync(fullPath)
    removedProgress++
    console.log(
      `updatePatches *REMOVED* ${removedProgress}/${toRemoveFilenames.length}: ${filename}`,
    )
  }
}

/**
 * Detects modifications to a git repo and creates or updates patch files for each modified file.
 * Removes patch files which are no longer relevant.
 *
 * @param {string} gitRepoPath Repo path to look for changes
 * @param {string} patchDirPath Directory to keep .patch files in
 * @param {string[]} [onlyFiles] If specified, only patches for these files will be updated.
 * @param {(file: string) => boolean} [repoPathFilter] Filter function for repo file paths to include or exclude (all included by default)
 * @param {string[]} [keepPatchFilenames=[]] Patch filenames to never delete
 */
async function updatePatches(
  gitRepoPath,
  patchDirPath,
  onlyFiles,
  repoPathFilter,
  keepPatchFilenames = [],
) {
  const { modified, untracked } = await getModifiedPaths(
    gitRepoPath,
    patchDirPath,
    repoPathFilter,
    onlyFiles,
  )
  const patchFilenames = await writePatchFiles(
    modified,
    untracked,
    gitRepoPath,
    patchDirPath,
  )
  // We only remove stale patch files if we're updating everything.
  if (onlyFiles && onlyFiles.length === 0) {
    await removeStalePatchFiles(
      patchFilenames,
      patchDirPath,
      keepPatchFilenames,
    )
  }
}

export default updatePatches
