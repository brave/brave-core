// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import type { Config } from './config.ts'
import { getApplicableFilters } from './testUtils.ts'

const hostPlatform = process.platform

// The platform name upstream filters are looked up under on this host.
const platform =
  { win32: 'win', darwin: 'mac' }[hostPlatform as string] ?? 'linux'

function setHostPlatform(value: string) {
  Object.defineProperty(process, 'platform', { value, configurable: true })
}

let srcDir: string
let upstreamFilterDir: string

function makeConfig(overrides: { is_asan?: boolean } = {}) {
  const config = {
    srcDir,
    braveCoreDir: path.join(srcDir, 'brave'),
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
  upstreamFilterDir = path.join(srcDir, 'testing', 'buildbot', 'filters')
})

afterEach(() => {
  fs.rmSync(srcDir, { recursive: true, force: true })
  setHostPlatform(hostPlatform)
})

describe('getApplicableFilters', () => {
  it.each([
    { platformName: 'linux', fileName: 'linux.asan.browser_tests.filter' },
    { platformName: 'darwin', fileName: 'mac.asan.browser_tests.filter' },
    { platformName: 'win32', fileName: 'win.asan.browser_tests.filter' },
  ])(
    'looks the upstream filter up as $fileName on $platformName',
    ({ platformName, fileName }) => {
      setHostPlatform(platformName)

      expect(
        getApplicableFilters(makeConfig({ is_asan: true }), 'browser_tests', {
          includeMissing: true,
        }),
      ).toContain(path.join(upstreamFilterDir, fileName))
    },
  )

  it('picks up the upstream asan filter of the suite being run', () => {
    const filterPath = writeFilter(
      upstreamFilterDir,
      `${platform}.asan.browser_tests.filter`,
    )

    expect(
      getApplicableFilters(makeConfig({ is_asan: true }), 'browser_tests'),
    ).toEqual([filterPath])
  })

  it('leaves out the upstream asan filter when not running asan', () => {
    writeFilter(upstreamFilterDir, `${platform}.asan.browser_tests.filter`)

    expect(getApplicableFilters(makeConfig(), 'browser_tests')).toEqual([])
  })

  it('leaves out an upstream filter upstream does not have', () => {
    writeFilter(upstreamFilterDir, `${platform}.asan.browser_tests.filter`)

    expect(
      getApplicableFilters(makeConfig({ is_asan: true }), 'unit_tests'),
    ).toEqual([])
  })

  it('appends the upstream filter to the brave ones', () => {
    const braveFilterPath = writeFilter(
      path.join(srcDir, 'brave', 'test', 'filters'),
      'browser_tests.filter',
    )
    const upstreamFilterPath = writeFilter(
      upstreamFilterDir,
      `${platform}.asan.browser_tests.filter`,
    )

    expect(
      getApplicableFilters(makeConfig({ is_asan: true }), 'browser_tests'),
    ).toEqual([braveFilterPath, upstreamFilterPath])
  })
})
