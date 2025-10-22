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
    const env = { ...process.env }
    // Setting the name and email for git is necessary in environments (such as
    // CI) where the values are not set. In that case, Git would error out.
    env.GIT_AUTHOR_NAME = env.GIT_COMMITTER_NAME = 'Brave build'
    env.GIT_AUTHOR_EMAIL = env.GIT_COMMITTER_EMAIL = 'brave@brave.com'
    // Fix the remaining attributes of the commit in order to make its hash
    // deterministic. This prevents `update_patches` from constantly updating
    // .patch files just because the hash has changed.
    env.GIT_AUTHOR_DATE = env.GIT_COMMITTER_DATE = '1970-01-01T00:00:00Z'
    const options = { skipLogging: true, logError: true, env: env }
    const result = runGit(repoPath, args, true, options)
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
