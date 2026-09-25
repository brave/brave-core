// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import type { Config } from './config.ts'
import { getApplicableFilters } from './testUtils.ts'

let srcDir: string
let braveFilterDir: string
let upstreamFilterDir: string

function makeConfig(overrides: { targetOS?: string; is_asan?: boolean } = {}) {
  const config = {
    srcDir,
    braveCoreDir: path.join(srcDir, 'brave'),
    targetOS: 'linux',
    targetArch: 'x64',
    is_asan: undefined,
    ...overrides,
  }
  return config as unknown as Config
}

function writeFilter(dir: string, fileName: string) {
  fs.mkdirSync(dir, { recursive: true })
  const filterPath = path.join(dir, fileName)
  fs.writeFileSync(filterPath, '-Foo.Bar\n', 'utf8')
  return filterPath
}

beforeEach(() => {
  srcDir = fs.mkdtempSync(path.join(os.tmpdir(), 'test-utils-'))
  braveFilterDir = path.join(srcDir, 'brave', 'test', 'filters')
  upstreamFilterDir = path.join(srcDir, 'testing', 'buildbot', 'filters')
})

afterEach(() => {
  fs.rmSync(srcDir, { recursive: true, force: true })
})

describe('getApplicableFilters', () => {
  it.each([
    { targetOS: 'linux', fileName: 'browser_tests-linux.filter' },
    { targetOS: 'mac', fileName: 'browser_tests-mac.filter' },
    { targetOS: 'win', fileName: 'browser_tests-win.filter' },
    { targetOS: 'android', fileName: 'browser_tests-android.filter' },
  ])(
    'names the brave filter after the target os, $targetOS',
    ({ targetOS, fileName }) => {
      expect(
        getApplicableFilters(makeConfig({ targetOS }), 'browser_tests', {
          includeMissing: true,
        }),
      ).toContain(path.join(braveFilterDir, fileName))
    },
  )

  it.each([
    { targetOS: 'linux', fileName: 'linux.asan.browser_tests.filter' },
    { targetOS: 'mac', fileName: 'mac.asan.browser_tests.filter' },
    { targetOS: 'win', fileName: 'win.asan.browser_tests.filter' },
    { targetOS: 'android', fileName: 'android.asan.browser_tests.filter' },
  ])(
    'names the upstream filter after the target os, $targetOS',
    ({ targetOS, fileName }) => {
      expect(
        getApplicableFilters(
          makeConfig({ targetOS, is_asan: true }),
          'browser_tests',
          { includeMissing: true },
        ),
      ).toContain(path.join(upstreamFilterDir, fileName))
    },
  )

  it('picks up the upstream asan filter of the suite being run', () => {
    const filterPath = writeFilter(
      upstreamFilterDir,
      'linux.asan.browser_tests.filter',
    )

    expect(
      getApplicableFilters(makeConfig({ is_asan: true }), 'browser_tests'),
    ).toEqual([filterPath])
  })

  it('leaves out the upstream asan filter when not running asan', () => {
    writeFilter(upstreamFilterDir, 'linux.asan.browser_tests.filter')

    expect(getApplicableFilters(makeConfig(), 'browser_tests')).toEqual([])
  })

  it('leaves out an upstream filter upstream does not have', () => {
    writeFilter(upstreamFilterDir, 'linux.asan.browser_tests.filter')

    expect(
      getApplicableFilters(makeConfig({ is_asan: true }), 'unit_tests'),
    ).toEqual([])
  })

  it('appends the upstream filter to the brave ones', () => {
    const braveFilterPath = writeFilter(
      braveFilterDir,
      'browser_tests-linux.filter',
    )
    const upstreamFilterPath = writeFilter(
      upstreamFilterDir,
      'linux.asan.browser_tests.filter',
    )

    expect(
      getApplicableFilters(makeConfig({ is_asan: true }), 'browser_tests'),
    ).toEqual([braveFilterPath, upstreamFilterPath])
  })
})
