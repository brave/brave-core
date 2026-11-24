// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import type { TestOptions } from './buildOptions.ts'
import { isTeamcity } from './ciDetect.ts'
import type { Config } from './config.ts'
import {
  getApplicableFilters,
  getChromiumTestsSuites,
  getDefaultTestSuiteArgs,
} from './testUtils.ts'

// 'absolute' paths are used to run tests in place. 'relative' paths are
// relative to config.outputDir (the directory tests run from), so the
// resulting command line stays valid after the files are moved elsewhere.
export type PathStyle = 'absolute' | 'relative'

export type TestRunSpecOptions = {
  pathStyle?: PathStyle
  isTeamcity?: boolean
}

function makePathMapper(config: Config, pathStyle: PathStyle) {
  return pathStyle === 'relative'
    ? (p: string) => path.relative(config.outputDir, p)
    : (p: string) => p
}

// Returns the command line arguments to run `testSuite` with.
export function getTestRunArgs(
  config: Config,
  testSuite: string,
  options: TestOptions,
  passthroughArgs: string[],
  {
    pathStyle = 'absolute',
    isTeamcity: teamcity = isTeamcity,
  }: TestRunSpecOptions = {},
): string[] {
  const toPath = makePathMapper(config, pathStyle)
  const isJunitTestSuite = testSuite.endsWith('_junit_tests')

  let braveArgs: string[] = []

  if (!isJunitTestSuite) {
    braveArgs.push('--enable-logging=stderr')
  }

  // Android and ios don't support --v
  if (!config.isMobile()) {
    braveArgs.push('--v=' + options.v)

    if (options.vmodule) {
      braveArgs.push('--vmodule=' + options.vmodule)
    }
  }

  if (options.filter) {
    braveArgs.push('--gtest_filter=' + options.filter)
  }

  if (options.run_disabled_tests) {
    if (config.isIOS()) {
      braveArgs.push('--gtest_also_run_disabled_tests')
    } else {
      braveArgs.push('--output-disabled-tests')
    }
  }

  if (options.disable_brave_extension && !config.isIOS()) {
    braveArgs.push('--disable-brave-extension')
  }

  if (options.single_process && !config.isIOS()) {
    braveArgs.push('--single_process')
  }

  if (!isJunitTestSuite) {
    if (
      options.test_launcher_jobs != null
      // --clones doesn't produce any test results and fails with AppLaunchError
      && !config.isIOS()
    ) {
      braveArgs.push('--test-launcher-jobs=' + options.test_launcher_jobs)
    }
    braveArgs = braveArgs.concat(passthroughArgs)
  } else {
    // Retain --json-results-file for junit tests.
    const jsonResultsArg = passthroughArgs.find((arg) =>
      arg.startsWith('--json-results-file='),
    )
    if (jsonResultsArg) {
      braveArgs.push(jsonResultsArg)
    }
  }

  // Prepend default test suite args
  const runArgs = getDefaultTestSuiteArgs(testSuite).concat(braveArgs)

  // Filter out upstream tests that are known to fail for Brave
  const filterFilePaths = getApplicableFilters(config, testSuite).map(toPath)
  if (filterFilePaths.length > 0) {
    runArgs.push(`--test-launcher-filter-file=${filterFilePaths.join(';')}`)
  }
  if (teamcity && !config.isIOS()) {
    if (getChromiumTestsSuites(config).includes(testSuite)) {
      const ignorePreliminaryFailures =
        '--test-launcher-teamcity-reporter-ignore-preliminary-failures'
      if (!runArgs.includes(ignorePreliminaryFailures)) {
        runArgs.push(ignorePreliminaryFailures)
      }
    }
  }

  return runArgs
}

// Returns the environment variables `testSuite` needs. Each one is a default:
// it should only be applied when not already set (see applyEnvDefaults).
export function getTestRunEnvDefaults(
  config: Config,
  testSuite: string,
  { pathStyle = 'absolute' }: TestRunSpecOptions = {},
): Record<string, string> {
  const toPath = makePathMapper(config, pathStyle)
  const env: Record<string, string> = {}

  // Set ASAN_OPTIONS only for test launching.
  // Note: other stages (like build) shouldn't set ASAN_OPTIONS to avoid
  // LSAN failures. Chromium uses the same approach.
  if (config.isAsan()) {
    const asanOptions = ['detect_odr_violation=0']
    if (config.isLsan()) {
      asanOptions.push('detect_leaks=1')
    }
    env.ASAN_OPTIONS = asanOptions.join(' ')
  }
  if (config.isLsan()) {
    const suppressionsFilePath = path.join(
      config.braveCoreDir,
      'test',
      'sanitizers',
      'lsan_suppressions.cfg',
    )
    env.LSAN_OPTIONS = `suppressions=${toPath(suppressionsFilePath)}`
  }

  // This defines where coverage raw data is stored.
  // %4m uses a pool of 4 files to write into and is used by chromium.
  // Note: that more files will take up more space but might be slightly
  // faster.
  // https://clang.llvm.org/docs/SourceBasedCodeCoverage.html#running-the-instrumented-program
  // This has no effect if project is built without coverage.
  if (!config.isIOS()) {
    env.LLVM_PROFILE_FILE = toPath(
      path.join(config.outputDir, 'coverage', testSuite, '%4m.profraw'),
    )
  }

  return env
}

// Sets each variable of `defaults` that is unset or empty in `env`.
export function applyEnvDefaults(
  env: NodeJS.ProcessEnv,
  defaults: Record<string, string>,
) {
  for (const [name, value] of Object.entries(defaults)) {
    if (!env[name]) {
      env[name] = value
    }
  }
  return env
}
