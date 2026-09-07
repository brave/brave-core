// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'fs-extra'
import path from 'node:path'
import type { Config } from './config.ts'

// HACK: determines the executable path from the gn target name
// Alternative: gn desc <buildDir> <target> outputs --format=json
// TODO(https://github.com/brave/brave-browser/issues/48118): is there a better way of doing this?
export function gnTargetToExecutableName(str: string): string {
  return str.split(':').at(-1)!.split('(')[0]!
}

export function getTestBinary(config: Config, suite: string) {
  let testBinary = suite
  if (testBinary === 'brave_java_unit_tests') {
    testBinary = path.join('bin', 'run_brave_java_unit_tests')
  } else if (testBinary === 'brave_junit_tests') {
    testBinary = path.join('bin', 'run_brave_junit_tests')
  }

  if (config.isIOS()) {
    testBinary = path.join(`${testBinary}.app`)
  }

  if (process.platform === 'win32') {
    testBinary = path.join(`${testBinary}.exe`)
  }
  return path.join(config.outputDir, testBinary)
}

export function getChromiumTestsSuites(config: Config) {
  return getTestsToRun(config, 'chromium_unit_tests').concat(['browser_tests'])
}

export function getTestGroupDeps(testDepFile: string) {
  if (fs.existsSync(testDepFile)) {
    const suiteDepNames = JSON.parse(
      fs.readFileSync(testDepFile, { encoding: 'utf-8' }),
    )
    return suiteDepNames.map(gnTargetToExecutableName)
  } else {
    return []
  }
}

export function getTestsToRun(config: Config, suite: string) {
  const testsToRun = getTestGroupDeps(
    path.join(config.outputDir, `${suite}.json`),
  )

  if (testsToRun.length === 0) {
    return [suite]
  }

  return testsToRun
}

// Returns a list of paths to files containing all the filters that would apply
// to the current test suite. Include missing paths when detecting changes to
// deleted filters.
//
// For instance, for Windows 64-bit and assuming all the filters files exist
// in the filesystem, this method would return paths to the following files:
//   - unit-tests.filter              -> Base filters
//   - unit_tests-windows.filters:    -> Platform specific
//   - unit_tests-windows-x86.filters -> Platform & Architecture specific
//
// Each filter is looked up both in test/filters/ (hand-written) and in
// test/filters/generated/ (auto-generated upstream flake filters, see
// tools/chromium_tests_analysis/update-upstream-flake-filters.py).
export function getApplicableFilters(
  config: Config,
  suite: string,
  { includeMissing = false } = {},
) {
  let filterFilePaths: string[] = []

  let targetPlatform: string = process.platform
  if (targetPlatform === 'win32') {
    targetPlatform = 'windows'
  } else if (targetPlatform === 'darwin') {
    targetPlatform = 'macos'
  }

  let possibleFilters: string[] = [
    suite,
    [suite, targetPlatform].join('-'),
    [suite, targetPlatform, config.targetArch].join('-'),
  ]

  // If you make changes to the list of *san variants here, also update
  // update-upstream-flake-filters.py.
  if (config.is_ubsan) {
    possibleFilters.push([suite, targetPlatform, 'ubsan'].join('-'))
  }

  if (config.is_asan) {
    possibleFilters.push([suite, targetPlatform, 'asan'].join('-'))
  }

  if (config.is_msan) {
    possibleFilters.push([suite, targetPlatform, 'msan'].join('-'))
  }

  const filterDirs = [
    path.join(config.braveCoreDir, 'test', 'filters'),
    path.join(config.braveCoreDir, 'test', 'filters', 'generated'),
  ]
  possibleFilters.forEach((filterName) => {
    for (const filterDir of filterDirs) {
      let filterFilePath = path.join(filterDir, `${filterName}.filter`)
      if (includeMissing || fs.existsSync(filterFilePath)) {
        filterFilePaths.push(filterFilePath)
      }
    }
  })

  return filterFilePaths
}

export function getDefaultTestSuiteArgs(testSuite: string): string[] {
  switch (testSuite) {
    case 'brave_network_audit_tests':
      return [
        '--ui-test-action-timeout=320000',
        '--test-launcher-timeout=2200000',
      ]
    default:
      return []
  }
}
