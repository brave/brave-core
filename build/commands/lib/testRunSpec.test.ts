// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import type { TestOptions } from './buildOptions.ts'
import type { Config } from './config.ts'
import {
  applyEnvDefaults,
  getTestRunArgs,
  getTestRunEnvDefaults,
} from './testRunSpec.ts'

let srcDir: string
let outputDir: string

function makeConfig(
  overrides: { targetOS?: string; asan?: boolean; lsan?: boolean } = {},
) {
  const { targetOS = 'linux', asan = false, lsan = false } = overrides
  return {
    srcDir,
    outputDir,
    braveCoreDir: path.join(srcDir, 'brave'),
    targetOS,
    targetArch: 'x64',
    isIOS: () => targetOS === 'ios',
    isAndroid: () => targetOS === 'android',
    isMobile: () => targetOS === 'ios' || targetOS === 'android',
    isAsan: () => asan,
    isLsan: () => lsan,
  } as unknown as Config
}

function makeOptions(overrides: Partial<TestOptions> = {}) {
  return { v: 0, test_launcher_jobs: 4, ...overrides } as TestOptions
}

function writeFilter(fileName: string) {
  const dir = path.join(srcDir, 'brave', 'test', 'filters')
  fs.mkdirSync(dir, { recursive: true })
  const filterPath = path.join(dir, fileName)
  fs.writeFileSync(filterPath, '-Foo.Bar\n', 'utf8')
  return filterPath
}

beforeEach(() => {
  srcDir = fs.mkdtempSync(path.join(os.tmpdir(), 'test-run-spec-'))
  outputDir = path.join(srcDir, 'out', 'Release')
  fs.mkdirSync(outputDir, { recursive: true })
})

afterEach(() => {
  fs.rmSync(srcDir, { recursive: true, force: true })
})

describe('getTestRunArgs', () => {
  it('builds default desktop args followed by passthrough args', () => {
    expect(
      getTestRunArgs(
        makeConfig(),
        'brave_unit_tests',
        makeOptions({ filter: 'A.*', vmodule: 'foo=2' }),
        ['--extra'],
        { isTeamcity: false },
      ),
    ).toEqual([
      '--enable-logging=stderr',
      '--v=0',
      '--vmodule=foo=2',
      '--gtest_filter=A.*',
      '--test-launcher-jobs=4',
      '--extra',
    ])
  })

  it('prepends default suite args', () => {
    expect(
      getTestRunArgs(
        makeConfig(),
        'brave_network_audit_tests',
        makeOptions(),
        [],
        { isTeamcity: false },
      ).slice(0, 2),
    ).toEqual([
      '--ui-test-action-timeout=320000',
      '--test-launcher-timeout=2200000',
    ])
  })

  it('keeps only --json-results-file passthrough arg for junit tests', () => {
    expect(
      getTestRunArgs(
        makeConfig({ targetOS: 'android' }),
        'brave_junit_tests',
        makeOptions(),
        ['--json-results-file=out.json', '--extra'],
        { isTeamcity: false },
      ),
    ).toEqual(['--json-results-file=out.json'])
  })

  it.each([
    { pathStyle: 'absolute' as const, relative: false },
    { pathStyle: 'relative' as const, relative: true },
  ])('emits $pathStyle filter file paths', ({ pathStyle, relative }) => {
    const filterA = writeFilter('browser_tests.filter')
    const filterB = writeFilter('browser_tests-linux.filter')
    const expected = relative
      ? [
          path.join(
            '..',
            '..',
            'brave',
            'test',
            'filters',
            'browser_tests.filter',
          ),
          path.join(
            '..',
            '..',
            'brave',
            'test',
            'filters',
            'browser_tests-linux.filter',
          ),
        ]
      : [filterA, filterB]
    expect(
      getTestRunArgs(makeConfig(), 'browser_tests', makeOptions(), [], {
        pathStyle,
        isTeamcity: false,
      }),
    ).toContain(`--test-launcher-filter-file=${expected.join(';')}`)
  })

  it('ignores preliminary failures of upstream suites on teamcity', () => {
    const flag = '--test-launcher-teamcity-reporter-ignore-preliminary-failures'
    const config = makeConfig()
    const options = makeOptions()
    expect(
      getTestRunArgs(config, 'browser_tests', options, [], {
        isTeamcity: true,
      }),
    ).toContain(flag)
    expect(
      getTestRunArgs(config, 'brave_unit_tests', options, [], {
        isTeamcity: true,
      }),
    ).not.toContain(flag)
    expect(
      getTestRunArgs(config, 'browser_tests', options, [], {
        isTeamcity: false,
      }),
    ).not.toContain(flag)
  })
})

describe('getTestRunEnvDefaults', () => {
  it('sets only the coverage profile file by default', () => {
    expect(getTestRunEnvDefaults(makeConfig(), 'brave_unit_tests')).toEqual({
      LLVM_PROFILE_FILE: path.join(
        outputDir,
        'coverage',
        'brave_unit_tests',
        '%4m.profraw',
      ),
    })
  })

  it('emits relative paths', () => {
    expect(
      getTestRunEnvDefaults(
        makeConfig({ asan: true, lsan: true }),
        'brave_unit_tests',
        { pathStyle: 'relative' },
      ),
    ).toEqual({
      ASAN_OPTIONS: 'detect_odr_violation=0 detect_leaks=1',
      LSAN_OPTIONS: `suppressions=${path.join('..', '..', 'brave', 'test', 'sanitizers', 'lsan_suppressions.cfg')}`,
      LLVM_PROFILE_FILE: path.join(
        'coverage',
        'brave_unit_tests',
        '%4m.profraw',
      ),
    })
  })

  it('sets nothing for ios', () => {
    expect(
      getTestRunEnvDefaults(makeConfig({ targetOS: 'ios' }), 'ios_tests'),
    ).toEqual({})
  })
})

describe('applyEnvDefaults', () => {
  it('only sets unset or empty variables', () => {
    expect(
      applyEnvDefaults(
        { A: 'keep', B: '' },
        { A: 'default', B: 'default', C: 'default' },
      ),
    ).toEqual({ A: 'keep', B: 'default', C: 'default' })
  })
})
