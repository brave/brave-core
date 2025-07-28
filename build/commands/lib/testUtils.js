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

const getChromiumTestsSuites = (config) => {
  return getTestsToRun(config, 'chromium_unit_tests').concat(['browser_tests'])
}

const getTestsToRun = (config, suite) => {
  const testDepFile = path.join(config.outputDir, `${suite}.json`)
  if (fs.existsSync(testDepFile)) {
    suiteDepNames = JSON.parse(
      fs.readFileSync(testDepFile, { encoding: 'utf-8' }),
    )
    return suiteDepNames.map((x) => x.split(':').at(-1))
  }

  return [suite]
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

  if (config.is_ubsan) {
    possibleFilters.push([suite, targetPlatform, 'ubsan'].join('-'))
  }

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
  getChromiumTestsSuites,
}
