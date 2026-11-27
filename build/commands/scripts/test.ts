// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { program } from '../lib/program.ts'
import * as buildOptions from '../lib/buildOptions.ts'
import fs from 'fs-extra'
import path from 'node:path'
import config from '../lib/config.ts'
import * as Log from '../lib/log.ts'
import util from '../lib/util.ts'
import assert from 'node:assert'
import { getAffectedTests } from '../lib/affectedTests.js'
import {
  getTestBinary,
  getTestsToRun,
  parseTestSuites,
} from '../lib/testUtils.ts'
import {
  applyEnvDefaults,
  getTestRunArgs,
  getTestRunEnvDefaults,
} from '../lib/testRunSpec.ts'
import { isCI, isTeamcity } from '../lib/ciDetect.ts'
import { getPassthroughArgs } from '../lib/commandsUtils.ts'

async function buildTests(testsToRun: string[]) {
  config.buildTargets = testsToRun
  util.touchOverriddenFiles()

  await util.buildTargets(config.buildTargets, config.defaultOptions)
}

// Runs the tests of `mainTestSuite`. Returns whether to keep running tests:
// false if a test failed and results aren't saved to files.
async function runTests(
  mainTestSuite: string,
  testSuitesToRun: string[],
  options: buildOptions.TestOptions,
  passthroughArgs: string[],
): Promise<boolean> {
  const allResultsFilePath = path.join(config.srcDir, `${mainTestSuite}.txt`)
  // Clear previous results file
  if (fs.existsSync(allResultsFilePath)) {
    fs.unlinkSync(allResultsFilePath)
  }

  // Run the tests
  return testSuitesToRun.every((testSuite) => {
    if (testSuite === 'brave_unit_tests' && isTeamcity && !config.isMobile()) {
      runChromiumTestLauncherTeamcityReporterIntegrationTests()
    }

    const isJunitTestSuite = testSuite.endsWith('_junit_tests')

    const runArgs = getTestRunArgs(config, testSuite, options, passthroughArgs)
    const runOptions = config.defaultOptions

    // Upstream tests expect to be run from the output directory
    runOptions.cwd = config.outputDir

    applyEnvDefaults(runOptions.env, getTestRunEnvDefaults(config, testSuite))

    let convertJSONToXML = false
    let outputFilename = path.join(config.srcDir, testSuite)

    if (isCI || options.output_xml) {
      // When test results are saved to a file, callers (such as CI) generate
      // and analyze test reports as a next step. These callers are typically
      // not interested in the exit code of running the tests, because they
      // get the information about test success or failure from the output
      // file. On the other hand, callers are interested in errors that
      // produce an exit code, such as test compilation failures. By ignoring
      // the test exit code here, we give callers a chance to distinguish test
      // failures (by looking at the output file) from compilation errors.
      runOptions.continueOnFail = true
    }

    if (options.output_xml) {
      // Add filename of xml output of each test suite into the results file
      if (config.isAndroid()) {
        // android only supports json output so use that here and convert
        // to xml afterwards
        runArgs.push(`--json-results-file=${outputFilename}.json`)
        convertJSONToXML = true
      } else if (!config.isIOS()) {
        runArgs.push(`--gtest_output=xml:${outputFilename}.xml`)
      }
      fs.appendFileSync(allResultsFilePath, `${testSuite}.xml\n`)
    }

    if (config.isAndroid() && !isJunitTestSuite) {
      assert(
        config.targetArch === 'x86'
          || config.targetArch === 'x64'
          || options.manual_android_test_device,
        'Only x86 and x64 builds can be run automatically. For other builds please run test device manually and specify manual_android_test_device flag.',
      )

      if (!options.manual_android_test_device) {
        const avdConfigPath = path.join(
          config.srcDir,
          'tools/android/avd/proto',
          `${options.android_test_emulator_name}.textpb`,
        )
        runArgs.push(`--avd-config=${avdConfigPath}`)
      }
    }

    if (isTeamcity) {
      // Stdout and stderr must be separate for a test launcher.
      runOptions.stdio = 'inherit'
    }

    let progStatus: number | null = null

    if (config.isIOS()) {
      const outputDir = path.join(config.outputDir, `run_${testSuite}_out`)
      if (fs.existsSync(outputDir)) {
        fs.rmSync(outputDir, { recursive: true })
      }
      fs.mkdirSync(path.join(outputDir, 'iossim'), { recursive: true })
      fs.mkdirSync(path.join(outputDir, 'output'), { recursive: true })

      let xcodeBuildVersion = options.ios_xcode_build_version
      if (xcodeBuildVersion == null) {
        xcodeBuildVersion = util
          .run('xcodebuild', ['-version'], { skipLogging: true })
          .stdout.toString()
          .trim()
          .split(' ')
          .at(-1)
      }
      assert(xcodeBuildVersion != null)

      runArgs.push('--app', getTestBinary(config, testSuite))
      runArgs.push('--runtime-cache-prefix', `${outputDir}/Runtime-ios-`)
      runArgs.push('--iossim', `${outputDir}/iossim`)
      runArgs.push('--out-dir', `${outputDir}/output`)
      runArgs.push('--xctest')
      runArgs.push('--xcode-build-version', xcodeBuildVersion)
      if (config.targetEnvironment === 'simulator') {
        runArgs.push('--platform', options.ios_simulator_platform)
        runArgs.push('--version', options.ios_simulator_version)
      } else if (config.targetEnvironment === 'device') {
        // TODO(bridiver)
        // runArgs.push('--xcodebuild-device-runner')
      }
      let iosRunOptions = Object.assign({}, runOptions)
      // run.py doesn't like it when you mess with PYTHONPATH
      delete iosRunOptions.env.PYTHONPATH
      progStatus = util.run(
        path.join(config.srcDir, 'ios/build/bots/scripts/run.py'),
        runArgs,
        iosRunOptions,
      ).status

      if (options.output_xml) {
        util.run(
          'python3',
          [
            path.join('tools', 'test', 'chromium_to_junit_converter.py'),
            path.join(outputDir, 'output', 'summary.json'),
            '-o',
            `${outputFilename}.xml`,
            '--format',
            'summary',
          ],
          {
            ...runOptions,
            cwd: config.braveCoreDir,
          },
        )
      }
    } else {
      const testRunner = getTestBinary(config, testSuite)
      if (!fs.existsSync(testRunner)) {
        console.error(`Missing test runner executable ${testRunner}`)
        progStatus = 1
      } else {
        progStatus = util.run(testRunner, runArgs, runOptions).status
      }
    }

    // convert json results to xml
    if (convertJSONToXML) {
      const inputFilename = `${outputFilename}.json`
      if (fs.existsSync(inputFilename)) {
        progStatus = util.run(
          'vpython3',
          [path.join('script', 'json2xunit.py')],
          {
            ...runOptions,
            cwd: config.braveCoreDir,
            stdio: [
              fs.openSync(inputFilename, 'r'),
              fs.openSync(`${outputFilename}.xml`, 'w'),
              'inherit',
            ],
          },
        ).status
      } else {
        console.error(
          `Missing json input file to convert to xml ${inputFilename}`,
        )
      }
    }
    // If we output results into an xml file (CI), then we want to run all
    // suites to get all potential failures. Otherwise, for example, if
    // running locally, it makes sense to stop once one suite has failures.
    return options.output_xml || progStatus === 0
  })
}

function runChromiumTestLauncherTeamcityReporterIntegrationTests() {
  const runnerPath = path.join(
    config.outputDir,
    'teamcity_reporter_integration_test.py',
  )
  const runOptions = {
    ...config.defaultOptions,
    cwd: config.outputDir,
  }
  util.run('python3', [runnerPath], runOptions)
}

program
  .argument(
    '<suites>',
    'comma-separated test suites, test groups or gn labels to run',
    parseTestSuites,
  )
  .apply(buildOptions.supportBuildConfigArg)
  .apply(buildOptions.supportBuildDir)
  .apply(buildOptions.supportTargetConfig)
  .apply(buildOptions.supportNinjaOptions)
  .apply(buildOptions.supportTestOptions)
  .allowUnknownOption(true)
  .allowExcessArguments(true)
  .action(async (suites, buildConfig, options, command) => {
    if (buildConfig) {
      config.buildConfig = buildConfig
    }
    config.update(options)

    if (config.isIOS() && config.targetEnvironment === 'device') {
      Log.error('Running ios tests on a device is not yet supported')
      process.exit(1)
    }

    // Each given suite runs like `test <suite>` would, with its own results
    // file, but all tests are built at once.
    const getSuiteRuns = async () => {
      const suiteRuns: { suite: string; tests: string[] }[] = []
      for (const suite of suites) {
        const tests = options.base
          ? await getAffectedTests({ ...options, suite })
          : getTestsToRun(config, suite)
        suiteRuns.push({ suite, tests })
      }
      return suiteRuns
    }

    // With --base, only the affected tests are built. Otherwise the given
    // suites are built as is and test groups are expanded after the build,
    // which regenerates their <out_dir>/<group>.json if BUILD.gn files changed.
    let suiteRuns = options.base ? await getSuiteRuns() : undefined
    const affectedTests = [...new Set(suiteRuns?.flatMap((r) => r.tests))]
    if (suiteRuns && affectedTests.length === 0 && !options.quiet) {
      console.warn('SKIP: No tests need to run')
      return
    }

    const passthroughArgs = getPassthroughArgs(command)
    // CI can --skip_run to build, then re-sign, then --skip_build to run
    // the signed binary. Used on macOS so Removable Volumes Allow sticks
    // across rebuilds.
    if (!options.skip_build) {
      await buildTests(suiteRuns ? affectedTests : suites)
    }
    suiteRuns ??= await getSuiteRuns()
    if (!options.skip_run) {
      for (const { suite, tests } of suiteRuns) {
        if (!(await runTests(suite, tests, options, passthroughArgs))) {
          break
        }
      }
    }
  })
  .parseAsync()
