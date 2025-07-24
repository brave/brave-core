// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs-extra')
const path = require('path')

const getTestBinary = (config, suite) => {
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

const getChromiumUnitTestsSuites = () => {
  return [
    'base_unittests',
    'components_unittests',
    'content_unittests',
    'net_unittests',
    'unit_tests',
  ]
}

const getBraveUnitTestsSuites = (config) => {
  let tests = []
  if (config.isIOS()) {
    tests.push('ios_brave_unit_tests')
  } else {
    tests.push('brave_unit_tests')
  }
  tests.push('brave_components_unittests')

  if (!config.isMobile()) {
    tests.push('brave_installer_unittests')
  }

  return tests
}

const getTestsToRun = (config, suite) => {
  let testsToRun = []
  if (suite === 'brave_all_unit_tests') {
    testsToRun = getBraveUnitTestsSuites(config)
  } else if (suite === 'chromium_unit_tests') {
    testsToRun = getChromiumUnitTestsSuites()
  } else {
    testsToRun = [suite]
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
const getApplicableFilters = (config, suite) => {
  let filterFilePaths = []

  let targetPlatform = process.platform
  if (targetPlatform === 'win32') {
    targetPlatform = 'windows'
  } else if (targetPlatform === 'darwin') {
    targetPlatform = 'macos'
  }

  let possibleFilters = [
    suite,
    [suite, targetPlatform].join('-'),
    [suite, targetPlatform, config.targetArch].join('-'),
  ]
  possibleFilters.forEach((filterName) => {
    let filterFilePath = path.join(
      config.braveCoreDir,
      'test',
      'filters',
      `${filterName}.filter`,
    )
    if (fs.existsSync(filterFilePath)) {
      filterFilePaths.push(filterFilePath)
    }
  })

  return filterFilePaths
}

module.exports = {
  getTestBinary,
  getTestsToRun,
  getApplicableFilters,
  getChromiumUnitTestsSuites,
}
