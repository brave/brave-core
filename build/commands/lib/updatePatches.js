// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import fs from 'fs-extra'
import rootDir from './rootDir.cjs'
import util from './util.js'

const desiredReplacementSeparator = '-'
const patchExtension = '.patch'

// A shared git attributes file used by plaster to make the patch output
// reproducible across machines.
const PLASTER_GITATTRIBUTES_PATH = path.join(
  rootDir,
  'src',
  'brave',
  'tools',
  'cr',
  'plaster_gitattributes',
)

/**
 * Gets a list of modified files in a git repo
 * @param {string} gitRepoPath The repository to get modified files from
 * @param {(file: string) => boolean} [filter] Filter function for file paths to include or exclude (all included by default)
 * @param {string[]} [onlyFiles] If not empty, only modified paths for these files will be considered.
 * @returns {Promise<{paths: string[], binaryPaths: Set<string>}>}
 */
async function getModifiedPaths(gitRepoPath, filter, onlyFiles) {
  const onlyFilesSet = new Set(onlyFiles)
  const modifiedDiffArgs = [
    'diff',
    '--ignore-submodules',
    '--diff-filter=M',
    '--numstat',
    '--ignore-space-at-eol',
  ]
  const cmdOutput = await util.runAsync('git', modifiedDiffArgs, {
    cwd: gitRepoPath,
    verbose: false,
  })
  // `--numstat` has two formats: "-\t-\t<path>" for a binary file, and
  // "<added>\t<removed>\t<path>" otherwise. We store the binary paths in its
  // own set, so we skip them for output reproducibility.
  const binaryPaths = new Set()
  const paths = cmdOutput
    .split('\n')
    .filter((s) => s)
    .map((line) => {
      const [added, removed, filePath] = line.split('\t')
      if (added === '-' && removed === '-') {
        binaryPaths.add(filePath)
      }
      return filePath
    })
    .filter((s) => (onlyFilesSet.size ? onlyFilesSet.has(s) : true))
    .filter(filter ?? (() => true))
  return { paths, binaryPaths }
}

/**
 * Generates per-file patch files for the modified paths.
 *
 * Write the updated version of each patch file to disk. This is very
 * straightforward for most files, but plaster-managed get skipped if filters
 * are provided.
 *
 * @param {string[]} modifiedPaths Repo-relative paths of modified files
 * @param {string} gitRepoPath The repository to diff against
 * @param {string} patchDirPath Directory to write .patch files to
 * @param {(file: string) => boolean} [plasterPathFilter] Returns true if a repo
 *   path's patch is owned by a plaster file
 * @param {Set<string>} [binaryPaths] Repo-relative paths, that are binary
 * files, which we do not want to use the plaster attributes for
 * reproducibility.
 * @returns {Promise<{patchFilenames: string[], outdatedPlasterPaths: string[]}>}
 *   `outdatedPlasterPaths` lists the repo-relative paths of plaster-managed
 *   sources whose patch is different.
 */
async function writePatchFiles(
  modifiedPaths,
  gitRepoPath,
  patchDirPath,
  plasterPathFilter,
  binaryPaths = new Set(),
) {
  // replacing forward slashes and adding the patch extension to get nice filenames
  // since git on Windows doesn't use backslashes, this is sufficient
  const patchFilenames = modifiedPaths.map(
    (s) => s.replace(/\//g, desiredReplacementSeparator) + patchExtension,
  )

  // When splitting one large diff into a per-file diff, there are a few ways
  // you can go about it. Because different files can have the same name
  // (by being located in different directories), you need to avoid collisions.
  // Mirroring the directory structure seems undesirable.
  // Prefixing with numbers works but is O(n) volatile for O(1) additions
  // We choose here to flatten the directory structure by replacing separators
  // In practice this will avoid collisions. Should a pathological case ever
  // appear, you can quickly patch this by changing the separator, even
  // to something longer

  if (modifiedPaths.length) {
    await fs.ensureDir(patchDirPath)
  }

  /** @type {string[]} */
  const outdatedPlasterPaths = []
  const writeTotal = modifiedPaths.filter(
    (old) => !(plasterPathFilter?.(old) ?? false),
  ).length

  let writeOpsDoneCount = 0
  let writePatchOps = modifiedPaths.map(async (old) => {
    const patchFilename =
      old.replace(/\//g, desiredReplacementSeparator) + patchExtension
    const patchFilePath = path.join(patchDirPath, patchFilename)
    const isPlasterManaged = plasterPathFilter?.(old) ?? false

    // Pinning the diff algorithm so we get the same output across machines.
    const gitConfigArgs = ['-c', 'diff.algorithm=histogram']
    if (!binaryPaths.has(old)) {
      gitConfigArgs.push(
        '-c',
        `core.attributesFile=${PLASTER_GITATTRIBUTES_PATH}`,
      )
    }
    const singleDiffArgs = [
      ...gitConfigArgs,
      'diff',
      '--src-prefix=a/',
      '--dst-prefix=b/',
      '--default-prefix',
      '--full-index',
      '--ignore-space-at-eol',
      old,
    ]
    const patchContents = await util.runAsync('git', singleDiffArgs, {
      cwd: gitRepoPath,
      verbose: false,
      env: { ...process.env, GIT_ATTR_NOSYSTEM: '1' },
    })

    if (isPlasterManaged) {
      // Plaster-owned patches are supposed to be generated by plaster. If we've
      // reached this block, it means we do not want to generate a patch that
      // should be handled by plaster.
      let existingContents = null
      try {
        existingContents = await fs.readFile(patchFilePath, 'utf-8')
      } catch (err) {
        if (err.code !== 'ENOENT') {
          throw err
        }
      }
      if (existingContents !== patchContents) {
        outdatedPlasterPaths.push(old)
      }
      return
    }

    await fs.writeFile(patchFilePath, patchContents)

    writeOpsDoneCount++
    const logRepoName = path.basename(gitRepoPath)
    console.log(
      `updatePatches [${logRepoName}] wrote ${writeOpsDoneCount} / ${writeTotal}: ${patchFilename}`,
    )
  })

  await Promise.all(writePatchOps)
  return { patchFilenames, outdatedPlasterPaths }
}

const readDirPromise = (pathName) =>
  new Promise((resolve, reject) =>
    fs.readdir(pathName, (err, fileList) => {
      if (err) {
        return reject(err)
      }
      return resolve(fileList)
    }),
  )

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
 * @param {(file: string) => boolean} [plasterPathFilter] Returns true for repo
 *   paths whose patch is owned by a plaster file. Those patches are not written
 *   here; their path is returned for each one that is different.
 * @returns {Promise<string[]>} Repo-relative paths of plaster-managed patches
 *   that are different and must be regenerated with plaster.
 */
async function updatePatches(
  gitRepoPath,
  patchDirPath,
  onlyFiles,
  repoPathFilter,
  keepPatchFilenames = [],
  plasterPathFilter,
) {
  const { paths: modifiedPaths, binaryPaths } = await getModifiedPaths(
    gitRepoPath,
    repoPathFilter,
    onlyFiles,
  )
  const { patchFilenames, outdatedPlasterPaths } = await writePatchFiles(
    modifiedPaths,
    gitRepoPath,
    patchDirPath,
    plasterPathFilter,
    binaryPaths,
  )
  // We only remove stale patch files if we're updating everything.
  if (onlyFiles && onlyFiles.length === 0) {
    await removeStalePatchFiles(
      patchFilenames,
      patchDirPath,
      keepPatchFilenames,
    )
  }
  return outdatedPlasterPaths
}

export default updatePatches
