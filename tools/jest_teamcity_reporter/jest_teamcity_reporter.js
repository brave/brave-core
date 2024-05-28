// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const assert = require('assert')
const path = require('path')
const util = require('util')

/**
 * @typedef {import('@jest/types').Config.GlobalConfig} GlobalConfig
 * @typedef {import('@jest/reporters').Reporter} Reporter
 * @typedef {import('@jest/reporters').Test} Test
 * @typedef {import('@jest/reporters').TestResult} TestResult
 */

/** @implements {Reporter} */
class TeamcityReporter {
  /** @type {string} */
  #suiteName

  /**
   * @param {GlobalConfig} _globalConfig
   * @param {Record<string, any>} reporterOptions
   */
  constructor(_globalConfig, reporterOptions) {
    this.#suiteName = reporterOptions.suiteName ?? 'jest'
  }

  /**
   * @type {Reporter["onRunStart"]}
   */
  onRunStart() {
    this.tcServiceMessage('testSuiteStarted', { name: this.#suiteName })
  }

  /**
   * @type {Reporter["onRunComplete"]}
   */
  onRunComplete() {
    this.tcServiceMessage('testSuiteFinished', { name: this.#suiteName })
  }

  /**
   * @type {Reporter["getLastError"]}
   */
  getLastError() {}

  /**
   * @type {Reporter["onTestFileResult"]}
   * @param {Test} test
   * @param {TestResult} testFileResult
   */
  onTestFileResult(test, testFileResult) {
    // A flag to assert the test file result was actually reported.
    let testFileResultWasReported = false

    // Get the test file path relative to the cwd.
    const testFilePath = path
      .relative(test.context.config.cwd, testFileResult.testFilePath)
      .replace(/\\/g, '/')

    // Convert the test file path into a Teamcity-compatible test name prefix.
    const tcTestPrefix = this.createTcTestPrefix(testFilePath)

    // Gather console messages from the test file.
    const consoleMessages = testFileResult.console?.map((logEntry) => {
      return `console.${logEntry.type}\n  ${logEntry.message}\n${logEntry.origin}`
    })

    // Report each test case in the test file.
    for (const testResult of testFileResult.testResults) {
      // Original test name.
      const testName = [...testResult.ancestorTitles, testResult.title].join(
        ' > '
      )

      this.reportTest(
        `${tcTestPrefix}.${this.createTcTestName(testName)}`,
        testFilePath,
        consoleMessages,
        testResult.duration ?? 0,
        (tcFullTestName) => {
          // Report the unmodified test name as metadata.
          this.tcServiceMessage('testMetadata', {
            name: 'testName',
            value: testName
          })

          // Report the test result.
          switch (testResult.status) {
            case 'passed':
              // No special handling for 'passed' tests.
              break
            case 'skipped':
            case 'pending':
            case 'todo':
            case 'disabled':
              this.tcServiceMessage('testIgnored', {
                name: tcFullTestName,
                message: testResult.status
              })
              break
            case 'failed':
            default:
              // Handle 'failed' and any other test status as failure to detect
              // a possible Jest breaking change.
              this.tcServiceMessage('testFailed', {
                name: tcFullTestName,
                details: [
                  testResult.status,
                  ...testResult.failureMessages
                ].join('\n')
              })
          }
        }
      )
      testFileResultWasReported = true
    }

    // Report the test file as a single test if it failed.
    if (testFileResult.testExecError) {
      this.reportTest(
        `${tcTestPrefix}.All`,
        testFilePath,
        consoleMessages,
        0,
        (tcFullTestName) => {
          this.tcServiceMessage('testFailed', {
            name: tcFullTestName,
            details:
              testFileResult.failureMessage ??
              util.format('%O', testFileResult.testExecError)
          })
        }
      )
      testFileResultWasReported = true
    }

    // Assert the test file result was reported.
    assert(
      testFileResultWasReported,
      util.format(
        'Test file result was not reported to Teamcity. It means this ' +
          "reporter doesn't handle some edge case or is not compatible with " +
          'the current Jest version.\nTest file result:\n%O',
        testFileResult
      )
    )
  }

  /**
   * Create a Teamcity-compatible test name prefix from a test file path.
   * @param {string} str
   * @returns {string}
   */
  createTcTestPrefix(testFilePath) {
    // This function creates a package name and a class name string to be used
    // as a prefix for all tests in a test file. Teamcity expects a specially
    // formatted test name to correctly parse it into:
    // - suite name (optional, stops at ':', can have spaces)
    // - package name (dot-delimited)
    // - class name (an identifier without dots)
    // - test name (an arbitrary string with any symbols, can have spaces)
    //
    // See for details:
    // https://www.jetbrains.com/help/teamcity/service-messages.html#Interpreting+Test+Names

    // The reporter omits the suite name in the test prefix since it's already
    // set via `testSuiteStarted`. Although suites can be nested, using a test
    // file as an additional suite name would lead to numerous ungroupable
    // suites in the Teamcity UI.

    // To group tests by file, the reporter impersonates a test file as a
    // package name.
    const tcPackageName = testFilePath
      .replace(/[/\\]/g, '.') // Replace slashes with dots.
      .replace(/\.+/g, '.') // Replace multiple dots with a single dot.
      .replace(/[^\w\-$.]/g, '_') // Replace invalid characters with underscores.
      .replace(/(^[^a-zA-Z_])/g, '_$1') // Prepend an underscore if the first character is invalid.

    // After the package name, Teamcity expects a class name. If the class name
    // is missing, Teamcity will use a previous dot-delimited word as the class
    // name, which in our case is a file extension. We don't want that.
    //
    // Jest doesn't provide a class name for tests, so we have to create one.
    // One of the possible solutions is to use describe() values as class names,
    // but this turned out to be problematic. These values can be nested and may
    // contain symbols not allowed in class names, requiring replacements. This
    // results in less readable full test names that are harder to match with
    // the actual tests.
    //
    // A simpler solution is to use a fixed class name for all tests. The class
    // name appears next to the actual test name, so 'Test' is chosen as a
    // neutral name to not change the meaning of any test name.
    const tcClassName = 'Test'

    return `${tcPackageName}.${tcClassName}`
  }

  /**
   * Create a Teamcity-compatible test name.
   * @param {string} testName
   * @returns {string}
   */
  createTcTestName(testName) {
    // Replace few things to not let Teamcity split the name into test suite,
    // package or class name.
    return testName
      .replace(/:/g, '_') // Replace all colons with underscores.
      .replace(/(^[^ ]*?)\./g, '$1_') // Replace dots with underscores if there are no spaces before them.
      .replace(/(^[^a-zA-Z_])/g, '_$1') // Prepend an underscore if the first character is invalid.
  }

  /**
   * Report test start/finish events, test metadata, console messages and the
   * result.
   * @param {string} tcFullTestName
   * @param {string} testFilePath
   * @param {string[] | undefined} consoleMessages
   * @param {number} duration
   * @param {Function} reportResultClosure
   */
  reportTest(
    tcFullTestName,
    testFilePath,
    consoleMessages,
    duration,
    reportResultClosure
  ) {
    // Report the test start.
    this.tcServiceMessage('testStarted', { name: tcFullTestName })

    // Report the test file path as metadata.
    this.tcServiceMessage('testMetadata', {
      name: 'testFilePath',
      value: testFilePath
    })

    // Report console messages as test stdout.
    if (consoleMessages && consoleMessages.length !== 0) {
      this.tcServiceMessage('testStdOut', {
        name: tcFullTestName,
        out:
          `Console messages from ${testFilePath}:\n` +
          consoleMessages.join('\n')
      })
    }

    // Report the test result.
    reportResultClosure(tcFullTestName)

    // Report the test finish.
    this.tcServiceMessage('testFinished', {
      name: tcFullTestName,
      duration
    })
  }

  /**
   * Output a Teamcity Service Message.
   * @param {string} name
   * @param {Record<string, any>} params
   */
  tcServiceMessage(name, params) {
    let paramsStr = ''
    for (const [paramName, paramValue] of Object.entries(params)) {
      assert(paramValue !== undefined && paramValue !== null)

      const strParamValue = paramValue.toString()
      if (strParamValue === '') {
        continue
      }

      // https://www.jetbrains.com/help/teamcity/service-messages.html#Escaped+Values
      const escapedParamValue = strParamValue
        // Remove ANSI escape sequences (colors, styles, etc.).
        .replace(/\u001B\[\d+(;\d+)*m/g, '')
        // Replace Teamcity Service Messages special characters.
        .replace(/[\n\r'|[\]]/g, (match) => {
          switch (match) {
            case '\n':
              return '|n'
            case '\r':
              return '|r'
            default:
              return '|' + match
          }
        })
      paramsStr += ` ${paramName}='${escapedParamValue}'`
    }
    process.stdout.write(`##teamcity[${name}${paramsStr}]\n`)
  }
}

module.exports = TeamcityReporter
