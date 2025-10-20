// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs-extra')
const GitPatcher = require('./gitPatcher')
const { TemporaryGitRepository } = require('./testUtils')
const os = require('os')

const file1InitialContent = 'this is a test'
const file1ModifiedContent = 'this is modified'
const file1Name = 'file1'
const writeReadFileOptions = { encoding: 'utf8' }

async function getPatch(repo, modifiedFilePath) {
  const singleDiffArgs = [
    'diff',
    '--src-prefix=a/',
    '--dst-prefix=b/',
    '--full-index',
    modifiedFilePath,
  ]
  return repo.runGit(singleDiffArgs)
}

describe('Apply Patches', function () {
  let gitPatcher, repo, patchPath, testFile1Path, testFile1PatchPath

  beforeEach(async function () {
    // Setup test Git repo and test Patch directory
    patchPath = await fs.mkdtemp(
      path.join(os.tmpdir(), 'brave-browser-test-git-apply-patches-'),
    )
    repo = await TemporaryGitRepository.create('brave-browser-test-git-apply-')
    testFile1Path = path.join(repo.path, file1Name)
    await fs.writeFile(testFile1Path, file1InitialContent, writeReadFileOptions)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', '"file1 initial"'])
    // modify content file
    await fs.writeFile(
      testFile1Path,
      file1ModifiedContent,
      writeReadFileOptions,
    )
    // get patch
    const file1PatchContent = await getPatch(repo, file1Name)
    // write patch
    testFile1PatchPath = path.join(patchPath, file1Name + '.patch')
    await fs.writeFile(testFile1PatchPath, file1PatchContent)
    // reset file change
    await repo.runGit(['reset', '--hard', 'HEAD'])
    // sanity test
    const testFile1Content = await fs.readFile(
      testFile1Path,
      writeReadFileOptions,
    )
    try {
      expect(testFile1Content).toBe(file1InitialContent)
    } catch (err) {
      console.error('Setup fail: file was not reset - ' + testFile1Path)
      throw new Error(err)
    }
    gitPatcher = new GitPatcher(patchPath, repo.path, false)
  })

  function validate() {
    if (!repo.path || !patchPath) {
      throw new Error('test setup failed!')
    }
  }

  afterEach(async function () {
    await repo.cleanup()
    try {
      await fs.remove(patchPath)
    } catch (err) {
      console.warn(`Test cleanup: could not remove directory at ${patchPath}`)
    }
  })

  test('applies simple patch to unmodified original', async function () {
    validate()
    const affectedPaths = await gitPatcher.applyPatches()
    // test file contents
    const testFile1Content = await fs.readFile(
      testFile1Path,
      writeReadFileOptions,
    )
    expect(testFile1Content).toBe(file1ModifiedContent)
    // test reporting
    expect(affectedPaths).toHaveLength(1)
    expect(affectedPaths[0]).toHaveProperty('path', file1Name)
    expect(affectedPaths[0]).toHaveProperty('error', undefined)
    expect(affectedPaths[0]).toHaveProperty(
      'reason',
      GitPatcher.patchApplyReasons.NO_PATCH_INFO,
    )
  })

  test('does not apply patch to still-patched', async function () {
    validate()
    const testFile1StatsInitial = await fs.stat(testFile1Path)
    // apply once
    await gitPatcher.applyPatches()
    // get modified file time
    const testFile1StatsModified = await fs.stat(testFile1Path)
    // apply again
    const affectedPaths = await gitPatcher.applyPatches()
    // Test if the function doesn't think it was modified
    expect(affectedPaths).toHaveLength(0)
    // Sanity check the file wasn't actually modified
    const testFile1StatsAgain = await fs.stat(testFile1Path)
    expect(testFile1StatsAgain.mtimeMs).toBe(testFile1StatsModified.mtimeMs)
    // Sanity check the file was modified the first time
    expect(testFile1StatsModified.mtimeMs).toBeGreaterThan(
      testFile1StatsInitial.mtimeMs,
    )
  })

  test('resets target file if patch file is removed', async function () {
    validate()
    // apply once
    await gitPatcher.applyPatches()
    // remove patch file
    await fs.unlink(testFile1PatchPath)
    // apply again
    const status = await gitPatcher.applyPatches()
    expect(status).toHaveLength(1)
    expect(status[0].path).toBe(file1Name)
    expect(status[0].error).toBeUndefined()
    expect(status[0].reason).toBe(GitPatcher.patchApplyReasons.PATCH_REMOVED)
    // check file was reset
    const testFile1Content = await fs.readFile(
      testFile1Path,
      writeReadFileOptions,
    )
    expect(testFile1Content).toBe(file1InitialContent)
  })

  test('handles missing file when resets target file if patch file is removed', async function () {
    validate()
    // apply once
    await gitPatcher.applyPatches()
    // remove patch file
    await fs.unlink(testFile1PatchPath)
    // remove target file
    await fs.unlink(testFile1Path)
    await repo.runGit(['rm', testFile1Path])
    await repo.runGit(['commit', '-a', '-m', '"remove target"'])
    // apply again
    const status = await gitPatcher.applyPatches()
    expect(status).toHaveLength(1)
    expect(status[0]).toHaveProperty('path', file1Name)
    expect(status[0]).toHaveProperty('warning')
    expect(status[0]).toHaveProperty(
      'reason',
      GitPatcher.patchApplyReasons.PATCH_REMOVED,
    )
  })

  test('handles missing file when patch file is still present', async function () {
    validate()
    // apply once
    await gitPatcher.applyPatches()
    // remove target file
    await fs.unlink(testFile1Path)
    await repo.runGit(['rm', testFile1Path])
    await repo.runGit(['commit', '-a', '-m', '"remove target"'])
    // apply again
    const status = await gitPatcher.applyPatches()
    expect(status).toHaveLength(1)
    expect(status[0]).toHaveProperty('patchPath', testFile1PatchPath)
    expect(status[0]).toHaveProperty('error')
    expect(status[0]).toHaveProperty(
      'reason',
      GitPatcher.patchApplyReasons.SRC_REMOVED,
    )
  })

  test('handles bad patch file', async function () {
    validate()
    // Create an invalid patch
    await fs.writeFile(testFile1PatchPath, 'bad patch', writeReadFileOptions)
    const status = await gitPatcher.applyPatches()
    expect(status).toHaveLength(1)
    expect(status[0].patchPath).toBe(testFile1PatchPath)
    expect(status[0].path).toBeUndefined()
    expect(status[0].error).toBeDefined()
  })

  test('handles non-JSON patchinfo file', async function () {
    validate()

    // Make a patchinfo file with non-JSON content
    const patchInfoPath = testFile1PatchPath.replace('.patch', '.patchinfo')
    await fs.writeFile(patchInfoPath, 'non-JSON content', writeReadFileOptions)

    // Apply patches and check that the patch was applied
    const status = await gitPatcher.applyPatches()
    expect(status).toHaveLength(1)
    expect(status[0].path).toBe(file1Name)
    expect(status[0].error).toBeUndefined()
    expect(status[0].reason).toBe(
      GitPatcher.patchApplyReasons.PATCH_INFO_OUTDATED,
    )

    // Verify the file content
    const finalContent = await fs.readFile(testFile1Path, writeReadFileOptions)
    expect(finalContent).toBe(file1ModifiedContent)
  })

  test('handles no patch dir', async function () {
    validate()
    const badPatchPath = path.join(patchPath, 'not-exist')
    const noDirPatcher = new GitPatcher(badPatchPath, repo.path)
    const status = await noDirPatcher.applyPatches()
    expect(status).toHaveLength(0)
  })

  test('handles no repo dir', async function () {
    const badRepoPath = path.join(repo.path, 'not-exist')
    const noRepoPatcher = new GitPatcher(patchPath, badRepoPath)
    await expect(noRepoPatcher.applyPatches()).rejects.toThrowError()
  })
})
