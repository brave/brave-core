// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const TeamcityReporter = require('./jest_teamcity_reporter')

describe('TeamcityReporter', () => {
  let reporter
  let mockStdoutWrite

  beforeEach(() => {
    mockStdoutWrite = jest.spyOn(process.stdout, 'write').mockImplementation()
    reporter = new TeamcityReporter({}, { suiteName: 'my_suite' })
  })

  it('onRunStart should report testSuiteStarted', () => {
    reporter.onRunStart()

    expect(mockStdoutWrite).toHaveBeenCalledWith(
      "##teamcity[testSuiteStarted name='my_suite']\n"
    )
  })

  it('onRunComplete should report testSuiteFinished', () => {
    reporter.onRunComplete()

    expect(mockStdoutWrite).toHaveBeenCalledWith(
      "##teamcity[testSuiteFinished name='my_suite']\n"
    )
  })

  describe('onTestFileResult', () => {
    it('should report each test case in the test file', () => {
      const test = {
        context: {
          config: {
            cwd: '/brave/tools/jest_teamcity_reporter'
          }
        }
      }

      const testFileResult = {
        testFilePath: '/brave/tools/jest_teamcity_reporter/example.test.js',
        testResults: [
          {
            ancestorTitles: ['Group 1:'],
            title: 'Test 1',
            duration: 100,
            status: 'passed',
            failureMessages: []
          },
          {
            ancestorTitles: ['Group 1:', 'Group 2'],
            title: 'Test 2',
            duration: 200,
            status: 'failed',
            failureMessages: ['Assertion error']
          }
        ],
        console: [{ type: 'log', message: 'Log message 1', origin: 'origin1' }]
      }

      reporter.onTestFileResult(test, testFileResult)

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='example.test.js.Test.Group 1_ > Test 1']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testMetadata name='testFilePath' value='example.test.js']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testMetadata name='testName' value='Group 1: > Test 1']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStdOut name='example.test.js.Test.Group 1_ > Test 1' out='Console messages from example.test.js:|nconsole.log|n  Log message 1|norigin1']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFinished name='example.test.js.Test.Group 1_ > Test 1' duration='100']\n"
      )

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='example.test.js.Test.Group 1_ > Group 2 > Test 2']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testMetadata name='testFilePath' value='example.test.js']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testMetadata name='testName' value='Group 1: > Group 2 > Test 2']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStdOut name='example.test.js.Test.Group 1_ > Group 2 > Test 2' out='Console messages from example.test.js:|nconsole.log|n  Log message 1|norigin1']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFailed name='example.test.js.Test.Group 1_ > Group 2 > Test 2' details='failed|nAssertion error']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFinished name='example.test.js.Test.Group 1_ > Group 2 > Test 2' duration='200']\n"
      )
    })

    it('should report the test file as a single test if it failed', () => {
      const test = {
        context: {
          config: {
            cwd: '/brave/tools/jest_teamcity_reporter'
          }
        }
      }

      const testFileResult = {
        testFilePath: '/brave/tools/jest_teamcity_reporter/example.test.js',
        testResults: [],
        testExecError: new Error('Test execution error'),
        failureMessage: 'Test failed',
        console: [{ type: 'log', message: 'Log message 1', origin: 'origin1' }]
      }

      reporter.onTestFileResult(test, testFileResult)

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='example.test.js.Test.All']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testMetadata name='testFilePath' value='example.test.js']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStdOut name='example.test.js.Test.All' out='Console messages from example.test.js:|nconsole.log|n  Log message 1|norigin1']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFailed name='example.test.js.Test.All' details='Test failed']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFinished name='example.test.js.Test.All' duration='0']\n"
      )
    })

    it('should assert if a test file result was not handled', () => {
      const test = {
        context: {
          config: {
            cwd: '/brave/tools/jest_teamcity_reporter'
          }
        }
      }

      const testFileResult = {
        testFilePath: '/brave/tools/jest_teamcity_reporter/example.test.js',
        testResults: [],
        // Let's pretend Jest added another way of failing a test file which we
        // don't handle. This member is not part of the Jest test result schema.
        testValidationError: new Error('Test validation error')
      }

      expect(() => {
        reporter.onTestFileResult(test, testFileResult)
      }).toThrow(/Test file result was not reported to Teamcity/)
    })

    it('should replace dots with underscores if there were no spaces before', () => {
      const test = {
        context: {
          config: {
            cwd: '/brave/tools/jest_teamcity_reporter'
          }
        }
      }

      const testFileResult = {
        testFilePath: '/brave/tools/jest_teamcity_reporter/example.test.js',
        testResults: [
          {
            ancestorTitles: ['Group.1'],
            title: 'Test.1',
            duration: 100,
            status: 'passed',
            failureMessages: []
          },
          {
            ancestorTitles: ['Group 2'],
            title: '.Test B',
            duration: 200,
            status: 'passed',
            failureMessages: []
          }
        ]
      }

      reporter.onTestFileResult(test, testFileResult)

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='example.test.js.Test.Group_1 > Test.1']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='example.test.js.Test.Group 2 > .Test B']\n"
      )
    })
  })

  describe('createTcTestPrefix', () => {
    it('should generate the correct test prefix for a test file', () => {
      const testCases = [
        {
          testFilePath: 'components/test/brave_new_tab_ui/state/gridSitesState_test.ts',
          expectedPrefix: 'components.test.brave_new_tab_ui.state.gridSitesState_test.ts.Test'
        },
        {
          testFilePath: 'components/test/brave_new_tab_ui/state/gridSitesState_test@#$%...ts',
          expectedPrefix: 'components.test.brave_new_tab_ui.state.gridSitesState_test__$_.ts.Test'
        },
        {
          testFilePath: '0components/test/brave_new_tab_ui/state/gridSitesState_test.ts',
          expectedPrefix: '_0components.test.brave_new_tab_ui.state.gridSitesState_test.ts.Test'
        },
        {
          testFilePath: 'components\\test\\brave_new_tab_ui\\state\\gridSitesState_test.ts',
          expectedPrefix: 'components.test.brave_new_tab_ui.state.gridSitesState_test.ts.Test'
        }
      ]
      for (const testCase of testCases) {
        const result = reporter.createTcTestPrefix(testCase.testFilePath)
        expect(result).toBe(testCase.expectedPrefix)
      }
    })
  })

  describe('createTcTestName', () => {
    it('should replace dots with underscores if there were no spaces before', () => {
      const testCases = [
        {
          testName: 'Test.1',
          expectedName: 'Test_1'
        },
        {
          testName: '.Test B',
          expectedName: '_Test B'
        },
        {
          testName: 'Test C.',
          expectedName: 'Test C.'
        },
        {
          testName: '.Test D.',
          expectedName: '_Test D.'
        }
      ]
      for (const testCase of testCases) {
        const result = reporter.createTcTestName(testCase.testName)
        expect(result).toBe(testCase.expectedName)
      }
    })

    it('should prepend an underscore if the first character is invalid', () => {
      const testCases = [
        {
          testName: '0Test',
          expectedName: '_0Test'
        },
        {
          testName: ' Test',
          expectedName: '_ Test'
        },
        {
          testName: 'Test',
          expectedName: 'Test'
        }
      ]
      for (const testCase of testCases) {
        const result = reporter.createTcTestName(testCase.testName)
        expect(result).toBe(testCase.expectedName)
      }
    })

    it('should replace all colons with underscores', () => {
      const testCases = [
        {
          testName: 'Test:1 2:3',
          expectedName: 'Test_1 2_3'
        },
        {
          testName: ':Test:2',
          expectedName: '_Test_2'
        }
      ]
      for (const testCase of testCases) {
        const result = reporter.createTcTestName(testCase.testName)
        expect(result).toBe(testCase.expectedName)
      }
    })
  })

  describe('reportTest', () => {
    it('should write the correct test start, metadata, result, and finish messages to stdout', () => {
      const testFilePath = 'test/file/path'
      const tcFullTestName = 'test > case'
      const consoleMessages = ['a', 'b']
      const duration = 1000

      reporter.reportTest(
        tcFullTestName,
        testFilePath,
        consoleMessages,
        duration,
        (tcFullTestName) => {
          reporter.tcServiceMessage('testFailed', {
            name: tcFullTestName,
            details: 'Test failed'
          })
        }
      )

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='test > case']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testMetadata name='testFilePath' value='test/file/path']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStdOut name='test > case' out='Console messages from test/file/path:|na|nb']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFailed name='test > case' details='Test failed']\n"
      )
      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testFinished name='test > case' duration='1000']\n"
      )
    })
  })

  describe('tcServiceMessage', () => {
    it('should write the correct service message to stdout', () => {
      reporter.tcServiceMessage('testStarted', { name: 'test' })

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='test']\n"
      )
    })

    it('should not write empty string', () => {
      reporter.tcServiceMessage('testStarted', { param: '' })

      expect(mockStdoutWrite).toHaveBeenCalledWith('##teamcity[testStarted]\n')
    })

    it('should write zero number', () => {
      reporter.tcServiceMessage('testStarted', { param: 0 })

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted param='0']\n"
      )
    })

    it('should escape special characters in the message parameters', () => {
      reporter.tcServiceMessage('testStarted', {
        name: 'test\nwith\rspecial|characters'
      })

      expect(mockStdoutWrite).toHaveBeenCalledWith(
        "##teamcity[testStarted name='test|nwith|rspecial||characters']\n"
      )
    })

    it('should assert if a parameter value is undefined or null', () => {
      expect(() => {
        reporter.tcServiceMessage('testStarted', {
          param: undefined
        })
      }).toThrow()

      expect(() => {
        reporter.tcServiceMessage('testStarted', {
          param: null
        })
      }).toThrow()
    })
  })
})
