// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { checkoutChromiumRef } from './chromiumFetch.ts'
import config from './config.ts'
import { snapshotConfig } from './configSnapshot.ts'
import * as Log from './log.ts'
import * as syncUtils from './syncUtils.ts'
import util from './util.ts'

// `var`, as imports run before this initialiser and read `isCI` while loading.
// eslint-disable-next-line no-var
var mockIsCI = false

jest.mock('./ciDetect.ts', () => ({
  get isCI() {
    return mockIsCI
  },
  isTeamcity: false,
}))

jest.mock('./log.ts', () => ({
  warn: jest.fn(),
  status: jest.fn(),
  error: jest.fn(),
}))

jest.mock('./chromiumFetch.ts', () => ({
  checkoutChromiumRef: jest.fn(),
}))

let tmpDir: string
let restoreConfig: () => void

beforeEach(() => {
  restoreConfig = snapshotConfig()

  tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'brave-sync-utils-'))
  config.rootDir = tmpDir
  config.srcDir = path.join(tmpDir, 'src')
  config.braveCoreDir = path.join(tmpDir, 'src', 'brave')
  config.gclientFile = path.join(tmpDir, '.gclient')
  fs.mkdirSync(config.braveCoreDir, { recursive: true })

  config.chromiumRepo = 'https://chromium.example.com/src.git'
  config.gitCachePath = path.join(tmpDir, 'cache')
  config.chromiumCustomDeps = {}
  config.chromiumCustomVars = {}
  config.gclientGlobalVars = {}
  config.leanSync = false
  config.gerritMirrorsUser = undefined

  mockIsCI = false

  jest.spyOn(console, 'log').mockImplementation(() => {})
  // `process.exit` would kill the worker; surface it as a test failure.
  jest.spyOn(process, 'exit').mockImplementation(((code: number) => {
    throw new Error(`process.exit(${code}) called`)
  }) as never)
})

afterEach(() => {
  restoreConfig()
  mockIsCI = false
  jest.restoreAllMocks()
  fs.rmSync(tmpDir, { recursive: true, force: true })
})

describe('writeGclientConfig', () => {
  const braveGclientFile = () =>
    path.join(config.braveCoreDir, '.brave_gclient')
  const read = (file: string) => fs.readFileSync(file, 'utf8')

  it('writes Python-like values into both files', () => {
    config.chromiumCustomVars = { checkout_clangd: true, other: false }
    config.chromiumCustomDeps = { 'src/third_party/dep': null }
    config.gclientGlobalVars = { delete_unversioned_trees: true }

    syncUtils.writeGclientConfig(['linux'], ['x64'])

    const gclient = read(config.gclientFile)
    expect(gclient).toContain('# Auto-updated on each sync.')
    expect(gclient).toContain('delete_unversioned_trees = True\n')
    expect(gclient).toContain('target_os = ["linux"]\n')
    expect(gclient).toContain('target_cpu = ["x64"]\n')
    expect(gclient).toContain(
      `cache_dir = ${JSON.stringify(config.gitCachePath)}`,
    )
    expect(gclient).toContain('"checkout_clangd": True')
    expect(gclient).toContain('"other": False')
    expect(gclient).toContain('"src/third_party/dep": None')
    expect(gclient).toContain('"name": "src/brave"')

    const braveGclient = read(braveGclientFile())
    expect(braveGclient).toContain('"name": "."')
    expect(braveGclient).toContain('target_os = ["linux"]')
  })

  it('puts values longer than 80 characters on multiple lines', () => {
    syncUtils.writeGclientConfig(['linux'], ['x64'])

    // `solutions` is long, `target_os` is not.
    expect(read(config.gclientFile)).toMatch(/^solutions = \[\n {2}\{\n/m)
    expect(read(config.gclientFile)).toMatch(/^target_os = \["linux"\]$/m)
  })

  it('omits undefined values', () => {
    config.gitCachePath = undefined

    syncUtils.writeGclientConfig(['linux'], ['x64'])

    expect(read(config.gclientFile)).not.toContain('cache_dir')
  })

  it('skips brave-core when only Chromium is wanted', () => {
    syncUtils.writeGclientConfig(['linux'], ['x64'], true)

    expect(fs.existsSync(braveGclientFile())).toBe(false)
    expect(read(config.gclientFile)).not.toContain('src/brave')
  })

  it('only reports the files that changed', () => {
    syncUtils.writeGclientConfig(['linux'], ['x64'])
    expect(Log.status).toHaveBeenCalledTimes(2)

    syncUtils.writeGclientConfig(['linux'], ['x64'])
    expect(Log.status).toHaveBeenCalledTimes(2)

    syncUtils.writeGclientConfig(['linux'], ['arm64'])
    expect(Log.status).toHaveBeenCalledTimes(4)
  })
})

describe('readGclientConfig', () => {
  it('returns an empty object without a .gclient', () => {
    expect(syncUtils.readGclientConfig()).toEqual({})
  })

  it('evaluates a written .gclient', () => {
    config.gclientGlobalVars = { delete_unversioned_trees: true }
    syncUtils.writeGclientConfig(['linux', 'android'], ['x64'], true)

    const result = syncUtils.readGclientConfig()

    expect(result.target_os).toEqual(['linux', 'android'])
    expect(result.target_cpu).toEqual(['x64'])
    expect(result.delete_unversioned_trees).toBe(true)
    expect(result.solutions[0].name).toBe('src')
  })

  it('reports a .gclient that cannot be evaluated', () => {
    fs.writeFileSync(config.gclientFile, 'this is not python(')

    expect(() => syncUtils.readGclientConfig()).toThrow('process.exit(1)')
    expect(Log.error).toHaveBeenCalledWith(
      expect.stringContaining(`Failed to read ${config.gclientFile}`),
    )
  })
})

describe('syncChromium', () => {
  const ref = 'origin/main'
  const syncInfoPath = () =>
    path.join(tmpDir, '.brave_latest_successful_sync.json')
  let headSHA: string
  let targetSHA: string
  let latestSyncInfo: object
  let exclusionExists: boolean
  let postSyncRef: string

  const expectedSyncInfo = () => ({
    chromiumRef: ref,
    gclientTimestamp: fs.statSync(config.gclientFile).mtimeMs.toString(),
  })

  const gclientArgs = () =>
    (util.runGclient as jest.Mock).mock.calls[0]![0] as string[]

  beforeEach(() => {
    fs.writeFileSync(config.gclientFile, '')
    headSHA = 'aaa'
    targetSHA = 'aaa'
    exclusionExists = true
    postSyncRef = 'aaa (HEAD)'
    latestSyncInfo = expectedSyncInfo()

    config.getProjectRef = jest.fn().mockReturnValue(ref)
    config.applyGerritMirrorsGitConfig = jest.fn()
    jest.spyOn(util, 'runGit').mockImplementation((_dir, args) => {
      return args[1] === 'HEAD' ? headSHA : targetSHA
    })
    jest.spyOn(util, 'readJSON').mockImplementation(() => latestSyncInfo)
    jest.spyOn(util, 'writeJSON').mockImplementation(() => {})
    jest.spyOn(util, 'runGclient').mockImplementation(() => {})
    jest.spyOn(util, 'run').mockImplementation(() => ({}) as never)
    jest.spyOn(util, 'modifyGitExclusions').mockImplementation(() => {})
    jest
      .spyOn(util, 'isGitExclusionExists')
      .mockImplementation(() => exclusionExists)
    jest
      .spyOn(util, 'getGitReadableLocalRef')
      .mockImplementation(() => postSyncRef)
    jest.mocked(checkoutChromiumRef).mockReturnValue(true)
  })

  it('does nothing when Chromium is already in sync', () => {
    expect(syncUtils.syncChromium({})).toBe(false)

    expect(util.runGclient).not.toHaveBeenCalled()
    expect(util.run).not.toHaveBeenCalled()
    expect(util.writeJSON).not.toHaveBeenCalled()
  })

  it('syncs to the required ref when HEAD is elsewhere', () => {
    headSHA = 'bbb'

    expect(syncUtils.syncChromium({})).toBe(true)

    expect(gclientArgs()).toEqual([
      'sync',
      '--nohooks',
      '--reset',
      '--upstream',
      '--revision',
      `src@${ref}`,
    ])
    expect(util.modifyGitExclusions).toHaveBeenCalledWith(config.srcDir, {
      remove: ['brave/', 'brave_origin/'],
      add: ['/brave/'],
    })
    expect(util.writeJSON).toHaveBeenCalledWith(
      syncInfoPath(),
      expectedSyncInfo(),
    )
    expect(Log.status).toHaveBeenCalledWith('Chromium is now at aaa (HEAD)')
  })

  it('syncs when the last successful sync was for something else', () => {
    latestSyncInfo = { chromiumRef: 'origin/old', gclientTimestamp: '1' }

    expect(syncUtils.syncChromium({})).toBe(true)
  })

  it('syncs when the checkout is missing', () => {
    headSHA = ''
    targetSHA = ''
    postSyncRef = ''

    expect(syncUtils.syncChromium({})).toBe(true)

    expect(Log.status).toHaveBeenCalledWith('Chromium is now at [unknown]')
  })

  it('forces a sync when already up to date', () => {
    expect(syncUtils.syncChromium({ init: true })).toBe(true)
    expect(gclientArgs()).toContain('--force')
  })

  it('forces a sync with --force', () => {
    expect(syncUtils.syncChromium({ force: true })).toBe(true)
    expect(gclientArgs()).toContain('--force')
  })

  it('fetches tags and branch heads', () => {
    headSHA = 'bbb'

    syncUtils.syncChromium({ fetch_all: true })

    expect(gclientArgs()).toEqual(
      expect.arrayContaining(['--with_tags', '--with_branch_heads']),
    )
  })

  it('passes --no-history', () => {
    headSHA = 'bbb'

    syncUtils.syncChromium({ history: false })

    expect(gclientArgs()).toContain('--no-history')
  })

  it('does not bootstrap when asked not to', () => {
    headSHA = 'bbb'

    syncUtils.syncChromium({ bootstrap: false })

    expect(gclientArgs()).toContain('--no-bootstrap')
  })

  it('refuses to skip bootstrap on CI', () => {
    mockIsCI = true
    headSHA = 'bbb'

    expect(() => syncUtils.syncChromium({ bootstrap: false })).toThrow(
      'process.exit(1)',
    )
    expect(Log.error).toHaveBeenCalledWith('--no-boostrap is not allowed on CI')
  })

  describe('--delete_unused_deps', () => {
    it('is passed on when the brave exclusion exists', () => {
      headSHA = 'bbb'

      syncUtils.syncChromium({ delete_unused_deps: true })

      expect(gclientArgs()).toContain('-D')
      expect(Log.warn).not.toHaveBeenCalled()
    })

    it('is ignored with a warning without the exclusion', () => {
      headSHA = 'bbb'
      exclusionExists = false

      syncUtils.syncChromium({ delete_unused_deps: true })

      expect(gclientArgs()).not.toContain('-D')
      expect(Log.warn).toHaveBeenCalledWith(
        expect.stringContaining('exclusion for the src/brave/'),
      )
    })

    it('is ignored silently on CI without the exclusion', () => {
      mockIsCI = true
      headSHA = 'bbb'
      exclusionExists = false

      syncUtils.syncChromium({ delete_unused_deps: true })

      expect(gclientArgs()).not.toContain('-D')
      expect(Log.warn).not.toHaveBeenCalled()
    })

    it('is ignored with a warning when Chromium needs no sync', () => {
      expect(syncUtils.syncChromium({ delete_unused_deps: true })).toBe(false)

      expect(Log.warn).toHaveBeenCalledWith(
        expect.stringContaining('ignored for src/ dir'),
      )
    })

    it('is ignored silently on CI when Chromium needs no sync', () => {
      mockIsCI = true

      expect(syncUtils.syncChromium({ delete_unused_deps: true })).toBe(false)

      expect(Log.warn).not.toHaveBeenCalled()
    })
  })

  describe('--sync_chromium', () => {
    it('skips a needed sync when false', () => {
      headSHA = 'bbb'

      expect(syncUtils.syncChromium({ sync_chromium: false })).toBe(false)

      expect(Log.warn).toHaveBeenCalledWith(
        expect.stringContaining('skip performing the update'),
      )
      expect(util.runGclient).not.toHaveBeenCalled()
    })

    it('syncs without a warning when one is needed anyway', () => {
      headSHA = 'bbb'

      expect(syncUtils.syncChromium({ sync_chromium: true })).toBe(true)

      expect(Log.warn).not.toHaveBeenCalled()
    })

    it('forces an unneeded sync, with a warning', () => {
      expect(syncUtils.syncChromium({ sync_chromium: true })).toBe(true)

      expect(Log.warn).toHaveBeenCalledWith(
        expect.stringContaining("doesn't need sync"),
      )
      expect(util.runGclient).toHaveBeenCalled()
    })
  })

  describe('lean checkout', () => {
    beforeEach(() => {
      config.leanSync = true
      headSHA = 'bbb'
    })

    const createVersionFile = () => {
      fs.mkdirSync(path.join(config.srcDir, 'chrome'), { recursive: true })
      fs.writeFileSync(path.join(config.srcDir, 'chrome', 'VERSION'), '')
    }

    it('checks out the ref itself instead of passing --revision', () => {
      createVersionFile()

      syncUtils.syncChromium({})

      expect(checkoutChromiumRef).toHaveBeenCalledWith(ref)
      expect(gclientArgs()).not.toContain('--revision')
    })

    it('falls back to --revision when the checkout fails', () => {
      createVersionFile()
      jest.mocked(checkoutChromiumRef).mockReturnValue(false)

      syncUtils.syncChromium({})

      expect(gclientArgs()).toEqual(expect.arrayContaining(['--revision']))
    })

    it('falls back to --revision without an existing clone', () => {
      syncUtils.syncChromium({})

      expect(checkoutChromiumRef).not.toHaveBeenCalled()
      expect(gclientArgs()).toContain('--revision')
    })

    it('does not check out anything when nothing needs updating', () => {
      headSHA = 'aaa'

      syncUtils.syncChromium({ sync_chromium: true })

      expect(checkoutChromiumRef).not.toHaveBeenCalled()
      expect(gclientArgs()).not.toContain('--revision')
    })
  })

  describe('Gerrit mirrors', () => {
    const installCall = () =>
      (util.run as jest.Mock).mock.calls.find(
        ([cmd]) => cmd === 'vpython3',
      ) as [string, string[]]

    beforeEach(() => {
      config.gerritMirrorsUser = 'mirror-user'
    })

    it('refreshes the mirror list alongside a sync', () => {
      headSHA = 'bbb'

      syncUtils.syncChromium({})

      const [, args] = installCall()
      expect(args[0]).toMatch(/mirror_git_config\.py$/)
      expect(args.slice(1)).toEqual([
        'install',
        '--user',
        'mirror-user',
        '--update',
      ])
      expect(config.applyGerritMirrorsGitConfig).toHaveBeenCalled()
    })

    it('keeps no-op syncs offline', () => {
      expect(syncUtils.syncChromium({})).toBe(false)

      expect(installCall()[1].slice(1)).toEqual([
        'install',
        '--user',
        'mirror-user',
      ])
      expect(config.applyGerritMirrorsGitConfig).toHaveBeenCalled()
    })

    it('is skipped without a configured user', () => {
      config.gerritMirrorsUser = undefined

      syncUtils.syncChromium({})

      expect(util.run).not.toHaveBeenCalled()
      expect(config.applyGerritMirrorsGitConfig).not.toHaveBeenCalled()
    })
  })
})

describe('checkInternalDepsEndpoint', () => {
  const originalFetch = globalThis.fetch
  let fetchMock: jest.Mock

  beforeEach(() => {
    config.useBraveHermeticToolchain = true
    config.internalDepsUrl = 'https://deps.example.com'
    fetchMock = jest.fn()
    globalThis.fetch = fetchMock as unknown as typeof fetch
  })

  afterEach(() => {
    globalThis.fetch = originalFetch
  })

  it('is reachable without the hermetic toolchain', async () => {
    config.useBraveHermeticToolchain = false

    await expect(syncUtils.checkInternalDepsEndpoint()).resolves.toBe(true)
    expect(fetchMock).not.toHaveBeenCalled()
  })

  it('is reachable when the endpoint redirects', async () => {
    fetchMock.mockResolvedValue({ status: 302 })

    await expect(syncUtils.checkInternalDepsEndpoint()).resolves.toBe(true)
    expect(fetchMock).toHaveBeenCalledWith(
      'https://deps.example.com/windows-hermetic-toolchain/test.txt',
      expect.objectContaining({ method: 'HEAD', redirect: 'manual' }),
    )
  })

  it('is unreachable on any other status', async () => {
    fetchMock.mockResolvedValue({ status: 200 })

    await expect(syncUtils.checkInternalDepsEndpoint()).resolves.toBe(false)
  })

  it('is unreachable when the request fails', async () => {
    fetchMock.mockRejectedValue(new Error('offline'))

    await expect(syncUtils.checkInternalDepsEndpoint()).resolves.toBe(false)
  })
})
