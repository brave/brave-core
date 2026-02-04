// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs-extra')
const path = require('path')

const Config = require('../lib/config')
const Log = require('../lib/logging')
const util = require('../lib/util')
const assert = require('assert')

const { getAffectedTests } = require('./affectedTests')
const {
  getTestBinary,
  getTestsToRun,
  getApplicableFilters,
  getChromiumTestsSuites,
} = require('./testUtils')

const test = async (
  passthroughArgs,
  suite,
  buildConfig = Config.defaultBuildConfig,
  options = {},
) => {
  Config.buildConfig = buildConfig
  Config.update(options)

  if (Config.isIOS() && Config.targetEnvironment === 'device') {
    Log.error('Running ios tests on a device is not yet supported')
    process.exit(1)
  }

  const testsToRun = options.base
    ? await getAffectedTests({ ...options, suite })
    : getTestsToRun(Config, suite)

  if (testsToRun.length === 0 && !options.quiet) {
    console.warn('SKIP: No tests need to run')
    return
  }

  await buildTests(testsToRun, Config, options)
  await runTests(passthroughArgs, { suite, testsToRun }, Config, options)
}

const deleteFile = (filePath) => {
  if (fs.existsSync(filePath)) {
    fs.unlinkSync(filePath)
  }
}

const buildTests = async (testsToRun, config) => {
  config.buildTargets = testsToRun
  util.touchOverriddenFiles()
  util.touchGsutilChangeLogFile()

  await util.buildTargets(config.buildTargets, config.defaultOptions)
}

const runTests = async (
  passthroughArgs,
  { suite, testsToRun },
  config,
  options,
) => {
  const isJunitTestSuite = suite.endsWith('_junit_tests')
  const allResultsFilePath = path.join(config.srcDir, `${suite}.txt`)
  // Clear previous results file
  deleteFile(allResultsFilePath)

  let braveArgs = []

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

  if (suite === 'brave_unit_tests' && config.isTeamcity && !config.isMobile()) {
    runChromiumTestLauncherTeamcityReporterIntegrationTests(Config)
  }

  const upstreamTestSuites = getChromiumTestsSuites(config)

  // Run the tests
  testsToRun.every((testSuite) => {
    let runArgs = braveArgs.slice()
    let runOptions = config.defaultOptions

    // Upstream tests expect to be run from the output directory
    runOptions.cwd = config.outputDir

    // Set ASAN_OPTIONS (if not already set) only for test launching.
    // Note: other stages (like build) shouldn't set ASAN_OPTIONS to avoid
    // LSAN failures. Chromium uses the same approach.
    if (config.isAsan() && !runOptions.env.ASAN_OPTIONS) {
      let asanOptions = ['detect_odr_violation=0']
      if (config.isLsan()) {
        asanOptions.push('detect_leaks=1')
      }
      runOptions.env.ASAN_OPTIONS = asanOptions.join(' ')
    }
    if (config.isLsan() && !runOptions.env.LSAN_OPTIONS) {
      const suppressionsFilePath = path.join(
        config.braveCoreDir,
        'test',
        'sanitizers',
        'lsan_suppressions.cfg',
      )
      runOptions.env.LSAN_OPTIONS = `suppressions=${suppressionsFilePath}`
    }

    // Filter out upstream tests that are known to fail for Brave
    const filterFilePaths = getApplicableFilters(Config, testSuite)
    if (filterFilePaths.length > 0) {
      runArgs.push(`--test-launcher-filter-file=${filterFilePaths.join(';')}`)
    }
    if (config.isTeamcity && !config.isIOS()) {
      if (upstreamTestSuites.includes(testSuite)) {
        const ignorePreliminaryFailures =
          '--test-launcher-teamcity-reporter-ignore-preliminary-failures'
        if (!runArgs.includes(ignorePreliminaryFailures)) {
          runArgs.push(ignorePreliminaryFailures)
        }
      }
    }

    let convertJSONToXML = false
    let outputFilename = path.join(config.srcDir, testSuite)

    if (config.isCI || options.output_xml) {
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

    if (config.isTeamcity) {
      // Stdout and stderr must be separate for a test launcher.
      runOptions.stdio = 'inherit'
    }

    let progStatus = 0

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

      runArgs.push('--app', getTestBinary(Config, testSuite))
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
        // this defines where coverage raw data is stored.
        // %4m uses a pool of 4 files to write into and is used by chromium.
        // note: that more files will take up more space but might be slightly faster
        // https://clang.llvm.org/docs/SourceBasedCodeCoverage.html#running-the-instrumented-program
        // this has no effect if project is built without coverage.
        runOptions.env.LLVM_PROFILE_FILE =
          runOptions.env.LLVM_PROFILE_FILE
          || `${Config.outputDir}/coverage/${testSuite}/%4m.profraw`
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

const runChromiumTestLauncherTeamcityReporterIntegrationTests = (config) => {
  const generalTestCase = {
    args: [
      '--test-launcher-bot-mode',
      '--gtest_filter=DISABLED_TeamcityReporterIntegration*',
      '--gtest_also_run_disabled_tests',
      // Enable retry limit explicitly, because it's set to 0 when
      // --gtest_filter is passed.
      '--test-launcher-retry-limit=1',
    ],

    expectedLines: [
      "##teamcity[testSuiteStarted name='brave_unit_tests']",
      "##teamcity[testRetrySupport enabled='true']",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Success'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Success'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testSuiteFinished name='brave_unit_tests']",
    ],
  }

  const ignorePreliminaryFailuresTestCase = {
    args: [
      ...generalTestCase.args,
      '--test-launcher-teamcity-reporter-ignore-preliminary-failures',
    ],

    expectedLines: [
      "##teamcity[testSuiteStarted name='brave_unit_tests']",
      "##teamcity[testRetrySupport enabled='true']",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Success'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Success'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
      "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
      "##teamcity[testSuiteFinished name='brave_unit_tests']",
    ],
  }

  const runOptions = config.defaultOptions
  runOptions.stdio = 'pipe'
  runOptions.continueOnFail = true

  for (const testCase of [generalTestCase, ignorePreliminaryFailuresTestCase]) {
    const prog = util.run(
      path.join(config.outputDir, 'brave_unit_tests'),
      testCase.args,
      runOptions,
    )
    const outputLines = prog.stdout.toString().split('\n')
    checkTeamcityReporterOutput(outputLines, testCase.expectedLines)
  }
}

const checkTeamcityReporterOutput = (outputLines, expectedTeamcityLines) => {
  const outputTeamcityLines = outputLines.filter((line) =>
    line.startsWith('##teamcity'),
  )

  const isMatched =
    outputTeamcityLines.length === expectedTeamcityLines.length
    && expectedTeamcityLines.every((expectedLine, index) => {
      const outputLine = outputTeamcityLines[index]
      return outputLine.startsWith(expectedLine)
    })

  if (!isMatched) {
    const notMatchedOutputLines = outputTeamcityLines.filter(
      (outputLine, index) => {
        const expectedLine = expectedTeamcityLines[index]
        return !outputLine.startsWith(expectedLine)
      },
    )

    const notMatchedExpectedLines = expectedTeamcityLines.filter(
      (expectedLine, index) => {
        const outputLine = outputTeamcityLines[index]
        return !outputLine?.startsWith(expectedLine)
      },
    )

    Log.error(
      'TeamcityReporter output test failed, output ##teamcity lines do not match expected lines (note ##teamcity was replaced with %%teamcity):',
    )
    console.error(
      [
        '\nNot matched output lines:',
        ...notMatchedOutputLines,
        '\nNot matched expected lines:',
        ...notMatchedExpectedLines,
        '\nTest output lines:',
        ...outputTeamcityLines,
        '\nExpected lines:',
        ...expectedTeamcityLines,
        '\nFull test output:',
        ...outputLines,
      ]
        .join('\n')
        .replace(/##teamcity/gm, '%%teamcity'),
    )
    process.exit(1)
  } else {
    console.log('TeamcityReporter output test passed')
  }
}

module.exports = test
