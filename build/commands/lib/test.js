const fs = require('fs-extra')
const path = require('path')

const config = require('../lib/config')
const Log = require('../lib/logging')
const util = require('../lib/util')
const assert = require('assert')

const getTestBinary = (suite) => {
  return (process.platform === 'win32') ? `${suite}.exe` : suite
}

const getTestsToRun = (config, suite) => {
  let testsToRun = [suite]
  if (suite === 'brave_unit_tests') {
    if (config.targetOS !== 'android') {
      testsToRun.push('brave_installer_unittests')
    } else {
      testsToRun.push('bin/run_brave_public_test_apk')
    }
  }
  return testsToRun
}

// Returns a list of paths to files containing all the filters that would apply
// to the current test suite, as long as such files exist in the filesystem.
//
// For instance, for Windows 64-bit and assuming all the filters files exist
// in the filesystem, this method would return paths to the following files:
//   - unit-tests.filter              -> Base filters
//   - unit_tests-windows.filters:    -> Platform specific
//   - unit_tests-windows-x86.filters -> Platform & Architecture specific
const getApplicableFilters = (suite) => {
  let filterFilePaths = []

  let targetPlatform = process.platform
  if (targetPlatform === "win32") {
    targetPlatform = "windows"
  } else if (targetPlatform === "darwin") {
    targetPlatform = "macos"
  }

  let possibleFilters = [
    suite,
    [suite, targetPlatform].join('-'),
    [suite, targetPlatform, config.targetArch].join('-'),
  ]
  possibleFilters.forEach(filterName => {
    let filterFilePath = path.join(config.braveCoreDir, 'test', 'filters',  `${filterName}.filter`)
    if (fs.existsSync(filterFilePath))
      filterFilePaths.push(filterFilePath)
  });

  return filterFilePaths
}

const test = async (passthroughArgs, suite, buildConfig = config.defaultBuildConfig, options = {}) => {
  await buildTests(suite, buildConfig, options)
  runTests(passthroughArgs, suite, buildConfig, options)
}

const buildTests = async (suite, buildConfig = config.defaultBuildConfig, options = {}) => {
  config.buildConfig = buildConfig
  config.update(options)

  let testSuites = [
    'brave_unit_tests',
    'brave_browser_tests',
    'brave_network_audit_tests',
  ]
  if (testSuites.includes(suite)) {
    config.buildTargets = ['brave/test:' + suite]
  } else {
    config.buildTargets = [suite]
  }
  util.touchOverriddenFiles()
  util.touchGsutilChangeLogFile()
  await util.buildTargets()
}

const runTests = (passthroughArgs, suite, buildConfig, options) => {
  config.buildConfig = buildConfig
  config.update(options)

  let braveArgs = [
    '--enable-logging=stderr'
  ]

  // Android doesn't support --v
  if (config.targetOS !== 'android') {
    braveArgs.push('--v=' + options.v)

    if (options.vmodule) {
      braveArgs.push('--vmodule=' + options.vmodule)
    }
  }

  if (options.filter) {
    braveArgs.push('--gtest_filter=' + options.filter)
  }

  if (options.run_disabled_tests) {
    braveArgs.push('--gtest_also_run_disabled_tests')
  }

  if (options.output) {
    braveArgs.push('--gtest_output=xml:' + options.output)
  }

  if (options.disable_brave_extension) {
    braveArgs.push('--disable-brave-extension')
  }

  if (options.single_process) {
    braveArgs.push('--single_process')
  }

  if (options.test_launcher_jobs) {
    braveArgs.push('--test-launcher-jobs=' + options.test_launcher_jobs)
  }

  braveArgs = braveArgs.concat(passthroughArgs)

  // Filter out upstream tests that are known to fail for Brave
  let upstreamTestSuites = [
    'unit_tests',
    'browser_tests',
  ]
  if (upstreamTestSuites.includes(suite)) {
    let filterFilePaths = getApplicableFilters(suite)
    if (filterFilePaths.length > 0)
      braveArgs.push(`--test-launcher-filter-file="${filterFilePaths.join(';')}"`)
    if (config.isTeamcity) {
      braveArgs.push('--test-launcher-teamcity-reporter-ignore-preliminary-failures')
    }
  }

  if (
    suite === 'brave_unit_tests' &&
    config.isTeamcity &&
    config.targetOS !== 'android' &&
    config.targetOS !== 'ios'
  ) {
    runChromiumTestLauncherTeamcityReporterIntegrationTests()
  }

  if (config.targetOS === 'ios') {
    util.run(path.join(config.outputDir, "iossim"), [
      "-d", "\"iPhone 14 Pro\"",
      path.join(config.outputDir, `${suite}.app`),
      path.join(config.outputDir, `${suite}.app/PlugIns/${suite}_module.xctest`)
    ], config.defaultOptions)
  } else {
    // Run the tests
    getTestsToRun(config, suite).every((testSuite) => {
      if (options.output) {
        braveArgs.splice(braveArgs.indexOf('--gtest_output=xml:' + options.output), 1)
        braveArgs.push(`--gtest_output=xml:${testSuite}.xml`)
      }
      if (config.targetOS === 'android') {
        assert(
            config.targetArch === 'x86' || options.manual_android_test_device,
            'Only x86 build can be run automatically. For other builds please run test device manually and specify manual_android_test_device flag.')
      }
      if (config.targetOS === 'android' && !options.manual_android_test_device) {
        // Specify emulator to run tests on
        braveArgs.push(
            `--avd-config tools/android/avd/proto/${options.android_test_emulator_name}.textpb`)
      }
      let runOptions = config.defaultOptions
      if (config.isTeamcity) {
        // Stdout and stderr must be separate for a test launcher.
        runOptions.stdio = 'inherit'
      }
      if (options.output)
        // When test results are saved to a file, callers (such as CI) generate
        // and analyze test reports as a next step. These callers are typically
        // not interested in the exit code of running the tests, because they
        // get the information about test success or failure from the output
        // file. On the other hand, callers are interested in errors that
        // produce an exit code, such as test compilation failures. By ignoring
        // the test exit code here, we give callers a chance to distinguish test
        // failures (by looking at the output file) from compilation errors.
        runOptions.continueOnFail = true
      let prog = util.run(path.join(config.outputDir, getTestBinary(testSuite)), braveArgs, runOptions)
      // Don't run other tests if one has failed already, especially because
      // this would overwrite the --output file (if given).
      return prog.status === 0
    })
  }
}

const runChromiumTestLauncherTeamcityReporterIntegrationTests = () => {
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
      "##teamcity[testSuiteFinished name='brave_unit_tests']"
    ]
  }

  const ignorePreliminaryFailuresTestCase = {
    args: [
      ...generalTestCase.args,
      '--test-launcher-teamcity-reporter-ignore-preliminary-failures'
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
      "##teamcity[testSuiteFinished name='brave_unit_tests']"
    ]
  }

  const runOptions = config.defaultOptions
  runOptions.stdio = 'pipe'
  runOptions.continueOnFail = true

  for (const testCase of [generalTestCase, ignorePreliminaryFailuresTestCase]) {
    const prog = util.run(
      path.join(config.outputDir, 'brave_unit_tests'),
      testCase.args,
      runOptions
    )
    const outputLines = prog.stdout.toString().split('\n')
    checkTeamcityReporterOutput(outputLines, testCase.expectedLines)
  }
}

const checkTeamcityReporterOutput = (outputLines, expectedTeamcityLines) => {
  const outputTeamcityLines = outputLines.filter((line) =>
    line.startsWith('##teamcity')
  )

  const isMatched =
    outputTeamcityLines.length === expectedTeamcityLines.length &&
    expectedTeamcityLines.every((expectedLine, index) => {
      const outputLine = outputTeamcityLines[index]
      return outputLine.startsWith(expectedLine)
    })

  if (!isMatched) {
    const notMatchedOutputLines = outputTeamcityLines.filter(
      (outputLine, index) => {
        const expectedLine = expectedTeamcityLines[index]
        return !outputLine.startsWith(expectedLine)
      }
    )

    const notMatchedExpectedLines = expectedTeamcityLines.filter(
      (expectedLine, index) => {
        const outputLine = outputTeamcityLines[index]
        return !outputLine?.startsWith(expectedLine)
      }
    )

    Log.error(
      'TeamcityReporter output test failed, output ##teamcity lines do not match expected lines (note ##teamcity was replaced with %%teamcity):'
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
        ...outputLines
      ]
        .join('\n')
        .replace(/##teamcity/gm, '%%teamcity')
    )
    process.exit(1)
  } else {
    console.log('TeamcityReporter output test passed')
  }
}

module.exports = test
