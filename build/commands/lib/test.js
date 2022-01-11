const fs = require('fs-extra')
const path = require('path')

const config = require('../lib/config')
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

const test = (passthroughArgs, suite, buildConfig = config.defaultBuildConfig, options) => {
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

  // Build the tests
  let testSuites = [
    'brave_unit_tests',
    'brave_browser_tests',
    'brave_network_audit_tests',
  ]
  if (testSuites.includes(suite)) {
    config.buildTarget = 'brave/test:' + suite
  } else {
    config.buildTarget = suite
  }
  util.buildTarget()

  // Filter out upstream tests that are known to fail for Brave
  let upstreamTestSuites = [
    'unit_tests',
    'browser_tests',
  ]
  if (upstreamTestSuites.includes(suite)) {
    let filterFilePaths = getApplicableFilters(suite)
    if (filterFilePaths.length > 0)
      braveArgs.push(`--test-launcher-filter-file="${filterFilePaths.join(';')}"`)
  }

  if (config.targetOS === 'ios') {
    util.run(path.join(config.outputDir, "iossim"), [
      path.join(config.outputDir, `${suite}.app`),
      path.join(config.outputDir, `${suite}.app/PlugIns/${suite}_module.xctest`)
    ], config.defaultOptions)
  } else {
    // Run the tests
    getTestsToRun(config, suite).forEach((testSuite) => {
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
        braveArgs.push(`--avd-config tools/android/avd/proto/generic_android28.textpb`)
      }
      util.run(path.join(config.outputDir, getTestBinary(testSuite)), braveArgs, config.defaultOptions)
    })
  }
}

module.exports = test
