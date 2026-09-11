// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import { Command } from 'commander'
import config from './config.ts'
import syncUtils from './syncUtils.js'
import util from './util.js'

jest.mock('./log.ts', () => ({
  divider: '',
  progressStart: jest.fn(),
  progressFinish: jest.fn(),
  progressScope: jest.fn((_message, callable) => callable()),
  progressScopeAsync: jest.fn(),
  status: jest.fn(),
  error: jest.fn(),
  warn: jest.fn(),
  updateStatus: jest.fn(),
  command: jest.fn(),
}))

const chromiumRepo = 'https://chromium.googlesource.com/chromium/src.git'
const gitCachePath = '/git-cache'
const mirrorDir = '/git-cache/chromium.googlesource.com-chromium-src'
const srcDir = '/checkout/src'
const tagRef = 'refs/tags/153.0.8010.37'
const tagSha = 'b75a5a95ea1a1b55bdbfd6d9f42d47be7507fb8b'
const headSha = '1111111111111111111111111111111111111111'

const chromeVersionFile = path.join('chrome', 'VERSION')

// `util.run` hands back a SpawnSyncReturns. Only `status` and `stdout` are read
// here, so the rest is filled in just to satisfy the type.
function spawnResult(stdout = '') {
  return /** @type {any} */ ({
    pid: 0,
    output: [],
    stdout: Buffer.from(stdout),
    stderr: Buffer.from(''),
    status: 0,
    signal: null,
  })
}

// Config fields the tests write to, restored after each one because `config` is
// a process-wide singleton.
const overriddenConfigFields = [
  'leanSync',
  'gitCachePath',
  'chromiumRepo',
  'srcDir',
  'rootDir',
  'gclientFile',
]

describe('syncChromium checkout of the pinned Chromium ref', () => {
  // Recorded `util.runGit(...)` and `util.run(...)` calls, in order.
  let gitCalls
  let runCalls
  // Args `gclient sync` was ultimately invoked with.
  let gclientArgs
  // Refs the fake `src/` already holds locally.
  let localRefs
  // Whether the fake `src/` is an existing Chromium checkout.
  let hasCheckout
  let savedConfigFields
  let getProjectRefSpy

  beforeEach(() => {
    gitCalls = []
    runCalls = []
    gclientArgs = null
    localRefs = new Set()
    hasCheckout = true

    savedConfigFields = {}
    for (const field of overriddenConfigFields) {
      savedConfigFields[field] = config[field]
    }
    config.gitCachePath = gitCachePath
    config.chromiumRepo = chromiumRepo
    config.srcDir = srcDir
    config.rootDir = '/checkout'
    config.gclientFile = '/checkout/.gclient'

    jest.spyOn(console, 'log').mockImplementation(() => {})
    getProjectRefSpy = jest
      .spyOn(config, 'getProjectRef')
      .mockReturnValue(tagRef)

    jest.spyOn(util, 'runGit').mockImplementation((cwd, args) => {
      gitCalls.push({ cwd, args })
      if (args[0] === 'rev-parse') {
        const rev = args[args.length - 1]
        if (rev === 'HEAD') {
          return headSha
        }
        // Both `chromiumRefExists()` (with --verify) and
        // `shouldUpdateChromium()` (without) resolve the required ref here.
        return localRefs.has(rev) ? tagSha : ''
      }
      return ''
    })

    jest.spyOn(util, 'run').mockImplementation((cmd, args = [], options) => {
      runCalls.push({ cmd, args, options })
      if (args[0] === 'cache' && args[1] === 'exists') {
        return spawnResult(`${mirrorDir}\n`)
      }
      return spawnResult()
    })

    // Marks the options as having gone through the default environment, which
    // is what puts depot_tools (and so `git cache`) on PATH.
    jest
      .spyOn(util, 'mergeWithDefault')
      .mockImplementation((options) => ({
        mergedWithDefault: true,
        ...options,
      }))

    jest.spyOn(util, 'runGclient').mockImplementation((args) => {
      gclientArgs = args
    })
    jest.spyOn(util, 'readJSON').mockReturnValue({})
    jest.spyOn(util, 'writeJSON').mockImplementation(() => {})
    jest.spyOn(util, 'modifyGitExclusions').mockImplementation(() => {})
    jest.spyOn(util, 'isGitExclusionExists').mockReturnValue(true)
    jest.spyOn(util, 'getGitReadableLocalRef').mockReturnValue(tagRef)

    jest
      .spyOn(fs, 'statSync')
      .mockReturnValue(/** @type {any} */ ({ mtimeMs: 1 }))
    const realExistsSync = fs.existsSync
    jest.spyOn(fs, 'existsSync').mockImplementation((filePath) => {
      if (String(filePath).endsWith(chromeVersionFile)) {
        return hasCheckout
      }
      return realExistsSync(filePath)
    })
  })

  afterEach(() => {
    jest.restoreAllMocks()
    for (const field of overriddenConfigFields) {
      config[field] = savedConfigFields[field]
    }
  })

  // `init` is the real entry point for this path: it forces the sync, so the
  // checkout is always re-pointed at the required ref.
  function runSync(program = {}) {
    return syncUtils.syncChromium({ init: true, ...program })
  }

  // Recorded commands as single strings, which read far better in a failure.
  const gitCommands = () => gitCalls.map((call) => call.args.join(' '))
  const runCommands = () => runCalls.map((call) => call.args.join(' '))
  const findRun = (text) =>
    runCalls.find((call) => call.args.join(' ').includes(text))

  it('does not fetch a tag the checkout already has', () => {
    config.leanSync = true
    localRefs.add(tagRef)

    expect(runSync()).toBe(true)

    // Chromium tags never move, so nothing should reach the mirror or network.
    expect(runCommands()).toEqual([])
    expect(gitCommands()).toContain(`reset --hard ${tagRef}`)
    expect(gclientArgs).not.toContain('--revision')
  })

  it('populates the git cache and fetches the tag from the mirror', () => {
    config.leanSync = true
    // `localRefs` stays empty: the tag has not reached the checkout yet.

    expect(runSync()).toBe(true)

    expect(runCommands()).toEqual([
      'cache populate'
        + ` --cache-dir ${gitCachePath}`
        + ' --no-fetch-tags'
        + ` --ref ${tagRef}`
        + ' --reset-fetch-config'
        + ` ${chromiumRepo}`,
      `cache exists --quiet --cache-dir ${gitCachePath} ${chromiumRepo}`,
      '-c advice.fetchShowForcedUpdates=false fetch'
        + ' --no-show-forced-updates --no-tags'
        + ` ${mirrorDir} ${tagRef}:${tagRef}`,
    ])
    expect(gitCommands()).toContain(`reset --hard ${tagRef}`)
    expect(gclientArgs).not.toContain('--revision')
  })

  it('runs the git cache commands with depot_tools on PATH', () => {
    config.leanSync = true

    runSync()

    // `git cache` is a depot_tools subcommand, so these must not run with a
    // bare environment.
    for (const call of runCalls) {
      expect(call.options).toMatchObject({ mergedWithDefault: true })
    }
    expect(findRun('cache populate').options.cwd).toBe(config.rootDir)
    expect(findRun('fetch --no-show-forced-updates').options.cwd).toBe(srcDir)
  })

  it('fetches the tag straight from origin when there is no git cache', () => {
    config.leanSync = true
    config.gitCachePath = undefined

    expect(runSync()).toBe(true)

    expect(runCommands()).toEqual([
      '-c advice.fetchShowForcedUpdates=false fetch'
        + ' --no-show-forced-updates --no-tags'
        + ` origin ${tagRef}:${tagRef}`,
    ])
    expect(gitCommands()).toContain(`reset --hard ${tagRef}`)
    expect(gclientArgs).not.toContain('--revision')
  })

  it('re-fetches a branch-heads ref even when it is already local', () => {
    config.leanSync = true
    const branchHeadsRef = 'refs/branch-heads/8010'
    getProjectRefSpy.mockReturnValue(branchHeadsRef)
    localRefs.add(branchHeadsRef)

    expect(runSync()).toBe(true)

    // Unlike a tag, a branch-heads ref moves, so having it is not enough.
    expect(runCommands()).toEqual([
      'cache populate'
        + ` --cache-dir ${gitCachePath}`
        + ' --no-fetch-tags'
        + ` --ref ${branchHeadsRef}`
        + ' --reset-fetch-config'
        + ` ${chromiumRepo}`,
      `cache exists --quiet --cache-dir ${gitCachePath} ${chromiumRepo}`,
      '-c advice.fetchShowForcedUpdates=false fetch'
        + ' --no-show-forced-updates --no-tags'
        + ` ${mirrorDir} ${branchHeadsRef}:${branchHeadsRef}`,
    ])
    expect(gclientArgs).not.toContain('--revision')
  })

  it('leaves a ref it cannot resolve itself to gclient', () => {
    config.leanSync = true
    getProjectRefSpy.mockReturnValue('origin/master')

    expect(runSync()).toBe(true)

    // A branch name has no refspec that fetches it under the same local name.
    expect(runCommands()).toEqual([])
    expect(gitCommands()).not.toContain('reset --hard origin/master')
    expect(gclientArgs).toEqual(
      expect.arrayContaining(['--revision', 'src@origin/master']),
    )
  })

  it('lets gclient clone a checkout that does not exist yet', () => {
    config.leanSync = true
    hasCheckout = false

    expect(runSync()).toBe(true)

    // Nothing to fetch into, so gclient needs the revision to clone at it.
    expect(runCommands()).toEqual([])
    expect(gitCommands()).not.toContain(`reset --hard ${tagRef}`)
    expect(gclientArgs).toEqual(
      expect.arrayContaining(['--revision', `src@${tagRef}`]),
    )
  })

  it('passes --revision to gclient when lean sync is off', () => {
    config.leanSync = false
    localRefs.add(tagRef)

    expect(runSync()).toBe(true)

    expect(runCommands()).toEqual([])
    expect(gitCommands()).not.toContain(`reset --hard ${tagRef}`)
    expect(gclientArgs).toEqual(
      expect.arrayContaining(['--revision', `src@${tagRef}`]),
    )
  })
})

describe('the --lean-sync option', () => {
  let savedLeanSync

  beforeEach(() => {
    savedLeanSync = config.leanSync
  })

  afterEach(() => {
    config.leanSync = savedLeanSync
  })

  // A dashed option name is camel-cased by commander, so `--lean-sync` arrives
  // as `leanSync`. Renaming one side without the other would silently stop the
  // flag from ever reaching the sync, so pin the whole chain down here.
  it('reaches config.leanSync when passed on the command line', () => {
    config.leanSync = false
    const program = new Command()
    program.option('--lean-sync', 'check out Chromium directly')
    program.parse(['node', 'sync', '--lean-sync'])

    config.update(program.opts())

    expect(program.opts()).toEqual({ leanSync: true })
    expect(config.leanSync).toBe(true)
  })

  it('stays off when the flag is absent', () => {
    config.leanSync = false
    const program = new Command()
    program.option('--lean-sync', 'check out Chromium directly')
    program.parse(['node', 'sync'])

    config.update(program.opts())

    expect(config.leanSync).toBe(false)
  })
})
