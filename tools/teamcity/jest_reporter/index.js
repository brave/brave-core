// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const assert = require('assert')
const path = require('path')

class CustomReporter {
  constructor(globalConfig, reporterOptions, reporterContext) {
    this._isTeamcity = process.env.TEAMCITY_VERSION !== undefined
    this._globalConfig = globalConfig
    this._options = reporterOptions
    this._context = reporterContext
  }

  onRunComplete(testContexts, results) {
    if (!this._isTeamcity) {
      return
    }
    assert(testContexts.size === 1, 'Only one testContext is expected')

    const tcSuiteName = this.tcEscape(this._options.suiteName ?? 'jest')
    this.tcServiceMessage('testSuiteStarted', { name: tcSuiteName })
    const cwd = [...testContexts][0].config.cwd
    for (const testFileResult of results.testResults) {
      this.logTestFileResults(cwd, testFileResult)
    }
    this.tcServiceMessage('testSuiteFinished', { name: tcSuiteName })
  }

  logTestFileResults(cwd, testFileResult) {
    // Get test file path relative to the cwd.
    const testFilePath = path
      .relative(cwd, testFileResult.testFilePath)
      .replace(/\\/g, '/')
    // Create TC-friendly package name from the test file path.
    const tcTestPackage = this.maybePrependUnderscore(
      testFilePath.replace(/[^\w-\.]/g, '.')
    )

    if (testFileResult.testExecError || testFileResult.skipped) {
      const tcFullTestName = [tcTestPackage, 'All', 'All'].join('.')
      this.tcServiceMessage('testStarted', { name: tcFullTestName })
      if (testFileResult.testExecError) {
        this.tcServiceMessage('testFailed', {
          name: tcFullTestName,
          details:
            testFileResult.failureMessage ??
            testFileResult.testExecError.message
        })
      } else if (testFileResult.skipped) {
        this.tcServiceMessage('testIgnored', {
          name: tcFullTestName,
          details: testFileResult.failureMessage ?? 'skipped'
        })
      } else {
        assert(false)
      }
      this.tcServiceMessage('testFinished', { name: tcFullTestName })
      return
    }

    for (const testResult of testFileResult.testResults) {
      // Create TC-friendly class name from the test ancestor titles (describe
      // blocks).
      const tcTestClass =
        this.maybePrependUnderscore(
          testResult.ancestorTitles.join('_').replace(/[^\w]/g, '_')
        ) || 'All'

      // Create TC-friendly test name without `:`, so TC won't treat it as a
      // test suite.
      const tcTestName = testResult.title.replace(/:/g, '')

      // Join all together to create the full test name.
      const tcFullTestName = [tcTestPackage, tcTestClass, tcTestName].join('.')

      this.tcServiceMessage('testStarted', { name: tcFullTestName })
      this.tcServiceMessage('testMetadata', {
        name: 'path',
        value: testFilePath
      })
      this.tcServiceMessage('testMetadata', {
        name: 'name',
        value: [...testResult.ancestorTitles, testResult.title].join(' > ')
      })

      switch (testResult.status) {
        case 'failed':
          if (testResult.failureDetails) {
            let failureStacks = []
            testResult.failureDetails.forEach((failureDetail) => {
              failureStacks.push(failureDetail.stack)
            })
            this.tcServiceMessage('testFailed', {
              name: tcFullTestName,
              details: failureStacks.join('\n')
            })
          }
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
      }

      this.tcServiceMessage('testFinished', {
        name: tcFullTestName,
        duration: testResult.duration ?? 0
      })
    }
  }

  maybePrependUnderscore(str) {
    if (!str.match(/^[\d\.]/)) {
      return str
    }
    return '_' + str
  }

  tcServiceMessage(name, params) {
    let paramsStr = ''
    for (const [pname, pvalue] of Object.entries(params)) {
      paramsStr += ` ${pname}='${this.tcEscape(pvalue)}'`
    }
    console.log(`##teamcity[${name}${paramsStr}]`)
  }

  tcEscape(str) {
    if (str === null || str === undefined) {
      return ''
    }

    return str
      .toString()
      .replace(/\x1B.*?m/g, '')
      .replace(/\|/g, '||')
      .replace(/\n/g, '|n')
      .replace(/\r/g, '|r')
      .replace(/\[/g, '|[')
      .replace(/\]/g, '|]')
      .replace(/\u0085/g, '|x')
      .replace(/\u2028/g, '|l')
      .replace(/\u2029/g, '|p')
      .replace(/'/g, "|'")
  }
}

module.exports = CustomReporter
