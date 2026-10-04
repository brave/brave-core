// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Fetching and checking out a Chromium ref in `src/` directly, rather than
// having gclient do it with `--revision`. See
// https://github.com/brave/brave-browser/issues/44921.

import config from './config.ts'
import * as Log from './log.ts'
import util from './util.js'

// This is a lean fetch command as chromium/src is really large, and certain
// types of fetch can easily traverse the whole history, resulting in a stall.
const chromiumFetchCmd = [
  '-c',
  'advice.fetchShowForcedUpdates=false',
  'fetch',
  '--no-show-forced-updates',
  '--no-tags',
]

// Checks that a given ref exists in the Chromium repository.
function chromiumRefExists(ref: string): boolean {
  return (
    util.runGit(config.srcDir, ['rev-parse', '--verify', '--quiet', ref], true)
    !== ''
  )
}

// Path of the shared chromium/src mirror (only for git-cache).
function gitCacheMirrorDir(url: string): string {
  const result = util.run(
    'git',
    ['cache', 'exists', '--quiet', '--cache-dir', config.gitCachePath, url],
    util.mergeWithDefault({
      stdio: 'pipe',
      encoding: 'utf8',
      continueOnFail: true,
    }),
  )
  return result.status === 0 ? result.stdout.toString().trim() : ''
}

// Fetches ref into src/ through git-cache
function fetchChromiumRef(ref: string): void {
  let remote = 'origin'

  if (config.gitCachePath) {
    util.run(
      'git',
      [
        'cache',
        'populate',
        '--cache-dir',
        config.gitCachePath,
        // Only the requested ref is wanted, not all of Chromium's tags.
        '--no-fetch-tags',
        '--ref',
        ref,
        // Otherwise the mirror keeps a refspec for every ref ever populated.
        '--reset-fetch-config',
        config.chromiumRepo,
      ],
      util.mergeWithDefault({ cwd: config.rootDir }),
    )
    // This should always have a valid value, as we just populated the cache.
    remote = gitCacheMirrorDir(config.chromiumRepo) || remote
  }

  util.run(
    'git',
    [...chromiumFetchCmd, remote, `${ref}:${ref}`],
    util.mergeWithDefault({ cwd: config.srcDir }),
  )
}

// Tries to do a lean checkout of the given Chromium ref.
export function checkoutChromiumRef(ref: string): boolean {
  if (!ref.startsWith('refs/')) {
    // We do not attempt a lean checkout for random refs.
    Log.warn(
      `${ref} is not a fully-qualified ref, letting gclient check it out.`,
    )
    return false
  }

  // We assume that chromium tags never move, which is the case for the project.
  if (!ref.startsWith('refs/tags/') || !chromiumRefExists(ref)) {
    fetchChromiumRef(ref)
  }

  util.runGit(config.srcDir, ['reset', '--hard', ref])
  return true
}
