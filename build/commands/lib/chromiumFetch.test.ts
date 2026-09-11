// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// These run against real git repositories in a temp directory, including a real
// git cache, so they assert where the checkout ends up rather than which
// commands got it there. Nothing here touches the network.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { spawnSync } from 'node:child_process'
import { checkoutChromiumRef } from './chromiumFetch.ts'
import config from './config.ts'
import * as Log from './log.ts'
import util from './util.js'

jest.mock('./log.ts', () => ({
  warn: jest.fn(),
  command: jest.fn(),
}))

// Release tags Chromium-style: on commits that no branch points at, so they are
// unreachable from `refs/heads/*` and a plain mirror fetch does not get them.
const releaseTag = '1.2.3.4'
const laterReleaseTag = '5.6.7.8'
const releaseRef = `refs/tags/${releaseTag}`
const branchHeadsRef = 'refs/branch-heads/8010'

// Config fields the tests point at the temp checkout, restored afterwards
// because `config` is a process-wide singleton.
type OverriddenConfig = Pick<
  typeof config,
  'gitCachePath' | 'chromiumRepo' | 'srcDir' | 'rootDir'
>

function git(cwd: string, ...args: string[]): string {
  const result = spawnSync('git', args, { cwd, encoding: 'utf8' })
  if (result.status !== 0) {
    throw new Error(`git ${args.join(' ')} failed:\n${result.stderr}`)
  }
  return result.stdout.trim()
}

describe('checkoutChromiumRef', () => {
  let tmpDir: string
  let upstream: string
  let srcDir: string
  let savedConfig: OverriddenConfig

  // Commit an empty change onto whatever `upstream` has checked out.
  function commit(message: string): string {
    git(
      upstream,
      '-c',
      'user.email=tests@local',
      '-c',
      'user.name=Unit Tests',
      'commit',
      '-q',
      '--allow-empty',
      '-m',
      message,
    )
    return git(upstream, 'rev-parse', 'HEAD')
  }

  // Tag a new commit on a throwaway branch and delete the branch, leaving the
  // tag unreachable from `refs/heads/*` as Chromium's release tags are.
  //
  // The branch also carries a decoy tag, so populating the mirror for the
  // release tag alone is distinguishable from dragging in every tag around it.
  function commitReleaseTag(tag: string): string {
    git(upstream, 'checkout', '-q', '-b', `rel-${tag}`, 'main')
    commit(`branch point for ${tag}`)
    git(upstream, 'tag', `decoy-on-${tag}`)
    const sha = commit(`release ${tag}`)
    git(upstream, 'tag', tag)
    git(upstream, 'checkout', '-q', 'main')
    git(upstream, 'branch', '-q', '-D', `rel-${tag}`)
    return sha
  }

  const headOf = (repo: string) => git(repo, 'rev-parse', 'HEAD')
  const tagsIn = (repo: string) => git(repo, 'tag').split('\n').filter(Boolean)

  // Where `git cache` put the mirror for the fake upstream.
  function mirrorDir(): string {
    return spawnSync(
      'git',
      [
        'cache',
        'exists',
        '--quiet',
        '--cache-dir',
        config.gitCachePath!,
        upstream,
      ],
      {
        encoding: 'utf8',
        env: {
          ...process.env,
          PATH: `${config.depotToolsDir}${path.delimiter}${process.env.PATH}`,
        },
      },
    ).stdout.trim()
  }

  beforeEach(() => {
    savedConfig = {
      gitCachePath: config.gitCachePath,
      chromiumRepo: config.chromiumRepo,
      srcDir: config.srcDir,
      rootDir: config.rootDir,
    }

    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'brave-chromium-fetch-'))
    upstream = path.join(tmpDir, 'upstream')
    srcDir = path.join(tmpDir, 'src')

    fs.mkdirSync(upstream)
    git(upstream, 'init', '-q', '-b', 'main')
    commit('first')
    // Decoy tags on trunk history. A fetch that follows tags would drag these
    // in alongside the release tag; a lean one leaves them behind.
    git(upstream, 'tag', 'decoy-1')
    commit('second')
    git(upstream, 'tag', 'decoy-2')

    // A checkout on trunk that has none of the tags yet, like a `src/` sitting
    // on the previous Chromium pin.
    git(tmpDir, 'clone', '-q', '--no-tags', upstream, srcDir)

    config.chromiumRepo = upstream
    config.gitCachePath = path.join(tmpDir, 'cache')
    config.srcDir = srcDir
    config.rootDir = tmpDir

    // Keep the real environment, so `git cache` really is resolved through
    // depot_tools, but pipe the output instead of inheriting the test's.
    const mergeWithDefault = util.mergeWithDefault
    jest.spyOn(util, 'mergeWithDefault').mockImplementation((options) => ({
      ...mergeWithDefault(options),
      stdio: 'pipe',
    }))

    // `util.run` exits the process on failure; surface that as a test failure
    // instead of killing the worker.
    jest.spyOn(process, 'exit').mockImplementation(((code: number) => {
      throw new Error(`process.exit(${code}) called`)
    }) as never)
  })

  afterEach(() => {
    jest.restoreAllMocks()
    Object.assign(config, savedConfig)
    fs.rmSync(tmpDir, { recursive: true, force: true })
  })

  it('checks out a release tag through the git cache', () => {
    const releaseSha = commitReleaseTag(releaseTag)

    expect(checkoutChromiumRef(releaseRef)).toBe(true)

    expect(headOf(srcDir)).toBe(releaseSha)
    // Only the pinned tag came across: Chromium has tens of thousands, and
    // following them all is what makes this slow. That holds for the shared
    // mirror as much as for the checkout.
    expect(tagsIn(srcDir)).toEqual([releaseTag])
    expect(tagsIn(mirrorDir())).toEqual([releaseTag])
  })

  it('checks out a release tag with no git cache configured', () => {
    config.gitCachePath = undefined
    const releaseSha = commitReleaseTag(releaseTag)

    expect(checkoutChromiumRef(releaseRef)).toBe(true)

    expect(headOf(srcDir)).toBe(releaseSha)
    expect(tagsIn(srcDir)).toEqual([releaseTag])
  })

  it('needs no access to the remote when the tag is already local', () => {
    const releaseSha = commitReleaseTag(releaseTag)
    git(srcDir, 'fetch', '--no-tags', upstream, `${releaseRef}:${releaseRef}`)
    const trunkSha = headOf(srcDir)

    // Nothing reachable to fetch from: any attempt to contact the remote or
    // populate the mirror fails the test.
    const gone = path.join(tmpDir, 'gone')
    config.chromiumRepo = gone
    git(srcDir, 'remote', 'set-url', 'origin', gone)

    expect(checkoutChromiumRef(releaseRef)).toBe(true)

    expect(headOf(srcDir)).toBe(releaseSha)
    expect(releaseSha).not.toBe(trunkSha)
  })

  it('follows a branch-heads ref that moved upstream', () => {
    const firstSha = commitReleaseTag(releaseTag)
    git(upstream, 'update-ref', branchHeadsRef, firstSha)

    expect(checkoutChromiumRef(branchHeadsRef)).toBe(true)
    expect(headOf(srcDir)).toBe(firstSha)

    // Unlike a tag, a branch-heads ref moves, so having it locally already is
    // not good enough.
    git(upstream, 'checkout', '-q', '-b', 'rel-moved', firstSha)
    const movedSha = commit('follow-up release')
    git(upstream, 'update-ref', branchHeadsRef, movedSha)
    git(upstream, 'checkout', '-q', 'main')
    git(upstream, 'branch', '-q', '-D', 'rel-moved')

    expect(checkoutChromiumRef(branchHeadsRef)).toBe(true)
    expect(headOf(srcDir)).toBe(movedSha)
  })

  it('keeps the mirror lean as the pinned tag moves on', () => {
    commitReleaseTag(releaseTag)
    const laterSha = commitReleaseTag(laterReleaseTag)

    expect(checkoutChromiumRef(releaseRef)).toBe(true)
    expect(checkoutChromiumRef(`refs/tags/${laterReleaseTag}`)).toBe(true)
    expect(headOf(srcDir)).toBe(laterSha)

    // Each populated ref becomes a refspec the mirror re-fetches from then on,
    // one `git fetch` per entry, so they must not pile up per Chromium bump.
    const mirror = mirrorDir()
    const fetchSpecs = git(
      mirror,
      '--git-dir',
      mirror,
      'config',
      '--get-all',
      'remote.origin.fetch',
    ).split('\n')

    expect(fetchSpecs).toEqual([
      '+refs/heads/*:refs/heads/*',
      `+refs/tags/${laterReleaseTag}:refs/tags/${laterReleaseTag}`,
    ])
  })

  it('declines a ref it cannot resolve, leaving the checkout alone', () => {
    const trunkSha = headOf(srcDir)

    // A branch name has no refspec that fetches it under the same local name,
    // so the caller has to fall back to `gclient sync --revision`.
    expect(checkoutChromiumRef('origin/main')).toBe(false)

    expect(headOf(srcDir)).toBe(trunkSha)
    expect(Log.warn).toHaveBeenCalledWith(
      expect.stringContaining('origin/main'),
    )
  })

  it('declines a bare commit hash', () => {
    const releaseSha = commitReleaseTag(releaseTag)
    const trunkSha = headOf(srcDir)

    expect(checkoutChromiumRef(releaseSha)).toBe(false)

    expect(headOf(srcDir)).toBe(trunkSha)
  })
})
