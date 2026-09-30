// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import config from './config.ts'
import { snapshotConfig } from './configSnapshot.ts'
import * as Log from './log.ts'
import * as sisoUtils from './sisoUtils.ts'

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
}))

describe('writeSisoRc', () => {
  let tmpDir: string
  let sisoRcPath: string
  let restoreConfig: () => void

  const setRbeReadOnly = (value: boolean) =>
    Object.defineProperty(config, 'rbeReadOnly', {
      value,
      configurable: true,
      writable: true,
    })

  const readSisoRc = () => fs.readFileSync(sisoRcPath, 'utf8')

  beforeEach(() => {
    restoreConfig = snapshotConfig()

    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'brave-siso-utils-'))
    sisoRcPath = path.join(tmpDir, 'build', 'config', 'siso', '.sisorc')
    fs.mkdirSync(path.dirname(sisoRcPath), { recursive: true })

    config.srcDir = tmpDir
    config.rbeService = ''
    config.sisoCacheDir = undefined
    setRbeReadOnly(false)
    mockIsCI = false
  })

  afterEach(() => {
    restoreConfig()
    mockIsCI = false
    fs.rmSync(tmpDir, { recursive: true, force: true })
  })

  it('writes an empty file without RBE', () => {
    sisoUtils.writeSisoRc()

    expect(readSisoRc()).toBe('')
  })

  it('writes the RBE flags as one ninja line', () => {
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = 'none'
    mockIsCI = true

    sisoUtils.writeSisoRc()

    expect(readSisoRc()).toBe(
      'ninja -reapi_keep_exec_stream -fs_min_flush_timeout 300s '
        + '-reapi_byte_stream_read_threshold 1024 -config googlechrome\n',
    )
  })

  it('turns off remote execution and cache writes when read only', () => {
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = 'none'
    mockIsCI = true
    setRbeReadOnly(true)

    sisoUtils.writeSisoRc()

    expect(readSisoRc()).toContain(
      '-re_exec_enable=false -re_cache_enable_write=false '
        + '-re_cache_enable_read',
    )
  })

  it('does not set read only flags when not read only', () => {
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = 'none'

    sisoUtils.writeSisoRc()

    expect(readSisoRc()).not.toContain('re_exec_enable')
  })

  it('warns when no cache directory is set', () => {
    config.rbeService = 'rbe.example.com:443'

    sisoUtils.writeSisoRc()

    expect(Log.warn).toHaveBeenCalledWith(
      expect.stringContaining('siso_cache_dir'),
    )
    expect(readSisoRc()).not.toContain('local_cache_enable')
  })

  it('does nothing about the cache when it is explicitly disabled', () => {
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = path.join(tmpDir, 'none')

    sisoUtils.writeSisoRc()

    expect(Log.warn).not.toHaveBeenCalled()
    expect(readSisoRc()).not.toContain('local_cache_enable')
    expect(fs.existsSync(config.sisoCacheDir)).toBe(false)
  })

  it('enables the local cache and creates its directory', () => {
    const cacheDir = path.join(tmpDir, 'cache', 'siso')
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = cacheDir

    sisoUtils.writeSisoRc()

    expect(readSisoRc()).toContain(
      `-local_cache_enable -cache_dir "${cacheDir}"`,
    )
    expect(fs.statSync(cacheDir).isDirectory()).toBe(true)
  })

  it('sets the interactive priority only outside CI', () => {
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = 'none'

    sisoUtils.writeSisoRc()
    expect(readSisoRc()).toContain('-reapi_priority 4')

    mockIsCI = true
    sisoUtils.writeSisoRc()
    expect(readSisoRc()).not.toContain('-reapi_priority')
  })

  it('does not rewrite an unchanged file', () => {
    config.rbeService = 'rbe.example.com:443'
    config.sisoCacheDir = 'none'

    sisoUtils.writeSisoRc()
    const before = fs.statSync(sisoRcPath).mtimeMs
    fs.utimesSync(sisoRcPath, new Date(1000), new Date(1000))
    sisoUtils.writeSisoRc()

    expect(before).toBeGreaterThan(1000)
    expect(fs.statSync(sisoRcPath).mtimeMs).toBe(1000)
  })
})
