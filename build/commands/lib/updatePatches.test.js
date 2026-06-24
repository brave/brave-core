// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import os from 'node:os'
import fs from 'fs-extra'
import updatePatches from './updatePatches.js'
import util from './util.js'

const dirPrefixTmp = 'brave-browser-test-update-patches-'

const sourceName = 'source.cc'
const initialContent = 'line one\nline two\n'
const modifiedContent = 'line one\nline two modified\n'

function runGit(repoPath, args) {
  return util.runAsync('git', args, { cwd: repoPath, verbose: false })
}

// Writes the patch for `repoRelativePath` the same way plaster and
// update_patches do, so a test can set up an "up-to-date" plaster-managed patch
// on disk for update_patches to compare against.
async function writePatchAsPlaster(repoPath, patchDirPath, repoRelativePath) {
  const patchContents = await runGit(repoPath, [
    'diff',
    '--src-prefix=a/',
    '--dst-prefix=b/',
    '--default-prefix',
    '--full-index',
    '--ignore-space-at-eol',
    repoRelativePath,
  ])
  const patchFilename = repoRelativePath.replace(/\//g, '-') + '.patch'
  await fs.writeFile(path.join(patchDirPath, patchFilename), patchContents)
  return patchFilename
}

describe('updatePatches plaster detection', function () {
  let repoPath, patchPath, sourcePath

  beforeEach(async function () {
    patchPath = await fs.mkdtemp(
      path.join(os.tmpdir(), dirPrefixTmp + 'patches-'),
    )
    repoPath = await fs.mkdtemp(path.join(os.tmpdir(), dirPrefixTmp + 'repo-'))
    sourcePath = path.join(repoPath, sourceName)

    await runGit(repoPath, ['init'])
    await runGit(repoPath, ['config', 'user.email', 'unittests@local'])
    await runGit(repoPath, ['config', 'user.name', 'Unit Tests'])
    await runGit(repoPath, ['config', 'commit.gpgsign', 'false'])
    await fs.writeFile(sourcePath, initialContent)
    await runGit(repoPath, ['add', '.'])
    await runGit(repoPath, ['commit', '-m', 'initial'])
    // Modify the source so it shows up as a modified path.
    await fs.writeFile(sourcePath, modifiedContent)
  })

  afterEach(async function () {
    await fs.remove(repoPath).catch(() => {})
    await fs.remove(patchPath).catch(() => {})
  })

  test('writes patches normally when no plaster filter is provided', async () => {
    const errors = await updatePatches(repoPath, patchPath, [])
    expect(errors).toEqual([])
    const patchFile = path.join(patchPath, sourceName + '.patch')
    expect(await fs.pathExists(patchFile)).toBe(true)
  })

  test('does not overwrite an up-to-date plaster-managed patch and reports no error', async () => {
    // Pre-generate the patch the way plaster would (it owns this file).
    const patchFilename = await writePatchAsPlaster(
      repoPath,
      patchPath,
      sourceName,
    )
    const patchFile = path.join(patchPath, patchFilename)
    const before = await fs.readFile(patchFile, 'utf-8')

    const errors = await updatePatches(
      repoPath,
      patchPath,
      [],
      undefined,
      [],
      (p) => p === sourceName,
    )

    expect(errors).toEqual([])
    // Patch left untouched.
    expect(await fs.readFile(patchFile, 'utf-8')).toBe(before)
  })

  test('reports an error and does not overwrite when a plaster-managed patch is out of date', async () => {
    // Write a stale/wrong patch on disk to simulate drift.
    const patchFilename = sourceName + '.patch'
    const patchFile = path.join(patchPath, patchFilename)
    const stale = 'this is not the right patch\n'
    await fs.writeFile(patchFile, stale)

    const errors = await updatePatches(
      repoPath,
      patchPath,
      [],
      undefined,
      [],
      (p) => p === sourceName,
    )

    // The lib reports the raw repo-relative path; the caller formats the
    // user-facing message (including the rewrite/ path).
    expect(errors).toEqual([sourceName])
    // Plaster owns the patch: update_patches must not overwrite it.
    expect(await fs.readFile(patchFile, 'utf-8')).toBe(stale)
  })

  test('reports an error when a plaster-managed patch is missing', async () => {
    const errors = await updatePatches(
      repoPath,
      patchPath,
      [],
      undefined,
      [],
      (p) => p === sourceName,
    )

    expect(errors).toEqual([sourceName])
    // No patch file was created for the plaster-managed source.
    expect(
      await fs.pathExists(path.join(patchPath, sourceName + '.patch')),
    ).toBe(false)
  })
})
