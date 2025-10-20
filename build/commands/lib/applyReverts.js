// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const { runGit } = require('./util')

/**
 * Reverts a list of git commits, in the given order. This function is
 * idempotent - if a commit has already been reverted, it will be skipped.
 */
function applyReverts(repoPath, commits, dryRun = false) {
  function git(...args) {
    const result = runGit(repoPath, args, true, { skipLogging: true })
    if (result === null)
      throw new Error(`Git command failed: git ${args.join(' ')}`)
    return result
  }

  const alreadyReverted = getAppliedReverts(git, commits.length)

  for (const commit of commits) {
    const fullHash = git('rev-parse', commit)

    if (alreadyReverted.includes(fullHash))
      continue

    if (dryRun) {
      return true
    } else {
      git('revert', '--no-edit', fullHash)
      git('commit', '--amend', '-m', `Revert ${fullHash} for Brave`)
    }
  }
}

function getAppliedReverts(git, numToCheck) {
  const subjects = git('log', '-n', numToCheck, '--format=%s')
  const result = []
  const lines = subjects.split('\n')
  for (const subject of lines) {
    const match = subject.match(/^Revert ([0-9a-f]+) for Brave$/)
    if (match !== null)
      result.push(match[1])
  }
  return result
}

module.exports = applyReverts
