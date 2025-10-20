/* Copyright (c) 2017 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const path = require('path')
const fs = require('fs-extra')
const applyReverts = require('./applyReverts')
const { TemporaryGitRepository } = require('./testUtils')

const file1InitialContent = 'line 1\nline 2\nline 3\n'
const file1ModifiedContent = 'line 1\nmodified line 2\nline 3\n'
const file1FileName = 'test-file.txt'
const writeReadFileOptions = { encoding: 'utf8' }

async function getCommitHash(repo, ref = 'HEAD') {
  const output = await repo.runGit(['rev-parse', '--short', ref])
  return output.trim()
}

describe('applyReverts', function () {
  let repo, testFilePath

  async function writeTestFile(content) {
    await fs.writeFile(testFilePath, content, writeReadFileOptions)
  }

  async function readTestFile() {
    return fs.readFile(testFilePath, writeReadFileOptions)
  }

  function expectApplyRevertsToThrow(hashes) {
    expect(() => applyReverts(repo.path, hashes, false)).toThrow()
  }

  beforeEach(async function () {
    repo = await TemporaryGitRepository.create('brave-browser-test-apply-reverts-')
    testFilePath = path.join(repo.path, file1FileName)

    await writeTestFile(file1InitialContent)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'Initial commit'])
  })

  afterEach(async function () {
    await repo.cleanup()
  })

  test('reverts a single commit', async function () {
    await writeTestFile(file1ModifiedContent)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'Modify file'])

    const commitToRevert = await getCommitHash(repo)

    applyReverts(repo.path, [commitToRevert])

    const fileContent = await readTestFile()
    expect(fileContent).toBe(file1InitialContent)

    const logOutput = await repo.runGit(['log', '--oneline', '-1'])
    expect(logOutput).toMatch(/Revert/)
  })

  test('reverts multiple commits in order', async function () {
    // Create first commit
    await writeTestFile(file1ModifiedContent)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'First change'])
    const firstCommit = await getCommitHash(repo)

    // Create second commit
    await writeTestFile('line 1\nmodified line 2\nmodified line 3\n')
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'Second change'])
    const secondCommit = await getCommitHash(repo)

    // Run in a loop to test idempotency.
    for (let i = 0; i < 2; i++) {
      applyReverts(repo.path, [secondCommit, firstCommit])

      // Verify the file was reverted to initial state
      const fileContent = await readTestFile()
      expect(fileContent).toBe(file1InitialContent)

      // Verify exactly 2 revert commits were created
      const logOutput = await repo.runGit(['log', '--oneline', '-3'])
      const lines = logOutput.trim().split('\n')
      expect(lines).toHaveLength(3)
      expect(lines[0]).toMatch(/Revert/)
      expect(lines[1]).toMatch(/Revert/)
      expect(lines[2]).not.toMatch(/Revert/)
    }
  })

  test('handles empty revisions array', async function () {
    const commitBefore = await getCommitHash(repo)
    applyReverts(repo.path, [])
    const commitAfter = await getCommitHash(repo)
    expect(commitAfter).toBe(commitBefore)

    const fileContent = await readTestFile()
    expect(fileContent).toBe(file1InitialContent)
  })

  test('handles invalid commit hash', async function () {
    const invalidHash = 'abc123def'
    expectApplyRevertsToThrow([invalidHash])
  })

  test('reverts commit that added a new file', async function () {
    const newFilePath = path.join(repo.path, 'new-file.txt')
    await fs.writeFile(newFilePath, 'new file content', writeReadFileOptions)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'Add new file'])

    const commitToRevert = await getCommitHash(repo)

    applyReverts(repo.path, [commitToRevert])

    const fileExists = await fs.pathExists(newFilePath)
    expect(fileExists).toBe(false)
  })

  test('reverts commit that deleted a file', async function () {
    await repo.runGit(['rm', testFilePath])
    await repo.runGit(['commit', '-m', 'Delete file'])

    const commitToRevert = await getCommitHash(repo)

    let fileExists = await fs.pathExists(testFilePath)
    expect(fileExists).toBe(false)

    applyReverts(repo.path, [commitToRevert])

    fileExists = await fs.pathExists(testFilePath)
    expect(fileExists).toBe(true)

    const fileContent = await readTestFile()
    expect(fileContent).toBe(file1InitialContent)
  })

  test('handles revert with merge conflict', async function () {
    // Create a commit to revert
    await writeTestFile(file1ModifiedContent)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'First modification'])
    const commitToRevert = await getCommitHash(repo)

    // Make a conflicting change
    await writeTestFile('line 1\nconflicting line 2\nline 3\n')
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'Conflicting change'])

    // Attempting to revert should fail due to conflict
    await expectApplyRevertsToThrow([commitToRevert])
  })

  test('reverts a reverted and re-introduced commit', async function () {
    await writeTestFile(file1ModifiedContent)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'First modification'])

    await repo.runGit(['revert', '--no-edit', 'HEAD'])

    await writeTestFile(file1ModifiedContent)
    await repo.runGit(['add', '.'])
    await repo.runGit(['commit', '-m', 'Second modification'])

    const commitToRevert = await getCommitHash(repo)
    applyReverts(repo.path, [commitToRevert])

    const fileContent = await readTestFile()
    expect(fileContent).toBe(file1InitialContent)
  })
})
